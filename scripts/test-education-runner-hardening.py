#!/usr/bin/env python3
"""Small, offline adversarial fixtures; never invokes the target recipe."""
import importlib.util
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time
import unittest

spec = importlib.util.spec_from_file_location('runner', Path(__file__).with_name('run-education-build.py'))
r = importlib.util.module_from_spec(spec)
spec.loader.exec_module(r)
m = r.m

class Hardening(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)

    def test_cargo_generated_cache_tag(self):
        (self.root / 'registry').mkdir()
        tag = self.root / 'registry/CACHEDIR.TAG'
        tag.write_bytes(b'Signature: 8a477f597d28d172789f06886806bc55\n# This file is a cache directory tag created by cargo.\n# For information about cache directory tags see https://bford.info/cachedir/\n')
        r.validate_private_cargo({'entries': {}}, self.root)
        tag.write_bytes(b'injected')
        with self.assertRaises(ValueError):
            r.validate_private_cargo({'entries': {}}, self.root)

    def test_glob_extra_rejected(self):
        (self.root / 'a.cpp').write_bytes(b'a')
        entries = {'a.cpp': {'bytes': 1, 'sha256': m.sha(b'a')}}
        r.validate_inputs(self.root, entries, {})
        (self.root / 'injected.cpp').write_bytes(b'injected')
        with self.assertRaises(ValueError):
            r.validate_inputs(self.root, entries, {})

    def test_directory_and_dangling_links_rejected(self):
        for target in [self.root.parent, self.root / 'absent']:
            p = self.root / 'extra'
            p.symlink_to(target)
            with self.assertRaises(ValueError):
                m.validate_tree(self.root, {}, exact=True)
            p.unlink()

    def test_write_links_and_existing_files(self):
        victim = self.root / 'victim'
        victim.write_bytes(b'preserve')
        for name in ['exports.sh', 'build.log', 'managed-inputs.json', 'build-status.json']:
            p = self.root / name
            p.symlink_to(victim)
            with self.assertRaises((ValueError, OSError)):
                m.exclusive_write(p, b'bad')
            self.assertEqual(victim.read_bytes(), b'preserve')
            p.unlink()
        with self.assertRaises((ValueError, OSError)):
            m.exclusive_write(victim, b'bad')

    def test_symlink_parent_before_creation(self):
        link = self.root / 'link'
        link.symlink_to(self.root, target_is_directory=True)
        with self.assertRaises(ValueError):
            m.fresh_directory(link / 'education-candidate-test')
        self.assertFalse((self.root / 'education-candidate-test').exists())

    def test_env_is_allowlist_and_private(self):
        old = dict(os.environ)
        self.addCleanup(lambda: (os.environ.clear(), os.environ.update(old)))
        os.environ.update(BASH_ENV='/evil', RUSTC_WRAPPER='/evil', CMAKE_PREFIX_PATH='/evil')
        env = r.build_environment(self.root, {})
        self.assertNotIn('BASH_ENV', env)
        self.assertNotIn('RUSTC_WRAPPER', env)
        self.assertNotIn('CMAKE_PREFIX_PATH', env)
        for key in ['HOME', 'TMPDIR', 'CARGO_HOME', 'IDF_COMPONENT_CACHE_PATH']:
            self.assertTrue(Path(env[key]).is_relative_to(self.root))
            self.assertEqual(Path(env[key]).stat().st_mode & 0o777, 0o700)

    def test_timeout_kills_term_ignoring_descendant(self):
        marker = self.root / 'late'
        child_code = "import signal,time,pathlib; signal.signal(signal.SIGTERM,signal.SIG_IGN); time.sleep(1); pathlib.Path(%r).write_text('bad'); time.sleep(5)" % str(marker)
        code = "import subprocess,sys,time; subprocess.Popen([sys.executable,'-c',%r]); time.sleep(10)" % child_code
        child = subprocess.Popen([sys.executable, '-c', code], start_new_session=True)
        with self.assertRaises(subprocess.TimeoutExpired):
            child.wait(timeout=.2)
        r.stop_group(child, grace=.1)
        time.sleep(1.1)
        self.assertFalse(marker.exists())
        self.assertIsNotNone(child.returncode)

    def test_atomic_receipt_preserves_old_on_serialization_error(self):
        p = self.root / 'build-status.json'
        r.atomic_status(p, {'status': 'running'})
        before = p.read_bytes()
        with self.assertRaises(TypeError):
            r.atomic_status(p, {'bad': object()})
        self.assertEqual(p.read_bytes(), before)
        r.atomic_status(p, {'status': 'failed'})
        self.assertIn(b'failed', p.read_bytes())

if __name__ == '__main__':
    unittest.main()
