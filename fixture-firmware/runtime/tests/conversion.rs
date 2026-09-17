use bip39::{Language, Mnemonic};
use entropylab_runtime::el_input_to_mnemonic;
const GOOD: &[u8] = b"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about";
fn check(input: &[u8], mode: u32, words: u32, expected: &str, status: i32) {
    let mut out = [0xa5; 218];
    assert_eq!(
        unsafe {
            el_input_to_mnemonic(
                input.as_ptr(),
                input.len(),
                mode,
                words,
                out.as_mut_ptr().add(1),
                216,
            )
        },
        status
    );
    assert_eq!(&out[1..1 + expected.len()], expected.as_bytes());
    assert_eq!(out[1 + expected.len()], 0);
    assert_eq!(out[0], 0xa5);
    assert!(out[expected.len() + 2..].iter().all(|&b| b == 0xa5));
}
fn decode(s: &str) -> Vec<u8> {
    s.as_bytes()
        .chunks_exact(2)
        .map(|c| u8::from_str_radix(std::str::from_utf8(c).unwrap(), 16).unwrap())
        .collect()
}
#[test]
fn all_modes_all_counts_public_oracles() {
    // Independent Python hashlib SHA256 of literal public transcripts, not dice-core.
    let hashes = [
        "8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92",
        "eb76e2254ff1cb43115299491e1906fbf39921b384b983cf529ebe674d9915c1",
    ];
    for words in [12, 15, 18, 21, 24] {
        let n = words as usize / 3 * 4;
        for hex in ["00".repeat(n), "a5".repeat(n), "ff".repeat(n)] {
            let bytes = decode(&hex);
            let expected = Mnemonic::from_entropy_in(Language::English, &bytes)
                .unwrap()
                .to_string();
            check(hex.as_bytes(), 0, words, &expected, 0);
            check(hex.to_uppercase().as_bytes(), 0, words, &expected, 0);
            let bits: String = bytes.iter().map(|b| format!("{b:08b}")).collect();
            check(bits.as_bytes(), 1, words, &expected, 0);
            check(expected.as_bytes(), 4, words, &expected, 0);
        }
        for mode in [2, 3] {
            let expected = Mnemonic::from_entropy_in(
                Language::English,
                &decode(hashes[(mode - 2) as usize])[..n],
            )
            .unwrap()
            .to_string();
            check(b"123456", mode, words, &expected, 1);
            // Same digest entropy via HEX/COINS/Words must produce identical text.
            check(
                &hashes[(mode - 2) as usize].as_bytes()[..n * 2],
                0,
                words,
                &expected,
                0,
            );
        }
    }
}
#[test]
fn dice_nominal_thresholds_and_maximum() {
    for (words, required) in [(12, 50), (15, 62), (18, 75), (21, 87), (24, 100)] {
        for mode in [2, 3] {
            for len in [1, required - 1, required, required + 1, 1024] {
                let input = vec![b'6'; len];
                let mut out = [0xa5; 216];
                assert_eq!(
                    unsafe {
                        el_input_to_mnemonic(
                            input.as_ptr(),
                            len,
                            mode,
                            words,
                            out.as_mut_ptr(),
                            216,
                        )
                    },
                    i32::from(len < required)
                );
                if len == 1024 {
                    let h = if mode == 2 {
                        "259339b0bb91cea0710ad54d2625fc2eb247929f6c388978db2225082f41bfdd"
                    } else {
                        "35ae5091b37e8f0f306833ef57a635f9dc06738d7f4e563a610eec2adb26fe28"
                    };
                    let m = Mnemonic::from_entropy_in(
                        Language::English,
                        &decode(h)[..words as usize / 3 * 4],
                    )
                    .unwrap()
                    .to_string();
                    assert_eq!(&out[..m.len()], m.as_bytes());
                }
            }
        }
    }
}
fn fail(input: &[u8], mode: u32, words: u32, cap: usize, rc: i32) {
    let mut out = [0xa5; 218];
    assert_eq!(
        unsafe {
            el_input_to_mnemonic(
                input.as_ptr(),
                input.len(),
                mode,
                words,
                out.as_mut_ptr().add(1),
                cap,
            )
        },
        rc
    );
    assert_eq!(out, [0xa5; 218]);
}
#[test]
fn malformed_and_capacities_preserve_output() {
    for mode in [5, u32::MAX] {
        fail(GOOD, mode, 12, 216, -1);
    }
    for words in [0, 11, 13, 25, u32::MAX] {
        fail(GOOD, 4, words, 216, -1);
    }
    for cap in 0..=217 {
        let mut out = [0xa5; 218];
        let rc = unsafe {
            el_input_to_mnemonic(
                GOOD.as_ptr(),
                GOOD.len(),
                4,
                12,
                out.as_mut_ptr().add(1),
                cap,
            )
        };
        let expected = if cap == 0 || cap > 216 {
            -1
        } else if cap <= GOOD.len() {
            -2
        } else {
            0
        };
        assert_eq!(rc, expected);
        if rc < 0 {
            assert_eq!(out, [0xa5; 218]);
        } else {
            assert_eq!(out[0], 0xa5);
            assert!(out[GOOD.len() + 2..].iter().all(|&b| b == 0xa5));
        }
    }
    for mode in 0..5 {
        fail(b"", mode, 12, 216, -1);
        fail(&vec![b'1'; 1025], mode, 12, 216, -1);
    }
    for n in [1, 31, 33, 64, 65] {
        fail(&vec![b'0'; n], 0, 12, 216, -1);
    }
    for n in [127, 129, 256, 257] {
        fail(&vec![b'0'; n], 1, 12, 216, -1);
    }
    for byte in [b'g', b' ', 0, 0xff] {
        let mut h = vec![b'0'; 32];
        h[31] = byte;
        fail(&h, 0, 12, 216, -4);
    }
    let mut bits = vec![b'0'; 128];
    bits[127] = b'2';
    fail(&bits, 1, 12, 216, -4);
    for mode in [2, 3] {
        for input in [b"0".as_slice(), b"7", b"1 2", b"1\0", b"\xff"] {
            fail(input, mode, 12, 216, -4);
        }
    }
    fail(GOOD, 4, 15, 216, -5);
    for input in [
        format!(" {}", std::str::from_utf8(GOOD).unwrap()),
        format!("{} ", std::str::from_utf8(GOOD).unwrap()),
        std::str::from_utf8(GOOD)
            .unwrap()
            .replace("abandon", "Abandon"),
        std::str::from_utf8(GOOD).unwrap().replacen(" ", "  ", 1),
        std::str::from_utf8(GOOD).unwrap().replace(" ", "\t"),
    ] {
        fail(input.as_bytes(), 4, 12, 216, -4);
    }
    fail(b"abandon", 4, 12, 216, -5);
    fail(
        std::str::from_utf8(GOOD)
            .unwrap()
            .replace("about", "abandon")
            .as_bytes(),
        4,
        12,
        216,
        -6,
    );
    fail(
        std::str::from_utf8(GOOD)
            .unwrap()
            .replace("about", "zzzz")
            .as_bytes(),
        4,
        12,
        216,
        -111,
    );
    fail(b"\xff", 4, 12, 216, -4);
    fail(b"abandon\0", 4, 12, 216, -4);
}
#[test]
fn null_overflow_full_capacity_overlap_preflight() {
    for which in 0..2 {
        for bad in [0usize, usize::MAX - 1] {
            let mut out = [0xa5; 216];
            let mut p = [GOOD.as_ptr() as *mut u8, out.as_mut_ptr()];
            p[which] = bad as *mut u8;
            assert_eq!(
                unsafe { el_input_to_mnemonic(p[0], GOOD.len(), 4, 12, p[1], 216) },
                -1
            );
            assert_eq!(out, [0xa5; 216]);
        }
    }
    for (input_offset, out_offset) in [(0, 0), (0, GOOD.len() - 1), (215, 0)] {
        let mut arena = [0xa5; 512];
        let before = arena;
        assert_eq!(
            unsafe {
                el_input_to_mnemonic(
                    arena.as_ptr().add(input_offset),
                    GOOD.len(),
                    4,
                    12,
                    arena.as_mut_ptr().add(out_offset),
                    216,
                )
            },
            -1
        );
        assert_eq!(arena, before);
    }
    fail(GOOD, 4, 12, usize::MAX, -1);
}
#[test]
fn public_zero_hex_conversion_only() {
    let input = b"00000000000000000000000000000000";
    let mut out = [0xa5; 216];
    assert_eq!(
        unsafe {
            el_input_to_mnemonic(
                input.as_ptr(),
                input.len(),
                0,
                12,
                out.as_mut_ptr(),
                out.len(),
            )
        },
        0
    );
    assert_eq!(&out[..GOOD.len()], GOOD);
    assert_eq!(out[GOOD.len()], 0);
    assert!(out[GOOD.len() + 1..].iter().all(|b| *b == 0xa5));
}
