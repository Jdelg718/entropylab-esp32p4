//! Tests existing upstream routines, not a new public seed/path interface.
use super::*;
use bitcoin::bip32::Xpub;
use std::str::FromStr;
#[test]
fn published_bip32_vectors_1_through_4_all_17_nodes() {
    for line in include_str!("../../vectors/bip32.tsv").lines() {
        let v:Vec<_>=line.split('\t').collect();
        let seed:Vec<u8>=v[0].as_bytes().chunks_exact(2).map(|p|u8::from_str_radix(std::str::from_utf8(p).unwrap(),16).unwrap()).collect();
        let mut node=[0u8;78];
        assert_eq!(unsafe { el_hd_master(seed.as_ptr(),seed.len(),node.as_mut_ptr()) },78);
        for step in v[1].split('/').skip(1) {
            let (n,h)=match step.strip_suffix('\'') {Some(n)=>(n,0x80000000),None=>(step,0)};
            let mut next=[0u8;78];
            assert_eq!(unsafe {el_hd_ckd_priv(node.as_ptr(),n.parse::<u32>().unwrap()|h,next.as_mut_ptr())},78);
            node=next;
        }
        let private=Xpriv::decode(&node).unwrap();
        assert_eq!(private.to_string(),v[3],"{}",v[1]);
        assert_eq!(Xpub::from_priv(ctx(),&private).to_string(),v[2],"{}",v[1]);
        // Serialized child metadata includes independently published parent fingerprint.
        let published=Xpub::from_str(v[2]).unwrap();
        assert_eq!(private.parent_fingerprint,published.parent_fingerprint);
    }
}
#[test]
fn bip32_master_fingerprint_matches_published_child_metadata() {
    let rows:Vec<_>=include_str!("../../vectors/bip32.tsv").lines().collect();
    let first:Vec<_>=rows[0].split('\t').collect();
    let next:Vec<_>=rows[1].split('\t').collect();
    let master=Xpriv::from_str(first[3]).unwrap();
    assert_eq!(master.fingerprint(ctx()),Xpub::from_str(next[2]).unwrap().parent_fingerprint);
    assert_eq!(master.fingerprint(ctx()).to_string(),"3442193e");
}
#[test]
fn upstream_capacity_errors_leave_canaries_unchanged() {
    let mut out=[0xa5;216];
    assert_eq!(unsafe {el_bip39_entropy_to_mnemonic([0u8;16].as_ptr(),16,out.as_mut_ptr(),1)},-1);
    assert_eq!(out,[0xa5;216]);
    assert_eq!(unsafe {el_bip39_entropy_to_mnemonic([0u8;15].as_ptr(),15,out.as_mut_ptr(),215)},-1);
    assert_eq!(out,[0xa5;216]);
    assert_eq!(unsafe {el_hd_ckd_priv([0u8;78].as_ptr(),0,out.as_mut_ptr())},-1);
    assert_eq!(out,[0xa5;216]);
}
#[test]
fn pipeline_fingerprint_matches_published_bip39_root() {
    for line in include_str!("../../vectors/bip39-english.tsv").lines() {
        let v:Vec<_>=line.split('\t').collect();
        assert_eq!(derive(v[0],"TREZOR",0,0).unwrap().fingerprint,Xpriv::from_str(v[3]).unwrap().fingerprint(ctx()).to_string());
    }
}
#[test]
fn supported_ascii_spaces_are_preserved() {
    let a=derive(&"00".repeat(16)," TREZOR ",0,0).unwrap();
    let b=derive(&"00".repeat(16),"TREZOR",0,0).unwrap();
    assert_ne!(a.seed,b.seed);
    let phrase=bip39::Mnemonic::parse_in_normalized(bip39::Language::English,&a.mnemonic).unwrap();
    assert_eq!(a.seed,phrase.to_seed_normalized(" TREZOR "));
    assert!(derive(&"ff".repeat(32),&"a".repeat(128),1,1000).is_ok());
    assert_eq!(derive(&"FF".repeat(16),"",0,0).unwrap().seed,derive(&"ff".repeat(16),"",0,0).unwrap().seed);
}
