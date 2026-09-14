#!/usr/bin/env python3
"""Portable wrapper regression: execute the public HEX C ABI harness."""
from pathlib import Path
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[1]

class HexIntegration(unittest.TestCase):
    def test_public_256_bit_inputs(self):
        harness = ROOT / 'fixture-firmware/build/host-test'
        for value, ending, fingerprint in [('0' * 64, 'art', '5436d724'),
                                           ('0' * 63 + 'F', 'ability trash', '53f6b5aa')]:
            result = subprocess.run([str(harness), value], text=True, capture_output=True, check=True)
            fields = result.stdout.strip().split('\t')
            self.assertEqual(len(fields), 3, 'harness must execute HEX, not old compiled fixture')
            mnemonic, actual, address = fields
            self.assertEqual(len(mnemonic.split()), 24)
            self.assertTrue(mnemonic.endswith(ending))
            self.assertEqual(actual, fingerprint)
            self.assertTrue(address.startswith('bc1q'))

if __name__ == '__main__':
    unittest.main()
