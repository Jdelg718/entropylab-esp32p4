//! Public never-fund fixtures only. Assertion messages contain labels, not secrets.
use entropylab_runtime::el_bip39_passphrase_run;

const GOOD: &[u8] = b"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about";

fn run(pass: Option<&[u8]>, entropy: &mut [u8], fingerprint: &mut [u8], address: &mut [u8]) -> i32 {
    let (p, n) = pass.map_or((core::ptr::null(), 0), |v| (v.as_ptr(), v.len()));
    unsafe {
        el_bip39_passphrase_run(
            GOOD.as_ptr(), GOOD.len(), p, n,
            entropy.as_mut_ptr(), entropy.len(),
            fingerprint.as_mut_ptr(), fingerprint.len(),
            address.as_mut_ptr(), address.len(),
        )
    }
}

#[test]
fn public_empty_bip84_and_null_zero() {
    let (mut e, mut f, mut a) = ([0xa5; 65], [0xa5; 9], [0xa5; 43]);
    assert!(run(None, &mut e, &mut f, &mut a) == 0, "empty/null fixture failed");
    assert!(e.starts_with(b"00000000000000000000000000000000\0"), "entropy fixture mismatch");
    assert_eq!(&f, b"73c5da0a\0");
    assert!(a == *b"bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu\0", "BIP84 fixture mismatch");
}

#[test]
fn trezor_passphrase_changes_public_derivation_once() {
    let (mut e, mut f, mut a) = ([0xa5; 65], [0xa5; 9], [0xa5; 43]);
    assert!(run(Some(b"TREZOR"), &mut e, &mut f, &mut a) == 0, "TREZOR public fixture failed");
    assert!(e.starts_with(b"00000000000000000000000000000000\0"), "TREZOR entropy mismatch");
    // Frozen by public_reference.py: Python hashlib/HMAC + OpenSSL EC, not this Rust backend.
    assert_eq!(&f, b"b4e3f5ed\0");
    assert_eq!(&a, b"bc1qv5rmq0kt9yz3pm36wvzct7p3x6mtgehjul0feu\0");
}

#[test]
fn preflight_failures_preserve_entire_outputs() {
    for (label, pass, expected) in [
        ("raw257", vec![b'x'; 257], -1),
        ("nul", b"x\0y".to_vec(), -7),
        ("invalid-utf8", vec![0x80], -7),
    ] {
        let (mut e, mut f, mut a) = ([0xa5; 65], [0x5a; 9], [0x3c; 43]);
        let before = (e, f, a);
        let rc = run(Some(&pass), &mut e, &mut f, &mut a);
        assert!(rc == expected, "{label}: wrong status");
        assert!(e == before.0 && f == before.1 && a == before.2, "{label}: output changed");
    }
}

#[test]
fn capacities_and_success_tails() {
    let (mut e, mut f, mut a) = ([0xa5; 65], [0xa5; 9], [0xa5; 43]);
    let rc = unsafe {
        el_bip39_passphrase_run(GOOD.as_ptr(), GOOD.len(), core::ptr::null(), 0,
            e.as_mut_ptr(), 32, f.as_mut_ptr(), 9, a.as_mut_ptr(), 43)
    };
    assert!(rc == -2, "short capacity precedence failed");
    assert!(e == [0xa5; 65] && f == [0xa5; 9] && a == [0xa5; 43], "short capacity changed output");
    assert!(run(None, &mut e, &mut f, &mut a) == 0, "tail fixture failed");
    assert!(e[33..] == [0xa5; 32], "success entropy tail changed");
}

#[test]
fn full_declared_overlap_is_rejected_before_dereference() {
    let mut storage = [0xa5u8; 128];
    let mut f = [0x5a; 9];
    let mut a = [0x3c; 43];
    let rc = unsafe {
        el_bip39_passphrase_run(GOOD.as_ptr(), GOOD.len(), core::ptr::null(), 0,
            storage.as_mut_ptr(), 65, f.as_mut_ptr(), 9, storage.as_mut_ptr().add(64), 43)
    };
    assert!(rc == -1, "full-range overlap accepted");
    assert!(storage == [0xa5; 128] && f == [0x5a; 9] && a == [0x3c; 43], "overlap changed output");
}

