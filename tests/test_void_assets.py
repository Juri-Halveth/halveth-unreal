"""Exercise the real VOID downloader with bounded, offline synthetic streams."""
import concurrent.futures
import hashlib
import importlib.util
import io
from pathlib import Path
import tempfile
import threading
import unittest
from unittest import mock

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("void_assets", ROOT / "experiments/void-native/scripts/assets.py")
ASSETS = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ASSETS)
REAL_MANIFEST = ASSETS.MANIFEST
REAL_SHA256 = hashlib.sha256
REAL_TEMPORARY = tempfile.NamedTemporaryFile


class Console(io.StringIO):
    def reconfigure(self, **kwargs):
        pass


class Response(io.BytesIO):
    def __init__(self, data, fault=None):
        super().__init__(data)
        self.fault = fault
        self.read_sizes = []

    def read(self, size):
        if self.fault == "read" and self.read_sizes:
            raise OSError("synthetic read failure")
        self.read_sizes.append(size)
        return super().read(size)

    def __exit__(self, *args):
        result = super().__exit__(*args)
        if self.fault == "response_close":
            raise OSError("synthetic response close failure")
        return result


class Output:
    def __init__(self, wrapped, fault=None):
        self.wrapped = wrapped
        self.name = wrapped.name
        self.fault = fault
        self.blocks = []

    def __enter__(self):
        self.wrapped.__enter__()
        return self

    def __exit__(self, *args):
        result = self.wrapped.__exit__(*args)
        if self.fault == "close":
            raise OSError("synthetic output close failure")
        return result

    def write(self, block):
        self.blocks.append(block)
        if self.fault == "write":
            self.wrapped.write(block[:1])
            raise OSError("synthetic write failure")
        if self.fault == "short":
            return self.wrapped.write(block[:-1])
        return self.wrapped.write(block)

    def flush(self):
        if self.fault == "flush":
            raise OSError("synthetic flush failure")
        self.wrapped.flush()


