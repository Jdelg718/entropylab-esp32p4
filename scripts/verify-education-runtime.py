#!/usr/bin/env python3
"""Supplemental successor runtime inspection; never rewrites the build receipt.

The immutable source's old verifier expects the shared heap. This successor check
retains its ABI/symbol/map gates and requires the source's private multi_heap shim.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess


def require(ok, message):
    if not ok:
        raise ValueError(message)


def check_allocator_calls(alloc, free, symbols=None):
    for text, expected in [(alloc, 'multi_heap_aligned_alloc'), (free, 'multi_heap_free')]:
        # Demand a disassembled instruction reference, not a function heading.
        refs = re.findall(r'^\s*[0-9a-f]+:\s+[0-9a-f]+\s+(?:jal|jalr|j|jr)\b[^\n]*?\b([0-9a-f]+)\s+<([^>]+)>', text, re.M)
        calls = [name for address, name in refs]
        if symbols is None:
            require(expected in calls, 'missing private allocator callee: ' + expected)
        else:
            require(expected in symbols and any(address == symbols[expected] == symbols.get(name)
                                                for address, name in refs), 'private callee address: ' + expected)
        require(not any(name.startswith('heap_caps_') for name in calls), 'shared heap allocator regression')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('candidate', type=Path)
    parser.add_argument('--tool-prefix', default='riscv32-esp-elf-')
    args = parser.parse_args()
    d = args.candidate.resolve()
    r = d / 'source/fixture-firmware'
    elf = r / 'build/entropylab_fixture.elf'
    archive = r / 'runtime/target/riscv32imafc-esp-espidf/release/libentropylab_runtime.a'
    run = lambda tool, *a: subprocess.check_output([args.tool_prefix + tool, *map(str, a)], text=True)
    sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
    receipt = json.loads((d / 'build-status.json').read_text())
    require(sha(elf) == receipt['artifacts']['entropylab_fixture.elf']['sha256'], 'ELF receipt drift')
    headers = run('readelf', '-h', archive)
    flags = [x for x in headers.splitlines() if 'Flags:' in x]
    require(flags and all('RVC, single-float ABI' in x for x in flags), 'archive ABI')
    for name in ['gui.c', 'main.c', 'fixture_diagnostics.c']:
        obj = r / 'build/esp-idf/main/CMakeFiles/__idf_main.dir' / (name + '.obj')
        require('RVC, single-float ABI' in run('readelf', '-h', obj), 'C ABI: ' + name)
    symbols = run('nm', '--defined-only', elf)
    archive_symbols = run('nm', '--defined-only', archive)
    apis = ['el_dice_to_hex', 'el_dice_required_rolls', 'el_coin_to_hex', 'el_input_to_mnemonic', 'el_bip39_passphrase_run']
    for name in apis + ['fixture_alloc', 'fixture_free', 'abort', 'fixture_allocator_init']:
        require(len(re.findall(r'\bT ' + name + r'$', symbols, re.M)) == 1, 'ELF symbol: ' + name)
    for name in apis + ['el_hex_run', 'el_mnemonic_run']:
        require(len(re.findall(r'\bT ' + name + r'$', archive_symbols, re.M)) == 1, 'archive symbol: ' + name)
    panic = [x.split()[-1] for x in symbols.splitlines() if ' T ' in x and 'rust_begin_unwind' in x]
    require(len(panic) == 1, 'panic count')
    require(len(re.findall(r'\bT ' + re.escape(panic[0]) + r'$', archive_symbols, re.M)) == 1, 'archive panic')
    link_map = (r / 'build/entropylab_fixture.map').read_text()
    require('libentropylab_runtime.a' in link_map, 'runtime map linkage')
    for old in ['libentropylab_dice_core.a', 'libentropylab_coin_core_linkable.a', 'libentropylab_hex_core.a']:
        require(old not in link_map, 'historical archive linkage: ' + old)
    for p in [r / 'build/build.ninja', r / 'app/main/CMakeLists.txt']:
        require(not any(x in p.read_text() for x in ['allow-multiple-definition', 'muldefs']), 'duplicate definition flags')
    disassembly = {s: run('objdump', '-d', '--disassemble=' + s, elf) for s in
                   [panic[0], 'fixture_alloc', 'fixture_free', 'fixture_allocator_init']}
    require('<abort>' in disassembly[panic[0]], 'panic abort')
    addresses = {line.split()[2]: line.split()[0] for line in symbols.splitlines() if len(line.split()) == 3}
    check_allocator_calls(disassembly['fixture_alloc'], disassembly['fixture_free'], addresses)
    for callee in ['multi_heap_register', 'multi_heap_set_lock']:
        require('<' + callee + '>' in disassembly['fixture_allocator_init'], 'private heap init: ' + callee)
    sizes = run('nm', '-S', '--defined-only', elf)
    require(re.search(r'\b00002000\s+[bBdD]\s+fixture_rust_memory$', sizes, re.M), 'private 8KiB arena')
    print(json.dumps({'schema': 'education-runtime-inspection-v1', 'status': 'PASS-supplemental-only',
                      'original_build_status': receipt['status'], 'original_build_exit_code': receipt['exit_code'],
                      'elf_sha256': sha(elf), 'runtime_archive_sha256': sha(archive),
                      'verifier_sha256': sha(Path(__file__)),
                      'disassembly_sha256': {k: hashlib.sha256(v.encode()).hexdigest() for k, v in disassembly.items()},
                      'allocator': 'private 8KiB multi_heap; no shared-heap fallback in shim',
                      'hardware': 'NOT TESTED', 'distribution': 'HOLD'}, indent=2))


if __name__ == '__main__':
    main()
