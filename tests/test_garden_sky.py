"""Offline integration checks for the native and compatibility sky caches."""
import copy
from contextlib import redirect_stdout
from datetime import datetime, timedelta, timezone
import hashlib
import importlib.util
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest import mock
from urllib.error import URLError
from urllib.parse import parse_qs, urlsplit


SPEC = importlib.util.spec_from_file_location(
    'garden_sky', Path(__file__).resolve().parents[1] / 'scripts/garden_sky.py'
)
SKY = importlib.util.module_from_spec(SPEC)
with mock.patch('urllib.request.urlopen', side_effect=AssertionError('Network during import')):
    SPEC.loader.exec_module(SKY)

DAY = '2001-02-03'


def public_response(day=DAY):
    """Synthetic public-format data, held entirely in memory."""
    start = datetime.fromisoformat(day).replace(tzinfo=timezone.utc)
    rows = [
        [(start + timedelta(hours=i)).strftime('%Y-%b-%d %H:%M'), '', '', str(i * 15), str(i - 12)]
        for i in range(25)
    ]
    return {
        'signature': {'source': 'synthetic-test-fixture', 'version': '1.2'},
        'result': 'Synthetic fixture\n$$SOE\n' + '\n'.join(','.join(row) for row in rows) + '\n$$EOE\n',
    }