class VoidAssetTests(unittest.TestCase):
    def setUp(self):
        directory = tempfile.TemporaryDirectory()
        self.addCleanup(directory.cleanup)
        self.art = Path(directory.name)
        self.enterContext(mock.patch.object(ASSETS, "ART", self.art))
        self.enterContext(mock.patch.object(ASSETS, "CHUNK_SIZE", 4))
        self.network = self.enterContext(mock.patch.object(
            ASSETS.urllib.request, "urlopen", side_effect=AssertionError("unexpected network access")))
        self.outputs = []

    def record(self, data=b"pinned bytes", name="models/fixture.bin"):
        return {"file": name, "size": len(data), "expected_size": len(data),
                "sha256": REAL_SHA256(data).hexdigest(),
                "url": "https://dl.polyhaven.org/file/ph-assets/" + name}

    def respond(self, data, fault=None):
        response = Response(data, fault)
        self.network.side_effect = None
        self.network.return_value = response
        return response

    def capture_outputs(self, fault=None):
        def create(*args, **kwargs):
            output = Output(REAL_TEMPORARY(*args, **kwargs), fault)
            self.outputs.append(output)
            return output
        return mock.patch.object(ASSETS.tempfile, "NamedTemporaryFile", side_effect=create)

    def save(self, record, data):
        destination = ASSETS.target(record)
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(data)
        return destination

    def assert_no_parts(self):
        self.assertEqual(list(self.art.rglob("*.part")), [])

    def main(self, manifest, mode="download"):
        with mock.patch.object(ASSETS, "MANIFEST", manifest), \
                mock.patch.object(ASSETS.sys, "argv", ["assets.py"] + ([mode] if mode else [])), \
                mock.patch.object(ASSETS.sys, "stdout", Console()) as output, \
                mock.patch.object(ASSETS.sys, "stderr", Console()):
            result = ASSETS.main()
        return result, output.getvalue()

    def test_current_manifest_is_admitted_without_downloads(self):
        records = ASSETS.admit_manifest(REAL_MANIFEST)
        self.assertEqual((len(records), sum(item["size"] for item in records), REAL_MANIFEST["byte_cap"]),
                         (31, 189251328, 200000000))
        self.network.assert_not_called()

    def test_aggregate_exact_cap_and_snapshot(self):
        records = [self.record(name="a"), self.record(name="b")]
        for record in records:
            record["size"] = record["expected_size"] = 100000000
        admitted = ASSETS.admit_manifest({"files": records, "byte_cap": 200000000})
        records[0]["size"] = 1
        self.assertEqual(admitted[0]["size"], 100000000)

    def test_invalid_admission_precedes_workers_network_and_temporary_files(self):
        valid = self.record()
        cases = [{"files": [dict(valid, size=size)], "byte_cap": 200000000}
                 for size in (-1, True, 1.0, "12", 200000001)]
        cases += [{"files": [valid], "byte_cap": cap}
                  for cap in (-1, True, 200000001, 200000000.0, "200000000")]
        cases += [{"files": [valid], "byte_cap": valid["size"] - 1},
                  {"files": [dict(valid, expected_size=1)], "byte_cap": 200000000},
                  {"files": [dict(valid, expected_size=True)], "byte_cap": 200000000},
                  {"files": [valid, dict(valid, file="models/../models/fixture.bin")], "byte_cap": 200000000},
                  {"files": [dict(valid, file="../outside.bin")], "byte_cap": 200000000}]
        for manifest in cases:
            with self.subTest(manifest=manifest), \
                    mock.patch.object(ASSETS.concurrent.futures, "ThreadPoolExecutor") as pool, \
                    mock.patch.object(ASSETS.tempfile, "NamedTemporaryFile") as temporary:
                with self.assertRaises(ValueError):
                    self.main(manifest)
                pool.assert_not_called()
                temporary.assert_not_called()
        self.network.assert_not_called()
        self.assert_no_parts()

    def test_aggregate_cap_plus_one_is_rejected(self):
        first = self.record(name="a")
        second = self.record(name="b")
        first["size"] = first["expected_size"] = 100000000
        second["size"] = second["expected_size"] = 100000001
        with self.assertRaisesRegex(ValueError, "aggregate"):
            self.main({"files": [first, second], "byte_cap": 200000000})
        self.network.assert_not_called()

    def test_direct_download_enforces_record_budget(self):
        with self.assertRaises(ValueError):
            ASSETS.download(dict(self.record(), size=200000001))
        self.network.assert_not_called()

    def test_exact_valid_download_replaces_only_after_output_is_closed(self):
        data = b"pinned bytes"
        record = self.record(data)
        destination = self.save(record, b"previous")
        response = self.respond(data)
        replace = Path.replace
        def publish(path, target):
            self.assertTrue(self.outputs[0].wrapped.closed)
            self.assertEqual(destination.read_bytes(), b"previous")
            self.assertEqual(path.read_bytes(), data)
            return replace(path, target)
        with self.capture_outputs(), mock.patch.object(Path, "replace", autospec=True, side_effect=publish) as publication:
            ASSETS.download(record)
        self.assertEqual(destination.read_bytes(), data)
        publication.assert_called_once()
        self.assertEqual(response.read_sizes[-1], 1)
        self.assertTrue(all(0 < size <= 4 for size in response.read_sizes))
        self.assertTrue(ASSETS.verify(record))
        self.assertEqual(self.network.call_args.kwargs["timeout"], 90)
        self.assert_no_parts()

    def test_new_download_and_empty_asset_are_valid(self):
        for data, name in ((b"new bytes", "new.bin"), (b"", "empty.bin")):
            with self.subTest(data=data):
                record = self.record(data, name)
                self.respond(data)
                ASSETS.download(record)
                self.assertEqual(ASSETS.target(record).read_bytes(), data)
                self.assertTrue(ASSETS.verify(record))
        self.assert_no_parts()

    def test_cached_valid_skip_uses_incremental_hash_without_network_or_temporary_file(self):
        record = self.record()
        self.save(record, b"pinned bytes")
        with mock.patch.object(ASSETS.tempfile, "NamedTemporaryFile") as temporary, \
                mock.patch.object(Path, "read_bytes", side_effect=AssertionError("whole-file hash read")):
            ASSETS.download(record)
            self.assertTrue(ASSETS.verify(record))
        temporary.assert_not_called()
        self.network.assert_not_called()

    def test_corrupt_cache_redownloads_incrementally(self):
        record = self.record()
        self.save(record, b"wrong bytes!")
        self.respond(b"pinned bytes")
        with mock.patch.object(Path, "read_bytes", side_effect=AssertionError("whole-file hash read")):
            ASSETS.download(record)
            self.assertTrue(ASSETS.verify(record))
        self.network.assert_called_once()

    def test_overrun_is_rejected_before_offending_write(self):
        for data in (b"", b"ab", b"abcdefgh", b"abcdefghi"):
            with self.subTest(data=data):
                record = self.record(data)
                destination = self.save(record, b"previous")
                self.respond(data + b"x")
                self.outputs = []
                with self.capture_outputs(), self.assertRaisesRegex(ValueError, "budget"):
                    ASSETS.download(record)
                written = b"".join(self.outputs[0].blocks)
                self.assertLessEqual(len(written), len(data))
                self.assertTrue(data.startswith(written))
                self.assertEqual(destination.read_bytes(), b"previous")
                self.assert_no_parts()

    def test_short_eof_and_same_length_wrong_digest_leave_no_temporary_file(self):
        for response in (b"short", b"wrong bytes!"):
            with self.subTest(response=response):
                record = self.record()
                destination = self.save(record, b"previous")
                self.respond(response)
                with self.assertRaisesRegex(ValueError, "pinned digest"):
                    ASSETS.download(record)
                self.assertEqual(destination.read_bytes(), b"previous")
                self.assert_no_parts()

    def test_read_and_response_close_errors_clean_up(self):
        for fault in ("read", "response_close"):
            with self.subTest(fault=fault):
                record = self.record()
                destination = self.save(record, b"previous")
                self.respond(b"pinned bytes", fault)
                with self.assertRaises(OSError):
                    ASSETS.download(record)
                self.assertEqual(destination.read_bytes(), b"previous")
                self.assert_no_parts()

    def test_open_error_cleans_up(self):
        self.network.side_effect = OSError("synthetic open failure")
        with self.assertRaises(OSError):
            ASSETS.download(self.record())
        self.assert_no_parts()
        self.assertFalse(ASSETS.target(self.record()).exists())

    def test_output_write_short_write_flush_and_close_errors_clean_up(self):
        for fault in ("write", "short", "flush", "close"):
            with self.subTest(fault=fault):
                record = self.record()
                destination = self.save(record, b"previous")
                self.respond(b"pinned bytes")
                with self.capture_outputs(fault), self.assertRaises(OSError):
                    ASSETS.download(record)
                self.assertTrue(self.outputs[-1].wrapped.closed)
                self.assertEqual(destination.read_bytes(), b"previous")
                self.assert_no_parts()

    def test_hash_constructor_update_and_finalization_errors_clean_up(self):
        for fault in ("constructor", "update", "hexdigest"):
            with self.subTest(fault=fault):
                record = self.record()
                destination = self.save(record, b"previous")
                self.respond(b"pinned bytes")
                digest = mock.Mock(wraps=REAL_SHA256())
                if fault != "constructor":
                    getattr(digest, fault).side_effect = OSError("synthetic hash failure")
                with mock.patch.object(ASSETS.hashlib, "sha256", return_value=digest) as constructor:
                    if fault == "constructor":
                        constructor.side_effect = OSError("synthetic hash failure")
                    with self.assertRaises(OSError):
                        ASSETS.download(record)
                self.assertEqual(destination.read_bytes(), b"previous")
                self.assert_no_parts()

    def test_replace_failure_cleans_up_and_preserves_previous_file(self):
        record = self.record()
        destination = self.save(record, b"previous")
        self.respond(b"pinned bytes")
        with mock.patch.object(Path, "replace", side_effect=OSError("synthetic replace failure")), \
                self.assertRaises(OSError):
            ASSETS.download(record)
        self.assertEqual(destination.read_bytes(), b"previous")
        self.assert_no_parts()

    def test_cleanup_failure_is_visible_with_original_failure_context(self):
        self.respond(b"x" * 20)
        unlink = Path.unlink
        try:
            with self.capture_outputs(), \
                    mock.patch.object(Path, "unlink", side_effect=OSError("synthetic cleanup failure")):
                with self.assertRaisesRegex(OSError, "cleanup") as caught:
                    ASSETS.download(self.record())
            self.assertIsInstance(caught.exception.__context__, ValueError)
        finally:
            for path in self.art.rglob("*.part"):
                unlink(path)

    def test_unique_temporary_preserves_unrelated_legacy_partial(self):
        record = self.record()
        destination = self.save(record, b"previous")
        legacy = destination.with_name(destination.name + ".part")
        legacy.write_bytes(b"unrelated partial")
        self.respond(b"pinned bytes")
        with self.capture_outputs():
            ASSETS.download(record)
        self.assertNotEqual(Path(self.outputs[0].name), legacy)
        self.assertEqual(legacy.read_bytes(), b"unrelated partial")
        self.assertEqual(list(self.art.rglob("*.part")), [legacy])

    def test_concurrent_same_target_downloads_have_distinct_owned_temporaries(self):
        record = self.record()
        ASSETS.target(record).parent.mkdir(parents=True, exist_ok=True)
        barrier = threading.Barrier(2)
        def response(*args, **kwargs):
            barrier.wait(timeout=5)
            return Response(b"pinned bytes")
        self.network.side_effect = response
        with self.capture_outputs(), concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
            list(pool.map(ASSETS.download, [record, record]))
        self.assertEqual(len({output.name for output in self.outputs}), 2)
        self.assertTrue(ASSETS.verify(record))
        self.assert_no_parts()

    def test_cached_file_growing_after_stat_is_rejected(self):
        record = self.record(b"abcd")
        destination = self.save(record, b"abcd")
        open_file = Path.open
        def growing_open(path, mode="r", *args, **kwargs):
            if path == destination and mode == "rb":
                with open_file(path, "ab") as output:
                    output.write(b"extra")
            return open_file(path, mode, *args, **kwargs)
        with mock.patch.object(Path, "open", growing_open):
            self.assertFalse(ASSETS.verify(record))
        self.network.assert_not_called()

    def test_verify_default_remains_offline_and_reports_cache_mismatch(self):
        record = self.record()
        manifest = {"files": [record], "byte_cap": 200000000}
        self.assertEqual(self.main(manifest, mode=None)[0], 2)
        self.save(record, b"pinned bytes")
        result, output = self.main(manifest, mode=None)
        self.assertEqual(result, 0)
        self.assertIn("ASSETS PASS", output)
        self.network.assert_not_called()

    def test_batch_failure_retains_valid_peer_without_success_summary(self):
        good = self.record(b"good", "good.bin")
        bad = self.record(b"ok", "bad.bin")
        self.network.side_effect = lambda request, **kwargs: Response(
            b"good" if request.full_url.endswith("good.bin") else b"too large")
        output = Console()
        with mock.patch.object(ASSETS, "MANIFEST", {"files": [good, bad], "byte_cap": 200000000}), \
                mock.patch.object(ASSETS.sys, "argv", ["assets.py", "download"]), \
                mock.patch.object(ASSETS.sys, "stdout", output):
            with self.assertRaises(ValueError):
                ASSETS.main()
        self.assertTrue(ASSETS.verify(good))
        self.assertFalse(ASSETS.target(bad).exists())
        self.assertNotIn("ASSETS PASS", output.getvalue())
        self.assert_no_parts()


if __name__ == "__main__":
    unittest.main()
