#![cfg_attr(target_os = "espidf", no_std)]
extern crate entropylab_hex_core;
/// # Safety
/// All accepted ranges must be live in one allocation each; input readable,
/// outputs exclusively writable. Integer checks cannot establish pointer validity.
/// See mnemonic_core.h. No caller bytes are written on any returned error.
#[no_mangle]
pub unsafe extern "C" fn el_mnemonic_run(p:*const u8,n:usize,e:*mut u8,ec:usize,f:*mut u8,fc:usize,a:*mut u8,ac:usize)->i32 {
 let ranges=[(p,n),(e as *const u8,ec),(f as *const u8,fc),(a as *const u8,ac)];
 for ((ptr,len),max) in ranges.iter().zip([215,65,9,43]) {
  if ptr.is_null() || *len==0 || *len>max || (*ptr as usize).checked_add(*len).is_none() {return -1}
 }
 for i in 0..4 {for j in i+1..4 {let(p,n)=ranges[i];let(q,k)=ranges[j];if (p as usize)<q as usize+k && (q as usize)<p as usize+n {return -1}}}
 let mnemonic=match validate(core::slice::from_raw_parts(p,n)) {Ok(m)=>m,Err(e)=>return e};
 let (raw,len)=mnemonic.to_entropy_array();
 let entropy=Secret(raw);
 if ec<=len*2 || fc<9 || ac<43 {return -2}
 let mut hex=Secret([0u8;65]);
 for (i,b) in entropy.0[..len].iter().enumerate() {hex.0[i*2]=b"0123456789abcdef"[(b>>4) as usize];hex.0[i*2+1]=b"0123456789abcdef"[(b&15) as usize];}
 // Only this post-validation path can derive a seed. Reuse unchanged HEX core.
 let(mut m,mut fp,mut addr)=(Secret([0u8;216]),Secret([0u8;9]),Secret([0u8;43]));
 let rc=entropylab_hex_core::el_hex_run(hex.0.as_ptr(),len*2,m.0.as_mut_ptr(),216,fp.0.as_mut_ptr(),9,addr.0.as_mut_ptr(),43);
 if rc!=0 {return -3}
 for (dest,src) in [(e,&hex.0[..len*2+1]),(f,&fp.0[..]),(a,&addr.0[..])] {core::ptr::copy_nonoverlapping(src.as_ptr(),dest,src.len());}
 0
}
struct Secret<const N:usize>([u8;N]);
impl<const N:usize> Drop for Secret<N> {fn drop(&mut self) {for b in &mut self.0 {unsafe{core::ptr::write_volatile(b,0)}}core::sync::atomic::compiler_fence(core::sync::atomic::Ordering::SeqCst);}}

pub fn validate(input: &[u8]) -> Result<bip39::Mnemonic, i32> {
 if input.len()>215 {return Err(-1)}
 if input.iter().any(|b| !b.is_ascii_lowercase() && *b!=b' ') || input.first()==Some(&b' ') || input.last()==Some(&b' ') || input.windows(2).any(|w| w==b"  ") {return Err(-4)}
 let text=core::str::from_utf8(input).map_err(|_|-4)?;
 let count=if input.is_empty(){0}else{text.split(' ').count()};
 if !matches!(count,12|15|18|21|24) {return Err(-5)}
 let words=bip39::Language::English.word_list();
 for (i,w) in text.split(' ').enumerate() {if words.binary_search(&w).is_err(){return Err(-100-i as i32)}}
 bip39::Mnemonic::parse_in_normalized(bip39::Language::English,text).map_err(|_|-6)
}
