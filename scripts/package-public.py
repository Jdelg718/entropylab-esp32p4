#!/usr/bin/env python3
"""Package only Git tracked files with fixed ZIP metadata; no firmware builds."""
import hashlib,json,subprocess,zipfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
paths=sorted(subprocess.check_output(['git','ls-files','-z'],cwd=root).decode().strip('\0').split('\0'))
files={p:{'bytes':(root/p).stat().st_size,'sha256':hashlib.sha256((root/p).read_bytes()).hexdigest()} for p in paths if p and p!='release/inventory.json'}
(root/'release/inventory.json').write_text(json.dumps(files,indent=2,sort_keys=True)+'\n')
files['release/inventory.json']={}
out=root/'release/public-release.zip'
with zipfile.ZipFile(out,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
 for p in sorted(files):
  i=zipfile.ZipInfo(p,(1980,1,1,0,0,0));i.create_system=3;i.external_attr=(0o100755 if (root/p).stat().st_mode&0o111 else 0o100644)<<16;i.compress_type=zipfile.ZIP_DEFLATED
  z.writestr(i,(root/p).read_bytes(),compresslevel=9)
print(hashlib.sha256(out.read_bytes()).hexdigest(),out.name)
