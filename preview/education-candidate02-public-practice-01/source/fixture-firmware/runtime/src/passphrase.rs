use alloc::string::{String, ToString};
use bip39::{Language, Mnemonic};
use bitcoin::{Address, CompressedPublicKey, Network, bip32::{DerivationPath, Xpriv}};
use secp256k1::Secp256k1;
use unicode_normalization::char::{canonical_combining_class, decompose_compatible};

#[cfg(test)]
std::thread_local! { static KDF_CALLS: core::cell::Cell<usize> = const { core::cell::Cell::new(0) }; }

// Fixed canonical reorder scratch, independent of combining-run length.
struct Scalars([char; NORMALIZED_MAX]);
impl Drop for Scalars {
    fn drop(&mut self) {
        for c in &mut self.0 { unsafe { core::ptr::write_volatile(c, '\0') } }
        core::sync::atomic::compiler_fence(core::sync::atomic::Ordering::SeqCst);
    }
}

const RAW_MAX: usize = 256;
const NORMALIZED_MAX: usize = 1024;

fn wipe(bytes: &mut [u8]) {
    for byte in bytes { unsafe { core::ptr::write_volatile(byte, 0) } }
    core::sync::atomic::compiler_fence(core::sync::atomic::Ordering::SeqCst);
}

struct Secret<const N: usize>([u8; N]);
impl<const N: usize> Drop for Secret<N> { fn drop(&mut self) { wipe(&mut self.0) } }

struct Texts(String, String);
impl Drop for Texts {
    fn drop(&mut self) { unsafe { wipe(self.0.as_bytes_mut()); wipe(self.1.as_bytes_mut()) } }
}

fn validate(input: &[u8]) -> Result<Mnemonic, i32> {
    if input.iter().any(|b| !b.is_ascii_lowercase() && *b != b' ')
        || input.first() == Some(&b' ') || input.last() == Some(&b' ')
        || input.windows(2).any(|w| w == b"  ") { return Err(-4) }
    let text = core::str::from_utf8(input).map_err(|_| -4)?;
    let count = text.split(' ').count();
    if !matches!(count, 12 | 15 | 18 | 21 | 24) { return Err(-5) }
    for (index, word) in text.split(' ').enumerate() {
        if Language::English.word_list().binary_search(&word).is_err() { return Err(-100 - index as i32) }
    }
    Mnemonic::parse_in_normalized(Language::English, text).map_err(|_| -6)
}

fn normalized(raw: &[u8]) -> Result<(Secret<NORMALIZED_MAX>, usize), i32> {
    let text = core::str::from_utf8(raw).map_err(|_| -7)?;
    if text.as_bytes().contains(&0) { return Err(-7) }
    let mut output = Secret([0; NORMALIZED_MAX]);
    let mut scratch = Scalars(['\0'; NORMALIZED_MAX]);
    let (mut count, mut bytes) = (0usize, 0usize);
    let mut overflow = false;
    for scalar in text.chars() {
        // This callback API traverses static fully-decomposed tables (or emits
        // at most three Hangul scalars); it never uses the allocating iterator.
        decompose_compatible(scalar, |c| {
            if overflow { return }
            let width = c.len_utf8();
            if bytes + width > NORMALIZED_MAX { overflow = true; return }
            bytes += width; // Every char consumes >=1 byte, so count <1024.
            let class = canonical_combining_class(c);
            let mut pos = count;
            if class != 0 {
                while pos > 0 && canonical_combining_class(scratch.0[pos-1]) > class {
                    scratch.0[pos] = scratch.0[pos-1];
                    pos -= 1;
                }
            }
            scratch.0[pos] = c;
            count += 1;
        });
        if overflow { return Err(-8) }
    }
    let mut used = 0usize;
    for scalar in &scratch.0[..count] {
        let width = scalar.len_utf8();
        scalar.encode_utf8(&mut output.0[used..used + width]);
        used += width;
    }
    Ok((output, used))
}

