use std::io::{self,Read};
use entropylab_native_spike::derive;
fn wipe(bytes: &mut [u8]) {
    for b in bytes { unsafe { std::ptr::write_volatile(b,0); } }
    std::sync::atomic::compiler_fence(std::sync::atomic::Ordering::SeqCst);
}
fn process(input:&[u8])->Result<(), &'static str> {
    let text=std::str::from_utf8(input).map_err(|_|"invalid UTF-8")?;
    let text=text.strip_suffix('\n').ok_or("four newline-terminated fields required")?;
    let fields:Vec<_>=text.split('\n').collect();
    if fields.len()!=4 { return Err("four fields required"); }
    let num=|s:&str| -> Result<u32,&'static str> {
        if s.is_empty() || s.len()>4 || !s.bytes().all(|b|b.is_ascii_digit()) { return Err("invalid index"); }
        s.parse().map_err(|_|"invalid index")
    };
    let (change,index)=(num(fields[2])?,num(fields[3])?);
    let output=derive(fields[0],fields[1],change,index).map_err(|_|"unsupported or invalid derivation input")?;
    println!("mnemonic={}",output.mnemonic);
    print!("seed="); for b in output.seed { print!("{b:02x}"); } println!();
    println!("fingerprint={}",output.fingerprint);
    println!("path=m/84'/0'/0'/{change}/{index}");
    println!("address={}",output.address);
    Ok(())
}
fn main() {
    eprintln!("HOST SPIKE: published test vectors only; stdout contains mnemonic and seed. Not a secure appliance.");
    let mut input=Vec::with_capacity(225);
    let result=if std::env::args_os().len()!=1 { Err("no command-line arguments; use four stdin fields") }
    else if io::stdin().take(225).read_to_end(&mut input).is_err() { Err("input read failed") }
    else if input.len()>224 { Err("input exceeds 224-byte cap") }
    else { process(&input) };
    wipe(&mut input);
    if let Err(e)=result { eprintln!("error: {e}"); std::process::exit(2); }
}
