from pathlib import Path
import os,shutil,tempfile,subprocess,json,runpy
R=Path(__file__).resolve().parents[2]
v=runpy.run_path(str(R/'scripts/d6-release/verify-source.py'));m=v['verify'](R)
with tempfile.TemporaryDirectory(prefix='d6-historical-') as tmp:
 h=Path(tmp);v['require'](v['sha'](R/'docs/d6-release/historical-host-delta.json')=='d98c8b4d00a891a713e9663f47ca91bf7873727fb4ddf06f710c36b7034b87bd','historical mapping drift');delta=json.loads((R/'docs/d6-release/historical-host-delta.json').read_text())
 for n in m['source_paths']:
  src=R/n
  if n in delta:
   src=R/'docs/d6-release/historical-inputs'/n
   v['require'](v['sha'](src)==delta[n]['historical_sha256'],'historical input drift')
   v['require'](m['source_paths'][n]==delta[n]['candidate_sha256'],'candidate input drift')
  dst=h/n;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copyfile(src,dst)
 rc=subprocess.run(['bash',str(R/'scripts/d6-release/test-host.sh')],env={**os.environ,'D6_HISTORICAL_ROOT':str(h),'PYTHONDONTWRITEBYTECODE':'1','PYTHONOPTIMIZE':'0'}).returncode
 v['verify'](R)
 raise SystemExit(rc)
