#![cfg_attr(target_os = "espidf", no_std)]
/// Pure direct-key transcript conversion; see coin_core.h.
/// # Safety
/// Accepted ranges must be live, disjoint and stable, input readable and output
/// exclusively writable. Non-null pointer liveness is the caller's obligation.
#[no_mangle]
pub unsafe extern "C" fn el_coin_to_hex(bits:*const u8,len:usize,words:u32,out:*mut u8,cap:usize)->i32 {
 let required=match words {12=>128,15=>160,18=>192,21=>224,24=>256,_=>return -1};
 if len!=required || cap==0 || cap>65 || bits.is_null() || out.is_null() { return -1; }
 let start=bits as usize; let dest=out as usize;
 let end=match start.checked_add(len) {Some(v)=>v,None=>return -1};
 let dest_end=match dest.checked_add(cap) {Some(v)=>v,None=>return -1};
 if start<dest_end && dest<end {return -1;}
 if cap<len/4+1 {return -2;}
 // Validate the ENTIRE transcript before the first output write.
 for i in 0..len {if !matches!(*bits.add(i),b'0'|b'1') {return -4;}}
 for i in 0..len/4 {
  let mut nibble=0;
  for j in 0..4 { nibble=(nibble<<1)|(*bits.add(i*4+j)-b'0'); }
  out.add(i).write(b"0123456789abcdef"[nibble as usize]);
 }
 out.add(len/4).write(0);
 0
}
