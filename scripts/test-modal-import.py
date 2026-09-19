#!/usr/bin/env python3
"""Finite modal successor: refreshed-pin semantic checks, not pin-only rejection."""
import hashlib,json,shutil,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SCRIPT=ROOT/'scripts/verify-dice-import.py'
SOURCE=SCRIPT.read_text().split('verify_identities(r)')[0]
MANIFEST='docs/modal-successors.json'
PIN=hashlib.sha256((ROOT/MANIFEST).read_bytes()).hexdigest()
class ModalIdentityTests(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup)
  self.root=Path(self.tmp.name)/'source';shutil.copytree(ROOT,self.root)
  self.original=(self.root/MANIFEST).read_bytes()
 def verify(self,refresh=False):
  source=SOURCE.replace(PIN,hashlib.sha256((self.root/MANIFEST).read_bytes()).hexdigest()) if refresh else SOURCE
  ns={'__file__':str(SCRIPT)};exec(compile(source,str(SCRIPT),'exec'),ns);ns['verify_identities'](self.root)
 def mutate(self,change,error):
  d=json.loads(self.original);change(d);(self.root/MANIFEST).write_text(json.dumps(d))
  with self.assertRaisesRegex(AssertionError,error):self.verify(True)
 def test_exact(self):self.verify()
 def test_serialization_control(self):
  # A refreshed predecessor pin cannot disconnect the exact terminal successor.
  (self.root/MANIFEST).write_text(json.dumps(json.loads(self.original),sort_keys=True))
  with self.assertRaisesRegex(AssertionError,'global-saver prior manifest mismatch'):self.verify(True)
 def test_pin(self):
  (self.root/MANIFEST).write_bytes(self.original+b'\n')
  with self.assertRaisesRegex(AssertionError,'unreviewed modal'):self.verify()
 def test_all_predecessors_classes_candidates_reasons(self):
  for i in range(2):
   for field,value,error in [('before_sha256','0'*64,'modal chain'),('classification','invalid','classification/path'),('candidate_sha256','0'*64,'modal current'),('reason','','modal reason')]:
    with self.subTest(entry=i,field=field):self.mutate(lambda d:d['entries'][i].update({field:value}),error)
 def test_order_inventory_and_traversal(self):
  for op in [lambda d:d['entries'].reverse(),lambda d:d['entries'].pop(),lambda d:d['entries'].append(d['entries'][0]),lambda d:d['entries'][0].update(destination='../escape'),lambda d:d['entries'][1].update(destination='/tmp/escape')]:
   self.mutate(op,'modal destination/order')
 def test_all_current_drift(self):
  for e in json.loads(self.original)['entries']:
   with self.subTest(path=e['destination']):
    p=self.root/e['destination'];old=p.read_bytes();p.write_bytes(old+b'\n')
    with self.assertRaisesRegex(AssertionError,'modal current'):self.verify()
    p.write_bytes(old)
 def test_predecessor_manifest_and_package(self):
  for field,value,error in [('prior_manifest','../escape','modal prior'),('prior_manifest_sha256','0'*64,'modal prior'),('predecessor_package_manifest_sha256','0'*64,'modal predecessor')]:
   self.mutate(lambda d:d.update({field:value}),error)
if __name__=='__main__':unittest.main()
