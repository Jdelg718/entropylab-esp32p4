#!/usr/bin/env python3
"""Clean-checkout onboarding checks; runs actual server, never opens serial hardware."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time
import urllib.error
import urllib.request
from playwright.sync_api import sync_playwright

ap = argparse.ArgumentParser()
ap.add_argument('checkout', type=Path)
ap.add_argument('--output', type=Path, required=True)
a = ap.parse_args()
root = a.checkout.resolve()
a.output.mkdir(parents=True, exist_ok=True)
subprocess.run([sys.executable, 'scripts/verify-publication.py'], cwd=root, check=True)
prefix = 'preview/education-candidate02-dev-test-01/'
package = root/prefix
manifest = json.loads((package/'package-manifest.json').read_text())
node = subprocess.run(['node','--test','tests/successor-smoke.test.mjs','flash/first-install/wire.test.mjs'],cwd=package,capture_output=True,text=True)
(a.output/'node.log').write_text(node.stdout+node.stderr)
assert node.returncode == 0
log = (a.output/'server.log').open('w')
server = subprocess.Popen([sys.executable,'scripts/serve-public.py'],cwd=root,stdout=log,stderr=log)
origin = 'http://127.0.0.1:8000'
try:
    for _ in range(100):
        if server.poll() is not None: raise RuntimeError('server exited; check log/port')
        try:
            with urllib.request.urlopen(origin+'/',timeout=1) as response:
                assert response.url.endswith(prefix+'flash/first-install/')
                break
        except urllib.error.URLError: time.sleep(.05)
    else: raise RuntimeError('server not ready')
    # Every package member, not merely the three firmware responses.
    for name, pin in manifest['files'].items():
        with urllib.request.urlopen(origin+'/'+prefix+name) as response: raw=response.read()
        assert hashlib.sha256(raw).hexdigest()==pin['sha256'],name
    (root/'private-untracked-test.txt').write_text('not served')
    for path in ['.git/config','private-untracked-test.txt','scripts/../private-untracked-test.txt']:
        try: urllib.request.urlopen(origin+'/'+path)
        except urllib.error.HTTPError as error: assert error.code==404
        else: raise AssertionError('private path served')
    results=[]
    with sync_playwright() as p:
        browser=p.chromium.launch(executable_path=os.environ.get('CHROMIUM_PATH'),args=['--no-sandbox'])
        version=browser.version
        for width in [320,390,1280]:
            page=browser.new_page(viewport={'width':width,'height':900},reduced_motion='reduce' if width==390 else 'no-preference')
            errors=[]
            page.on('pageerror',lambda e:errors.append(str(e)))
            page.goto(origin+'/')
            page.locator('#prepare').click()
            page.wait_for_function("document.querySelector('#status').textContent.startsWith('Images verified.')")
            assert page.locator('#flash').is_disabled()
            assert page.evaluate('document.documentElement.scrollWidth <= innerWidth')
            pins=page.evaluate("async()=>{const {IMAGES}=await import('./assets.mjs');return IMAGES.map(i=>i.sha256)}")
            assert pins==[i['sha256'] for i in json.loads((package/'provenance.json').read_text())['images']]
            assert not errors
            page.screenshot(path=str(a.output/f'ui-{width}.png'),full_page=True)
            results.append({'width':width,'errors':errors,'images':'verified'})
            page.close()
        browser.close()
    (a.output/'result.json').write_text(json.dumps({'status':'PASS','chromium':version,'http_members':len(manifest['files']),'browser':results,'boundary':'no hardware; no private checkout dependency'},indent=2)+'\n')
finally:
    server.terminate()
    server.wait(timeout=10)
    log.close()
    (root/'private-untracked-test.txt').unlink(missing_ok=True)
print('PASS: documented verifier/server, exact HTTP members, safe paths, Node and Chromium')
