use entropylab_mnemonic_core::el_mnemonic_run;
const GOOD:&[u8]=b"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about";
#[test]
fn published_bip84_abi() {
 let(mut e,mut f,mut a)=([0xa5;65],[0xa5;9],[0xa5;43]);
 let rc=unsafe{el_mnemonic_run(GOOD.as_ptr(),GOOD.len(),e.as_mut_ptr(),65,f.as_mut_ptr(),9,a.as_mut_ptr(),43)};
 assert_eq!(rc,0);
 assert_eq!(&e[..33],b"00000000000000000000000000000000\0");
 assert_eq!(&a,b"bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu\0");
 assert_eq!(f[8],0);
 assert!(e[33..].iter().all(|b|*b==0xa5));
}
