//! Conversion only: no seed derivation, normalization, or extra word list.
use alloc::string::{String, ToString};
use bip39::{Language, Mnemonic};

struct Scratch<const N: usize>([u8; N]);
impl<const N: usize> Drop for Scratch<N> {
    fn drop(&mut self) {
        for b in &mut self.0 {
            unsafe { core::ptr::write_volatile(b, 0) }
        }
        core::sync::atomic::compiler_fence(core::sync::atomic::Ordering::SeqCst);
    }
}
struct Text(String);
impl Drop for Text {
    fn drop(&mut self) {
        // Best effort for this owned allocation only; not all library temporaries.
        for b in unsafe { self.0.as_bytes_mut() } {
            unsafe { core::ptr::write_volatile(b, 0) }
        }
        core::sync::atomic::compiler_fence(core::sync::atomic::Ordering::SeqCst);
    }
}
fn nibble(b: u8) -> Result<u8, i32> {
    match b {
        b'0'..=b'9' => Ok(b - b'0'),
        b'a'..=b'f' => Ok(b - b'a' + 10),
        b'A'..=b'F' => Ok(b - b'A' + 10),
        _ => Err(-4),
    }
}

/// Convert the five GUI modes into canonical English BIP39 without any KDF.
/// # Safety
/// Input and full declared output capacity must be live in single allocations,
/// stable and disjoint; input readable and output exclusively writable. Integer
/// checks cannot establish pointer liveness. No output bytes change on errors.
#[no_mangle]
pub unsafe extern "C" fn el_input_to_mnemonic(
    input: *const u8,
    input_len: usize,
    mode: u32,
    words: u32,
    mnemonic: *mut u8,
    mnemonic_cap: usize,
) -> i32 {
    if mode > 4
        || !matches!(words, 12 | 15 | 18 | 21 | 24)
        || input.is_null()
        || mnemonic.is_null()
        || input_len == 0
        || !(1..=216).contains(&mnemonic_cap)
    {
        return -1;
    }
    let bytes = words as usize / 3 * 4;
    let length_ok = match mode {
        0 => input_len == bytes * 2,
        1 => input_len == bytes * 8,
        2 | 3 => input_len <= 1024,
        4 => input_len <= 215,
        _ => false,
    };
    if !length_ok {
        return -1;
    }
    let start = input as usize;
    let dest = mnemonic as usize;
    let Some(end) = start.checked_add(input_len) else {
        return -1;
    };
    let Some(dest_end) = dest.checked_add(mnemonic_cap) else {
        return -1;
    };
    if start < dest_end && dest < end {
        return -1;
    }
    let mut hex = Scratch([0u8; 65]);
    let mut entropy = Scratch([0u8; 32]);
    let mut status = 0;
    let parsed = if mode == 4 {
        let m =
            match entropylab_mnemonic_core::validate(core::slice::from_raw_parts(input, input_len))
            {
                Ok(m) => m,
                Err(e) => return e,
            };
        if m.word_count() != words as usize {
            return -5;
        }
        m
    } else {
        let source = if mode == 0 {
            core::slice::from_raw_parts(input, input_len)
        } else {
            status = if mode == 1 {
                entropylab_coin_core::el_coin_to_hex(
                    input,
                    input_len,
                    words,
                    hex.0.as_mut_ptr(),
                    65,
                )
            } else {
                entropylab_dice_core::el_dice_to_hex(
                    input,
                    input_len,
                    mode - 1,
                    words,
                    hex.0.as_mut_ptr(),
                    65,
                )
            };
            if status < 0 {
                return status;
            }
            &hex.0[..bytes * 2]
        };
        for (i, pair) in source.chunks_exact(2).enumerate() {
            let hi = match nibble(pair[0]) {
                Ok(v) => v,
                Err(e) => return e,
            };
            let lo = match nibble(pair[1]) {
                Ok(v) => v,
                Err(e) => return e,
            };
            entropy.0[i] = (hi << 4) | lo;
        }
        match Mnemonic::from_entropy_in(Language::English, &entropy.0[..bytes]) {
            Ok(m) => m,
            Err(_) => return -3,
        }
    };
    let text = Text(parsed.to_string());
    if mnemonic_cap <= text.0.len() {
        return -2;
    }
    core::ptr::copy_nonoverlapping(text.0.as_ptr(), mnemonic, text.0.len());
    mnemonic.add(text.0.len()).write(0);
    status
}
