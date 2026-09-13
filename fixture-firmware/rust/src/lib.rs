#![cfg_attr(target_os = "espidf", no_std)]
extern crate alloc;
use alloc::string::ToString;
use bitcoin::{bip32::{Xpriv, DerivationPath}, Network, Address, CompressedPublicKey};
use secp256k1::Secp256k1;

#[cfg(target_os = "espidf")]
mod runtime {
    use core::alloc::{GlobalAlloc, Layout};
    extern "C" { fn fixture_alloc(size:usize,align:usize)->*mut u8; fn fixture_free(p:*mut u8); fn abort()->!; }
    struct Heap;
    unsafe impl GlobalAlloc for Heap {
        unsafe fn alloc(&self,l:Layout)->*mut u8 { fixture_alloc(l.size(),l.align()) }
        unsafe fn dealloc(&self,p:*mut u8,_:Layout) { fixture_free(p) }
    }
    #[global_allocator] static HEAP:Heap=Heap;
    #[panic_handler] fn panic(_: &core::panic::PanicInfo<'_>)->! { unsafe { abort() } }
}

fn compute()->Result<(alloc::string::String,alloc::string::String,alloc::string::String),()> {
    // Only compiled public zero entropy, empty passphrase, and fixed receive path.
    let mnemonic=bip39::Mnemonic::from_entropy(&[0u8;16]).map_err(|_|())?;
    let mut seed=mnemonic.to_seed_normalized("");
    let secp=Secp256k1::new();
    let master=Xpriv::new_master(Network::Bitcoin,&seed).map_err(|_|())?;
    for b in &mut seed { unsafe { core::ptr::write_volatile(b,0) }; }
    let fingerprint=master.fingerprint(&secp).to_string();
    let path:DerivationPath="m/84'/0'/0'/0/0".parse().map_err(|_|())?;
    let child=master.derive_priv(&secp,&path).map_err(|_|())?;
    let public=CompressedPublicKey(secp256k1::PublicKey::from_secret_key(&secp,&child.private_key));
    let address=Address::p2wpkh(&public,Network::Bitcoin).to_string();
    Ok((mnemonic.to_string(),fingerprint,address))
}

/// Caller supplies three live writable NONOVERLAPPING buffers, owned exclusively
/// for the duration of this call. Capacities are bounded 1..4096. On ANY error no
/// buffer is modified. Success includes NUL terminators. No Rust objects escape.
/// -1 invalid arguments, -2 insufficient capacity, -3 calculation error.
/// OOM/panic abort the process/device: never unwind across C. No arbitrary input.
#[no_mangle]
pub unsafe extern "C" fn fixture_run(m:*mut u8,mc:usize,f:*mut u8,fc:usize,a:*mut u8,ac:usize)->i32 {
    let ranges=[(m,mc),(f,fc),(a,ac)];
    for &(p,n) in &ranges {
        if p.is_null() || n==0 || n>4096 || (p as usize).checked_add(n).is_none() { return -1; }
    }
    for i in 0..3 { for j in i+1..3 {
        let (p,n)=ranges[i];let (q,k)=ranges[j];
        if (p as usize)<q as usize+k && (q as usize)<p as usize+n {return -1;}
    }}
    let (ms,fs,ads)=match compute(){Ok(x)=>x,Err(_)=>return -3};
    if mc<=ms.len() || fc<=fs.len() || ac<=ads.len() {return -2;}
    for (p,s) in [(m,ms.as_bytes()),(f,fs.as_bytes()),(a,ads.as_bytes())] {
        core::ptr::copy_nonoverlapping(s.as_ptr(),p,s.len());p.add(s.len()).write(0);
    }
    0
}
#[cfg(test)] mod tests {
 use super::*;
 #[test] fn kat(){let (m,f,a)=compute().unwrap();assert_eq!(m,"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about");assert_eq!(f,"73c5da0a");assert_eq!(a,"bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu");}
 #[test] fn bounds(){unsafe {
 let mut m=[0xa5;128];let mut f=[0xa5;9];let mut a=[0xa5;64];
 for cap in 1..94 {assert_eq!(fixture_run(m.as_mut_ptr(),cap,f.as_mut_ptr(),9,a.as_mut_ptr(),64),-2);assert_eq!(m,[0xa5;128]);assert_eq!(f,[0xa5;9]);assert_eq!(a,[0xa5;64]);}
 for cap in [0,4097,usize::MAX] {assert_eq!(fixture_run(m.as_mut_ptr(),cap,f.as_mut_ptr(),9,a.as_mut_ptr(),64),-1);}
 assert_eq!(fixture_run(core::ptr::null_mut(),128,f.as_mut_ptr(),9,a.as_mut_ptr(),64),-1);
 assert_eq!(fixture_run(m.as_mut_ptr(),128,m.as_mut_ptr(),9,a.as_mut_ptr(),64),-1);
 assert_eq!(fixture_run(m.as_mut_ptr(),128,f.as_mut_ptr(),8,a.as_mut_ptr(),64),-2);
 assert_eq!(fixture_run(m.as_mut_ptr(),128,f.as_mut_ptr(),9,a.as_mut_ptr(),42),-2);
 assert_eq!(fixture_run(m.as_mut_ptr(),128,f.as_mut_ptr(),9,a.as_mut_ptr(),64),0);
 }}
}
