from pathlib import Path
import argparse,hashlib,json
IDENTITY='07d03ba6f421a25316e5b73662f1a6f9d79243c7bdaba8119c3cd27a16573d4d'
PREDECESSOR='fc354f8127655af09fa05066bc44b8e8044041b9ff768d052c65b25a77e8f3df'
CHANGES={'fixture-firmware/app/main/fixture_diagnostics.c','fixture-firmware/app/main/main.c'}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def require(ok,message):
 if not ok:raise ValueError(message)
def verify(root):
 d=root/'docs/d6-release';p=d/'candidate-source-identity.json';q=d/'predecessor-source-identity.json'
 require(sha(p)==IDENTITY,'candidate identity drift');require(sha(q)==PREDECESSOR,'predecessor drift')
 m=json.loads(p.read_text());b=json.loads(q.read_text())
 require(m['predecessor_identity_sha256']==PREDECESSOR,'chain mismatch')
 require(len(m['source_paths'])==436 and set(m['source_paths'])==set(b['source_paths']),'inventory mismatch')
 require(set(m['changed_paths'])==CHANGES,'unauthorized changes')
 require({n for n,h in m['source_paths'].items() if h!=b['source_paths'][n]}==CHANGES,'delta mismatch')
 for n,h in m['source_paths'].items():
  p=root/n;require(not Path(n).is_absolute() and '..' not in Path(n).parts and not p.is_symlink(),'unsafe path')
  require(sha(p)==h,'source drift: '+n)
 return m
if __name__=='__main__':
 a=argparse.ArgumentParser();a.add_argument('--source',type=Path,default=Path(__file__).resolve().parents[2]);o=a.parse_args();verify(o.source);print('PASS exact 436-path D6 source identity and finite two-file predecessor mapping; not reproducibility')
