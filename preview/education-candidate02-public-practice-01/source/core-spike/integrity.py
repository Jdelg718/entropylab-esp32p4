"""Fail closed on drift; the reviewed manifest is an input, never regenerated."""
from pathlib import Path
import hashlib, json
MANIFEST_SHA256 = '11457e7d1ad7cf2329bbddec6abcdb75b651a7a4f67418f5f59f013548b64b61'

def verify_fixtures(root):
    p = root/'vectors'
    raw = (p/'sha256.json').read_bytes()
    if hashlib.sha256(raw).hexdigest() != MANIFEST_SHA256:
        raise SystemExit('fixture manifest drift; explicit reviewed baseline update required')
    manifest = json.loads(raw)
    for name, expected in manifest.items():
        f = p/name
        if not f.is_file() or hashlib.sha256(f.read_bytes()).hexdigest() != expected:
            raise SystemExit('fixture drift: '+name)
    return manifest

def verify_generated(name, data, manifest):
    if hashlib.sha256(data.encode()).hexdigest() != manifest[name]:
        raise SystemExit('generated fixture drift: '+name)
