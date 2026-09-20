#!/usr/bin/env python3
"""Verify the exact accepted dev package from a clone or GitHub source ZIP."""
import hashlib
import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PACKAGE = ROOT / 'preview/education-candidate02-dev-test-01'
MANIFEST = '64371d3c19a483170ba37119b44d5566b2cc1df5d2cbe76179522a2fffbeded5'

def verify():
    assert hashlib.sha256((PACKAGE/'package-manifest.json').read_bytes()).hexdigest() == MANIFEST, 'immutable manifest changed'
    spec = importlib.util.spec_from_file_location('package_verifier', PACKAGE/'verify-package.py')
    # Authenticate verifier bytes before importing executable package code.
    import json
    manifest = json.loads((PACKAGE/'package-manifest.json').read_text())
    raw = (PACKAGE/'verify-package.py').read_bytes()
    assert hashlib.sha256(raw).hexdigest() == manifest['files']['verify-package.py']['sha256'], 'verifier changed'
    import sys
    sys.dont_write_bytecode = True
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    result = module.validate(module.load(PACKAGE))
    assert len(module.load(PACKAGE)) == 690, 'exact member count'
    print(result)
    print('PASS: exact candidate02 package; publication HOLD remains; no device accessed')

if __name__ == '__main__':
    verify()
