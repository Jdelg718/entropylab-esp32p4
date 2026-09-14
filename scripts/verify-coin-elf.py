#!/usr/bin/env python3
from pathlib import Path
import subprocess,re
r=Path(__file__).resolve().parents[1]/'fixture-firmware'
elf=r/'build/entropylab_fixture.elf'
s=subprocess.check_output(['riscv32-esp-elf-nm','--defined-only',str(elf)],text=True)
for name in ['el_coin_to_hex','el_hex_run','fixture_alloc','fixture_free','abort']:
 assert len(re.findall(r'\bT '+name+r'$',s,re.M))==1,name
panic=[x.split()[-1] for x in s.splitlines() if ' T ' in x and 'rust_begin_unwind' in x];assert len(panic)==1
m=(r/'build/entropylab_fixture.map').read_text()
assert 'libentropylab_hex_core.a' in m and 'libentropylab_coin_core_linkable.a' in m
# The sole global panic is defined by hex (adapter is localized before linking).
hexnm=subprocess.check_output(['riscv32-esp-elf-nm','--defined-only',str(r/'rust/target/riscv32imafc-esp-espidf/release/libentropylab_hex_core.a')],text=True,stderr=subprocess.DEVNULL)
assert re.search(r'\bT '+re.escape(panic[0])+r'$',hexnm,re.M)
dis=subprocess.check_output(['riscv32-esp-elf-objdump','-d','--disassemble='+panic[0],str(elf)],text=True)
assert '<abort>' in dis,dis
assert 'malloc(size)' in (r/'app/main/main.c').read_text() or 'heap_caps_aligned_alloc' in (r/'app/main/main.c').read_text()
print('PASS final ELF coin/hex/allocator/abort symbols and hex-owned aborting panic')
