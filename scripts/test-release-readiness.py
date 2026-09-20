#!/usr/bin/env python3
"""Mutation tests for the offline release evidence/documentation contract."""
import importlib.util
import json
from pathlib import Path
import shutil
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('readiness', ROOT / 'scripts/check-release-readiness.py')
checker = importlib.util.module_from_spec(spec)
spec.loader.exec_module(checker)


class ReadinessTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        paths = ['evidence/release-readiness.json', *checker.EVIDENCE, *checker.DOCUMENTS]
        for name in paths:
            dst = self.root / name
            dst.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / name, dst)

    def ledger(self, mutate):
        path = self.root / 'evidence/release-readiness.json'
        data = json.loads(path.read_text())
        mutate(data)
        path.write_text(json.dumps(data))

    def test_current_snapshot(self):
        checker.check(self.root)

    def test_evidence_drift(self):
        for name in checker.EVIDENCE:
            with self.subTest(name=name):
                path = self.root / name
                original = path.read_bytes()
                path.write_bytes(original + b'\n')
                with self.assertRaisesRegex(ValueError, 'evidence drift'):
                    checker.check(self.root)
                path.write_bytes(original)

    def test_missing_evidence(self):
        (self.root / 'evidence/physical-acceptance.md').unlink()
        with self.assertRaisesRegex(ValueError, 'missing evidence'):
            checker.check(self.root)

    def test_bad_runtime(self):
        self.ledger(lambda d: d.update(runtime_commit='0' * 40))
        with self.assertRaisesRegex(ValueError, 'runtime'):
            checker.check(self.root)

    def test_promoted_speed_claim(self):
        self.ledger(lambda d: d['gates'].update(physical_speed='accepted'))
        with self.assertRaisesRegex(ValueError, 'gate contract'):
            checker.check(self.root)

    def test_removed_gate(self):
        self.ledger(lambda d: d['gates'].pop('recovery'))
        with self.assertRaisesRegex(ValueError, 'gate contract'):
            checker.check(self.root)

    def test_document_status_drift(self):
        for name in checker.DOCUMENTS:
            with self.subTest(name=name):
                path = self.root / name
                original = path.read_text()
                path.write_text(original.replace('| physical_speed | not_measured |', '| physical_speed | accepted |'))
                with self.assertRaisesRegex(ValueError, 'documentation gate summary'):
                    checker.check(self.root)
                path.write_text(original)

    def test_duplicate_summary(self):
        path = self.root / checker.DOCUMENTS[0]
        path.write_text(path.read_text() + checker.summary(checker.GATES))
        with self.assertRaisesRegex(ValueError, 'documentation gate summary'):
            checker.check(self.root)

    def test_unknown_evidence_path(self):
        self.ledger(lambda d: d['evidence_sha256'].update({'../private': '0' * 64}))
        with self.assertRaisesRegex(ValueError, 'evidence path set'):
            checker.check(self.root)


if __name__ == '__main__':
    unittest.main()