#[test]
fn byte_and_encoding_matrix() {
    let mut cases: Vec<(Vec<u8>,i32)> = [0,1,127,128,129,255,256,257].into_iter().map(|n|(vec![b'x';n],if n>256 {-1}else{0})).collect();
    for n in [0,1,2] { cases.push((("\u{fdfa}".repeat(31)+&"x".repeat(n)).into_bytes(),if n==2 {-8}else{0})); }
    for s in ["é".repeat(128),"😀".repeat(64),"\u{315}\u{300}".repeat(64),"\t\n\r\u{1}".into()] {cases.push((s.into_bytes(),0));}
    for b in [vec![0xc0,0xaf],vec![0xed,0xa0,0x80],vec![0xf0,0x9f],vec![0xff],vec![0]] {cases.push((b,-7));}
    for (pass,rc) in cases {
        let (mut e,mut f,mut a)=([0xa5;65],[0xa5;9],[0xa5;43]);
        assert_eq!(run(Some(&pass),&mut e,&mut f,&mut a),rc);
        if rc!=0 {assert_eq!((e,f,a),([0xa5;65],[0xa5;9],[0xa5;43]));}
    }
}
#[test]
fn equivalent_unicode_and_significant_spaces_case() {
    fn public(p:&str)->([u8;9],[u8;43]) {let(mut e,mut f,mut a)=([0;65],[0;9],[0;43]);assert_eq!(run(Some(p.as_bytes()),&mut e,&mut f,&mut a),0);(f,a)}
    for (x,y) in [("é","e\u{301}"),("ＴＲＥＺＯＲ","TREZOR"),("\u{a0}"," ")] {assert_eq!(public(x),public(y));}
    for (x,y) in [(""," "),("TREZOR","trezor"),("TREZOR"," TREZOR"),("TREZOR","TREZOR "),("A B","A  B")] {assert_ne!(public(x),public(y));}
}
#[test]
fn every_capacity_and_combined_precedence() {
    for which in 0..3 {for cap in 0..=[66,10,44][which] {
        let (mut e,mut f,mut a)=([0xa5;67],[0xa5;11],[0xa5;45]);
        let mut caps=[65,9,43];caps[which]=cap;
        let expected=if cap==0||cap>[65,9,43][which] {-1}else if cap<[33,9,43][which] {-2}else{-7};
        let rc=unsafe {el_bip39_passphrase_run(GOOD.as_ptr(),GOOD.len(),b"\0".as_ptr(),1,e.as_mut_ptr().add(1),caps[0],f.as_mut_ptr().add(1),caps[1],a.as_mut_ptr().add(1),caps[2])};
        assert_eq!(rc,expected);assert_eq!((e,f,a),([0xa5;67],[0xa5;11],[0xa5;45]));
    }}
    for (m,expected) in [(b"Abandon".to_vec(),-4),(b"abandon".to_vec(),-5),(GOOD.replace_last_word("zzzz"),-111),(GOOD.replace_last_word("abandon"),-6)] {
        let(mut e,mut f,mut a)=([0xa5;65],[0xa5;9],[0xa5;43]);
        let rc=unsafe {el_bip39_passphrase_run(m.as_ptr(),m.len(),b"\0".as_ptr(),1,e.as_mut_ptr(),1,f.as_mut_ptr(),1,a.as_mut_ptr(),1)};
        assert_eq!(rc,expected);assert_eq!((e,f,a),([0xa5;65],[0xa5;9],[0xa5;43]));
    }
}
trait LastWord {fn replace_last_word(&self,w:&str)->Vec<u8>;}
impl LastWord for [u8] {fn replace_last_word(&self,w:&str)->Vec<u8>{let s=std::str::from_utf8(self).unwrap();format!("{} {}",s.rsplit_once(' ').unwrap().0,w).into_bytes()}}
#[test]
fn all_ten_range_pairs_nulls_overflow_and_canaries() {
    for i in 0..5 {for j in i+1..5 {
        let mut arena=[0xa5u8;512];let before=arena;
        let mut ptrs=[0usize,128,160,240,280].map(|n|unsafe{arena.as_mut_ptr().add(n)});
        let lens=[GOOD.len(),8,65,9,43];
        ptrs[j]=unsafe{ptrs[i].add(lens[i]-1)};
        let rc=unsafe{el_bip39_passphrase_run(ptrs[0],lens[0],ptrs[1],lens[1],ptrs[2],lens[2],ptrs[3],lens[3],ptrs[4],lens[4])};
        assert_eq!(rc,-1);assert_eq!(arena,before);
    }}
    for which in 0..5 {for overflow in [false,true] {
        let(mut e,mut f,mut a)=([0xa5;65],[0xa5;9],[0xa5;43]);
        let mut ptrs=[GOOD.as_ptr() as *mut u8,b"hello".as_ptr() as *mut u8,e.as_mut_ptr(),f.as_mut_ptr(),a.as_mut_ptr()];
        ptrs[which]=if overflow {(usize::MAX-1) as *mut u8}else{core::ptr::null_mut()};
        let rc=unsafe{el_bip39_passphrase_run(ptrs[0],GOOD.len(),ptrs[1],5,ptrs[2],65,ptrs[3],9,ptrs[4],43)};
        assert_eq!(rc,-1);assert_eq!((e,f,a),([0xa5;65],[0xa5;9],[0xa5;43]));
    }}
    let(mut e,mut f,mut a)=([0xa5;67],[0xa5;11],[0xa5;45]);
    let rc=unsafe{el_bip39_passphrase_run(GOOD.as_ptr(),GOOD.len(),e.as_ptr(),0,e.as_mut_ptr().add(1),65,f.as_mut_ptr().add(1),9,a.as_mut_ptr().add(1),43)};
    assert_eq!(rc,0);assert_eq!((e[0],e[66],f[0],f[10],a[0],a[44]),(0xa5,0xa5,0xa5,0xa5,0xa5,0xa5));assert_eq!(&e[34..66],&[0xa5;32]);
}

