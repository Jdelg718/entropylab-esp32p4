#!/usr/bin/env python3
"""Finite reviewed saver successor: raw pins AND refreshed-pin semantics."""
import hashlib
import json
from pathlib import Path
import shutil
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / 'scripts/verify-dice-import.py'
SOURCE = SCRIPT.read_text().split('verify_identities(r)')[0]
MANIFEST = 'docs/global-saver-successors.json'
ORIGINAL = (ROOT / MANIFEST).read_bytes()
PIN = hashlib.sha256(ORIGINAL).hexdigest()


class GlobalSaverIdentityTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        # Same finite manifest/destination inputs as the D6 provenance gate.
        paths = set()
        for p in (ROOT / 'docs').glob('*.json'):
            paths.add(p.relative_to(ROOT).as_posix())
            data = json.loads(p.read_text())
            if isinstance(data, dict):
                paths.update(e['destination'] for e in data.get('entries', [])
                             if 'destination' in e)
        for name in paths:
            src = ROOT / name
            if src.is_file():
                dst = self.root / name
                dst.parent.mkdir(parents=True, exist_ok=True)
                shutil.copyfile(src, dst)

    def verify(self, refresh=False):
        source = SOURCE
        if refresh:
            self.assertEqual(source.count(PIN), 1)
            source = source.replace(PIN, hashlib.sha256(
                (self.root / MANIFEST).read_bytes()).hexdigest())
        ns = {'__file__': str(SCRIPT)}
        exec(compile(source, str(SCRIPT), 'exec'), ns)
        ns['verify_identities'](self.root)

    def mutate(self, change, error):
        data = json.loads(ORIGINAL)
        change(data)
        (self.root / MANIFEST).write_text(json.dumps(data))
        with self.assertRaisesRegex(AssertionError, error):
            self.verify(refresh=True)

    def test_exact(self):
        self.verify()

    def test_serialization_control(self):
        (self.root / MANIFEST).write_text(json.dumps(json.loads(ORIGINAL), sort_keys=True))
        self.verify(refresh=True)

    def test_manifest_bytes(self):
        (self.root / MANIFEST).write_bytes(ORIGINAL + b'\n')
        with self.assertRaisesRegex(AssertionError, 'unreviewed global-saver'):
            self.verify()

    def test_all_four_current_files(self):
        for entry in json.loads(ORIGINAL)['entries']:
            with self.subTest(path=entry['destination']):
                p = self.root / entry['destination']
                old = p.read_bytes()
                p.write_bytes(old + b'\n')
                with self.assertRaises(AssertionError):
                    self.verify()
                p.write_bytes(old)

    def test_missing_added_include(self):
        (self.root / 'fixture-firmware/tests/global_saver_tests.inc').unlink()
        with self.assertRaisesRegex(AssertionError, 'global-saver missing file'):
            self.verify()

    def test_every_entry_semantics(self):
        for i, entry in enumerate(json.loads(ORIGINAL)['entries']):
            cases = [('before_sha256', '0' * 64, 'global-saver predecessor'),
                     ('candidate_sha256', '0' * 64, 'global-saver reviewed candidate'),
                     ('classification', 'invalid', 'global-saver classification'),
                     ('operation', 'add' if entry['operation'] == 'modify' else 'modify', 'global-saver operation'),
                     ('reason', '', 'global-saver reason'),
                     ('reason', '  ', 'global-saver reason'),
                     ('reason', None, 'global-saver reason')]
            if entry['operation'] == 'modify':
                cases.append(('before_sha256', None, 'global-saver predecessor'))
            for field, value, error in cases:
                with self.subTest(entry=i, field=field, value=value):
                    self.mutate(lambda d: d['entries'][i].update({field: value}), error)

    def test_schema_and_prior(self):
        for field, value, error in [
            ('schema', 'unknown-v1', 'global-saver schema'),
            ('prior_manifest', 'docs/review-repair-successors.json', 'global-saver prior'),
            ('prior_manifest_sha256', '0' * 64, 'global-saver prior')]:
            with self.subTest(field=field):
                self.mutate(lambda d: d.update({field: value}), error)

    def test_exact_inventory(self):
        for change in [lambda d: d['entries'].reverse(),
                       lambda d: d['entries'].pop(),
                       lambda d: d['entries'].append(d['entries'][0]),
                       lambda d: d['entries'][2].update(destination='fixture-firmware/tests/unknown.inc'),
                       lambda d: d['entries'][0].update(destination='../escape'),
                       lambda d: d['entries'][0].update(destination='/tmp/escape')]:
            self.mutate(change, 'global-saver destination/order')

    def test_unknown_fields(self):
        self.mutate(lambda d: d.update(accept_unreviewed=True), 'global-saver fields')
        self.mutate(lambda d: d['entries'][0].update(accept_unreviewed=True), 'global-saver entry fields')

    def test_matching_file_and_refreshed_candidate_still_rejected(self):
        for i, entry in enumerate(json.loads(ORIGINAL)['entries']):
            with self.subTest(entry=i):
                p = self.root / entry['destination']
                old = p.read_bytes()
                p.write_bytes(old + b'\n')
                digest = hashlib.sha256(p.read_bytes()).hexdigest()
                self.mutate(lambda d: d['entries'][i].update(candidate_sha256=digest),
                            'global-saver reviewed candidate')
                p.write_bytes(old)


if __name__ == '__main__':
    unittest.main()
