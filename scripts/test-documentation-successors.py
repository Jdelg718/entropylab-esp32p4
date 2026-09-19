#!/usr/bin/env python3
"""Finite mapping regression: exact docs only; rejected mutations always restored."""
import json,subprocess
from pathlib import Path
r=Path(__file__).resolve().parents[1]
def check(ok):
 p=subprocess.run(['python3',str(r/'scripts/retention-host.py'),'--check'],capture_output=True,text=True)
 assert (p.returncode==0)==ok,p.stdout+p.stderr
check(True)
for name in ['CONTRIBUTING.md','fixture-firmware/app/main/gui.c','scripts/verify-dice-import.py','docs/retention-host/documentation-successors.json']:
 p=r/name;original=p.read_bytes()
 try:
  if name.endswith('.json'):
   data=json.loads(original);data['fixture-firmware/app/main/gui.c']=next(iter(data.values()));p.write_text(json.dumps(data))
  else:p.write_bytes(original+b'\nUNACCEPTED MUTATION\n')
  check(False)
 finally:p.write_bytes(original)
 print('PASS rejects mutation:',name)
check(True)
print('PASS finite exact documentation successors; no added paths or source/script drift accepted')
