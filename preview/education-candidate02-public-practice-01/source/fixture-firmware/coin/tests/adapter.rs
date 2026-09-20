use entropylab_coin_core::el_coin_to_hex;
#[test]
fn public_zero_256() {
 let bits=[b'0';256]; let mut out=[0xa5;67];
 assert_eq!(unsafe{el_coin_to_hex(bits.as_ptr(),256,24,out.as_mut_ptr().add(1),65)},0);
 assert_eq!(&out[1..65], &[b'0';64]); assert_eq!(out[65],0);
 assert_eq!((out[0],out[66]),(0xa5,0xa5));
}