fn derive(mnemonic: &Mnemonic, passphrase: &[u8]) -> Result<Texts, i32> {
    let normalized = core::str::from_utf8(passphrase).map_err(|_| -7)?;
    #[cfg(test)]
    KDF_CALLS.with(|n| n.set(n.get() + 1));
    let seed = Secret(mnemonic.to_seed_normalized(normalized));
    let secp = Secp256k1::new();
    let mut master = Xpriv::new_master(Network::Bitcoin, &seed.0).map_err(|_| -3)?;
    let path: DerivationPath = "m/84'/0'/0'/0/0".parse().map_err(|_| -3)?;
    let child_result = master.derive_priv(&secp, &path);
    let fingerprint = master.fingerprint(&secp).to_string();
    master.private_key.non_secure_erase();
    let mut child = child_result.map_err(|_| -3)?;
    let public = CompressedPublicKey(secp256k1::PublicKey::from_secret_key(&secp, &child.private_key));
    child.private_key.non_secure_erase();
    Ok(Texts(fingerprint, Address::p2wpkh(&public, Network::Bitcoin).to_string()))
}

fn overlaps(a: (usize, usize), b: (usize, usize)) -> bool { a.0 < b.1 && b.0 < a.1 }

/// Derive public BIP84 outputs from a canonical English BIP39 mnemonic and optional passphrase.
/// # Safety
/// Each accepted range is live for its declared length, inputs are readable, outputs are exclusively
/// writable, and no accepted nonempty ranges alias. Pointer liveness cannot be established here.
#[no_mangle]
pub unsafe extern "C" fn el_bip39_passphrase_run(
    mnemonic: *const u8, mnemonic_len: usize, passphrase: *const u8, passphrase_len: usize,
    entropy: *mut u8, entropy_cap: usize, fingerprint: *mut u8, fingerprint_cap: usize,
    address: *mut u8, address_cap: usize,
) -> i32 {
    #[cfg(test)]
    let before = KDF_CALLS.with(|n| n.get());
    let rc = run_impl(mnemonic, mnemonic_len, passphrase, passphrase_len, entropy, entropy_cap, fingerprint, fingerprint_cap, address, address_cap);
    #[cfg(test)]
    KDF_CALLS.with(|n| assert_eq!(n.get() - before, usize::from(rc == 0), "KDF count for status {rc}"));
    rc
}

unsafe fn run_impl(
    mnemonic: *const u8, mnemonic_len: usize, passphrase: *const u8, passphrase_len: usize,
    entropy: *mut u8, entropy_cap: usize, fingerprint: *mut u8, fingerprint_cap: usize,
    address: *mut u8, address_cap: usize,
) -> i32 {
    if mnemonic.is_null() || mnemonic_len == 0 || mnemonic_len > 215 || passphrase_len > RAW_MAX
        || (passphrase_len != 0 && passphrase.is_null())
        || entropy.is_null() || !(1..=65).contains(&entropy_cap)
        || fingerprint.is_null() || !(1..=9).contains(&fingerprint_cap)
        || address.is_null() || !(1..=43).contains(&address_cap) { return -1 }
    let specs = [(mnemonic as usize, mnemonic_len), (passphrase as usize, passphrase_len),
        (entropy as usize, entropy_cap), (fingerprint as usize, fingerprint_cap), (address as usize, address_cap)];
    let mut ranges = [(0usize, 0usize); 5];
    for (i, (start, len)) in specs.into_iter().enumerate() {
        if len == 0 { continue }
        let Some(end) = start.checked_add(len) else { return -1 };
        ranges[i] = (start, end);
    }
    for i in 0..5 { for j in i + 1..5 {
        if ranges[i].0 != ranges[i].1 && ranges[j].0 != ranges[j].1 && overlaps(ranges[i], ranges[j]) { return -1 }
    }}
    let mnemonic = match validate(core::slice::from_raw_parts(mnemonic, mnemonic_len)) { Ok(v) => v, Err(e) => return e };
    let (raw_entropy, entropy_len) = mnemonic.to_entropy_array();
    let entropy_secret = Secret(raw_entropy);
    if entropy_cap <= entropy_len * 2 || fingerprint_cap < 9 || address_cap < 43 { return -2 }
    let raw = if passphrase_len == 0 { &[][..] } else { core::slice::from_raw_parts(passphrase, passphrase_len) };
    let (normalized, normalized_len) = match normalized(raw) { Ok(v) => v, Err(e) => return e };
    let texts = match derive(&mnemonic, &normalized.0[..normalized_len]) { Ok(v) => v, Err(e) => return e };
    let mut encoded = Secret([0u8; 65]);
    for (i, byte) in entropy_secret.0[..entropy_len].iter().enumerate() {
        encoded.0[2*i] = b"0123456789abcdef"[(byte >> 4) as usize];
        encoded.0[2*i+1] = b"0123456789abcdef"[(byte & 15) as usize];
    }
    let entropy_size = entropy_len * 2;
    core::ptr::copy_nonoverlapping(encoded.0.as_ptr(), entropy, entropy_size); entropy.add(entropy_size).write(0);
    core::ptr::copy_nonoverlapping(texts.0.as_ptr(), fingerprint, 8); fingerprint.add(8).write(0);
    core::ptr::copy_nonoverlapping(texts.1.as_ptr(), address, 42); address.add(42).write(0);
    0
}

