#!/usr/bin/env python3
"""Regression checks for the finite, pinned Words successor map."""
import json
from pathlib import Path
import runpy
import shutil
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
VERIFY = runpy.run_path(str(ROOT / 'scripts/verify-dice-import.py'))['verify_identities']


class ImportIdentityTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        paths = ['docs/dice-import-manifest.json', 'docs/words-import-manifest.json', 'docs/input-explanations-native-edit.json']
        paths.append('fixture-firmware/tests/explanation_tests.inc')
        paths += [x['destination'] for x in json.loads((ROOT / paths[0]).read_text())['entries']]
        for path in paths:
            target = self.root / path
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / path, target)

    def test_reviewed_successors_and_unchanged_entries(self):
        VERIFY(self.root)

    def test_changed_successor_rejected(self):
        path = self.root / 'fixture-firmware/app/main/compute.h'
        path.write_bytes(path.read_bytes() + b'\n')
        with self.assertRaises(AssertionError):
            VERIFY(self.root)

    def test_changed_historical_entry_rejected(self):
        path = self.root / 'fixture-firmware/app/main/main.c'
        path.write_bytes(path.read_bytes() + b'\n')
        with self.assertRaises(AssertionError):
            VERIFY(self.root)

    def test_manifest_rebaseline_and_path_addition_rejected(self):
        path = self.root / 'docs/words-import-manifest.json'
        original = path.read_bytes()
        for mutation in ('hash', 'path', 'duplicate', 'remove'):
            with self.subTest(mutation=mutation):
                data = json.loads(original)
                if mutation == 'hash':
                    data['entries'][0]['candidate_sha256'] = '0' * 64
                elif mutation == 'path':
                    data['entries'][0]['destination'] = '../unreviewed'
                elif mutation == 'duplicate':
                    data['entries'].append(data['entries'][0])
                else:
                    data['entries'].pop()
                path.write_text(json.dumps(data))
                with self.assertRaises(AssertionError):
                    VERIFY(self.root)

    def test_native_destination_mutation_rejected(self):
        path = self.root / 'fixture-firmware/tests/gui_host.c'
        path.write_bytes(path.read_bytes() + b'\n')
        with self.assertRaises(AssertionError):
            VERIFY(self.root)

    def test_native_test_include_mutation_rejected(self):
        path = self.root / 'fixture-firmware/tests/explanation_tests.inc'
        path.write_bytes(path.read_bytes() + b'\n')
        with self.assertRaises(AssertionError):
            VERIFY(self.root)

    def test_native_manifest_mutations_rejected(self):
        path = self.root / 'docs/input-explanations-native-edit.json'
        original = path.read_bytes()
        for mutation in ('before', 'after', 'prior', 'path', 'duplicate', 'remove'):
            with self.subTest(mutation=mutation):
                data = json.loads(original)
                if mutation == 'before':
                    data['entries'][0]['before_candidate_sha256'] = '0' * 64
                elif mutation == 'after':
                    data['entries'][0]['candidate_sha256'] = '0' * 64
                elif mutation == 'prior':
                    data['prior_manifest_sha256'] = '0' * 64
                elif mutation == 'path':
                    data['entries'][0]['destination'] = '../unreviewed'
                elif mutation == 'duplicate':
                    data['entries'].append(data['entries'][0])
                else:
                    data['entries'].pop()
                path.write_text(json.dumps(data))
                with self.assertRaises(AssertionError):
                    VERIFY(self.root)

    def test_historical_manifest_edit_rejected(self):
        path = self.root / 'docs/dice-import-manifest.json'
        path.write_bytes(path.read_bytes() + b'\n')
        with self.assertRaises(AssertionError):
            VERIFY(self.root)


if __name__ == '__main__':
    unittest.main()
