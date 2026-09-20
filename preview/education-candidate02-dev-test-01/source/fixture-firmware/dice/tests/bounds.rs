use entropylab_dice_core::*;
#[path="../../rust/tests/contract.rs"] mod inherited_same_core;
#[test] fn pointer_caps_overlap_and_overflow() {unsafe {
 let input=[b'1';1025];let mut out=[0xa5;65];
 for (p,n,q,c) in [(core::ptr::null(),1,out.as_mut_ptr(),65),(input.as_ptr(),1,core::ptr::null_mut(),65),(input.as_ptr(),0,out.as_mut_ptr(),65),(input.as_ptr(),1025,out.as_mut_ptr(),65),((usize::MAX-1) as *const u8,3,out.as_mut_ptr(),65),(input.as_ptr(),1,(usize::MAX-1) as *mut u8,65),(input.as_ptr(),1,out.as_mut_ptr(),0),(input.as_ptr(),1,out.as_mut_ptr(),usize::MAX),(out.as_ptr(),1,out.as_mut_ptr(),65),(out.as_ptr().add(64),1,out.as_mut_ptr(),65)] {
 assert_eq!(el_dice_to_hex(p,n,1,12,q,c),-1);assert_eq!(out,[0xa5;65]);
 }
}}
#[test] fn strict_thresholds_and_all_caps() {
 for (words,count,bits) in [(12,50,128),(15,62,160),(18,75,192),(21,87,224),(24,100,256)] {
 assert_eq!(el_dice_required_rolls(words),count as i32);
 for n in [1,count-1,count,1024] {for mode in [1,2] {
 let input=vec![b'6';n];let mut out=[0xa5;65];
 assert_eq!(unsafe{el_dice_to_hex(input.as_ptr(),n,mode,words,out.as_mut_ptr(),65)},if n<count{1}else{0});
 assert_eq!(out[bits/4],0);assert!(out[bits/4+1..].iter().all(|b|*b==0xa5));
 for cap in 1..=65 {let mut out=[0xa5;65];let result=unsafe{el_dice_to_hex(input.as_ptr(),n,mode,words,out.as_mut_ptr(),cap)};
 if cap<=bits/4{assert_eq!(result,-2);assert_eq!(out,[0xa5;65]);}else{assert!(result>=0);}}
 }}
 }
 assert_eq!(el_dice_required_rolls(0),-1);
}
#[test] fn full_transcript_and_invalid_final_byte() {unsafe {
 for mode in [1,2] {
 let mut input=[b'1';1024];let(mut a,mut b)=([0;65],[0;65]);
 assert_eq!(el_dice_to_hex(input.as_ptr(),1024,mode,12,a.as_mut_ptr(),65),0);
 input[1023]=b'6';assert_eq!(el_dice_to_hex(input.as_ptr(),1024,mode,12,b.as_mut_ptr(),65),0);assert_ne!(a,b);
 for invalid in [0,b'0',b'7',b' ',b'\n',b',',b';',b'|',255] {input[1023]=invalid;let mut out=[0xa5;65];assert_eq!(el_dice_to_hex(input.as_ptr(),1024,mode,12,out.as_mut_ptr(),65),-4);assert_eq!(out,[0xa5;65]);}
 }
}}
