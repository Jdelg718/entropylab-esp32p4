#!/usr/bin/env python3
"""Semantic negative tests do not rely on a stale outer manifest hash."""
import copy
import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('successor', Path(__file__).with_name('education-successor.py'))
m = importlib.util.module_from_spec(spec)
spec.loader.exec_module(m)


class Contract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.trusted = m.expected()

    def test_valid_serialization(self):
        import json
        m.validate_manifest(json.loads(json.dumps(self.trusted)), self.trusted)
        m.load()

    def test_semantic_mutations(self):
        mutations = [lambda d: d.update(source_revision='0' * 40),
                     lambda d: d.update(accepted_archive_sha256='0' * 64),
                     lambda d: d.update(firmware_changes=[]),
                     lambda d: d['firmware_changes'][0].update(before_sha256='0' * 64),
                     lambda d: d['entries'].pop(m.CHANGED),
                     lambda d: d['entries'].update({'../escape': {'bytes': 0, 'sha256': m.sha(b'')}}),
                     lambda d: d['entries'][m.CHANGED].update(sha256='0' * 64),
                     lambda d: d.update(status='accepted')]
        for mutate in mutations:
            with self.subTest(mutate=mutate):
                data = copy.deepcopy(self.trusted)
                mutate(data)
                with self.assertRaises(ValueError):
                    m.validate_manifest(data, self.trusted)

    def test_checkout_matches(self):
        m.validate_tree(m.ROOT, self.trusted['entries'])

    def test_bytes_extra_and_symlink_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            root = Path(d)
            p = root / 'input'
            p.write_bytes(b'known')
            entries = {'input': {'bytes': 5, 'sha256': m.sha(b'known')}}
            m.validate_tree(root, entries, exact=True)
            p.write_bytes(b'other')
            with self.assertRaises(ValueError):
                m.validate_tree(root, entries)
            p.write_bytes(b'known')
            (root / 'extra').write_bytes(b'')
            with self.assertRaises(ValueError):
                m.validate_tree(root, entries, exact=True)
            p.unlink()
            p.symlink_to('extra')
            with self.assertRaises(ValueError):
                m.validate_tree(root, entries)


if __name__ == '__main__':
    unittest.main()
