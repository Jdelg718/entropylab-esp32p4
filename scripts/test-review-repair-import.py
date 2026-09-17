#!/usr/bin/env python3
"""Refreshed-pin checks for both appended production successors."""
import hashlib,json,tempfile,shutil,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SCRIPT=ROOT/'scripts/verify-dice-import.py'
SOURCE=SCRIPT.read_text().split('verify_identities(r)')[0]
MANIFEST='docs/review-repair-successors.json'
PIN=hashlib.sha256((ROOT/MANIFEST).read_bytes()).hexdigest()
class ReviewRepairTests(unittest.TestCase):
 def test_exact(self):
  ns={'__file__':str(SCRIPT)};exec(compile(SOURCE,str(SCRIPT),'exec'),ns);ns['verify_identities'](ROOT)
 def test_refreshed_matrix(self):
  with tempfile.TemporaryDirectory() as tmp:
   root=Path(tmp)/'source';shutil.copytree(ROOT,root)
   original=(root/MANIFEST).read_bytes()
   for i in range(len(json.loads(original)['entries'])):
    for field,value,error in [('before_sha256','0'*64,'review-repair chain'),('classification','invalid','classification/path'),('reason','','reason'),('candidate_sha256','0'*64,'review-repair current')]:
     with self.subTest(entry=i,field=field):
      d=json.loads(original);d['entries'][i][field]=value;p=root/MANIFEST;p.write_text(json.dumps(d))
      ns={'__file__':str(SCRIPT)};exec(compile(SOURCE.replace(PIN,hashlib.sha256(p.read_bytes()).hexdigest()),str(SCRIPT),'exec'),ns)
      with self.assertRaisesRegex(AssertionError,error):ns['verify_identities'](root)
if __name__=='__main__':unittest.main()