#[cfg(test)]
mod tests {
    use super::*;
    use unicode_normalization::UnicodeNormalization;
    #[test]
    fn bounded_normalizer_matches_pinned_reference() {
        for c in (1..=0x10ffff).filter_map(char::from_u32) {
            let mut b = [0;4];
            let s = c.encode_utf8(&mut b);
            let (got,n) = normalized(s.as_bytes()).unwrap();
            let expected: String = s.nfkd().collect();
            assert_eq!(&got.0[..n], expected.as_bytes(), "scalar {c:?}");
        }
        for s in ["\u{315}\u{300}".repeat(64), "a\u{315}\u{300}".repeat(51), "\u{fdfa}".repeat(31)] {
            let (got,n) = normalized(s.as_bytes()).unwrap();
            assert_eq!(&got.0[..n], s.nfkd().collect::<String>().as_bytes());
        }
    }
    #[test]
    fn thread_local_kdf_count_success_and_preflight() {
        let jobs: alloc::vec::Vec<_> = (0..4).map(|_| std::thread::spawn(|| {
            let m = b"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about";
            for (pass, expected, calls) in [(&b"TREZOR"[..],0,1), (&b"x\0"[..],-7,0)] {
                let (mut e,mut f,mut a)=([0u8;65],[0u8;9],[0u8;43]);
                KDF_CALLS.with(|n|n.set(0));
                let rc=unsafe { el_bip39_passphrase_run(m.as_ptr(),m.len(),pass.as_ptr(),pass.len(),e.as_mut_ptr(),65,f.as_mut_ptr(),9,a.as_mut_ptr(),43) };
                assert_eq!(rc,expected);
                KDF_CALLS.with(|n|assert_eq!(n.get(),calls));
            }
        })).collect();
        for job in jobs {job.join().unwrap();}
    }
}

#[cfg(test)]
mod acceptance_tests {
    use super::*;
    #[test]
    fn published_japanese_seed_primitive_not_product_scope() {
        let text = "あいこくしん　あいこくしん　あいこくしん　あいこくしん　あいこくしん　あいこくしん　あいこくしん　あいこくしん　あいこくしん　あいこくしん　あいこくしん　あおぞら";
        let pass = "㍍ガバヴァぱばぐゞちぢ十人十色";
        let (m, mn) = normalized(text.as_bytes()).unwrap();
        let mnemonic = Mnemonic::parse_in_normalized(Language::Japanese, core::str::from_utf8(&m.0[..mn]).unwrap()).unwrap();
        let (p, pn) = normalized(pass.as_bytes()).unwrap();
        let seed = mnemonic.to_seed_normalized(core::str::from_utf8(&p.0[..pn]).unwrap());
        let expected: [u8;64] = [162, 98, 214, 251, 97, 34, 236, 244, 91, 224, 156, 80, 73, 43, 49, 249, 46, 155, 235, 125, 154, 132, 89, 135, 160, 44, 239, 218, 87, 161, 95, 156, 70, 122, 23, 135, 32, 41, 169, 233, 34, 153, 181, 203, 223, 48, 110, 58, 14, 230, 32, 36, 92, 189, 80, 137, 89, 182, 203, 124, 166, 55, 189, 85];
        assert_eq!(seed, expected);
        assert_ne!(mnemonic.to_seed_normalized(pass), expected);
        assert!(matches!(validate(text.as_bytes()), Err(-4)));
    }
}

#[cfg(test)]
mod instrumented_abi_matrix {
// Public fixtures only.
use super::el_bip39_passphrase_run;

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
                let rc=unsafe {crate::el_mnemonic_run(m.as_ptr(),m.len(),old_e.as_mut_ptr(),65,old_f.as_mut_ptr(),9,old_a.as_mut_ptr(),43)};
                assert_eq!(rc,0);assert_eq!((e,f,a),(old_e,old_f,old_a));
            }
        }
    }
}

}