#[test]
fn all_five_word_counts_empty_nonempty_unicode_and_legacy() {
    use bip39::Mnemonic;
    for n in [16usize,20,24,28,32] {
        let bytes=vec![0u8;n];
        let m=Mnemonic::from_entropy(&bytes).unwrap().to_string();
        for pass in ["", "TREZOR", "é", "e\u{301}"] {
            let (mut e,mut f,mut a)=([0xa5;65],[0xa5;9],[0xa5;43]);
            let rc=unsafe {el_bip39_passphrase_run(m.as_ptr(),m.len(),pass.as_ptr(),pass.len(),e.as_mut_ptr(),65,f.as_mut_ptr(),9,a.as_mut_ptr(),43)};
            assert_eq!(rc,0);assert_eq!(&e[..n*2],vec![b'0';n*2]);assert_eq!(e[n*2],0);assert!(e[n*2+1..].iter().all(|&b|b==0xa5));
            if pass.is_empty() {
                let(mut old_e,mut old_f,mut old_a)=([0xa5;65],[0xa5;9],[0xa5;43]);
                let rc=unsafe {entropylab_runtime::el_mnemonic_run(m.as_ptr(),m.len(),old_e.as_mut_ptr(),65,old_f.as_mut_ptr(),9,old_a.as_mut_ptr(),43)};
                assert_eq!(rc,0);assert_eq!((e,f,a),(old_e,old_f,old_a));
            }
        }
    }
}
