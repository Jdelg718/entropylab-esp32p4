#!/usr/bin/env python3
"""Fail closed on the actual generated IDF Ninja graph, before target compilation."""
import collections
import hashlib
import json
import os
from pathlib import Path
import re
import runpy
import shlex
import sys


def expand(tokens, cwd, seen=()):
    result = []
    for token in tokens:
        if token.startswith('@'):
            path = (cwd / token[1:]).resolve()
            if path in seen or len(seen) >= 16:
                raise ValueError('Recursive response file: ' + str(path))
            result.extend(expand(shlex.split(path.read_text()), cwd, seen + (path,)))
        else:
            result.append(token)
    return result


def check(build, root, kind):
    hygiene = runpy.run_path(str(root / 'scripts/release-hygiene.py'))
    pairs = hygiene['mappings'](root)
    required = ['-ffile-prefix-map=' + a + '=' + b for a, b in pairs]
    # Do not trust the injection environment as the independent expectation.
    if shlex.split(os.environ['ENTROPYLAB_C_PREFIX_FLAGS']) != required:
        raise ValueError('Injection flags differ from independently derived maps')
    db = build / 'compile_commands.json'
    commands = json.loads(db.read_text())
    if not isinstance(commands, list) or not commands:
        raise ValueError('Missing/empty compilation database')
    ninja = (build / 'build.ninja').read_text()
    graph = re.findall(r'^build ([^\n]+?): (?:C|CXX|ASM)_COMPILER__[^\n ]+ ', ninja, re.M)
    outputs = [entry['output'] for entry in commands]
    if not graph or len(set(outputs)) != len(outputs) or set(graph) != set(outputs):
        raise ValueError('Compilation database does not cover complete Ninja compile graph')
    counts = collections.Counter()
    for entry in commands:
        tokens = expand(entry.get('arguments') or shlex.split(entry['command']), Path(entry['directory']))
        actual = [x for x in tokens if x.startswith('-ffile-prefix-map=')]
        if actual != required:
            raise ValueError('Missing, reordered or unexpected file maps: ' + entry['file'])
        if '-mabi=ilp32f' not in tokens or '-march=rv32imafc_zicsr_zifencei_xesppie' not in tokens:
            raise ValueError('Missing pinned target ABI/architecture: ' + entry['file'])
        if any(x == '-DNDEBUG' or x.startswith('-DNDEBUG=') for x in tokens):
            raise ValueError('Assertions disabled: ' + entry['file'])
        if '-c' not in tokens or '-o' not in tokens or tokens[tokens.index('-o') + 1] != entry['output']:
            raise ValueError('Invalid compilation command')
        suffix = Path(entry['file']).suffix
        if suffix not in ('.c', '.cpp', '.cc', '.cxx', '.S', '.s'):
            raise ValueError('Unrecognized compilation language: ' + entry['file'])
        counts['ASM' if suffix in ('.S', '.s') else 'C' if suffix == '.c' else 'CXX'] += 1
    names = [e['file'] for e in commands]
    # Bootloader currently has C only; still verify all three initialized flag files.
    for name in ('cflags', 'cxxflags', 'asmflags'):
        flags = expand(['@' + str(build / 'toolchain' / name)], build)
        if [x for x in flags if x.startswith('-ffile-prefix-map=')] != required:
            raise ValueError('Invalid toolchain maps: ' + name)
    if not counts['C']:
        raise ValueError('Incomplete target language graph')
    if kind == 'app' and (not counts['ASM'] or not counts['CXX'] or not any(n.endswith('/cell-grid.cpp') for n in names)
                          or not any(n.endswith('/project_elf_src_esp32p4.c') for n in names)):
        raise ValueError('Missing real application/generated/LifeHash compilation')
    return {'status': 'PASS', 'kind': kind, 'entries': len(commands), 'languages': dict(counts),
            'required_maps': required, 'compile_commands_sha256': hashlib.sha256(db.read_bytes()).hexdigest(),
            'build_ninja_sha256': hashlib.sha256((build / 'build.ninja').read_bytes()).hexdigest()}


if __name__ == '__main__':
    if len(sys.argv) != 3 or sys.argv[2] not in ('app', 'bootloader'):
        raise SystemExit('usage: check-idf-remaps.py BUILD_DIRECTORY app|bootloader')
    print(json.dumps(check(Path(sys.argv[1]).resolve(), Path(__file__).resolve().parents[1], sys.argv[2]), indent=2))