class GardenSkyTests(unittest.TestCase):
    def setUp(self):
        self.root = Path(self.enterContext(tempfile.TemporaryDirectory()))
        self.enterContext(mock.patch.object(SKY, 'ROOT', self.root))
        self.urlopen = self.enterContext(mock.patch.object(
            SKY.urllib.request, 'urlopen', side_effect=AssertionError('Unconfigured network request')
        ))
        self.raw = self.root / 'assets/sky/horizons-response.json'
        self.cached = self.root / 'assets/sky/ephemeris.json'
        self.native = self.root / 'Content/sky/ephemeris.json'

    def serve(self, document, status=200):
        data = document if isinstance(document, bytes) else json.dumps(document).encode('utf8')
        response = io.BytesIO(data)
        response.status = status
        self.urlopen.side_effect = None
        self.urlopen.return_value = response
        return data

    def sync(self):
        with redirect_stdout(io.StringIO()):
            return SKY.sync(DAY)

    def seed_old_caches(self):
        old = {}
        for path in (self.raw, self.cached, self.native):
            path.parent.mkdir(parents=True, exist_ok=True)
            # Distinct old bytes ensure rollback restores each file's own version.
            data = json.dumps({
                'retainedCopy': path.relative_to(self.root).as_posix(),
                'fetchedAtUtc': '1999-01-01T00:00:00+00:00',
                'samples': [{'utc': '1998-12-31T00:00:00Z'}],
            }).encode('utf8')
            path.write_bytes(data)
            old[path] = data
        return old

    def assert_preserved(self, old):
        self.assertEqual({path: path.read_bytes() for path in old}, old)
        self.assertEqual(set(path for path in self.root.rglob('*') if path.is_file()), set(old))

    def assert_published(self, output, response):
        self.assertEqual(self.native.read_bytes(), self.cached.read_bytes())
        self.assertEqual(json.loads(self.native.read_bytes()), output)
        self.assertEqual(self.raw.read_bytes(), response)
        self.assertEqual(output['rawResponseSha256'], hashlib.sha256(response).hexdigest())
        self.assertEqual(len(output['samples']), 25)
        first = datetime(2001, 2, 3, tzinfo=timezone.utc)
        for index, sample in enumerate(output['samples']):
            expected = first + timedelta(hours=index)
            self.assertEqual(sample['unixSeconds'], int(expected.timestamp()))
            self.assertEqual(sample['utc'], expected.isoformat().replace('+00:00', 'Z'))
        self.assertEqual(set(path for path in self.root.rglob('*') if path.is_file()),
                         {self.raw, self.cached, self.native})

    def test_valid_25_samples_create_both_caches_without_network(self):
        response = self.serve(public_response())
        before = datetime.now(timezone.utc)
        output = self.sync()
        after = datetime.now(timezone.utc)
        self.assert_published(output, response)
        # Fetch time describes this retrieval; historical sample dates remain historical.
        self.assertLessEqual(before, datetime.fromisoformat(output['fetchedAtUtc']))
        self.assertLessEqual(datetime.fromisoformat(output['fetchedAtUtc']), after)
        self.urlopen.assert_called_once()
        args, kwargs = self.urlopen.call_args
        url = urlsplit(args[0].full_url)
        self.assertEqual((url.scheme, url.netloc, url.path),
                         ('https', 'ssd.jpl.nasa.gov', '/api/horizons.api'))
        params = parse_qs(url.query)
        self.assertEqual(params['START_TIME'], ["'2001-02-03'"])
        self.assertEqual(params['STOP_TIME'], ["'2001-02-04'"])
        self.assertEqual(params['SITE_COORD'], ["'0,45,0'"])
        self.assertEqual(kwargs['timeout'], 30)

    def test_valid_refresh_replaces_existing_copies_with_identical_bytes(self):
        old = self.seed_old_caches()
        document = public_response()
        document['signature']['version'] = '1.3'
        response = self.serve(document)
        output = self.sync()
        self.assert_published(output, response)
        for path, previous in old.items():
            self.assertNotEqual(path.read_bytes(), previous)

    def test_invalid_payloads_leave_all_old_copies_and_dates_unchanged(self):
        valid = public_response()
        bad_signature = copy.deepcopy(valid)
        bad_signature['signature']['version'] = 'unsupported'
        incomplete = copy.deepcopy(valid)
        incomplete['result'] = incomplete['result'].replace('2001-Feb-04 00:00,,,360,12\n', '')
        uneven = copy.deepcopy(valid)
        uneven['result'] = uneven['result'].replace('2001-Feb-03 12:00', '2001-Feb-03 12:30')
        bad_angle = copy.deepcopy(valid)
        bad_angle['result'] = bad_angle['result'].replace(',360,12', ',361,12')
        nonfinite = copy.deepcopy(valid)
        nonfinite['result'] = nonfinite['result'].replace(',360,12', ',NaN,12')
        cases = {
            'invalid_json': b'{',
            'non_object': [],
            'bad_signature': bad_signature,
            'missing_table': {'signature': valid['signature'], 'result': 'no samples'},
            'only_24_samples': incomplete,
            'nonhourly_sample': uneven,
            'out_of_range_angle': bad_angle,
            'nonfinite_angle': nonfinite,
            'different_response_date': public_response('2001-02-02'),
        }
        old = self.seed_old_caches()
        for name, document in cases.items():
            with self.subTest(name=name):
                self.serve(document)
                with self.assertRaises(ValueError):
                    self.sync()
                self.assert_preserved(old)

    def test_invalid_response_creates_no_cache_directories(self):
        self.serve(b'{')
        with self.assertRaises(ValueError):
            self.sync()
        self.assertEqual(list(self.root.iterdir()), [])

    def test_failed_request_keeps_cached_dates_without_relabeling(self):
        old = self.seed_old_caches()
        self.urlopen.side_effect = URLError('Synthetic offline failure')
        with self.assertRaises(URLError):
            self.sync()
        self.assert_preserved(old)

    def test_failed_native_replacement_restores_both_old_copies(self):
        old = self.seed_old_caches()
        self.serve(public_response())
        replace = SKY.os.replace
        failed = False

        def fail_native_once(source, destination):
            nonlocal failed
            if Path(destination) == self.native and not failed:
                failed = True
                raise OSError('Synthetic native replacement failure')
            return replace(source, destination)

        with mock.patch.object(SKY.os, 'replace', side_effect=fail_native_once):
            with self.assertRaisesRegex(OSError, 'Synthetic native replacement failure'):
                self.sync()
        self.assertTrue(failed)
        self.assert_preserved(old)

    def test_failed_first_publication_leaves_no_partial_cache_files(self):
        self.serve(public_response())
        replace = SKY.os.replace

        def fail_native(source, destination):
            if Path(destination) == self.native:
                raise OSError('Synthetic first publication failure')
            return replace(source, destination)

        with mock.patch.object(SKY.os, 'replace', side_effect=fail_native):
            with self.assertRaisesRegex(OSError, 'Synthetic first publication failure'):
                self.sync()
        self.assertEqual([path for path in self.root.rglob('*') if path.is_file()], [])


if __name__ == '__main__':
    unittest.main()
