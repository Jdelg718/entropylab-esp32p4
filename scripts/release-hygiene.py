#!/usr/bin/env python3
"""Fail-closed fresh release recipe; prints shell exports, never Git-derived identity."""
import hashlib
import json
import os
from pathlib import Path
import shlex
import stat
import re
import subprocess

RECIPE = 'h2'

def identity(root):
    manifest = root / 'docs/PACKAGE-SOURCE-MANIFEST.json'
    # Inspect every node before reading anything, including the manifest itself.
    # Fresh owned trees only: no .git or special-node exceptions; no link following.
    actual = set()
    def visit(path):
        mode = path.lstat().st_mode
        if stat.S_ISDIR(mode):
            for child in path.iterdir():
                visit(child)
        elif stat.S_ISREG(mode):
            actual.add(path.relative_to(root).as_posix())
        else:
            raise ValueError('Unsupported source node: ' + str(path))
    visit(root)
    def unique_object(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError('Duplicate JSON key: ' + key)
            result[key] = value
        return result
    data = json.loads(manifest.read_text(), object_pairs_hook=unique_object)
    hex256 = lambda value: isinstance(value, str) and re.fullmatch(r'[0-9a-f]{64}', value)
    if (not isinstance(data, dict) or
        set(data) != {'schema', 'predecessor_manifest_sha256', 'recipe', 'status', 'entries'} or
        data['schema'] != 'release-hygiene-candidate-v1' or data['recipe'] != RECIPE or
        not hex256(data['predecessor_manifest_sha256']) or
        data['status'] != 'candidate; target rebuild and independent review required; distribution HOLD' or
        not isinstance(data['entries'], list)):
        raise ValueError('Invalid manifest schema or recipe')
    entries = data['entries']
    expected = {manifest.relative_to(root).as_posix()}
    for e in entries:
        if not isinstance(e, dict) or set(e) != {'path', 'bytes', 'sha256'}:
            raise ValueError('Invalid manifest entry')
        name = e['path']
        if (not isinstance(name, str) or not name or '\\' in name or
            any(ord(c) < 32 or ord(c) == 127 for c in name) or
            any(part in ('', '.', '..') for part in name.split('/')) or
            name in expected or type(e['bytes']) is not int or e['bytes'] < 0 or
            not hex256(e['sha256'])):
            raise ValueError('Unsafe, duplicate or malformed manifest entry')
        expected.add(name)
    if actual != expected:
        raise ValueError('Fresh source inventory required; unexpected or missing files')
    for e in entries:
        p = root / e['path']
        if p.lstat().st_size != e['bytes'] or hashlib.sha256(p.read_bytes()).hexdigest() != e['sha256']:
            raise ValueError('Source manifest mismatch: ' + e['path'])
    digest = hashlib.sha256(manifest.read_bytes()).hexdigest()
    # ESP app descriptor provides 32 bytes including terminating NUL.
    return 's' + digest[:24] + '-' + RECIPE, digest

def mappings(root):
    sysroot = subprocess.check_output(['rustc', '+nightly-2026-04-15', '--print', 'sysroot'], text=True).strip()
    pairs = []
    for path, dest in [(root, '/src/entropylab'), (Path(os.environ['IDF_PATH']), '/IDF'), (Path(os.environ['CARGO_HOME']), '/deps/cargo'), (Path(sysroot), '/toolchain/rust')]:
        for p in {str(path.absolute()), str(path.resolve())}:
            # GCC preserves a double-leading-slash spelling in some generated inputs.
            pairs.extend([(p, dest), ('/' + p, dest)])
    # Within our GCC file-map class (and rustc maps), last match wins.
    # Pinned GCC checks file maps before macro maps regardless of option order.
    # Specific prefixes go last within this file-map class.
    return sorted(set(pairs), key=lambda p: (len(p[0]), p[0]))

def main():
    root = Path(__file__).resolve().parents[1]
    version, digest = identity(root)
    pairs = mappings(root)
    if any(any(c.isspace() or c in '=;\"\'\\' for c in p) for pair in pairs for p in pair):
        raise ValueError('Unsupported build path characters')
    cflags = ' '.join('-ffile-prefix-map=' + a + '=' + b for a, b in pairs)
    rustflags = '-C relocation-model=static ' + ' '.join('--remap-path-prefix=' + a + '=' + b for a, b in pairs)
    for k, v in {'ENTROPYLAB_C_PREFIX_FLAGS': cflags,
                 'CFLAGS_riscv32imafc_esp_espidf': '-march=rv32imafc -mabi=ilp32f -fno-pic -fno-pie ' + cflags,
                 'CARGO_TARGET_RISCV32IMAFC_ESP_ESPIDF_RUSTFLAGS': rustflags,
                 'ENTROPYLAB_PROJECT_VER': version, 'ENTROPYLAB_SOURCE_SHA256': digest}.items():
        print('export ' + k + '=' + shlex.quote(v))

if __name__ == '__main__':
    main()
