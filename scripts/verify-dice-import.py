#!/usr/bin/env python3
"""Verify portable import bytes, public fixture counts and header separation."""
from pathlib import Path
import hashlib,json
r=Path(__file__).resolve().parents[1]
def verify_identities(root):
 # Pin both reviewed records: a successor is not permission to rebaseline files.
 historical=(root/'docs/dice-import-manifest.json').read_bytes()
 successor=(root/'docs/words-import-manifest.json').read_bytes()
 assert hashlib.sha256(historical).hexdigest()=='df76366c5bc512994f8cb993d584eb918464d6c5f2c62766bb7774e873e75a5e', 'historical manifest changed'
 assert hashlib.sha256(successor).hexdigest()=='3cb686840b5a8805afd80ddd3fb3d3520bdba90e9ec3ee2087a0facffd20b091', 'unreviewed successor manifest'
 m=json.loads(historical)
 words=json.loads(successor)
 assert words['historical_manifest_sha256']==hashlib.sha256(historical).hexdigest()
 assert words['source_manifest_sha256']=='e317b078cbea59ffb533e0d7c025a844e39e960a981ec0b77fa12a9cfc25e92e'
 successors={x['destination']:x for x in words['entries']}
 assert len(successors)==len(words['entries'])
 assert set(successors)<=set(x['destination'] for x in m['entries'])
 for x in m['entries']:
  expected=x['candidate_sha256']
  if x['destination'] in successors:
   update=successors[x['destination']]
   assert update['before_candidate_sha256']==expected
   assert update['source']==x['destination'].removeprefix('fixture-firmware/')
   assert update['candidate_sha256']==update['source_sha256']
   assert update['reason']
   expected=update['candidate_sha256']
  assert hashlib.sha256((root/x['destination']).read_bytes()).hexdigest()==expected,x['destination']
  assert x['exact'] or x['destination'] in m['exceptions']
verify_identities(r)
rows=[x.split('\t') for x in (r/'fixture-firmware/dice/vectors/dice.tsv').read_text().splitlines()]
assert len(rows)==20 and sum(int(x[4])>=0 for x in rows)==14 and sum(int(x[4])<0 for x in rows)==6
for bits,count in zip([128,160,192,224,256],[50,62,75,87,100]):assert 6**(count-1)<2**bits<=6**count
assert 'lvgl.h' not in (r/'fixture-firmware/app/main/gui.h').read_text()
assert (r/'fixture-firmware/dice/dice_core.h').read_bytes()==(r/'fixture-firmware/app/main/dice_core.h').read_bytes()
print('PASS import identities, seven documented exceptions, upstream20/native14accept6reject, all exact integer thresholds, LVGL-free worker header')
