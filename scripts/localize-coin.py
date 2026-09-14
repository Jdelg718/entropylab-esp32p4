#!/usr/bin/env python3
"""Localize exactly one adapter panic symbol; assert loadable payload identity."""
from pathlib import Path
import subprocess, tempfile
import struct, sys

def main():
 r=Path(__file__).resolve().parents[1]/'fixture-firmware'
 src=r/'coin/target/riscv32imafc-esp-espidf/release/libentropylab_coin_core.a'
 out=r/'coin/target/libentropylab_coin_core_linkable.a'
 hx=r/'rust/target/riscv32imafc-esp-espidf/release/libentropylab_hex_core.a'
 def run(*args): return subprocess.check_output(args,text=True,stderr=subprocess.DEVNULL)
 def symbols(p):
  return {tuple(x.split()[-2:]) for x in run('riscv32-esp-elf-nm','--defined-only',str(p)).splitlines() if len(x.split())==3 and x.split()[-2].isupper()}
 a=symbols(src);panic={s for k,s in a if k=='T' and 'rust_begin_unwind' in s};assert len(panic)==1
 symbol=panic.pop()
 subprocess.run(['riscv32-esp-elf-objcopy','--localize-symbol='+symbol,str(src),str(out)],check=True)
 b=symbols(out);assert a-b=={('T',symbol)} and not b-a
 assert ('T',symbol) in symbols(hx)
 assert ['t',symbol] in [x.split()[-2:] for x in run('riscv32-esp-elf-nm','--defined-only',str(out)).splitlines()]
 # Parse members in order, including duplicates; no filesystem extraction.
 import struct
 
 def allocated(archive):
  data=archive.read_bytes();assert data[:8]==b'!<arch>\n';pos=8;members=[]
  while pos<len(data):
   header=data[pos:pos+60];assert len(header)==60 and header[58:]==b'`\n'
   size=int(header[48:58]);body=data[pos+60:pos+60+size];assert len(body)==size
   pos+=60+size+(size%2)
   if not body.startswith(b'\x7fELF'):continue
   assert body[4:6]==b'\x01\x01'
   off=struct.unpack_from('<I',body,32)[0];ent,num,strings=struct.unpack_from('<HHH',body,46)
   sections=[struct.unpack_from('<10I',body,off+i*ent) for i in range(num)]
   sh=sections[strings];names=body[sh[4]:sh[4]+sh[5]];payload=[]
   for sh in sections:
    name,kind,flags,addr,offset,length,*_=sh
    if flags&2:
     label=names[name:].split(b'\0',1)[0]
     chunk=b'' if kind==8 else body[offset:offset+length]
     assert kind==8 or len(chunk)==length
     payload.append((label,kind,flags,addr,length,chunk))
   members.append((header[:16],payload))
  return members
 assert allocated(src)==allocated(out), 'allocated member payload changed'
 subprocess.run(['riscv32-esp-elf-ld','-r','-u','el_coin_to_hex','-u','el_hex_run',str(out),str(hx),'-o',str(r/'coin/target/composed.o')],check=True)
 print('PASS single adapter panic localization, allocated payload identity, unchanged hex panic owner, strict composed link')
 

if __name__ == "__main__":
 try:
  if not __debug__:
   raise ValueError("checker requires Python assertions enabled")
  main()
 except (AssertionError, OSError, ValueError, IndexError, struct.error, subprocess.CalledProcessError) as exc:
  print("FAIL coin archive verification: " + (str(exc) or "archive invariant violated"), file=sys.stderr)
  sys.exit(1)
