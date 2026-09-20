// Extracted from pinned EntropyLab; run ../extract.py to reproduce.
use bitcoin_hashes::{sha512, Hash, HashEngine, Hmac, HmacEngine};
use secp256k1::{PublicKey, Scalar, Secp256k1, SecretKey};
use std::sync::OnceLock;
use bitcoin::bip32::{ChainCode, ChildNumber, Xpriv};
use bip39::{Language, Mnemonic};
use bitcoin::{Network, ScriptBuf};
use bitcoin::address::Address;
static CONTEXT: OnceLock<Secp256k1<secp256k1::All>> = OnceLock::new();
pub(super) fn ctx() -> &'static Secp256k1<secp256k1::All> {
    CONTEXT.get_or_init(Secp256k1::new)
}

unsafe fn wipe(ptr: *mut u8, len: usize) {
    if !ptr.is_null() {
        for i in 0..len {
            std::ptr::write_volatile(ptr.add(i), 0u8);
        }
    }
    std::sync::atomic::compiler_fence(std::sync::atomic::Ordering::SeqCst);
}

pub(super) fn wipe_bytes(bytes: &mut [u8]) {
    unsafe { wipe(bytes.as_mut_ptr(), bytes.len()) };
}

fn wipe_val<T>(value: &mut T) {
    unsafe { wipe(value as *mut T as *mut u8, std::mem::size_of::<T>()) };
}

pub(super) fn wipe_string(text: &mut String) {
    unsafe { wipe(text.as_mut_ptr(), text.len()) };
}

pub(super) fn wipe_xpriv(node: &mut Xpriv) {
    node.private_key.non_secure_erase();
    wipe_val(&mut node.chain_code);
}

unsafe fn read<'a>(ptr: *const u8, len: usize) -> &'a [u8] {
    if len == 0 {
        return &[];
    }
    std::slice::from_raw_parts(ptr, len)
}

fn write78(node: &[u8; 78], out: *mut u8) -> i32 {
    unsafe { std::ptr::copy_nonoverlapping(node.as_ptr(), out, 78) };
    78
}

pub(super) unsafe fn el_hd_master(seed: *const u8, seed_len: usize, out: *mut u8) -> i32 {
    match Xpriv::new_master(bitcoin::Network::Bitcoin, read(seed, seed_len)) {
        Ok(mut master) => {
            let mut encoded = master.encode();
            let written = write78(&encoded, out);
            wipe_bytes(&mut encoded);
            wipe_xpriv(&mut master);
            written
        }
        Err(_) => -1,
    }
}

pub(super) unsafe fn el_hd_ckd_priv(node: *const u8, index: u32, out: *mut u8) -> i32 {
    let mut parent = match Xpriv::decode(read(node, 78)) {
        Ok(parent) => parent,
        Err(_) => return -1,
    };
    let depth = match parent.depth.checked_add(1) {
        Some(depth) => depth,
        None => {
            wipe_xpriv(&mut parent);
            return -1;
        }
    };
    let i = ChildNumber::from(index);
    let mut engine = HmacEngine::<sha512::Hash>::new(&parent.chain_code[..]);
    if i.is_hardened() {
        engine.input(&[0u8]);
        engine.input(&parent.private_key[..]);
    } else {
        engine.input(&PublicKey::from_secret_key(ctx(), &parent.private_key).serialize());
    }
    engine.input(&u32::from(i).to_be_bytes());
    let mut hmac = Hmac::<sha512::Hash>::from_engine(engine).to_byte_array();
    let mut chain_code = [0u8; 32];
    chain_code.copy_from_slice(&hmac[32..]);
    // Single exit, so every secret temporary is wiped on every path below.
    let result = 'ckd: {
        let il: &[u8] = &hmac[..32];
        let child_key = if il.iter().all(|b| *b == 0) {
            // I_L == 0: BIP32 says proceed with the next value for i, and so
            // do we — the retry verdict (statistically unreachable) goes to
            // the caller's retry loop instead of inventing a node BIP32 never
            // defines (issue #359).
            break 'ckd 1;
        } else {
            let mut tweak = match SecretKey::from_slice(il) {
                Ok(tweak) => tweak,
                Err(_) => break 'ckd 1, // I_L >= n: retry with the next index
            };
            let mut scalar = Scalar::from(tweak);
            let sum = parent.private_key.add_tweak(&scalar);
            tweak.non_secure_erase();
            scalar.non_secure_erase();
            match sum {
                Ok(sum) => sum,
                Err(_) => break 'ckd 1, // child would be zero: retry
            }
        };
        let mut child = Xpriv {
            network: parent.network,
            depth,
            parent_fingerprint: parent.fingerprint(ctx()),
            child_number: i,
            private_key: child_key,
            chain_code: ChainCode::from(chain_code),
        };
        let mut encoded = child.encode();
        let written = write78(&encoded, out);
        wipe_bytes(&mut encoded);
        wipe_xpriv(&mut child);
        written
    };
    wipe_xpriv(&mut parent);
    wipe_bytes(&mut hmac);
    wipe_bytes(&mut chain_code);
    result
}

pub(super) unsafe fn el_bip39_entropy_to_mnemonic(entropy: *const u8, len: usize, out: *mut u8, cap: usize) -> i32 {
    let mnemonic = match Mnemonic::from_entropy_in(Language::English, read(entropy, len)) {
        Ok(mnemonic) => mnemonic,
        Err(_) => return -1,
    };
    let mut phrase = mnemonic.words().collect::<Vec<&str>>().join(" ");
    if phrase.len() > cap {
        wipe_string(&mut phrase);
        return -1;
    }
    std::ptr::copy_nonoverlapping(phrase.as_ptr(), out, phrase.len());
    let len = phrase.len() as i32;
    wipe_string(&mut phrase);
    len
}

fn network_from_selector(sel: u8) -> Option<Network> {
    match sel {
        0 => Some(Network::Bitcoin),
        1 => Some(Network::Testnet),
        2 => Some(Network::Signet),
        3 => Some(Network::Regtest),
        _ => None,
    }
}

fn write_script(script: ScriptBuf, out: *mut u8, cap: usize) -> i32 {
    let bytes = script.as_bytes();
    if bytes.len() > cap {
        return -1;
    }
    unsafe { std::ptr::copy_nonoverlapping(bytes.as_ptr(), out, bytes.len()) };
    bytes.len() as i32
}

pub(super) unsafe fn el_spk_p2wpkh(pubkey: *const u8, len: usize, out: *mut u8, cap: usize) -> i32 {
    if len != 33 {
        return -1;
    }
    let pk = match bitcoin::CompressedPublicKey::from_slice(read(pubkey, len)) {
        Ok(pk) => pk,
        Err(_) => return -1,
    };
    write_script(ScriptBuf::new_p2wpkh(&pk.wpubkey_hash()), out, cap)
}

pub(super) unsafe fn el_addr_from_script(script: *const u8, len: usize, net_sel: u8, out: *mut u8, cap: usize) -> i32 {
    let network = match network_from_selector(net_sel) {
        Some(network) => network,
        None => return -1,
    };
    let script = ScriptBuf::from(read(script, len).to_vec());
    let addr = match Address::from_script(&script, network) {
        Ok(addr) => addr.to_string(),
        Err(_) => return -1,
    };
    let bytes = addr.as_bytes();
    if bytes.len() > cap {
        return -1;
    }
    std::ptr::copy_nonoverlapping(bytes.as_ptr(), out, bytes.len());
    bytes.len() as i32
}
