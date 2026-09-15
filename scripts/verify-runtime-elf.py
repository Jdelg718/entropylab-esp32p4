#!/usr/bin/env python3
"""Unified source runtime final-ELF gate; not a build-receipt verifier."""
from pathlib import Path
import subprocess
import re
r = Path(__file__).resolve().parents[1] / 'fixture-firmware'
elf = r / 'build/entropylab_fixture.elf'
archive = r / 'runtime/target/riscv32imafc-esp-espidf/release/libentropylab_runtime.a'
def run(tool, *args):
    return subprocess.check_output(['riscv32-esp-elf-' + tool, *map(str, args)], text=True)
headers = run('readelf', '-h', archive)
flags = [x for x in headers.splitlines() if 'Flags:' in x]
assert flags and all('RVC, single-float ABI' in x for x in flags)
for name in ['gui.c', 'main.c']:
    obj = r / 'build/esp-idf/main/CMakeFiles/__idf_main.dir' / (name + '.obj')
    assert 'RVC, single-float ABI' in run('readelf', '-h', obj)
s = run('nm', '--defined-only', elf)
archive_symbols = run('nm', '--defined-only', archive)
apis = ['el_dice_to_hex', 'el_dice_required_rolls', 'el_coin_to_hex', 'el_hex_run', 'el_mnemonic_run']
for name in apis + ['fixture_alloc', 'fixture_free', 'abort']:
    assert len(re.findall(r'\bT ' + name + r'$', s, re.M)) == 1, name
for name in apis:
    assert len(re.findall(r'\bT ' + name + r'$', archive_symbols, re.M)) == 1, name
panic = [x.split()[-1] for x in s.splitlines() if ' T ' in x and 'rust_begin_unwind' in x]
assert len(panic) == 1
assert len(re.findall(r'\bT ' + re.escape(panic[0]) + r'$', archive_symbols, re.M)) == 1
m = (r / 'build/entropylab_fixture.map').read_text()
assert 'libentropylab_runtime.a' in m
for old in ['libentropylab_dice_core.a', 'libentropylab_coin_core_linkable.a', 'libentropylab_hex_core.a']:
    assert old not in m, old
for p in [r / 'build/build.ninja', r / 'app/main/CMakeLists.txt']:
    assert not any(x in p.read_text() for x in ['allow-multiple-definition', 'muldefs'])
for symbol, callee in [(panic[0], 'abort'), ('fixture_alloc', 'heap_caps_aligned_alloc'), ('fixture_free', 'heap_caps_free')]:
    assert '<' + callee + '>' in run('objdump', '-d', '--disassemble=' + symbol, elf), symbol
print('PASS unified runtime APIs, single panic, allocator/free, target ABI and no historical archive linkage')
