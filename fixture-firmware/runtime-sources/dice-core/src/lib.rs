#![cfg_attr(target_os = "espidf", no_std)]
// Same dependency supplies the sole target allocator/panic runtime and hex ABI.
pub use entropylab_hex_core::el_hex_run;
use bitcoin_hashes::{sha256, Hash, HashEngine};
pub const RAW_DIGITS_COLDCARD_STYLE:u32=1;
pub const COLEMAN_6_TO_0_BEFORE_SHA256:u32=2;
pub const MAX_ROLLS:usize=1024;
/// Strict nominal fair-independent-die count; NOT a source-quality certificate.
#[no_mangle]
pub extern "C" fn el_dice_required_rolls(words:u32)->i32 {
 match words {12=>50,15=>62,18=>75,21=>87,24=>100,_=>-1}
}
/// SHA256 complete keypad transcript, then first selected bytes as lowercase hex.
/// Success 1 is explicitly WEAK_INPUT_LAB_ONLY; success 0 is count-only, not proof.
/// # Safety
/// Caller supplies live readable input and exclusive writable output, stable for
/// the entire call. Ranges must be disjoint. Non-null liveness cannot be checked.
#[no_mangle]
pub unsafe extern "C" fn el_dice_to_hex(p:*const u8,n:usize,mode:u32,words:u32,out:*mut u8,cap:usize)->i32 {
 let required=el_dice_required_rolls(words);
 if required<0 || !matches!(mode,RAW_DIGITS_COLDCARD_STYLE|COLEMAN_6_TO_0_BEFORE_SHA256) || n==0 || n>MAX_ROLLS || cap==0 || cap>65 || p.is_null() || out.is_null(){return -1;}
 let start=p as usize;let dest=out as usize;
 let end=match start.checked_add(n){Some(v)=>v,None=>return -1};
 let dest_end=match dest.checked_add(cap){Some(v)=>v,None=>return -1};
 if start<dest_end && dest<end{return -1;}
 let bytes=words as usize/3*4;
 if cap<bytes*2+1{return -2;}
 let input=core::slice::from_raw_parts(p,n);
 if input.iter().any(|b|!matches!(b,b'1'..=b'6')){return -4;}
 let mut engine=sha256::Hash::engine();
 for &b in input {engine.input(&[if mode==COLEMAN_6_TO_0_BEFORE_SHA256 && b==b'6'{b'0'}else{b}]);}
 let hash=sha256::Hash::from_engine(engine).to_byte_array();
 for (i,b) in hash[..bytes].iter().enumerate(){out.add(i*2).write(b"0123456789abcdef"[(b>>4) as usize]);out.add(i*2+1).write(b"0123456789abcdef"[(b&15) as usize]);}
 out.add(bytes*2).write(0);
 if n<(required as usize){1}else{0}
}
