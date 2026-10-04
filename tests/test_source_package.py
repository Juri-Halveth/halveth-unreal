"""Check exported bytes, excluded private state, asset integrity and reproducibility."""
import base64
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("package_source", ROOT / "scripts/package_source.py")
EXPORT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(EXPORT)
PNG = base64.b64decode("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mP8/x8AAusB9Wl6xXkAAAAASUVORK5CYII=")


def fixture(root: Path):
    for folder in EXPORT.TREE_EXTENSIONS:
        (root / folder).mkdir(parents=True)
    for name in EXPORT.ROOT_FILES | EXPORT.EXACT_EXTRA_FILES:
        path = root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("public fixture\n", encoding="utf-8")
    (root / "Config/DefaultGame.ini").write_text(
        "[/Script/EngineSettings.GeneralProjectSettings]\nProjectVersion=1.2.3\n", encoding="utf-8")
    (root / "Source/Layout.h").write_text("// original fixture source\n", encoding="utf-8")
    art = root / "ArtSource/PolyHaven"
    (art / "test_asset").mkdir(parents=True)
    image = art / "test_asset/color.png"
    image.write_bytes(PNG)
    manifest = {"license": "CC0-1.0", "filesCount": 1, "downloadedMapBytes": len(PNG),
                "files": [{"relativePath": "test_asset/color.png", "bytes": len(PNG),
                           "expectedBytes": len(PNG), "sha256": hashlib.sha256(PNG).hexdigest(),
                           "width": 1, "height": 1}]}
    (art / "MANIFEST.json").write_text(json.dumps(manifest), encoding="utf-8")
    return image


class SourcePackageTests(unittest.TestCase):
    def test_character_source_is_exported_and_changed_bytes_are_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            fixture(root)
            art = root / 'ArtSource/Characters'
            art.mkdir()
            fbx = art / 'Guide.fbx'
            fbx.write_bytes(b'BOUND_CHARACTER_SOURCE')
            inventory = {'license': 'CC0-1.0', 'files': [
                {'path': 'Guide.fbx', 'bytes': fbx.stat().st_size,
                 'sha256': hashlib.sha256(fbx.read_bytes()).hexdigest()}]}
            (art / 'MANIFEST.json').write_text(json.dumps(inventory), encoding='utf8')
            files, manifest = EXPORT.snapshot(root)
            self.assertEqual(files['ArtSource/Characters/Guide.fbx'], fbx.read_bytes())
            self.assertIn('GARDEN.sh', files)
            fbx.write_bytes(b'CHANGED_CHARACTER_SOURCE')
            with self.assertRaisesRegex(EXPORT.ExportError, 'Character source byte/hash mismatch'):
                EXPORT.snapshot(root)

    def test_archive_contains_original_source_but_no_runtime_or_engine_data(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            fixture(root)
            for name in ["Saved/private.log", "Binaries/game.exe", "Content/import.uasset", ".git/config",
                         "Intermediate/cache.bin", "Builds/game.exe", "DerivedDataCache/cache.bin",
                         "Engine/Source/vendor.cpp", "Build/Windows/private.ini", "docs/.private.json",
                         "assets/credentials.pem", "scripts/__pycache__/secret.pyc"]:
                target = root / name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_bytes(b"PRIVATE_SENTINEL_DO_NOT_EXPORT")
            files, manifest = EXPORT.snapshot(root)
            archive, digest = EXPORT.write_archive(root, files, manifest, "1.2.3")
            with zipfile.ZipFile(archive) as bundle:
                self.assertIn("HALVETH-Unreal/Source/Layout.h", bundle.namelist())
                self.assertIn("HALVETH-Unreal/SOURCE-MANIFEST.json", bundle.namelist())
                self.assertEqual(bundle.read("HALVETH-Unreal/Source/Layout.h"), (root / "Source/Layout.h").read_bytes())
                for name in bundle.namelist():
                    self.assertNotIn(b"PRIVATE_SENTINEL_DO_NOT_EXPORT", bundle.read(name))
                exported = json.loads(bundle.read("HALVETH-Unreal/SOURCE-MANIFEST.json"))
                for item in exported["files"]:
                    self.assertEqual(item["sha256"], hashlib.sha256(bundle.read("HALVETH-Unreal/" + item["path"])).hexdigest())
            self.assertEqual(digest, hashlib.sha256(archive.read_bytes()).hexdigest())
            again, repeated_digest = EXPORT.write_archive(root, files, manifest, "1.2.3")
            self.assertEqual((again, repeated_digest), (archive, digest))

    def test_rejects_public_credential_without_echoing_value(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            fixture(root)
            sentinel = "UNPUBLISHED_TEST_CREDENTIAL_VALUE"
            (root / "Config/private.ini").write_text("Security" + "Token=" + sentinel)
            with self.assertRaises(EXPORT.ExportError) as caught:
                EXPORT.snapshot(root)
            self.assertIn("credential_assignment", str(caught.exception))
            self.assertNotIn(sentinel, str(caught.exception))
            self.assertFalse((root / "dist").exists())

    def test_rejects_absolute_personal_path_in_public_text(self):
        value = "C:" + chr(92) + "Users" + chr(92) + "Example" + chr(92) + "file.dat"
        with self.assertRaisesRegex(EXPORT.ExportError, "absolute_user_directory"):
            EXPORT.validate_text("docs/import.json", json.dumps({"path": value}).encode())

    def test_changed_texture_bytes_reject_entire_export(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            image = fixture(root)
            changed = bytearray(image.read_bytes())
            changed[-1] ^= 1
            image.write_bytes(changed)
            with self.assertRaisesRegex(EXPORT.ExportError, "byte/hash mismatch"):
                EXPORT.snapshot(root)

    def test_unreviewed_art_png_rejects_export(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            fixture(root)
            (root / "ArtSource/unreviewed.png").write_bytes(PNG)
            with self.assertRaisesRegex(EXPORT.ExportError, "Every source-art PNG"):
                EXPORT.snapshot(root)

    def test_manifest_traversal_is_rejected(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            fixture(root)
            path = root / "ArtSource/PolyHaven/MANIFEST.json"
            manifest = json.loads(path.read_text())
            manifest["files"][0]["relativePath"] = "../private.png"
            path.write_text(json.dumps(manifest))
            with self.assertRaisesRegex(EXPORT.ExportError, "Invalid source-art manifest path"):
                EXPORT.snapshot(root)

    def test_linked_public_file_cannot_export_an_outside_file(self):
        with tempfile.TemporaryDirectory() as temporary, tempfile.TemporaryDirectory() as outside:
            root = Path(temporary)
            fixture(root)
            outside_file = Path(outside) / "private.md"
            outside_file.write_text("outside data")
            try:
                (root / "docs/link.md").symlink_to(outside_file)
            except OSError:
                self.skipTest("Creating symlinks is unavailable for this test process")
            with self.assertRaisesRegex(EXPORT.ExportError, "Linked source entries"):
                EXPORT.snapshot(root)

    def test_output_budget_is_enforced_before_writing_an_archive(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            fixture(root)
            with self.assertRaisesRegex(EXPORT.ExportError, "byte budget"):
                EXPORT.snapshot(root, maximum_bytes=10)
            self.assertFalse((root / "dist").exists())


if __name__ == "__main__":
    unittest.main()
