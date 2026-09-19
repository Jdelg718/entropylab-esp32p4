#![cfg_attr(target_os = "espidf", no_std)]
extern crate alloc;
use alloc::string::{String,ToString};
use bitcoin::{bip32::{Xpriv,DerivationPath},Address,Network,CompressedPublicKey};
use secp256k1::Secp256k1;
#[cfg(target_os = "espidf")]
mod runtime {
 use core::alloc::{GlobalAlloc,Layout};
 extern "C" {fn fixture_alloc(size:usize,align:usize)->*mut u8;fn fixture_free(p:*mut u8);fn abort()->!;}
 struct Heap;
 unsafe impl GlobalAlloc for Heap {unsafe fn alloc(&self,l:Layout)->*mut u8{fixture_alloc(l.size(),l.align())} unsafe fn dealloc(&self,p:*mut u8,_:Layout){fixture_free(p)}}
 #[global_allocator] static HEAP:Heap=Heap;
 #[panic_handler] fn panic(_: &core::panic::PanicInfo<'_>)->!{unsafe{abort()}}
}
fn wipe(b:&mut[u8]){for x in b{unsafe{core::ptr::write_volatile(x,0)}}core::sync::atomic::compiler_fence(core::sync::atomic::Ordering::SeqCst);}
struct Secret<const N:usize>([u8;N]);
impl<const N:usize> Drop for Secret<N>{fn drop(&mut self){wipe(&mut self.0)}}
struct ResultText(String,String,String);
impl Drop for ResultText{fn drop(&mut self){unsafe{wipe(self.0.as_bytes_mut());wipe(self.1.as_bytes_mut());wipe(self.2.as_bytes_mut());}}}
fn nibble(b:u8)->Result<u8,i32>{match b{b'0'..=b'9'=>Ok(b-b'0'),b'a'..=b'f'=>Ok(b-b'a'+10),b'A'..=b'F'=>Ok(b-b'A'+10),_=>Err(-4)}}
fn compute(h:&[u8])->Result<ResultText,i32>{
 let mut entropy=Secret([0;32]);
 for (i,p) in h.chunks_exact(2).enumerate(){entropy.0[i]=(nibble(p[0])?<<4)|nibble(p[1])?;}
 let mnemonic=bip39::Mnemonic::from_entropy(&entropy.0[..h.len()/2]).map_err(|_|-3)?;
 let seed=Secret(mnemonic.to_seed_normalized(""));
 let secp=Secp256k1::new();
 let mut master=Xpriv::new_master(Network::Bitcoin,&seed.0).map_err(|_|-3)?;
 let path:DerivationPath="m/84'/0'/0'/0/0".parse().map_err(|_|-3)?;
 let child_result=master.derive_priv(&secp,&path);
 let fingerprint=master.fingerprint(&secp).to_string();
 master.private_key.non_secure_erase();
 let mut child=child_result.map_err(|_|-3)?;
 let public=CompressedPublicKey(secp256k1::PublicKey::from_secret_key(&secp,&child.private_key));
 child.private_key.non_secure_erase();
 Ok(ResultText(mnemonic.to_string(),fingerprint,Address::p2wpkh(&public,Network::Bitcoin).to_string()))
}
/// See hex_core.h for pointer validity, bounds, ownership and error contract.
/// # Safety
/// All accepted ranges must be live, with readable input and exclusively writable outputs.
#[no_mangle]
pub unsafe extern "C" fn el_hex_run(h:*const u8,hn:usize,m:*mut u8,mc:usize,f:*mut u8,fc:usize,a:*mut u8,ac:usize)->i32 {
 if !matches!(hn,32|40|48|56|64){return -1;}
 let ranges=[(h,hn),(m as *const u8,mc),(f as *const u8,fc),(a as *const u8,ac)];
 for ((p,n),max) in ranges.iter().zip([64,216,9,43]){if p.is_null()||*n==0||*n>max||(*p as usize).checked_add(*n).is_none(){return -1;}}
 for i in 0..4{for j in i+1..4{let(p,n)=ranges[i];let(q,k)=ranges[j];if (p as usize)<q as usize+k&&(q as usize)<p as usize+n{return -1;}}}
 let result=match compute(core::slice::from_raw_parts(h,hn)){Ok(v)=>v,Err(e)=>return e};
 if mc<=result.0.len()||fc<=result.1.len()||ac<=result.2.len(){return -2;}
 for(p,s)in[(m,result.0.as_bytes()),(f,result.1.as_bytes()),(a,result.2.as_bytes())]{core::ptr::copy_nonoverlapping(s.as_ptr(),p,s.len());p.add(s.len()).write(0);}
 0
}
