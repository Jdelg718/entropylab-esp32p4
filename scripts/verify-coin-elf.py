#!/usr/bin/env python3
from pathlib import Path
import subprocess,re
r=Path(__file__).resolve().parents[1]/'fixture-firmware'
elf=r/'build/entropylab_fixture.elf'
for archive in [r/'dice/target/riscv32imafc-esp-espidf/release/libentropylab_dice_core.a',r/'coin/target/libentropylab_coin_core_linkable.a']:
 headers=subprocess.check_output(['riscv32-esp-elf-readelf','-h',str(archive)],text=True)
 flags=[x for x in headers.splitlines() if 'Flags:' in x]
 assert flags and all('RVC, single-float ABI' in x for x in flags),archive
for name in ['gui.c','main.c']:
 obj=r/'build/esp-idf/main/CMakeFiles/__idf_main.dir'/f'{name}.obj'
 assert (r/'app/main'/name).stat().st_mtime_ns<=obj.stat().st_mtime_ns<=elf.stat().st_mtime_ns
 for header in (r/'app/main').glob('*.h'):assert header.stat().st_mtime_ns<=obj.stat().st_mtime_ns
 assert 'RVC, single-float ABI' in subprocess.check_output(['riscv32-esp-elf-readelf','-h',str(obj)],text=True)
s=subprocess.check_output(['riscv32-esp-elf-nm','--defined-only',str(elf)],text=True)
for name in ['el_dice_to_hex','el_dice_required_rolls','el_coin_to_hex','el_hex_run','fixture_alloc','fixture_free','abort']:
 assert len(re.findall(r'\bT '+name+r'$',s,re.M))==1,name
panic=[x.split()[-1] for x in s.splitlines() if ' T ' in x and 'rust_begin_unwind' in x];assert len(panic)==1
m=(r/'build/entropylab_fixture.map').read_text()
assert 'libentropylab_dice_core.a' in m and 'libentropylab_coin_core_linkable.a' in m
assert 'libentropylab_hex_core.a' not in m
for p in [r/'build/build.ninja',r/'app/main/CMakeLists.txt']:
 assert not any(x in p.read_text() for x in ['allow-multiple-definition','muldefs'])
# The sole global panic is defined by hex (adapter is localized before linking).
hexnm=subprocess.check_output(['riscv32-esp-elf-nm','--defined-only',str(r/'dice/target/riscv32imafc-esp-espidf/release/libentropylab_dice_core.a')],text=True,stderr=subprocess.DEVNULL)
assert re.search(r'\bT '+re.escape(panic[0])+r'$',hexnm,re.M)
dis=subprocess.check_output(['riscv32-esp-elf-objdump','-d','--disassemble='+panic[0],str(elf)],text=True)
assert '<abort>' in dis,dis
assert 'malloc(size)' in (r/'app/main/main.c').read_text() or 'heap_caps_aligned_alloc' in (r/'app/main/main.c').read_text()
for symbol,callee in [('fixture_alloc','heap_caps_aligned_alloc'),('fixture_free','heap_caps_free')]:
 dis=subprocess.check_output(['riscv32-esp-elf-objdump','-d','--disassemble='+symbol,str(elf)],text=True)
 assert '<'+callee+'>' in dis,(symbol,callee)
print('PASS final ELF dice/coin/hex APIs, single dice archive runtime, allocator/free hooks, aborting panic')
