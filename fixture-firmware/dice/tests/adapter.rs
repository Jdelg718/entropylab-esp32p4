use entropylab_dice_core::el_dice_to_hex;
#[test] fn single_six_raw_weak_lab_output() {
 let mut out=[0xa5;65];
 assert_eq!(unsafe {el_dice_to_hex(b"6".as_ptr(),1,1,12,out.as_mut_ptr(),65)},1);
 assert_eq!(&out[..33],b"e7f6c011776e8db7cd330b54174fd76f\0");
 assert_eq!(out[33],0xa5);
}
