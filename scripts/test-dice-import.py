#!/usr/bin/env python3
"""Regression checks for the finite, pinned Words successor map."""
import hashlib
import json
from pathlib import Path
import runpy
import shutil
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
VERIFY_PATH = ROOT / 'scripts/verify-dice-import.py'
VERIFY_SOURCE = VERIFY_PATH.read_text()
ORIGINAL_CHECKOUT_PIN = '1e2206cd513084284192a93e615bfe0bdf9e63db875517fc18a7b48ed18a5140'
VERIFY = runpy.run_path(str(VERIFY_PATH))['verify_identities']
EXPECTED_CLASSIFICATIONS = {
    'fixture-firmware/app/main/gui.c': 'production-source',
    'fixture-firmware/app/main/gui08_native.c': 'production-source',
    'fixture-firmware/app/main/mnemonic_editor.inc': 'production-source',
    'fixture-firmware/app/main/navigation.inc': 'production-source',
    'fixture-firmware/tests/gui_host.c': 'host-test-only',
    'fixture-firmware/tests/layout_tests.inc': 'host-test-only',
    'fixture-firmware/tests/modal_touch_tests.inc': 'host-test-only',
    'fixture-firmware/tests/explanation_tests.inc': 'host-test-only',
}


def validator_with_refreshed_checkout_pin(checkout_sha256):
    """Load the validator with only its exact successor-byte pin refreshed."""
    source = VERIFY_SOURCE.replace(ORIGINAL_CHECKOUT_PIN, checkout_sha256)
    source = source[:source.index('verify_identities(r)')]
    namespace = {'__file__': str(VERIFY_PATH)}
    exec(compile(source, str(VERIFY_PATH), 'exec'), namespace)
    return namespace['verify_identities']


class ImportIdentityTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        paths = ['docs/dice-import-manifest.json', 'docs/words-import-manifest.json', 'docs/input-explanations-native-edit.json']
        paths.append('fixture-firmware/tests/explanation_tests.inc')
        paths.append('docs/combined-import-successors.json')
        paths.append('docs/ui-repair-successors.json')
        paths.append('docs/public-checkout-successors.json')
        paths.append('docs/d6-release-successors.json')
        paths.append('docs/review-repair-successors.json')
        paths.append('docs/modal-successors.json')
        paths.append('docs/global-saver-successors.json')
        paths += [x['destination'] for x in json.loads((ROOT / 'docs/global-saver-successors.json').read_text())['entries']]
        paths += [x['destination'] for x in json.loads((ROOT / 'docs/review-repair-successors.json').read_text())['entries']]
        paths += [x['destination'] for x in json.loads((ROOT / 'docs/d6-release-successors.json').read_text())['entries']]
        paths += [x['destination'] for x in json.loads((ROOT / 'docs/ui-repair-successors.json').read_text())['entries']]
        paths += [x['destination'] for x in json.loads((ROOT / paths[0]).read_text())['entries']]
        for path in paths:
            target = self.root / path
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / path, target)

    def validator_for_relabel(self, destination, classification):
        path = self.root / 'docs/public-checkout-successors.json'
        data = json.loads(path.read_text())
        for entry in data['entries']:
            entry['classification'] = EXPECTED_CLASSIFICATIONS[entry['destination']]
        next(entry for entry in data['entries'] if entry['destination'] == destination)['classification'] = classification
        path.write_text(json.dumps(data, indent=2) + '\n')
        refreshed_pin = hashlib.sha256(path.read_bytes()).hexdigest()
        return validator_with_refreshed_checkout_pin(refreshed_pin)

    def test_production_to_test_relabel_rejected_after_byte_pin_refresh(self):
        validator = self.validator_for_relabel('fixture-firmware/app/main/gui.c', 'host-test-only')
        with self.assertRaisesRegex(AssertionError, 'classification/path mismatch'):
            validator(self.root)

    def test_test_to_production_relabel_rejected_after_byte_pin_refresh(self):
        validator = self.validator_for_relabel('fixture-firmware/tests/gui_host.c', 'production-source')
        with self.assertRaisesRegex(AssertionError, 'classification/path mismatch'):
            validator(self.root)

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

    def test_every_ui_repair_destination_drift_rejected(self):
        data = json.loads((self.root / 'docs/ui-repair-successors.json').read_text())
        for entry in data['entries']:
            with self.subTest(path=entry['destination']):
                path = self.root / entry['destination']
                original = path.read_bytes()
                path.write_bytes(original + b'\n')
                with self.assertRaises(AssertionError):
                    VERIFY(self.root)
                path.write_bytes(original)

    def test_ui_and_combined_manifest_mutations_rejected(self):
        for name in ('ui-repair-successors', 'combined-import-successors'):
            path = self.root / ('docs/' + name + '.json')
            original = path.read_bytes()
            for mutation in ('before', 'after', 'path', 'duplicate', 'remove'):
                with self.subTest(manifest=name, mutation=mutation):
                    data = json.loads(original)
                    if mutation == 'before':
                        data['entries'][0]['before_sha256'] = '0' * 64
                    elif mutation == 'after':
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
            path.write_bytes(original)

    def test_every_checkout_successor_drift_rejected(self):
        data = json.loads((self.root / 'docs/public-checkout-successors.json').read_text())
        for entry in data['entries']:
            with self.subTest(path=entry['destination']):
                path = self.root / entry['destination']
                original = path.read_bytes()
                path.write_bytes(original + b'\n')
                with self.assertRaises(AssertionError):
                    VERIFY(self.root)
                path.write_bytes(original)

    def test_checkout_manifest_mutations_rejected(self):
        path = self.root / 'docs/public-checkout-successors.json'
        original = path.read_bytes()
        for mutation in ('before', 'after', 'prior', 'native', 'path', 'duplicate', 'remove'):
            with self.subTest(mutation=mutation):
                data = json.loads(original)
                if mutation in ('before', 'after'):
                    data['entries'][0]['before_sha256' if mutation == 'before' else 'candidate_sha256'] = '0' * 64
                elif mutation in ('prior', 'native'):
                    data['prior_manifest_sha256' if mutation == 'prior' else 'native_manifest_sha256'] = '0' * 64
                elif mutation == 'path':
                    data['entries'][0]['destination'] = '../unreviewed'
                elif mutation == 'duplicate':
                    data['entries'].append(data['entries'][0])
                else:
                    data['entries'].pop()
                path.write_text(json.dumps(data))
                with self.assertRaisesRegex(AssertionError, 'unreviewed checkout successor manifest'):
                    VERIFY(self.root)
        path.write_bytes(original)

    def test_historical_manifest_edit_rejected(self):
        path = self.root / 'docs/dice-import-manifest.json'
        path.write_bytes(path.read_bytes() + b'\n')
        with self.assertRaises(AssertionError):
            VERIFY(self.root)


if __name__ == '__main__':
    unittest.main()
