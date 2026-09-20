#!/usr/bin/env python3
"""Verify exact successor TEST archive inventory and semantic anchors, offline."""
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import sys
import zipfile

VERSION = 'education-candidate02-dev-test-01'
SOURCE = '2b919dc73c9cbcbb5e845850650d71c6782ad68b'
RECIPE = '0ec7f6dd8ec62d6d554fea6726deeeb5a795a102'
RECEIPT = '96e78c97c730a5a14a565ede5a375812a067af1333fc157887ca454c71bce2a8'
PINS = [('boot','bootloader.bin',8192,21264,'4f00f81aad82838f4e33555e322abfbfff7d1de947d339c86e50be820cbd7bb4'),('table','partition-table.bin',32768,3072,'d3e6663d9cbd407623c82f215df58a5c9bd1e353fd937ad519018c06fd9298fb'),('app','entropylab.bin',65536,1542288,'42d5e3b40a3869157171552162189a281c84b893c8533fe3ae8266bfc4c47550')]

def sha(b): return hashlib.sha256(b).hexdigest()
def record(b): return {'bytes':len(b),'sha256':sha(b)}
def need(ok, why):
    if not ok: raise ValueError(why)
def validate(files):
    for name in files:
        p=PurePosixPath(name)
        need(not p.is_absolute() and '..' not in p.parts and '\\' not in name and str(p)==name, 'unsafe member')
        private = rb'/opt/' + rb'data/|/Us' + rb'ers/|/ho' + rb'me/[^ /\n]+/'
        need(not re.search(private,files[name]),'private path '+name)
    manifest=json.loads(files['package-manifest.json'])
    need(manifest['schema']=='entropylab-successor-test-package-v1' and manifest['version']==VERSION and manifest['distribution']=='HOLD','package identity')
    need(set(files)==set(manifest['files'])|{'package-manifest.json'},'exact inventory')
    for name,pin in manifest['files'].items(): need(record(files[name])==pin,'member hash '+name)
    p=json.loads(files['provenance.json'])
    need(p['package_version']==VERSION and p['source_revision']==SOURCE and p['recipe_revision']==RECIPE and p['build_receipt_sha256']==RECEIPT,'candidate source/receipt binding')
    need(p['source_manifest_sha256']=='66231dae53a45e30470f7f26ab50cb8a9aff3d500aafb7c78fcdedfd755bbb13','source manifest binding')
    need(p['descriptor']=='e66231dae53a45e30470f7f26-e1','descriptor identity')
    need(p['distribution']=='HOLD' and p['hardware']=='NOT TESTED' and p['reproducibility']=='NOT TESTED' and p['writes_enabled'] is True and p['diagnostics_enabled'] is True and p['dev_test_only'] is True,'assurance scope')
    expected=[{'role':r,'path':'flash/first-install/firmware/'+n,'offset':o,'bytes':s,'sha256':h} for r,n,o,s,h in PINS]
    need(p['images']==expected,'exact candidate tuple')
    for i in expected: need(record(files[i['path']])=={'bytes':i['bytes'],'sha256':i['sha256']},'candidate bytes')
    text=files['flash/first-install/assets.mjs'].decode()
    pins=json.loads(re.search(r'export const IMAGES=Object.freeze\((\[.*\])\.map',text)[1])
    need(pins==[{'name':r,'url':'./firmware/'+n,'offset':o,'size':s,'sha256':h} for r,n,o,s,h in PINS],'download pins')
    adapter=files['flash/first-install/adapter/adapter.mjs'].decode()
    profile=json.loads(re.search(r'export const FACTORY_PROFILE = freeze\((\{.*\})\);',adapter)[1])
    assets=[{'role':r,'offset':o,'length':s,'sha256':h,'eraseEnd':((o+s+4095)//4096)*4096} for r,n,o,s,h in PINS]
    need(profile['assets']==assets and profile['profileId']==VERSION and profile['writeHold'] is False,'adapter pins/hold')
    need(profile['tailReadbacks']==[{'offset':a['offset']+a['length'],'length':a['eraseEnd']-a['offset']-a['length'],'sha256':sha(b'\xff'*(a['eraseEnd']-a['offset']-a['length']))} for a in assets],'tail geometry')
    need(profile['preserveToolRegions']==[[0,8192],[36864,65536],[assets[-1]['eraseEnd'],33554432]],'preservation geometry')
    need(sha(files["flash/first-install/adapter/adapter.mjs"]) == 'c6d7b17b5e1c75b15781157858a922b40a7c9c78a3e7c4003e414041aa747e21', "exact dev adapter incl safety gates")
    need(sha(files["compatibility.json"]) == '45564045c9c899b02dbf528ee38a11b89ceab8f2b6e66e824e01bed5a3563ff4' == p["compatibility_sha256"], "compatibility binding")
    source_manifest=files['source/build-recipe/source.json']
    need(sha(source_manifest)==p['source_manifest_sha256'],'immutable source inventory')
    all_entries=json.loads(source_manifest)['entries']
    selected={name:pin for name,pin in all_entries.items() if name.startswith(('fixture-firmware/','core-spike/','docs/release-notices/')) or name.startswith('LICENSE') or name in ('THIRD_PARTY_NOTICES.md','docs/COMBINED-RUST-LICENSES.json','docs/RUST-DEPENDENCIES.md','docs/VECTOR-LICENSE-SOURCES.md')}
    inv=json.loads(files['source/source-inventory.json'])
    need(inv['source_revision']==SOURCE and inv['files']==selected,'corresponding source inventory')
    for name,pin in inv['files'].items(): need(record(files['source/'+name])==pin,'source hash')
    need('fixture-firmware/app/main/education_content.inc' in inv['files'],'education source required')
    notices=json.loads(files['source/docs/release-notices/manifest.json'])
    combined=files['flash/first-install/NOTICES.txt']
    for name,pin in notices['files'].items():
        data=files['source/'+name]
        need(record(data)=={'bytes':pin['bytes'],'sha256':pin['sha256']} and data in combined,'exact notice '+name)
    for name in ('LICENSE','LICENSE-MIT','LICENSE-OOGA-BOOGA','LICENSE-TREZOR-MIT','LICENSE-BIP32-BSD-2-CLAUSE'):
        need(files[name]==files['source/'+name],'license coverage')
    return {'status':'PASS-local-test-package-only','members':len(files),'source_files':len(inv['files']),'notices':len(notices['files']),'version':VERSION}

def load(path):
    if path.is_dir():
        paths=list(path.rglob('*'))
        need(not any(p.is_symlink() for p in paths),'symlink')
        return {p.relative_to(path).as_posix():p.read_bytes() for p in paths if p.is_file()}
    with zipfile.ZipFile(path) as z:
        need(z.testzip() is None,'CRC')
        need(len(z.namelist())==len(set(z.namelist())),'duplicate members')
        need(all(i.external_attr>>16==0o100644 for i in z.infolist()),'member modes')
        return {n:z.read(n) for n in z.namelist()}
if __name__=='__main__':
    print(json.dumps(validate(load(Path(sys.argv[1]))),indent=2))
