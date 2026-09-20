use entropylab_native_spike::{derive, Error};
fn hex(bytes: &[u8]) -> String { bytes.iter().map(|b| format!("{b:02x}")).collect() }
#[test]
fn published_bip39_english_vectors() {
    for line in include_str!("../../vectors/bip39-english.tsv").lines() {
        let v: Vec<_> = line.split('\t').collect();
        let out = derive(v[0], "TREZOR", 0, 0).unwrap();
        assert_eq!(out.mnemonic, v[1]);
        assert_eq!(hex(&out.seed), v[2]);
    }
}
#[test]
fn published_bip84_addresses() {
    for (change,index,address) in [
        (0,0,"bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu"),
        (0,1,"bc1qnjg0jd8228aq7egyzacy8cys3knf9xvrerkf9g"),
        (1,0,"bc1q8c6fshw2dlwun7ekn9qwf37cu2rn755upcp6el")] {
        let out = derive(&"00".repeat(16), "", change,index).unwrap();
        assert_eq!(out.address,address);
    }
}
#[test]
fn rejects_invalid_entropy_without_trimming_or_prefixes() {
    for s in ["".to_owned(), "00".repeat(15), "00".repeat(17),"00".repeat(33),"0".repeat(31),format!(" {}", "00".repeat(16)),format!("0x{}", "00".repeat(16))] {
        assert_eq!(derive(&s,"",0,0).unwrap_err(),Error::EntropyLength);
    }
    for s in ["gg".repeat(16), "  ".repeat(16), "é".repeat(16)] {
        assert_eq!(derive(&s,"",0,0).unwrap_err(),Error::Hex);
    }
}
#[test]
fn rejects_unsupported_passphrase_explicitly() {
    for p in ["é".to_owned(),"e\u{301}".to_owned(),"a\0b".to_owned(),"a\nb".to_owned(),"a".repeat(129)] {
        assert_eq!(derive(&"00".repeat(16),&p,0,0).unwrap_err(),Error::Passphrase);
    }
}
#[test]
fn rejects_unbounded_or_hardened_address_indices() {
    for (c,i) in [(2,0),(0,1001),(0,0x80000000),(u32::MAX,u32::MAX)] {
        assert_eq!(derive(&"00".repeat(16),"",c,i).unwrap_err(),Error::Path);
    }
}
