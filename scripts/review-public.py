#!/usr/bin/env python3
"""Offline exact-notice and accepted firmware-source continuity checks."""
from pathlib import Path
import hashlib, json
root = Path(__file__).resolve().parents[1]
m = json.loads((root/'docs/release-notices/manifest.json').read_text())
notices = (root/'flash/first-install/NOTICES.txt').read_bytes()
for name, pin in m['files'].items():
    data = (root/name).read_bytes()
    assert hashlib.sha256(data).hexdigest() == pin['sha256'], name
    assert data in notices, name
print('PASS exact notice hashes and concatenation:', len(m['files']))
archive_root = root/'.review-source/retention-public-source/source'
if archive_root.is_dir():
    count = 0
    for p in (archive_root/'fixture-firmware').rglob('*'):
        if p.is_file():
            assert p.read_bytes() == (root/p.relative_to(archive_root)).read_bytes(), p
            count += 1
    print('PASS accepted fixture source byte equality:', count)
