#!/usr/bin/env python3
"""Tiny offline installation/admission fixtures; no compiler/build invocation."""
import contextlib
import importlib.util
import io
import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location('runner', Path(__file__).with_name('run-education-build.py'))
r = importlib.util.module_from_spec(spec)
spec.loader.exec_module(r)
m = r.m

class Invocation(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.base = Path(self.tmp.name)
        self.external = {'roots': {}, 'managed_components': {}, 'external_interpreters': {}}
        for key in ['IDF_PATH', 'IDF_TOOLS_PATH', 'RUSTUP_HOME', 'TOOL_BIN', 'PYTHON_BIN']:
            root = self.base / key
            root.mkdir(mode=0o700)
            self.external['roots'][key] = {'path': str(root)}
        tools = self.base / 'TOOL_BIN'
        (tools / 'rustup').write_text('proxy bytes')
        (tools / 'rustc').symlink_to('rustup')
        idf = self.base / 'IDF_TOOLS_PATH'
        (idf / 'lib').mkdir()
        (idf / 'lib/module.py').write_text('module')
        (idf / 'lib64').symlink_to('lib')
        self.python = self.base / 'interpreter'
        self.python.write_text('interpreter bytes')
        self.python.chmod(0o700)
        (self.base / 'PYTHON_BIN/python3').symlink_to(self.python)
        self.external['external_interpreters'][str(self.python)] = {'bytes': self.python.stat().st_size, 'sha256': m.sha(self.python.read_bytes())}
        self.refresh()
        cargo = self.base / 'cargo-input'
        (cargo / 'registry/src/local/pkg').mkdir(parents=True)
        (cargo / 'registry/src/local/pkg/lib.rs').write_text('pub fn tiny() {}')
        self.external['cargo_inputs'] = {'path': str(cargo), 'entries': r.inventory(cargo)}

    def refresh(self):
        for item in self.external['roots'].values():
            item['entries'] = r.installation_inventory(Path(item['path']))

    def test_real_shaped_links_and_strict_candidate(self):
        r.validate_external(self.external)
        with self.assertRaises(ValueError):
            m.tree_files(self.base / 'TOOL_BIN')
        (self.base / 'TOOL_BIN/rustup').write_text('changed')
        with self.assertRaises(ValueError):
            r.validate_external(self.external)

    def test_link_text_and_external_escape_rejected(self):
        link = self.base / 'TOOL_BIN/rustc'
        link.unlink()
        link.symlink_to('./rustup')
        with self.assertRaises(ValueError):
            r.validate_external(self.external)
        link.unlink()
        link.symlink_to(self.python)
        self.refresh()
        with self.assertRaises(ValueError):
            r.validate_external(self.external)

    def test_unapproved_interpreter_and_target_drift(self):
        self.python.write_text('different')
        with self.assertRaises(ValueError):
            r.validate_external(self.external)
        self.external['external_interpreters'] = {}
        with self.assertRaises(ValueError):
            r.validate_external(self.external)

    def test_cargo_provision_and_mutation_policy(self):
        home = self.base / 'private'
        home.mkdir(mode=0o700)
        item = self.external['cargo_inputs']
        r.provision_cargo(item, home)
        (home / '.global-cache').write_text('usage')
        r.validate_private_cargo(item, home)
        (home / 'config.toml').write_text('bad')
        with self.assertRaises(ValueError):
            r.validate_private_cargo(item, home)
        (home / 'config.toml').unlink()
        (home / next(iter(item['entries']))).write_text('drift')
        with self.assertRaises(ValueError):
            r.validate_private_cargo(item, home)

    def test_cargo_config_and_links_rejected(self):
        item = self.external['cargo_inputs']
        root = Path(item['path'])
        (root / 'config.toml').write_text('bad')
        item['entries'] = r.inventory(root)
        with self.assertRaises(ValueError):
            r.validate_cargo(item)
        (root / 'config.toml').unlink()
        (root / 'registry/escape').symlink_to(self.python)
        with self.assertRaises(ValueError):
            r.validate_cargo(item)

    def test_main_rejects_recipe_cargo_contamination(self):
        self.test_main_mock_recipe_admission_and_sealing(contaminate=True)

    def test_main_mock_recipe_admission_and_sealing(self, contaminate=False):
        # Mock identity anchor and process only, exercising real main admission,
        # external checks, provisioning, post-check and atomic receipt logic.
        candidate = self.base / 'education-candidate-test'
        source = candidate / 'source'
        source.mkdir(parents=True, mode=0o700)
        (source / 'fixture-firmware').mkdir()
        (source / 'fixture-firmware/input').write_text('tiny')
        entries = r.inventory(source)
        external_file = self.base / 'external.json'
        external_file.write_bytes(m.canonical(self.external))
        class Child:
            pid = 123456789
            returncode = 0
            def wait(self, timeout=None):
                return 0
        def recipe(command, cwd, env, **kwargs):
            self.assertNotIn('BASH_ENV', env)
            self.assertNotIn('CARGO_CONFIG', env)
            r.validate_private_cargo(self.external['cargo_inputs'], Path(env['CARGO_HOME']))
            self.assertTrue(env['PATH'].startswith(str(self.base / 'PYTHON_BIN')))
            build = source / 'fixture-firmware/build'
            for name in r.OUTPUTS:
                target = build / name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_text('mock output')
            (Path(env['EDUCATION_RUN']) / 'effective-environment.json').write_text('{}')
            if contaminate:
                (Path(env['CARGO_HOME']) / 'config.toml').write_text('unapproved')
            return Child()
        with patch.object(m, 'load', return_value={'entries': entries}), patch.object(m, 'git', side_effect=lambda *a: b'' if a[0] == 'status' else b'fixture-revision'), patch.object(r.subprocess, 'Popen', side_effect=recipe), patch.object(r, 'stop_group'), patch.object(sys, 'argv', ['runner', str(candidate), '--external-inputs', str(external_file), '--external-inputs-sha256', m.sha(external_file.read_bytes())]), patch.dict(os.environ, {'BASH_ENV': '/evil', 'CARGO_CONFIG': '/evil'}), contextlib.redirect_stdout(io.StringIO()):
            with self.assertRaises(SystemExit) as result:
                r.main()
        receipt = json.loads((candidate / 'run/build-status.json').read_text())
        if contaminate:
            self.assertEqual(result.exception.code, 1, receipt)
            self.assertEqual(receipt['status'], 'failed')
            self.assertEqual(receipt['artifacts'], {})
            return
        self.assertEqual(result.exception.code, 0, receipt)
        self.assertEqual(receipt['status'], 'local-build-passed')
        self.assertEqual(set(receipt['artifacts']), set(r.OUTPUTS))

if __name__ == '__main__':
    unittest.main()
