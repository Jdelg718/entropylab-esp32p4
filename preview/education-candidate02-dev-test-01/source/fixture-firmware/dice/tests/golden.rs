use entropylab_dice_core::{el_dice_to_hex,el_hex_run};
fn text(b:&[u8])->&str {std::str::from_utf8(b.split(|x|*x==0).next().unwrap()).unwrap()}
#[test] fn all_upstream_dice_fixtures_and_same_hex_core_mnemonics() {
 let mut count=0;
 for line in include_str!("../vectors/dice.tsv").lines() {
 let v:Vec<_>=line.split('\t').collect();let words:u32=v[1].parse().unwrap();let mode=v[2].parse().unwrap();
 let input:Vec<u8>=v[3].as_bytes().chunks(2).map(|c|u8::from_str_radix(std::str::from_utf8(c).unwrap(),16).unwrap()).collect();
 let mut out=[0xa5;65];let status:i32=v[4].parse().unwrap();
 assert_eq!(unsafe{el_dice_to_hex(input.as_ptr(),input.len(),mode,words,out.as_mut_ptr(),65)},status,"{}",v[0]);
 if status>=0 {
 assert_eq!(text(&out),v[5],"{}",v[0]);
 let(mut m,mut f,mut a)=([0;216],[0;9],[0;43]);
 assert_eq!(unsafe{el_hex_run(out.as_ptr(),(words as usize/3)*8,m.as_mut_ptr(),216,f.as_mut_ptr(),9,a.as_mut_ptr(),43)},0);
 assert_eq!(text(&m),v[6]);
 }else{assert_eq!(out,[0xa5;65]);}count+=1;
 }assert_eq!(count,20);
}
#[test] fn invalid_arguments_no_writes() {
 for (s,mode,words,cap,status) in [(b"1".as_slice(),0,12,65,-1),(b"1",1,13,65,-1),(b"",1,12,65,-1),(b"12 3",1,12,65,-4),(b"1230",2,12,65,-4),(b"1",1,24,64,-2),(b"1",1,12,66,-1)] {
 let mut out=[0xa5;66];assert_eq!(unsafe{el_dice_to_hex(s.as_ptr(),s.len(),mode,words,out.as_mut_ptr(),cap)},status);assert_eq!(out,[0xa5;66]);
 }
}
