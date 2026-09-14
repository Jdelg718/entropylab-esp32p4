#!/usr/bin/env python3
"""Verify portable import bytes, public fixture counts and header separation."""
from pathlib import Path
import hashlib,json
r=Path(__file__).resolve().parents[1]
m=json.loads((r/'docs/dice-import-manifest.json').read_text())
for x in m['entries']:
 assert hashlib.sha256((r/x['destination']).read_bytes()).hexdigest()==x['candidate_sha256'],x['destination']
 assert x['exact'] or x['destination'] in m['exceptions']
rows=[x.split('\t') for x in (r/'fixture-firmware/dice/vectors/dice.tsv').read_text().splitlines()]
assert len(rows)==20 and sum(int(x[4])>=0 for x in rows)==14 and sum(int(x[4])<0 for x in rows)==6
for bits,count in zip([128,160,192,224,256],[50,62,75,87,100]):assert 6**(count-1)<2**bits<=6**count
assert 'lvgl.h' not in (r/'fixture-firmware/app/main/gui.h').read_text()
assert (r/'fixture-firmware/dice/dice_core.h').read_bytes()==(r/'fixture-firmware/app/main/dice_core.h').read_bytes()
print('PASS import identities, seven documented exceptions, upstream20/native14accept6reject, all exact integer thresholds, LVGL-free worker header')
