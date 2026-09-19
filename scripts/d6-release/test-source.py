"""Portable candidate validator negative tests; no firmware build."""
from pathlib import Path
import json,runpy,tempfile,shutil,unittest
R=Path(__file__).resolve().parents[2]
V=runpy.run_path(str(R/'scripts/d6-release/verify-source.py'))
class Tests(unittest.TestCase):
 def setUp(self):
  self.tmp=tempfile.TemporaryDirectory();self.addCleanup(self.tmp.cleanup);self.root=Path(self.tmp.name)
  m=json.loads((R/'docs/d6-release/candidate-source-identity.json').read_text())
  for n in list(m['source_paths'])+['docs/d6-release/candidate-source-identity.json','docs/d6-release/predecessor-source-identity.json']:
   p=self.root/n;p.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(R/n,p)
 def test_exact(self):V['verify'](self.root)
 def test_changed_source(self):
  p=self.root/'fixture-firmware/app/main/main.c';p.write_bytes(p.read_bytes()+b'\n')
  with self.assertRaisesRegex(ValueError,'source drift'):V['verify'](self.root)
 def test_missing_source(self):
  (self.root/'LICENSE').unlink()
  with self.assertRaises(FileNotFoundError):V['verify'](self.root)
 def test_candidate_manifest_tamper(self):
  p=self.root/'docs/d6-release/candidate-source-identity.json';p.write_bytes(p.read_bytes()+b'\n')
  with self.assertRaisesRegex(ValueError,'identity drift'):V['verify'](self.root)
 def test_predecessor_manifest_tamper(self):
  p=self.root/'docs/d6-release/predecessor-source-identity.json';p.write_bytes(p.read_bytes()+b'\n')
  with self.assertRaisesRegex(ValueError,'predecessor drift'):V['verify'](self.root)
 def test_symlink(self):
  p=self.root/'LICENSE';p.unlink();p.symlink_to(R/'LICENSE')
  with self.assertRaisesRegex(ValueError,'unsafe path'):V['verify'](self.root)
 def test_refreshed_manifest_cannot_authorize_extra_delta(self):
  p=self.root/'docs/d6-release/candidate-source-identity.json';m=json.loads(p.read_text());m['source_paths']['LICENSE']='0'*64;p.write_text(json.dumps(m))
  old=V['verify'].__globals__['IDENTITY'];V['verify'].__globals__['IDENTITY']=V['sha'](p)
  try:
   with self.assertRaisesRegex(ValueError,'delta mismatch'):V['verify'](self.root)
  finally:V['verify'].__globals__['IDENTITY']=old
if __name__=='__main__':unittest.main()
