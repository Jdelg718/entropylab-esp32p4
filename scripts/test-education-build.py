#!/usr/bin/env python3
"""Read-only checks against a completed local candidate; never mutate evidence."""
import argparse
import importlib.util
import subprocess
import sys
import unittest
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('candidate', type=Path)
args = parser.parse_args()
script = Path(__file__).with_name('verify-education-build.py')
spec = importlib.util.spec_from_file_location('check', script)
v = importlib.util.module_from_spec(spec)
spec.loader.exec_module(v)

class GeneratedChecks(unittest.TestCase):
    def test_failed_build_remains_failure_but_can_be_inspected(self):
        run = subprocess.run([sys.executable, str(script), str(args.candidate), '--inspect-failed'], capture_output=True, text=True)
        self.assertEqual(run.returncode, 2, run.stderr)
        import json
        self.assertTrue(run.stdout.strip(), run.stderr)
        report = json.loads(run.stdout)
        self.assertEqual(report['status'], 'INSPECTED-build-failed')
        self.assertEqual(report['distribution'], 'HOLD')
        self.assertEqual(report['image_checks'], 'PASS')
        self.assertEqual(report['build_exit_code'], 1)

    def test_receipt_inventory_cannot_omit_outputs(self):
        import json
        receipt = json.loads((args.candidate / 'build-status.json').read_text())
        self.assertTrue(hasattr(v, 'receipt_inventory_check'), 'receipt inventory validator missing')
        v.receipt_inventory_check(receipt)
        import copy
        for key in ['artifacts', 'recipe_sha256']:
            bad = copy.deepcopy(receipt)
            bad[key].pop(next(iter(bad[key])))
            with self.subTest(key=key), self.assertRaises(ValueError):
                v.receipt_inventory_check(bad)

    def test_generated_config(self):
        import json
        config = json.loads((args.candidate / 'source/fixture-firmware/build/config/sdkconfig.json').read_text())
        self.assertTrue(hasattr(v, 'config_check'), 'generated config validator missing')
        v.config_check(config)
        for key, bad in [('IDF_TARGET', 'esp32'), ('ESP32P4_REV_MIN_FULL', 0), ('ESPTOOLPY_FLASHSIZE', '4MB'), ('SPIRAM', False)]:
            with self.subTest(key=key), self.assertRaises(ValueError):
                v.config_check(dict(config, **{key: bad}))

    def test_image_negative_cases(self):
        raw = (args.candidate / 'source/fixture-firmware/build/entropylab_fixture.bin').read_bytes()
        self.assertEqual(v.image(raw)['checksum_and_digest'], 'PASS')
        for offset in [0, 1, 12, 15, 23, 28, 128, len(raw)-1]:
            changed = bytearray(raw)
            changed[offset] ^= 0xff
            with self.subTest(offset=offset), self.assertRaises(ValueError):
                v.image(changed)
        for cut in [0, 23, 31, len(raw)-1]:
            with self.subTest(cut=cut), self.assertRaises(ValueError):
                v.image(raw[:cut])
        with self.assertRaises(ValueError):
            v.image(raw + b'\0')

if __name__ == '__main__':
    unittest.main(argv=[sys.argv[0]], verbosity=2)
