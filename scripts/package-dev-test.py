#!/usr/bin/env python3
"""Create a fresh local operator DEV TEST from immutable test02; never build or flash."""
import argparse, hashlib, importlib.util, json, re, zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
VERSION='education-candidate02-dev-test-01'
TEST02='36fc343d58f89d333079c107598dc417a7bcd5664974ae81f0a0c5b1e6d4d20f'
def sha(b): return hashlib.sha256(b).hexdigest()
def encoded(o): return (json.dumps(o,indent=2,sort_keys=True)+'\n').encode()
def need(ok,why):
    if not ok: raise ValueError(why)
def module(name,path):
    spec=importlib.util.spec_from_file_location(name,path); m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--test02',required=True,type=Path)
    ap.add_argument('--candidate',required=True,type=Path)
    ap.add_argument('--accepted-build',required=True,type=Path)
    ap.add_argument('--output',required=True,type=Path)
    a=ap.parse_args()
    need(a.output.name==VERSION+'.zip' and not a.output.exists(),'fresh dev-test namespace required')
    need(sha(a.test02.read_bytes())==TEST02,'immutable test02 hash')
    v=module('held_verifier',ROOT/'scripts/verify-successor-test-package.py')
    files=v.load(a.test02); original=dict(files); v.validate(files)
    check=module('image_verifier',ROOT/'scripts/verify-education-build.py')
    build=a.candidate/'source/fixture-firmware/build'; old=a.accepted_build
    receipt_raw=(a.candidate/'run/build-status.json').read_bytes(); receipt=json.loads(receipt_raw)
    need(sha(receipt_raw)==v.RECEIPT,'immutable candidate receipt')
    provenance=json.loads(files['provenance.json'])
    names={'boot':'bootloader/bootloader.bin','table':'partition_table/partition-table.bin','app':'entropylab_fixture.bin'}
    accepted_profile=json.loads(re.search(r'export const FACTORY_PROFILE = freeze\((\{.*\})\);',(ROOT/'flash/first-install/adapter/adapter.mjs').read_text())[1])
    configs={}; images={}
    for name in ['config/sdkconfig.json','bootloader/config/sdkconfig.json']:
        raw=(build/name).read_bytes(); prior=(old/name).read_bytes()
        c=json.loads(raw); p=json.loads(prior)
        need(c==p,'accepted/candidate generated configuration differs: '+name)
        if name=='config/sdkconfig.json': check.config_check(c)
        else:
            for key,value in {'IDF_TARGET':'esp32p4','ESP32P4_REV_MIN_FULL':100,'ESP32P4_REV_MAX_FULL':199,'ESPTOOLPY_FLASHMODE':'dio','ESPTOOLPY_FLASHSIZE':'32MB','ESPTOOLPY_FLASHFREQ':'80m','PARTITION_TABLE_OFFSET':32768}.items():
                need(c.get(key)==value,'bootloader config '+key)
        configs[name]={'candidate_sha256':sha(raw),'accepted_sha256':sha(prior),'semantic_equality':True}
    flash=json.loads((build/'flasher_args.json').read_text()); prior_flash=json.loads((old/'flasher_args.json').read_text())
    need(flash==prior_flash,'flash arguments differ')
    need(v.record((build/'flasher_args.json').read_bytes())==receipt['artifacts']['flasher_args.json'],'flash args receipt binding')
    for index,pin in enumerate(provenance['images']):
        role=pin['role']; name=names[role]; raw=(build/name).read_bytes(); prior=(old/name).read_bytes()
        need(raw==files[pin['path']] and v.record(raw)==receipt['artifacts'][name],'candidate image binding')
        need(sha(prior)==accepted_profile['assets'][index]['sha256'],'accepted binary baseline binding')
        need(int(next(k for k,val in flash['flash_files'].items() if val==name),0)==pin['offset'],'offset binding')
        if role!='table':
            current=check.image(raw); previous=check.image(prior)
            need(current==previous,'image hardware headers differ')
            images[role]={'candidate':current,'accepted':previous}
        else: need(raw==prior,'partition table changed')
    # Parse the accepted, byte-identical table including its MD5 record.
    import struct
    table=(build/names['table']).read_bytes(); partitions=[]
    for pos in range(0,len(table),32):
        record=table[pos:pos+32]; magic=struct.unpack_from('<H',record)[0]
        if magic==0xebeb:
            need(record[16:]==hashlib.md5(table[:pos]).digest(),'partition MD5');break
        need(magic==0x50aa,'partition record')
        _,kind,subtype,offset,size,label,flags=struct.unpack('<HBBII16sI',record)
        partitions.append({'type':kind,'subtype':subtype,'offset':offset,'size':size,'label':label.split(b'\0')[0].decode(),'flags':flags})
    else: raise ValueError('partition digest missing')
    factory=[p for p in partitions if p['type']==0 and p['subtype']==0]
    need(len(factory)==1 and factory[0]['offset']==65536,'factory app layout')
    adapter_name='flash/first-install/adapter/adapter.mjs'; adapter=files[adapter_name].decode()
    match=re.search(r'export const FACTORY_PROFILE = freeze\((\{.*\})\);',adapter); profile=json.loads(match[1])
    need(profile['assets'][-1]['eraseEnd']<=factory[0]['offset']+factory[0]['size'],'rounded application erase fit')
    need(profile['assets'][0]['eraseEnd']==32768 and profile['assets'][1]['eraseEnd']==36864,'boot/table rounded fit')
    compatibility={'status':'PASS-static-dev-test-only','accepted_evidence_sha256':sha((ROOT/'evidence/physical-acceptance.md').read_bytes()),'accepted_runtime':'dc5c65420aa96013ae3847ab4771a7f520f1c3a8','baseline_images':accepted_profile['assets'],'candidate_images':provenance['images'],'generated_configs':configs,'headers':images,'partitions':partitions,'flash_arguments_equal':True,'candidate_app_erase_end':profile['assets'][-1]['eraseEnd'],'accepted_app_erase_end':accepted_profile['assets'][-1]['eraseEnd'],'new_app_sector_scope':'inside same factory application partition; explicit destructive overwrite consent required','hardware':'NOT TESTED','reproducibility':'NOT TESTED'}
    files['compatibility.json']=encoded(compatibility)
    files['evidence/accepted-physical-run.md']=(ROOT/'evidence/physical-acceptance.md').read_bytes()
    # Only policy HOLD and profile identity change. Actual security/IO code is untouched.
    profile['profileId']=VERSION;profile['writeHold']=False
    adapter=adapter[:match.start()]+'export const FACTORY_PROFILE = freeze('+json.dumps(profile)+');'+adapter[match.end():]
    guard="      if (this.#p.writeHold) fail('SUCCESSOR_TEST_HOLD');\n"
    need(adapter.count(guard)==1,'exact artificial hold removal')
    adapter=adapter.replace(guard,''); files[adapter_name]=adapter.encode()
    provenance.update(package_version=VERSION,writes_enabled=True,diagnostics_enabled=True,dev_test_only=True,compatibility_sha256=sha(files['compatibility.json']))
    files['provenance.json']=encoded(provenance)
    html=files['flash/first-install/index.html'].decode().replace('EntropyLab successor TEST — HOLD','EntropyLab DEV TEST — user operated').replace('EntropyLab — successor TEST','EntropyLab — DEV TEST')
    html=html.replace('Physical install and diagnostics are disabled by an enforced adapter hold. Image verification and presentation remain runnable. No inherited device acceptance.','Diagnostic and installation are ENABLED for this local user-operated dev-board test. Preserve the existing verified backup before continuing. No inherited device acceptance; no automatic retry.')
    html=html.replace('I have exported anything needed and accept destructive factory replacement without guaranteed recovery or automatic backup.','I have preserved my verified dev-board backup and accept overwriting the three image sector windows, including erased tails, without automatic backup, recovery, or retry.')
    html=html.replace('Replace factory firmware','Install DEV TEST firmware')
    files['flash/first-install/index.html']=html.encode()
    readme=f'''# EntropyLab {VERSION}

LOCAL DEV TEST ONLY. Public distribution HOLD; **diagnostic and install are enabled**.
Not a production release or hardware-accepted candidate. Never use real secrets.

Firmware is unchanged candidate02, source `{v.SOURCE}`, recipe `{v.RECIPE}`.
Static compatibility against the accepted EL-002 tuple passed: see compatibility.json.
Generated app and bootloader configurations are identical; image chip/revision and flash
settings match; partition table bytes and flash offsets match. New app erase end is
{profile['assets'][-1]['eraseEnd']} (one extra sector inside the same application partition).
No app-only or prior-content authentication claim: this is the explicit three-image
replacement route, preserving all bytes outside its sector-rounded write windows by
command scope, not by a full-chip readback proof.

## Operator steps — one attempt, preserved backup
1. Keep the existing verified dev-board backup and recovery records outside this served
   directory. This installer does not make a backup. Do not proceed if that backup is missing.
2. Extract into a new directory. Run `python3 verify-package.py .`.
3. Serve only this extraction locally: `python3 -m http.server 8765 --bind 127.0.0.1`.
   Open `http://127.0.0.1:8765/flash/first-install/` in desktop Chrome/Chromium.
4. Confirm Waveshare P4 4.3 PCB rev1.3 / 32 MiB, Verify three images, select the
   USB-to-UART port, and run the no-write diagnostic. It loads the official RAM stub.
   ROM ECO must be 0 or 2; chip 18 / revision 100–199; secure boot/download off;
   encryption count zero; raw JEDEC density 32 MiB. Any refusal means STOP.
5. After diagnostic completion, reload, verify, select again, and explicitly check board
   and preserved-backup/destructive consent. Click **Install DEV TEST firmware** once.
   Expected: 3 image writes and SHA-256 verification of 3 images + 3 all-FF tails.
   Baud changes 115200 → 460800 only after security and official stub checks.
6. Only after verified completion, manually RESET. Check boot, touch, public 24-word
   fixture/fingerprint, education pages, Back and retained Test results. These are tests
   to perform, not claims already established for candidate02.
7. On failure/cancel/unplug, STOP. No automatic retry, rollback, erase-all, eFuse change,
   security bypass or C6 write. Preserve the finite failure report for review before
   authorizing another attempt. Do not press reset as a substitute for readback success.

## Sources, notices and assurance
Complete tracked corresponding firmware/core sources and exact original notices are
under source/; installer/vendor notices and root licenses are retained unchanged.
External dependencies remain lock-pinned, not vendored. Public distribution licensing,
independent reproducibility, exact-candidate physical acceptance, recovery qualification
and publication approval remain open; none is represented as satisfied by this DEV TEST.
Tests model transport and public memory, never private backups. Hashes establish local
byte consistency, not authenticity. No firmware rebuild was performed.
'''
    files['README.md']=readme.encode();files['docs/PUBLIC-FIRST-INSTALL.md']=readme.encode()
    verifier=(ROOT/'scripts/verify-successor-test-package.py').read_text().replace("VERSION = 'education-candidate02-test-02'",f'VERSION = {VERSION!r}')
    verifier=verifier.replace("p['writes_enabled'] is False and p['diagnostics_enabled'] is False","p['writes_enabled'] is True and p['diagnostics_enabled'] is True and p['dev_test_only'] is True")
    verifier=verifier.replace("profile['writeHold'] is True","profile['writeHold'] is False")
    verifier=verifier.replace('need("if (this.#p.writeHold) fail(\'SUCCESSOR_TEST_HOLD\');" in adapter,\'enforced hold\')',f'need(sha(files["{adapter_name}"]) == {sha(files[adapter_name])!r}, "exact dev adapter incl safety gates")\n    need(sha(files["compatibility.json"]) == {sha(files["compatibility.json"])!r} == p["compatibility_sha256"], "compatibility binding")')
    files['verify-package.py']=verifier.encode()
    smoke=(ROOT/'scripts/test-successor-smoke.mjs').read_text().split("test('package refuses both hardware entry points")[0].replace('education-candidate02-test-02',VERSION)
    files['tests/successor-smoke.test.mjs']=smoke.encode()
    for name in ['wire.test.mjs','speed-browser.mjs']:
        files['flash/first-install/'+name]=(ROOT/'flash/first-install'/name).read_bytes()
    # Source/notices and binaries must stay byte-for-byte, not only semantically similar.
    for name,raw in original.items():
        if name.startswith('source/') or '/firmware/' in name or 'LICENSE' in name or 'NOTICES' in name:
            need(files[name]==raw,'immutable source/license/image changed '+name)
    files['package-manifest.json']=encoded({'schema':'entropylab-successor-test-package-v1','version':VERSION,'distribution':'HOLD','files':{k:v.record(b) for k,b in sorted(files.items()) if k!='package-manifest.json'}})
    a.output.parent.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(a.output,'x',compression=zipfile.ZIP_DEFLATED) as z:
        for name,raw in sorted(files.items()):
            info=zipfile.ZipInfo(name,(1980,1,1,0,0,0));info.create_system=3;info.external_attr=0o100644<<16;info.compress_type=zipfile.ZIP_DEFLATED;z.writestr(info,raw)
    print(json.dumps({'zip':str(a.output),'sha256':sha(a.output.read_bytes()),'members':len(files),'compatibility':compatibility},indent=2))
if __name__=='__main__': main()
