use std::io::Write;
use std::process::{Command,Stdio};
fn run(input: &[u8]) -> std::process::Output {
    let mut child=Command::new(env!("CARGO_BIN_EXE_entropylab-native-spike")).stdin(Stdio::piped()).stdout(Stdio::piped()).stderr(Stdio::piped()).spawn().unwrap();
    child.stdin.take().unwrap().write_all(input).unwrap();
    child.wait_with_output().unwrap()
}
#[test]
fn native_cli_published_vector() {
    let out=run(b"00000000000000000000000000000000\n\n0\n0\n");
    assert!(out.status.success(),"{}",String::from_utf8_lossy(&out.stderr));
    let s=String::from_utf8(out.stdout).unwrap();
    assert!(s.contains("address=bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu\n"));
    assert!(s.contains("mnemonic=abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about\n"));
    assert!(s.contains("path=m/84'/0'/0'/0/0\n"));
}
#[test]
fn cli_rejects_invalid_protocol_without_partial_secret_output() {
    for input in [vec![],vec![b'a';225], b"00\n\n0\n0\n".to_vec(), b"00000000000000000000000000000000\n\n+0\n0\n".to_vec(), b"00000000000000000000000000000000\n\n0\n0\nextra\n".to_vec(), vec![0xff;32]] {
        let out=run(&input);
        assert!(!out.status.success());
        assert!(out.stdout.is_empty());
    }
}
