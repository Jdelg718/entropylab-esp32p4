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
import tarfile
with tarfile.open(root/'release/retention-public-source.tar.gz') as archive:
    prefix = 'retention-public-source/source/'
    count = 0
    for member in archive.getmembers():
        if member.isfile() and member.name.startswith(prefix + 'fixture-firmware/'):
            name = member.name[len(prefix):]
            data = archive.extractfile(member)
            assert data is not None, name
            assert data.read() == (root/name).read_bytes(), name
            count += 1
    assert count == 330, count
    print('PASS accepted fixture source byte equality:', count)
# This is the fixture subtree only, not the 437 target-identity paths or the
# complete 745-file archive; retention-host.py --check verifies those scopes.
