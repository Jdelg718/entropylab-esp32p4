#!/usr/bin/env python3
"""Compare the runner's actual export block with the immutable checker contract.

Pass a captured effective-environment.json to replay a real post-IDF export
context. No build or mutation of the captured candidate is performed.
"""
import ast
import importlib.util
import json
import os
from pathlib import Path
import runpy
import shlex
import sys

scripts = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location('runner', scripts / 'run-education-build.py')
assert spec is not None and spec.loader is not None
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)

def export_values(source, env, digest):
    # Execute the production block, not a test-side reimplementation.
    tree = ast.parse((scripts / 'run-education-build.py').read_text())
    main = next(n for n in tree.body if isinstance(n, ast.FunctionDef) and n.name == 'main')
    body = next(n for n in main.body if isinstance(n, ast.Try)).body
    start = next(i for i, n in enumerate(body) if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'pairs' for t in n.targets))
    end = next(i for i, n in enumerate(body) if isinstance(n, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'exports' for t in n.targets))
    scope = {'source': source, 'env': env, 'digest': digest, 'Path': Path, 'm': runner.m}
    exec(compile(ast.Module(body=body[start:end], type_ignores=[]), '<runner export block>', 'exec'), scope)
    return scope['values']

if __name__ == '__main__':
    env = json.loads(Path(sys.argv[1]).read_text())['environment']
    os.environ.clear()
    os.environ.update(env)
    source = Path(env['EDUCATION_SOURCE'])
    values = export_values(source, env, env['ENTROPYLAB_SOURCE_SHA256'])
    pairs = runpy.run_path(str(source / 'scripts/release-hygiene.py'))['mappings'](source)
    actual = shlex.split(values['ENTROPYLAB_C_PREFIX_FLAGS'])
    expected = ['-ffile-prefix-map=' + a + '=' + b for a, b in pairs]
    assert actual == expected, json.dumps({'export_only': sorted(set(actual)-set(expected)), 'derived_only': sorted(set(expected)-set(actual))}, indent=2)
    assert shlex.split(values['CARGO_TARGET_RISCV32IMAFC_ESP_ESPIDF_RUSTFLAGS'])[2:] == ['--remap-path-prefix=' + a + '=' + b for a, b in pairs]
    print('PASS exact ordered C/Rust remaps against immutable post-export derivation')
