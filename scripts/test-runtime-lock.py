#!/usr/bin/env python3
"""Check the effective unified runtime graph, including multiversion edges."""
from pathlib import Path
from collections import Counter
import copy
import tomllib
import unittest
ROOT = Path(__file__).resolve().parents[1] / 'fixture-firmware'
ADAPTERS = {'entropylab-runtime', 'entropylab-mnemonic-core', 'entropylab-dice-core', 'entropylab-coin-core'}
def records(packages):
    return Counter((p['name'], p['version'], p.get('source', ''), p.get('checksum', ''), tuple(sorted(p.get('dependencies', [])))) for p in packages if p['name'] not in ADAPTERS)
def load(path):
    return tomllib.loads(path.read_text())['package']
class RuntimeLockTests(unittest.TestCase):
    def test_effective_graph(self):
        baseline = records(load(ROOT / 'rust/Cargo.lock'))
        # Combined passphrase runtime adds unicode-normalization and its exact
        # transitive graph. Pin the complete installed-source resolution, not
        # the older HEX-only graph (nested HEX/mnemonic checks remain below).
        import hashlib
        self.assertEqual(hashlib.sha256((ROOT / 'runtime/Cargo.lock').read_bytes()).hexdigest(),
                         '1433e9c1c1eed687b69011e2ac067d147d65de31f58e9bb2e194d30d043b7646')
        # Dice's original inert nested lock is preserved, not the resolution owner.
        for core in ['hex-core', 'mnemonic-core']:
            self.assertEqual(records(load(ROOT / f'runtime-sources/{core}/Cargo.lock')), baseline)
    def test_adapter_edges(self):
        actual = {p['name']: sorted(p.get('dependencies', [])) for p in load(ROOT / 'runtime/Cargo.lock') if p['name'] in ADAPTERS}
        self.assertEqual(actual, {
            # Cargo.lock includes the exact bip39 integration-test dev edge.
            'entropylab-runtime': sorted(ADAPTERS - {'entropylab-runtime'} | {'entropylab-hex-core', 'bip39', 'bitcoin', 'secp256k1', 'unicode-normalization'}),
            'entropylab-mnemonic-core': ['bip39', 'entropylab-hex-core'],
            'entropylab-dice-core': ['bitcoin_hashes', 'entropylab-hex-core'],
            'entropylab-coin-core': [],
        })
    def test_deliberate_drift_rejected(self):
        packages = load(ROOT / 'runtime/Cargo.lock')
        baseline = records(packages)
        for i, p in enumerate(packages):
            if p['name'] == 'hex-conservative':
                for field in ['version', 'source', 'checksum', 'dependencies']:
                    changed = copy.deepcopy(packages)
                    changed[i][field] = ['wrong 9.9.9'] if field == 'dependencies' else 'wrong'
                    self.assertNotEqual(records(changed), baseline)
                self.assertNotEqual(records(packages[:i] + packages[i+1:]), baseline)
if __name__ == '__main__':
    unittest.main(verbosity=2)
