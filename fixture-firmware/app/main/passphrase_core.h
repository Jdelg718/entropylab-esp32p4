#pragma once
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Conversion only, no KDF. GUI modes: HEX=0, COINS=1, D6_RAW=2,
 * D6_COLEMAN=3, MNEMONIC=4; words: 12/15/18/21/24.
 * HEX exact ENT/4 ASCII hex (upper accepted); COINS exact ENT bits;
 * dice 1..1024 ASCII 1..6 (Coleman maps 6 to 0 before SHA256);
 * Words 1..215 bytes, strict canonical English with matching count/checksum.
 * Input and FULL declared output capacity must be live, stable, disjoint
 * single-allocation ranges; input readable, output exclusively writable.
 * Pointer liveness is caller responsibility. Output cap 1..216, success writes
 * <=215 canonical bytes plus NUL, preserving unused tail. Errors write nothing.
 * Precedence: -1 invalid mode/count/length/null/cap/range/overlap; then input
 * validation (-4 syntax, -5 Words count, -100-index unknown word, -6 checksum);
 * -3 internal conversion failure; -2 insufficient capacity for validated text.
 * 0 normal, 1 weak dice nominal count (not an entropy-quality certificate).
 */
int32_t el_input_to_mnemonic(const uint8_t *input, size_t input_len,
                           uint32_t mode, uint32_t words,
                           uint8_t *mnemonic, size_t mnemonic_cap);
/* Approved passphrase ABI, unchanged argument order/types. Canonical English
 * mnemonic 1..215; raw passphrase <=256 (NULL allowed only for zero length).
 * All nonempty input/output FULL-capacity ranges live, stable and disjoint.
 * Output caps 1..65 / 1..9 / 1..43, required entropy hex+NUL / 9 / 43.
 * Invalid UTF-8/NUL rejected; NFKD <=1024 bytes. All outputs unchanged on error.
 * Status: -1 ABI bounds/ranges, -4 syntax, -5 word count, -100-index unknown
 * word, -6 checksum, -2 short output, -7 passphrase encoding/NUL,
 * -8 normalized overflow, -3 derivation error; 0 success.
 */
int32_t el_bip39_passphrase_run(
    const uint8_t *mnemonic, size_t mnemonic_len,
    const uint8_t *passphrase, size_t passphrase_len,
    uint8_t *entropy, size_t entropy_cap,
    uint8_t *fingerprint, size_t fingerprint_cap,
    uint8_t *address, size_t address_cap);
#ifdef __cplusplus
}
#endif
