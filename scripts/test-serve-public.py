#!/usr/bin/env python3
import importlib.util, threading, urllib.request, urllib.error
from pathlib import Path
spec=importlib.util.spec_from_file_location('server',Path(__file__).with_name('serve-public.py'))
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
s=m.ThreadingHTTPServer(('127.0.0.1',0),m.Handler)
threading.Thread(target=s.serve_forever,daemon=True).start()
try:
 for path,want in [('/README.md',200),('/flash/first-install/',200),('/docs/PUBLIC-FIRST-INSTALL.md',200),('/.git/config',404),('/%2e%2e/.git/config',404),('/factory-backup.bin',404),('/browser-review.png',404),('/docs/',404)]:
  try:
   with urllib.request.urlopen(f'http://127.0.0.1:{s.server_port}{path}') as r:code=r.status
  except urllib.error.HTTPError as e:code=e.code
  assert code==want,(path,code)
 print('PASS eight public-server allow/deny regressions')
 for asset in sorted((m.ROOT/'flash/first-install').rglob('*')):
  if asset.is_file() and asset.relative_to(m.ROOT).as_posix() in m.ALLOWED:
   with urllib.request.urlopen(f'http://127.0.0.1:{s.server_port}/'+asset.relative_to(m.ROOT).as_posix()) as r:
    assert r.status == 200 and r.read() == asset.read_bytes(), asset
 print('PASS all installer assets served byte-identically')
finally:s.shutdown();s.server_close()
