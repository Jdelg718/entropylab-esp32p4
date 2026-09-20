#!/usr/bin/env python3
"""Explicit local successor TEST packaging. Does not change official release checks/pins."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE = '2b919dc73c9cbcbb5e845850650d71c6782ad68b'
RECIPE = '0ec7f6dd8ec62d6d554fea6726deeeb5a795a102'
VERSION = 'education-candidate02-test-02'

def sha(b): return hashlib.sha256(b).hexdigest()
def record(b): return {'bytes': len(b), 'sha256': sha(b)}
def encoded(d): return (json.dumps(d, sort_keys=True, indent=2)+'\n').encode()
def git(*args): return subprocess.check_output(['git', '-C', str(ROOT), *args])
def require(c, m):
    if not c: raise ValueError(m)
def blob(rev, name): return git('show', rev+':'+name)

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--candidate', type=Path, required=True)
    ap.add_argument('--external-inputs', type=Path, required=True)
    ap.add_argument('--tool-prefix', required=True)
    ap.add_argument('--output', type=Path, required=True)
    args = ap.parse_args()
    out = args.output.resolve()
    require(out.name == 'successor-test-package', 'isolated output namespace required')
    out.mkdir(parents=True, exist_ok=True)
    target = out/(VERSION+'.zip')
    require(not target.exists(), 'never overwrite a candidate ZIP; choose a new version')
    ev = out/'evidence'; ev.mkdir(exist_ok=True)
    reports = {}
    for kind, script, options in [('image','verify-education-build.py',['--external-inputs',str(args.external_inputs)]), ('runtime','verify-education-runtime.py',['--tool-prefix',args.tool_prefix])]:
        raw = subprocess.check_output(['python3','-B',str(ROOT/'scripts'/script),str(args.candidate),*options])
        (ev/(kind+'.json')).write_bytes(raw)
        reports[kind] = json.loads(raw)
    require(reports['image']['status']=='PASS-local-only', 'image verification')
    require(reports['runtime']['status']=='PASS-supplemental-only', 'runtime verification')
    receipt_raw = (args.candidate/'run/build-status.json').read_bytes()
    receipt = json.loads(receipt_raw)
    require(receipt['source_revision']==SOURCE and receipt['recipe_revision']==RECIPE, 'candidate revisions')
    require(receipt['status']=='local-build-passed' and receipt['process_group_quiescent'] is True, 'completed build required')
    require(reports['image']['build_receipt_sha256']==sha(receipt_raw), 'receipt binding')
    payload = {}
    # Corresponding first-party firmware/core source and all existing license notices:
    # exact Git blobs, never the writable build tree, debug ELF, MAP, or raw receipts.
    paths = git('ls-tree','-r','--name-only','-z',SOURCE).decode().rstrip('\0').split('\0')
    source_files = {}
    for name in paths:
        if name.startswith(('fixture-firmware/','core-spike/','docs/release-notices/')) or name.startswith('LICENSE') or name in ('THIRD_PARTY_NOTICES.md','docs/COMBINED-RUST-LICENSES.json','docs/RUST-DEPENDENCIES.md','docs/VECTOR-LICENSE-SOURCES.md'):
            data = blob(SOURCE,name)
            payload['source/'+name] = data
            source_files[name] = record(data)
    require(any(n.endswith('education_content.inc') for n in source_files), 'education source missing')
    for name in receipt['recipe_sha256']:
        data = blob(RECIPE,'scripts/'+name)
        require(sha(data)==receipt['recipe_sha256'][name], 'recipe mismatch')
        payload['source/build-recipe/'+name] = data
    payload['source/source-inventory.json'] = encoded({'source_revision':SOURCE,'source_tree':git('rev-parse',SOURCE+'^{tree}').decode().strip(),'files':source_files,'scope':'Complete tracked firmware and core source; exact license notices. Historical release binaries, receipts, unrelated documentation and Git metadata excluded. External dependencies remain pinned by locks, not vendored.'})
    payload['source/build-recipe/source.json'] = (ROOT/'docs/education-successor/source.json').read_bytes()
    payload['THIRD_PARTY_NOTICES.md'] = blob(SOURCE,'THIRD_PARTY_NOTICES.md')
    for name in paths:
        if name.startswith('LICENSE'): payload[name] = blob(SOURCE,name)
    for name in paths:
        if name.startswith('flash/first-install/') and '/firmware/' not in name and not name.endswith('.test.mjs') and not name.endswith('speed-browser.mjs'):
            payload[name] = blob(SOURCE,name)
    images = []
    for role, src, dest in [('boot','bootloader/bootloader.bin','bootloader.bin'),('table','partition_table/partition-table.bin','partition-table.bin'),('app','entropylab_fixture.bin','entropylab.bin')]:
        data = (args.candidate/'source/fixture-firmware/build'/src).read_bytes()
        require(record(data)==receipt['artifacts'][src]==reports['image']['artifacts'][src], 'artifact binding '+role)
        path = 'flash/first-install/firmware/'+dest
        payload[path] = data
        images.append({'role':role,'path':path,'offset':reports['image']['flash_offsets'][src],**record(data)})
    asset_path = 'flash/first-install/assets.mjs'
    tail = payload[asset_path].decode().split('export async function downloadImages',1)[1]
    pins = [{'name':i['role'],'url':'./firmware/'+Path(i['path']).name,'offset':i['offset'],'size':i['bytes'],'sha256':i['sha256']} for i in images]
    payload[asset_path] = ('export const IMAGES=Object.freeze('+json.dumps(pins)+'.map(Object.freeze));\nexport async function downloadImages'+tail).encode()
    adapter_path = 'flash/first-install/adapter/adapter.mjs'
    adapter = payload[adapter_path].decode()
    match = re.search(r'export const FACTORY_PROFILE = freeze\((\{.*\})\);',adapter)
    require(match is not None,'exact profile declaration')
    profile = json.loads(match[1])
    profile['profileId'] = VERSION
    profile['writeHold'] = True
    profile['assets'] = [{'role':i['role'],'offset':i['offset'],'length':i['bytes'],'sha256':i['sha256'],'eraseEnd':((i['offset']+i['bytes']+4095)//4096)*4096} for i in images]
    profile['tailReadbacks'] = [{'offset':i['offset']+i['length'],'length':i['eraseEnd']-i['offset']-i['length'],'sha256':sha(b'\xff'*(i['eraseEnd']-i['offset']-i['length']))} for i in profile['assets']]
    profile['preserveToolRegions'] = [[0,8192],[36864,65536],[profile['assets'][-1]['eraseEnd'],33554432]]
    adapter = adapter[:match.start()]+ 'export const FACTORY_PROFILE = freeze('+json.dumps(profile)+');'+adapter[match.end():]
    needle = '      this.#check();\n      if (boardConfirmation !== BOARD)'
    require(adapter.count(needle)==1, 'hold insertion anchor')
    adapter = adapter.replace(needle, "      this.#check();\n      if (this.#p.writeHold) fail('SUCCESSOR_TEST_HOLD');\n      if (boardConfirmation !== BOARD)")
    payload[adapter_path] = adapter.encode()
    index = payload['flash/first-install/index.html'].decode()
    index = index.replace('<title>EntropyLab destructive first installation</title>', '<title>EntropyLab successor TEST — HOLD</title>').replace('<h1>EntropyLab — first installation</h1>','<h1>EntropyLab — successor TEST</h1><p><strong>LOCAL TEST ONLY — NOT AN ACCEPTED RELEASE.</strong> Candidate02 firmware. Physical install and diagnostics are disabled by an enforced adapter hold. Image verification and presentation remain runnable. No inherited device acceptance.</p>')
    payload['flash/first-install/index.html'] = index.encode()
    payload['docs/PUBLIC-FIRST-INSTALL.md'] = b'# Successor TEST safety boundary\n\nLocal packaging and browser preview only. Do not connect hardware. The adapter refuses both install and diagnostic execution before constructing a transport. This package has no physical acceptance and no recovery qualification. It is not an app-only update or a public release. Never use real wallet secrets. A separately reviewed successor and explicit authorization are required to remove the hold.\n'
    payload['README.md'] = (f'# EntropyLab {VERSION}\n\nLOCAL TEST ONLY. Distribution HOLD. Not a production-accepted release.\n\nFirmware source: `{SOURCE}`; completed local candidate02; recipe `{RECIPE}`. Firmware descriptor is unchanged: `{receipt["descriptor"]}`. Package version is a separate identity, not an edited firmware descriptor. Partition bytes are unchanged from the predecessor; bootloader and app are new candidate bytes.\n\nRun `python3 verify-package.py .` and `node --test tests/successor-smoke.test.mjs`. Serve this extracted directory on loopback only with `python3 -m http.server 8765 --bind 127.0.0.1`, then open `/flash/first-install/`. Use Verify three images; do not connect hardware. The adapter enforces SUCCESSOR_TEST_HOLD before transport creation for both operations.\n\nSource and exact original notices are under `source/`; installer vendor notices and combined binary notices are preserved. Dependencies remain external and lock-pinned; this is not complete binary distribution clearance. Historical license-manifest candidate identity is retained solely as notice provenance, not this firmware acceptance.\n\nNo raw local receipts, environment, ELF or MAP are included. provenance.json contains only whitelisted summaries and receipt hashes. Hashes prove byte consistency, not authenticity. No independent reproducibility or physical behavior claimed.\n\nRemaining gates: independent exact-package review; exact candidate first-install/app-update compatibility review; authorized device write/readback/boot/touch/education/retention/recovery acceptance; repeat outside-window qualification; independent build/reproducibility; complete dependency-license clearance; explicit publication authorization. All remain open.\n').encode()
    provenance = {'schema':'entropylab-successor-test-provenance-v1','package_version':VERSION,'source_revision':SOURCE,'recipe_revision':RECIPE,'supplemental_verifier_revision':git('rev-parse','33c90ec').decode().strip(),'animation_test_revision':git('rev-parse','ce73b91').decode().strip(),'source_manifest_sha256':receipt['source_manifest_sha256'],'build_receipt_sha256':sha(receipt_raw),'candidate':'education-candidate-hardened-02','descriptor':receipt['descriptor'],'images':images,'image_verifier_status':reports['image']['status'],'runtime_verifier_status':reports['runtime']['status'],'supplemental_report_sha256':{k:sha((ev/(k+'.json')).read_bytes()) for k in reports},'distribution':'HOLD','hardware':'NOT TESTED','reproducibility':'NOT TESTED','writes_enabled':False,'diagnostics_enabled':False,'source_unchanged':True,'partition_unchanged':True}
    payload['provenance.json'] = encoded(provenance)
    for src,dest in [('verify-successor-test-package.py','verify-package.py'),('test-successor-smoke.mjs','tests/successor-smoke.test.mjs'),('test-progress-animation.py','scripts/test-progress-animation.py')]:
        payload[dest]=(ROOT/'scripts'/src).read_bytes()
    # Public payload privacy tripwire; generated reports outside the archive are private.
    for name,data in payload.items():
        require(not re.search(rb'/opt/data/|/Users/|/home/[^ /\n]+/|education-candidate-hardened-02/run/',data),'private path in '+name)
    payload['package-manifest.json'] = encoded({'schema':'entropylab-successor-test-package-v1','version':VERSION,'distribution':'HOLD','files':{k:record(v) for k,v in sorted(payload.items())}})
    with zipfile.ZipFile(target,'x',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as z:
        for name,data in sorted(payload.items()):
            info=zipfile.ZipInfo(name,(1980,1,1,0,0,0));info.create_system=3;info.external_attr=0o100644<<16;info.compress_type=zipfile.ZIP_DEFLATED
            z.writestr(info,data,compresslevel=9)
    print(json.dumps({'zip':str(target),'sha256':sha(target.read_bytes()),'bytes':target.stat().st_size,'members':len(payload)},indent=2))

if __name__=='__main__': main()
