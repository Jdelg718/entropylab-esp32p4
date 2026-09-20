use entropylab_coin_core::el_coin_to_hex;
use entropylab_hex_core::el_hex_run;
fn text(b:&[u8])->&str { std::str::from_utf8(b.split(|b|*b==0).next().unwrap()).unwrap() }
#[test] fn upstream_raw_golden_and_same_core_integration() {
 let mut count=0;
 for line in include_str!("../vectors/raw-binary.tsv").lines() {
  let v:Vec<_>=line.split('\t').collect();let words=v[1].parse().unwrap();let bits=v[2].as_bytes();
  let mut h=[0xa5;65];assert_eq!(unsafe{el_coin_to_hex(bits.as_ptr(),bits.len(),words,h.as_mut_ptr(),65)},0,"{}",v[0]);
  assert_eq!(text(&h),v[3],"{}",v[0]);
  let(mut m,mut f,mut a)=([0;216],[0;9],[0;43]);
  assert_eq!(unsafe{el_hex_run(h.as_ptr(),bits.len()/4,m.as_mut_ptr(),216,f.as_mut_ptr(),9,a.as_mut_ptr(),43)},0);
  assert_eq!(text(&m),v[4]);
  if v[0]=="zero-24" { assert_eq!(text(&f),"5436d724"); println!("PUBLIC zero256: {} {} {}",text(&m),text(&f),text(&a)); }
  if v[0]=="leading-zeros-24" { assert_eq!(text(&f),"53f6b5aa"); println!("PUBLIC final1111: {} {} {}",text(&m),text(&f),text(&a)); }
  count+=1;
 }
 assert_eq!(count,22);
}
