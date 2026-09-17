#!/usr/bin/env python3
"""Finite D6 successor identity and refreshed-pin semantic regression tests."""
import hashlib
import json
from pathlib import Path
import runpy
import shutil
import tempfile
import unittest
ROOT=Path(__file__).resolve().parents[1]
SCRIPT=ROOT/'scripts/verify-dice-import.py'
SOURCE=SCRIPT.read_text().split('verify_identities(r)')[0]
MANIFEST='docs/d6-release-successors.json'
PIN=hashlib.sha256((ROOT/MANIFEST).read_bytes()).hexdigest()
VERIFY=runpy.run_path(str(SCRIPT))['verify_identities']
class D6ReleaseTests(unittest.TestCase):
 def setUp(self):
  self.temp=tempfile.TemporaryDirectory();self.addCleanup(self.temp.cleanup)
  self.root=Path(self.temp.name)
  # Copy finite verifier inputs only, including historical import destinations.
  paths=set()
  for p in (ROOT/'docs').glob('*.json'):
   paths.add(p.relative_to(ROOT).as_posix())
   data=json.loads(p.read_text())
   if isinstance(data,dict):
    for entry in data.get('entries',[]):
     if 'destination' in entry:paths.add(entry['destination'])
  for name in paths:
   source=ROOT/name
   if not source.is_file():continue
   dest=self.root/name;dest.parent.mkdir(parents=True,exist_ok=True)
   shutil.copyfile(source,dest)
 def test_exact(self):VERIFY(self.root)
 def test_every_successor_drift(self):
  for e in json.loads((self.root/MANIFEST).read_text())['entries']:
   p=self.root/e['destination'];old=p.read_bytes();p.write_bytes(old+b'\n')
   with self.assertRaises(AssertionError):VERIFY(self.root)
   p.write_bytes(old)
 def test_manifest_tamper(self):
  p=self.root/MANIFEST;p.write_bytes(p.read_bytes()+b'\n')
  with self.assertRaisesRegex(AssertionError,'unreviewed D6'):VERIFY(self.root)
 def semantic(self,change,error):
  p=self.root/MANIFEST;d=json.loads(p.read_text());change(d);p.write_text(json.dumps(d))
  namespace={'__file__':str(SCRIPT)}
  exec(compile(SOURCE.replace(PIN,hashlib.sha256(p.read_bytes()).hexdigest()),str(SCRIPT),'exec'),namespace)
  with self.assertRaisesRegex(AssertionError,error):namespace['verify_identities'](self.root)
 def test_production_relabel(self):
  self.semantic(lambda d:d['entries'][0].update(classification='host-test-only'),'classification/path mismatch')
 def test_test_relabel(self):
  self.semantic(lambda d:d['entries'][-1].update(classification='production-source'),'classification/path mismatch')
 def test_escape(self):
  self.semantic(lambda d:d['entries'][0].update(destination='../escape'),'destination/order mismatch')
 def test_duplicate(self):
  self.semantic(lambda d:d['entries'].append(d['entries'][0]),'destination/order mismatch')
 def test_chain(self):
  original=(self.root/MANIFEST).read_bytes()
  for index,entry in enumerate(json.loads(original)['entries']):
   with self.subTest(destination=entry['destination']):
    (self.root/MANIFEST).write_bytes(original)
    self.semantic(lambda d:d['entries'][index].update(before_sha256='0'*64),'chain discontinuity')
 def test_every_classification_and_reason(self):
  original=(self.root/MANIFEST).read_bytes()
  for index,entry in enumerate(json.loads(original)['entries']):
   for field,value,error in [('classification','host-test-only' if entry['classification']=='production-source' else 'production-source','classification/path mismatch'),('reason','','')]:
    with self.subTest(destination=entry['destination'],field=field):
     (self.root/MANIFEST).write_bytes(original)
     self.semantic(lambda d:d['entries'][index].update({field:value}),error)
if __name__=='__main__':unittest.main()
