use entropylab_mnemonic_core::{validate,el_mnemonic_run};
#[test]
fn all_word_counts_and_published_vectors() {
 for n in [16,20,24,28,32] {for byte in [0u8,255,0x55] {
 let entropy=vec![byte;n];
 let m=bip39::Mnemonic::from_entropy(&entropy).unwrap();
 let text=m.to_string();let (raw,k)=validate(text.as_bytes()).unwrap().to_entropy_array();assert_eq!(&raw[..k],entropy);
 let(mut e,mut f,mut a)=([0xa5;65],[0xa5;9],[0xa5;43]);
 assert_eq!(unsafe{el_mnemonic_run(text.as_ptr(),text.len(),e.as_mut_ptr(),65,f.as_mut_ptr(),9,a.as_mut_ptr(),43)},0);
 let expected=entropy.iter().map(|b|format!("{b:02x}")).collect::<String>();assert_eq!(&e[..n*2],expected.as_bytes());
 }}
 for line in include_str!("../vectors/bip39-english.tsv").lines() {
 let v:Vec<_>=line.split('\t').collect();let(raw,k)=validate(v[1].as_bytes()).unwrap().to_entropy_array();
 assert_eq!(raw[..k].iter().map(|b|format!("{b:02x}")).collect::<String>(),v[0]);
 }
}
#[test]
fn bound_is_computed_from_actual_dictionary() {
 let words=bip39::Language::English.word_list();assert_eq!(words.len(),2048);
 assert_eq!(words.iter().map(|w|w.len()).max(),Some(8));
 for n in [12,15,18,21,24] {assert!(n*8+n-1<=215)}
 // Exact 215-byte canonical dictionary input: checksum result, never length rejection.
 let s=vec!["abstract";24].join(" ");assert_eq!(s.len(),215);assert_ne!(validate(s.as_bytes()).err(),Some(-1));
}
