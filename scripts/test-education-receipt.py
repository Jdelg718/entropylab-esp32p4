#!/usr/bin/env python3
"""Synthetic tiny fixtures only: not firmware/build evidence."""
import copy
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('receipt', Path(__file__).with_name('education-receipt.py'))
r = importlib.util.module_from_spec(spec)
spec.loader.exec_module(r)

class Receipts(unittest.TestCase):
    def fixture(self, d, version):
        schema = 'education-local-build-v' + str(version)
        work = d / r.SCHEMAS[schema][0]
        work.mkdir(exist_ok=True)
        rev = r.m.git('rev-parse', '91b10e8' if version == 1 else '0ec7f6d').decode().strip()
        receipt = dict(schema=schema, recipe_revision=rev, recipe_sha256={n: r.m.sha(r.m.git('show', rev + ':scripts/' + n)) for n in r.SCHEMAS[schema][1]}, artifacts={}, status='failed' if version == 1 else 'local-build-passed', exit_code=1 if version == 1 else 0, child_exit_code=1 if version == 1 else 0, finished='synthetic-finished', post_source_check='PASS', process_group_quiescent=True)
        for name, key in [('build.log','log_sha256'), ('exports.sh','exports_sha256')]:
            (work / name).write_bytes(b'fixture')
            receipt[key] = r.m.sha(b'fixture')
        for name in r.ARTIFACTS:
            p = d / 'source/fixture-firmware/build' / name
            p.parent.mkdir(parents=True, exist_ok=True)
            p.write_bytes(b'fixture')
            receipt['artifacts'][name] = dict(bytes=7, sha256=r.m.sha(b'fixture'))
        (work / 'build-status.json').write_text(json.dumps(receipt))
        return receipt, work

    def test_schema_positive_and_failure_truth(self):
        for version in [1, 2]:
            with tempfile.TemporaryDirectory() as tmp:
                d = Path(tmp); self.fixture(d, version)
                self.assertEqual(r.load(d, True)[3], version == 2)
                if version == 1:
                    with self.assertRaises(ValueError): r.load(d)
                else: self.assertTrue(r.load(d)[3])

    def test_mutations(self):
        for version in [1, 2]:
            for mutation in ['wrong', 'missing', 'extra', 'mixed', 'missing-artifact', 'unknown-schema', 'running', 'exit-mismatch', 'wrong-layout']:
                with self.subTest(version=version, mutation=mutation), tempfile.TemporaryDirectory() as tmp:
                    d = Path(tmp); rec, work = self.fixture(d, version)
                    name = next(iter(rec['recipe_sha256']))
                    if mutation == 'wrong': rec['recipe_sha256'][name] = '0' * 64
                    if mutation == 'missing': del rec['recipe_sha256'][name]
                    if mutation == 'extra': rec['recipe_sha256']['extra.py'] = '0' * 64
                    if mutation == 'mixed':
                        other = d / 'run' if version == 1 else d
                        other.mkdir(exist_ok=True); (other / 'exports.sh').write_text('mixed')
                    if mutation == 'missing-artifact': (d / 'source/fixture-firmware/build/entropylab_fixture.elf').unlink()
                    if mutation == 'unknown-schema': rec['schema'] = 'unknown'
                    if mutation == 'running': rec['status'] = 'running'
                    if mutation == 'exit-mismatch': rec.update(status='local-build-passed', exit_code=1)
                    if mutation == 'wrong-layout': rec['schema'] = 'education-local-build-v' + str(3-version)
                    (work / 'build-status.json').write_text(json.dumps(rec))
                    with self.assertRaises((ValueError, FileNotFoundError)): r.load(d, True)

if __name__ == '__main__': unittest.main(verbosity=2)
