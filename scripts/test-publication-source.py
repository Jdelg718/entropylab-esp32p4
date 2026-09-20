#!/usr/bin/env python3
"""Current firmware/core byte contract, excluding only three empty editor files."""
import hashlib
import json
from pathlib import Path
R=Path(__file__).resolve().parents[1]
P=R/'preview/education-candidate02-public-practice-01'
manifest=json.loads((P/'source/source-inventory.json').read_text())['files']
excluded=set(json.loads((P/'provenance.json').read_text())['packaging_exclusions'])
expected={n:p for n,p in manifest.items() if n.startswith(('fixture-firmware/','core-spike/')) and n not in excluded}
for n,p in expected.items():
    f=R/n
    assert not f.is_symlink() and f.is_file(),n
    b=f.read_bytes()
    assert {'bytes':len(b),'sha256':hashlib.sha256(b).hexdigest()}==p,n
for n in excluded: assert not (R/n).exists(),n
print('PASS current firmware/core inputs:',len(expected),'exact bytes; no target build')
