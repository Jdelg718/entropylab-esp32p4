use entropylab_mnemonic_core::el_mnemonic_run;
const GOOD:&[u8]=b"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about";
#[test]
fn errors_leave_all_outputs_untouched() {
 for case in 0..13 {
 let(mut e,mut f,mut a)=([0xa5;65],[0xa5;9],[0xa5;43]);
 let (mut p,mut n,mut ep,mut ec,mut fp,mut fc,mut ap,mut ac)=(GOOD.as_ptr(),GOOD.len(),e.as_mut_ptr(),65,f.as_mut_ptr(),9,a.as_mut_ptr(),43);
 let bad=GOOD[..GOOD.len()-5].iter().copied().chain(b"abandon".iter().copied()).collect::<Vec<_>>();
 let expected=match case {
 0=>{p=core::ptr::null();-1},1=>{ep=core::ptr::null_mut();-1},2=>{p=(usize::MAX-4) as *const u8;-1},3=>{fp=ep;-1},4=>{ep=p as *mut u8;-1},5=>{ec=0;-1},6=>{ec=66;-1},7=>{n=216;-1},8=>{ec=32;-2},9=>{fc=8;-2},10=>{ac=42;-2},11=>{p=bad.as_ptr();n=bad.len();-6},_=>{ap=fp;-1}};
 assert_eq!(unsafe{el_mnemonic_run(p,n,ep,ec,fp,fc,ap,ac)},expected,"case {case}");
 assert_eq!(e,[0xa5;65]);assert_eq!(f,[0xa5;9]);assert_eq!(a,[0xa5;43]);
 }
}
