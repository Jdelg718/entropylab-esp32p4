use entropylab_coin_core::el_coin_to_hex;
#[test] fn capacity_and_atomic_invalid_symbol() { unsafe {
 let mut b=[b'0';256]; let mut o=[0xa5;67];
 assert_eq!(el_coin_to_hex(b.as_ptr(),128,12,o.as_mut_ptr().add(1),33),0);
 for pos in 0..256 { for bad in [0,b' ',b'\n',b'\t',b'\r',b'H',b'T',b'2',b',',0xa0,0xef,255] {
  b[pos]=bad; o.fill(0xa5);
  assert_eq!(el_coin_to_hex(b.as_ptr(),256,24,o.as_mut_ptr().add(1),65),-4);
  assert_eq!(o,[0xa5;67]); assert_eq!(b[pos],bad); b[pos]=b'0';
 }}
}}
#[test] fn lengths_selectors_caps_null_overflow_overlap() { unsafe {
 let b=[b'0';300]; let mut o=[0xa5;67];let p=o.as_mut_ptr().add(1);
 for (w,n) in [(12,128),(15,160),(18,192),(21,224),(24,256)] {
  for len in 0..=300 { if len==n {continue} assert_eq!(el_coin_to_hex(b.as_ptr(),len,w,p,65),-1); assert_eq!(o,[0xa5;67]); }
  for cap in 0..=70 { if (n/4+1..=65).contains(&cap) {continue} assert_eq!(el_coin_to_hex(b.as_ptr(),n,w,p,cap),if cap==0||cap>65 {-1}else{-2}); assert_eq!(o,[0xa5;67]); }
 }
 for w in [0,1,11,13,16,23,25,u32::MAX] {assert_eq!(el_coin_to_hex(b.as_ptr(),256,w,p,65),-1);}
 for (bp,n,op,c) in [(core::ptr::null(),256,p,65),(b.as_ptr(),256,core::ptr::null_mut(),65),(usize::MAX as *const u8,256,p,65),(b.as_ptr(),256,usize::MAX as *mut u8,65),(b.as_ptr(),usize::MAX,p,65),(b.as_ptr(),256,p,usize::MAX)] {assert_eq!(el_coin_to_hex(bp,n,24,op,c),-1);}
 assert_eq!(o,[0xa5;67]);
 let mut overlap=[b'0';400];let p=overlap.as_mut_ptr();
 for (a,z) in [(0,0),(0,1),(0,255),(64,0)] { assert_eq!(el_coin_to_hex(p.add(a),256,24,p.add(z),65),-1); assert_eq!(overlap,[b'0';400]); }
 // Adjacent ranges are valid, not overlaps.
 assert_eq!(el_coin_to_hex(p,256,24,p.add(256),65),0);
}}
