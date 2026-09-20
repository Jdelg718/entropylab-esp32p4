#!/usr/bin/env python3
"""Exercise the immutable TEST ZIP, freshly extracted, never source-tree UI."""
import argparse
import functools
import hashlib
import http.server
import importlib.util
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import threading
import zipfile
from playwright.sync_api import sync_playwright

ap = argparse.ArgumentParser()
ap.add_argument('archive', type=Path)
ap.add_argument('--output', type=Path, required=True)
a = ap.parse_args()
a.output.mkdir(parents=True, exist_ok=True)
sha = lambda b: hashlib.sha256(b).hexdigest()
assert sha(a.archive.read_bytes()) == '36fc343d58f89d333079c107598dc417a7bcd5664974ae81f0a0c5b1e6d4d20f'
result = {'zip_sha256': sha(a.archive.read_bytes()), 'boundary': 'Fresh ZIP extraction; loopback Chromium; no hardware or firmware build'}
with tempfile.TemporaryDirectory(prefix='successor-closeout-') as tmp:
    root = Path(tmp)
    with zipfile.ZipFile(a.archive) as z:
        z.extractall(root)
    spec = importlib.util.spec_from_file_location('verify', root/'verify-package.py')
    v = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(v)
    files = v.load(root)
    result['verification'] = v.validate(files)
    node = subprocess.run(['node', '--test', 'tests/successor-smoke.test.mjs'], cwd=root, capture_output=True, text=True)
    (a.output/'node-smoke.log').write_text(node.stdout + node.stderr)
    assert node.returncode == 0
    result['node'] = 'PASS (3 tests)'
    profile = json.loads(re.search(r'export const FACTORY_PROFILE = freeze\((\{.*\})\);', files['flash/first-install/adapter/adapter.mjs'].decode())[1])
    result['firmware'] = json.loads(files['provenance.json'])['images']
    result['ff_tails'] = []
    for image, tail in zip(profile['assets'], profile['tailReadbacks']):
        start = image['offset'] + image['length']
        end = ((start + 4095)//4096)*4096
        assert tail == {'offset': start, 'length': end-start, 'sha256': sha(b'\xff'*(end-start))}
        result['ff_tails'].append(tail)
    # Rehash changed members to challenge semantic anchors, not only inventory hashes.
    def mutated(name, data, rehash=True):
        f = dict(files)
        f[name] = data
        if rehash:
            m = json.loads(f['package-manifest.json'])
            m['files'][name] = v.record(data)
            f['package-manifest.json'] = json.dumps(m).encode()
        return f
    adapter_name = 'flash/first-install/adapter/adapter.mjs'
    adapter = files[adapter_name]
    cases = {
        'firmware-bitflip': mutated('flash/first-install/firmware/entropylab.bin', files['flash/first-install/firmware/entropylab.bin'][:-1]+b'X'),
        'extra-member': {**files, 'unexpected.txt': b'extra'},
        'missing-notices': {k: b for k,b in files.items() if k != 'flash/first-install/NOTICES.txt'},
        'hold-disabled': mutated(adapter_name, adapter.replace(b'"writeHold": true', b'"writeHold": false')),
        'hold-removed': mutated(adapter_name, adapter.replace(b"if (this.#p.writeHold) fail('SUCCESSOR_TEST_HOLD');", b'')),
        'wrong-tail-length': mutated(adapter_name, adapter.replace(json.dumps(profile['tailReadbacks']).encode(), json.dumps([{**t, 'length': t['length']+1} for t in profile['tailReadbacks']]).encode())),
        'wrong-download-hash': mutated('flash/first-install/assets.mjs', files['flash/first-install/assets.mjs'].replace(v.PINS[2][4].encode(), b'0'*64)),
        'wrong-source': mutated('provenance.json', files['provenance.json'].replace(v.SOURCE.encode(), b'0'*40)),
        'private-path': mutated('README.md', files['README.md'] + b'\n/opt/' + b'data/private'),
        'traversal': {**files, '../escape': b'bad'},
    }
    result['negative_mutations'] = {}
    for name, f in cases.items():
        try:
            v.validate(f)
        except (ValueError, KeyError) as e:
            result['negative_mutations'][name] = str(e)
        else:
            raise AssertionError('Mutation accepted: ' + name)
    class Quiet(http.server.SimpleHTTPRequestHandler):
        def log_message(self, *_): pass
    server = http.server.ThreadingHTTPServer(('127.0.0.1', 0), functools.partial(Quiet, directory=str(root)))
    thread = threading.Thread(target=server.serve_forever, daemon=True)
    thread.start()
    origin = f'http://127.0.0.1:{server.server_port}'
    result['browser'] = []
    try:
        with sync_playwright() as p:
            browser = p.chromium.launch(executable_path=os.environ.get('CHROMIUM_PATH'), args=['--no-sandbox'])
            result['chromium'] = browser.version
            for width, motion in [(320, 'no-preference'), (390, 'reduce'), (1280, 'no-preference')]:
                page = browser.new_page(viewport={'width': width, 'height': 900}, reduced_motion=motion)
                errors = []
                downloads = []
                page.on('pageerror', lambda e: errors.append(str(e)))
                page.on('response', lambda r: downloads.append(r.url) if '/firmware/' in r.url and r.status == 200 else None)
                page.goto(origin+'/flash/first-install/')
                assert 'HOLD' in page.title()
                assert page.get_by_text('LOCAL TEST ONLY — NOT AN ACCEPTED RELEASE.', exact=True).is_visible()
                assert page.locator('#flash').is_disabled() and page.locator('#check').is_disabled()
                page.locator('#prepare').click()
                page.wait_for_function("document.querySelector('#status').textContent.startsWith('Images verified.')")
                assert len(downloads) == 3
                pins = page.evaluate("""async () => {
                    const {IMAGES}=await import('./assets.mjs');
                    const {FACTORY_PROFILE}=await import('./adapter/adapter.mjs');
                    const provenance=await (await fetch('../../provenance.json')).json();
                    const manifest=await (await fetch('../../package-manifest.json')).json();
                    return {images:IMAGES,profile:FACTORY_PROFILE,provenance,manifest};
                }""")
                assert pins['profile']['writeHold'] is True
                assert pins['provenance']['images'] == result['firmware']
                for i, image in enumerate(result['firmware']):
                    assert pins['images'][i]['sha256'] == image['sha256']
                    assert pins['manifest']['files'][image['path']]['sha256'] == image['sha256']
                assert page.locator('#status').get_attribute('aria-live') == 'polite'
                assert page.evaluate('document.documentElement.scrollWidth <= innerWidth')
                assert page.locator('#flash').is_disabled() and page.locator('#check').is_disabled()
                page.screenshot(path=str(a.output/f'extracted-ui-{width}.png'), full_page=True)
                page.locator('#cancel').click()
                assert page.locator('#status').inner_text().startswith('Cancelled.')
                assert page.locator('#connect').is_disabled()
                assert not errors, errors
                result['browser'].append({'width': width, 'motion': motion, 'download_count': len(downloads), 'errors': errors, 'status': 'PASS'})
                page.close()
            # Real downloader error path over HTTP, only response bytes synthetic.
            page = browser.new_page()
            page.route('**/firmware/entropylab.bin', lambda route: route.fulfill(body=b'corrupt', content_type='application/octet-stream'))
            page.goto(origin+'/flash/first-install/')
            page.locator('#prepare').click()
            page.wait_for_function("document.querySelector('#status').textContent.startsWith('Asset preparation refused.')")
            assert page.locator('#connect').is_disabled()
            result['browser_corrupt_download'] = 'PASS: refused; port selection disabled'
            browser.close()
    finally:
        server.shutdown()
        server.server_close()
result['status'] = 'PASS-local-test-package-only'
(a.output/'closeout.json').write_text(json.dumps(result, indent=2)+'\n')
print(json.dumps(result, indent=2))
