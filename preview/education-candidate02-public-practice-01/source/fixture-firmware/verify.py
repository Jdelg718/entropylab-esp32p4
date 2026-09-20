#!/usr/bin/env python3
"""Fail-closed candidate audit; invoke after build.sh."""
from pathlib import Path
import json, struct, hashlib, re, subprocess
r=Path(__file__).parent
cfg=(r/'app/sdkconfig').read_text()
for v in ['CONFIG_ESP32P4_REV_MIN_FULL=100','CONFIG_ESP32P4_REV_MAX_FULL=199','CONFIG_SPIRAM_SPEED=200','CONFIG_ESPTOOLPY_FLASHSIZE="32MB"']:
    assert v in cfg,v
for name in ['entropylab_fixture.bin','bootloader/bootloader.bin']:
    data=(r/'build'/name).read_bytes()
    assert data[0]==0xe9
    minimum,maximum=struct.unpack_from('<HH',data,15)
    assert (minimum,maximum)==(100,199),(name,minimum,maximum)
headers=(r/'logs/archive-headers.txt').read_text()
flags=re.findall(r'Flags:\s+(.*)',headers)
assert flags and all('single-float ABI' in f for f in flags),set(flags)
symbols=(r/'logs/symbols.txt').read_text()
for symbol in ['el_input_to_mnemonic','el_bip39_passphrase_run','el_coin_to_hex','el_dice_to_hex','el_dice_required_rolls','fixture_alloc','fixture_free']:
    assert re.search(r'\bT '+symbol+r'$',symbols,re.M),symbol
# The combined app uses conversion/passphrase APIs. Older HEX/mnemonic entry
# points are still required in the source-built archive, but are unreferenced
# and garbage-collected from both the recorded installed ELF and a fresh link.
archive_symbols=subprocess.check_output(['riscv32-esp-elf-nm','--defined-only',str(r/'runtime/target/riscv32imafc-esp-espidf/release/libentropylab_runtime.a')],text=True)
for symbol in ['el_hex_run','el_mnemonic_run']:
    assert re.search(r'\bT '+symbol+r'$',archive_symbols,re.M),symbol
# No application activation of networking, storage, signing or entropy acquisition.
source=''.join(p.read_text() for p in (r/'app/main').rglob('*') if p.suffix in {'.c','.h','.inc'})
source+=''.join(p.read_text().split('#[cfg(test)]')[0] for p in (r/'runtime-sources').glob('*/src/*.rs'))
for forbidden in ['esp_wifi_init(', 'esp_netif_init(', 'nvs_flash_init(', 'esp_fill_random(', 'esp_random(', '.sign_ecdsa(', '.sign_schnorr(']:
    assert forbidden not in source,forbidden
report={'status':'PASS','runtime_hardware_tested':False,'archive_members':len(flags),'revision':[100,199],'artifacts':{str(p.relative_to(r)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [r/'build/entropylab_fixture.bin',r/'build/entropylab_fixture.elf']}}
(r/'logs/verification.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
