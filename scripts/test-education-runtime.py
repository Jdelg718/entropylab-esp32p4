#!/usr/bin/env python3
"""Regression for current private allocator, including shared-heap regressions."""
import importlib.util
from pathlib import Path
import unittest

class AllocatorCheck(unittest.TestCase):
    def test_private_heap_calls_required(self):
        p = Path(__file__).with_name('verify-education-runtime.py')
        self.assertTrue(p.exists(), 'successor runtime verifier missing')
        spec = importlib.util.spec_from_file_location('runtime_check', p)
        v = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(v)
        # Minimal objdump-shaped test input, not a build receipt or hardware claim.
        alloc = '48000000: 000000ef jal 48001000 <multi_heap_aligned_alloc>\n'
        free = '48000004: 000000ef jal 48002000 <multi_heap_free>\n'
        v.check_allocator_calls(alloc, free)
        symbols = {'multi_heap_aligned_alloc': '48001000', 'multi_heap_free': '48002000', 'multi_heap_aligned_free': '48002000'}
        alias_free = free.replace('jal ', 'jr ').replace('<multi_heap_free>', '<multi_heap_aligned_free>')
        v.check_allocator_calls(alloc, alias_free, symbols)
        with self.assertRaises(ValueError):
            v.check_allocator_calls(alloc, alias_free, dict(symbols, multi_heap_aligned_free='48003000'))
        for a, f in [(alloc.replace('multi_heap_aligned_alloc', 'heap_caps_aligned_alloc'), free),
                     (alloc, free.replace('multi_heap_free', 'heap_caps_free')),
                     ('', free), (alloc, ''), (alloc + '48000008: 000000ef jal 48003000 <heap_caps_malloc>\n', free)]:
            with self.subTest(alloc=a, free=f), self.assertRaises(ValueError):
                v.check_allocator_calls(a, f)

if __name__ == '__main__':
    unittest.main(verbosity=2)
