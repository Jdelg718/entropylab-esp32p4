#!/usr/bin/env python3
"""Unified source runtime final-ELF gate; not a build-receipt verifier."""
from pathlib import Path
import subprocess
import re
import argparse, os
a=argparse.ArgumentParser();a.add_argument('--source',type=Path,required=True);a.add_argument('--build',type=Path,required=True);a.add_argument('--archive',type=Path,required=True);a.add_argument('--tool-prefix',default=os.environ.get('RISCV_TOOL_PREFIX','riscv32-esp-elf-'));o=a.parse_args()
r=o.source/'fixture-firmware'
elf = o.build/'entropylab_fixture.elf'
archive = o.archive
def run(tool, *args):
    return subprocess.check_output([o.tool_prefix + tool, *map(str, args)], text=True)
headers = run('readelf', '-h', archive)
flags = [x for x in headers.splitlines() if 'Flags:' in x]
assert flags and all('RVC, single-float ABI' in x for x in flags)
for name in ['gui.c', 'main.c']:
    obj = o.build/'esp-idf/main/CMakeFiles/__idf_main.dir' / (name + '.obj')
    assert 'RVC, single-float ABI' in run('readelf', '-h', obj)
s = run('nm', '--defined-only', elf)
archive_symbols = run('nm', '--defined-only', archive)
apis = ['el_dice_to_hex', 'el_dice_required_rolls', 'el_coin_to_hex', 'el_input_to_mnemonic', 'el_bip39_passphrase_run']
for name in apis + ['fixture_alloc', 'fixture_free', 'abort']:
    assert len(re.findall(r'\bT ' + name + r'$', s, re.M)) == 1, name
for name in apis + ['el_hex_run', 'el_mnemonic_run']:
    assert len(re.findall(r'\bT ' + name + r'$', archive_symbols, re.M)) == 1, name
panic = [x.split()[-1] for x in s.splitlines() if ' T ' in x and 'rust_begin_unwind' in x]
assert len(panic) == 1
assert len(re.findall(r'\bT ' + re.escape(panic[0]) + r'$', archive_symbols, re.M)) == 1
m = (o.build/'entropylab_fixture.map').read_text()
assert 'libentropylab_runtime.a' in m
for old in ['libentropylab_dice_core.a', 'libentropylab_coin_core_linkable.a', 'libentropylab_hex_core.a']:
    assert old not in m, old
for p in [o.build/'build.ninja', r / 'app/main/CMakeLists.txt']:
    assert not any(x in p.read_text() for x in ['allow-multiple-definition', 'muldefs'])
for symbol, callee in [(panic[0], 'abort'), ('fixture_alloc', 'multi_heap_aligned_alloc'), ('fixture_free', 'multi_heap_aligned_free')]:
    assert '<' + callee + '>' in run('objdump', '-d', '--disassemble=' + symbol, elf), symbol
print('PASS unified runtime APIs, single panic, allocator/free, target ABI and no historical archive linkage')

import struct,json,hashlib
E=elf;F=r
syms=run('objdump','-t',E);rows=[x for x in syms.splitlines() if x.endswith(' fixture_rust_memory')];assert len(rows)==1,rows
row=rows[0].split();assert int(row[-2],16)==8192,rows;assert row[-3]=='.dram0.data',rows
alloc=run('objdump','-d','--disassemble=fixture_alloc',E);free=run('objdump','-d','--disassemble=fixture_free',E)
assert 'heap_caps_aligned_alloc' not in alloc and 'heap_caps_free' not in free
main=run('objdump','-d','--disassemble=app_main',E);assert main.index('<fixture_allocator_init>')<main.index('<bsp_display_start_with_config>')
init=run('objdump','-d','--disassemble=fixture_allocator_init',E);assert 'multi_heap_set_lock' in init
blob=(o.build/'entropylab_fixture.bin').read_bytes();assert blob[0]==0xe9
out={'status':'PASS','note':'compiled static allocation/ABI/linkage evidence only, not device or whole-app capacity','pool_symbol':rows[0],'app_revision_limits':list(struct.unpack_from('<HH',blob,15)),'elf_sha256':hashlib.sha256(E.read_bytes()).hexdigest(),'bin_sha256':hashlib.sha256(blob).hexdigest(),'bin_size':len(blob)}
print(json.dumps(out))
