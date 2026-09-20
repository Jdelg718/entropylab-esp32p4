use entropylab_mnemonic_core::validate;
const GOOD:&str="abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about";
#[test]
fn strict_boundary_contract() {
 for s in [format!(" {GOOD}"),format!("{GOOD} "),GOOD.replacen(' ',"  ",1),GOOD.replacen(' ',"\t",1),GOOD.replacen('a',"A",1),GOOD.replacen('a',"а",1),GOOD.replacen(' ',"\0",1),GOOD.replacen(' ',"\n",1),GOOD.replacen(' ',",",1)] {assert_eq!(validate(s.as_bytes()).err(),Some(-4));}
 assert_eq!(validate(&[b'a';216]).err(),Some(-1));
 for n in [0,1,11,13,25] {assert_eq!(validate(vec!["abandon";n].join(" ").as_bytes()).err(),Some(-5));}
 assert_eq!(validate(GOOD.replacen("abandon","zzzz",1).as_bytes()).err(),Some(-100));
 assert_eq!(validate(GOOD.replace("about","abandon").as_bytes()).err(),Some(-6));
}
