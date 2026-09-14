#!/usr/bin/env python3
"""The sole Dice/HEX runtime must preserve the repository HEX dependency graph."""
from pathlib import Path
import tomllib
import unittest
from collections import Counter

ROOT = Path(__file__).resolve().parents[1]


def inherited_records(packages):
    # Full identity, never a name-keyed dict: both hex-conservative versions matter.
    return Counter((p['name'], p['version'], p.get('source', ''),
                    p.get('checksum', ''), tuple(sorted(p.get('dependencies', []))))
                   for p in packages if p['name'] != 'entropylab-dice-core')


class EffectiveLockTests(unittest.TestCase):
    def test_effective_hex_graph_exact(self):
        def load(core):
            return tomllib.loads((ROOT / f'fixture-firmware/{core}/Cargo.lock').read_text())['package']
        expected, actual = inherited_records(load('rust')), inherited_records(load('dice'))
        self.assertEqual(actual, expected,
                         f'Effective HEX dependency drift: missing={list((expected-actual).elements())}; '
                         f'unexpected={list((actual-expected).elements())}')

    def test_multiversion_checksum_and_edges_are_not_collapsed(self):
        packages = [dict(name='same', version=v, source='registry', checksum=v,
                         dependencies=['child 1.0']) for v in ['0.2.2', '1.2.0']]
        baseline = inherited_records(packages)
        for index in range(2):
            for field in ['version', 'source', 'checksum', 'dependencies']:
                changed = [dict(p) for p in packages]
                changed[index][field] = ['child 2.0'] if field == 'dependencies' else 'drift'
                self.assertNotEqual(baseline, inherited_records(changed))
        self.assertNotEqual(baseline, inherited_records(packages[1:]))


if __name__ == '__main__':
    unittest.main(verbosity=2)
