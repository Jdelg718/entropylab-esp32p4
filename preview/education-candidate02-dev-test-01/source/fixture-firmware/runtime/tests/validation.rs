use entropylab_mnemonic_core::validate;
#[test]
fn published_bip84_recovers_entropy() {
 let input=b"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about";
 let (entropy,n)=validate(input).unwrap().to_entropy_array();
 assert_eq!(n,16); assert_eq!(&entropy[..n], &[0u8;16]);
}
