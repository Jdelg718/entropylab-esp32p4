//! Host-only bounded deterministic extraction. PUBLIC TEST VECTORS ONLY.
//! ASCII passphrases only: reject unsupported Unicode, never normalize/trim implicitly.
mod upstream_core;
#[cfg(test)]
mod known_answers;
use upstream_core::*;
use bitcoin::bip32::Xpriv;
#[derive(Debug, PartialEq)]
pub enum Error { EntropyLength, Hex, Passphrase, Path, Derivation }
pub struct Output { pub mnemonic: String, pub seed: [u8;64], pub fingerprint: String, pub address: String }
// Do not derive Debug: avoid incidental secret logging.
impl std::fmt::Debug for Output {
    fn fmt(&self,f:&mut std::fmt::Formatter<'_>)->std::fmt::Result { f.write_str("Output([REDACTED])") }
}
impl Drop for Output {
    fn drop(&mut self) { wipe_string(&mut self.mnemonic); wipe_bytes(&mut self.seed); }
}
struct Bytes<const N:usize>([u8;N]);
impl<const N:usize> Drop for Bytes<N> { fn drop(&mut self) { wipe_bytes(&mut self.0); } }
/// Only mainnet m/84'/0'/0'/change/index. change <= 1, index <= 1000.
/// Entropy: strict 32/40/48/56/64 ASCII hex characters (case-insensitive).
/// Passphrase: 0..128 printable ASCII bytes, spaces preserved verbatim.
pub fn derive(hex: &str, passphrase: &str, change: u32, index: u32) -> Result<Output, Error> {
    if ![32,40,48,56,64].contains(&hex.len()) { return Err(Error::EntropyLength); }
    if !hex.bytes().all(|b| b.is_ascii_hexdigit()) { return Err(Error::Hex); }
    if passphrase.len()>128 || !passphrase.bytes().all(|b| (0x20..=0x7e).contains(&b)) { return Err(Error::Passphrase); }
    if change>1 || index>1000 { return Err(Error::Path); }
    let mut entropy=Bytes([0u8;32]);
    for (i,pair) in hex.as_bytes().chunks_exact(2).enumerate() {
        let nibble=|b:u8| if b<=b'9' { b-b'0' } else { b.to_ascii_lowercase()-b'a'+10 };
        entropy.0[i]=nibble(pair[0])*16+nibble(pair[1]);
    }
    let mut phrase=Bytes([0u8;215]);
    let n=unsafe { el_bip39_entropy_to_mnemonic(entropy.0.as_ptr(),hex.len()/2,phrase.0.as_mut_ptr(),215) };
    if n<0 { return Err(Error::Derivation); }
    let text=std::str::from_utf8(&phrase.0[..n as usize]).map_err(|_| Error::Derivation)?;
    // English canonical phrase and printable ASCII passphrase are already NFKD.
    // rust-bip39 performs standard PBKDF2-HMAC-SHA512 (2048 rounds), replacing
    // upstream JS hashing without introducing new cryptographic code.
    let mnemonic=bip39::Mnemonic::parse_in_normalized(bip39::Language::English,text).map_err(|_|Error::Derivation)?;
    let seed=Bytes(mnemonic.to_seed_normalized(passphrase));
    let mut node=Bytes([0u8;78]);
    if unsafe { el_hd_master(seed.0.as_ptr(),64,node.0.as_mut_ptr()) } !=78 { return Err(Error::Derivation); }
    let mut master=Xpriv::decode(&node.0).map_err(|_|Error::Derivation)?;
    let fingerprint=master.fingerprint(ctx()).to_string();
    wipe_xpriv(&mut master);
    for child in [84|0x80000000,0x80000000,0x80000000,change,index] {
        let mut next=Bytes([0u8;78]);
        // Fail closed on upstream's retry verdict: never silently change requested path.
        if unsafe { el_hd_ckd_priv(node.0.as_ptr(),child,next.0.as_mut_ptr()) } !=78 { return Err(Error::Derivation); }
        node.0.copy_from_slice(&next.0);
    }
    let mut child=Xpriv::decode(&node.0).map_err(|_|Error::Derivation)?;
    let public=secp256k1::PublicKey::from_secret_key(ctx(),&child.private_key).serialize();
    wipe_xpriv(&mut child);
    let mut script=[0u8;22];
    if unsafe { el_spk_p2wpkh(public.as_ptr(),33,script.as_mut_ptr(),22) } !=22 { return Err(Error::Derivation); }
    let mut address=[0u8;90];
    let n=unsafe { el_addr_from_script(script.as_ptr(),22,0,address.as_mut_ptr(),90) };
    if n<0 { return Err(Error::Derivation); }
    Ok(Output { mnemonic:text.to_owned(),seed:seed.0,fingerprint,address:std::str::from_utf8(&address[..n as usize]).map_err(|_|Error::Derivation)?.to_owned() })
}
