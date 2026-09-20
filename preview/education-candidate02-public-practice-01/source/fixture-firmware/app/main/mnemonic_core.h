#ifndef EL_MNEMONIC_CORE_H
#define EL_MNEMONIC_CORE_H
#include <stddef.h>
#include <stdint.h>
/* Public-fixture-only candidate. No normalization. Input length excludes NUL;
 * NUL inside input is rejected. Canonical lowercase ASCII English, 12/15/18/21/24
 * words, one ASCII space, max 215 bytes. Count, dictionary, checksum precede
 * derivation. Empty passphrase, mainnet m/84'/0'/0'/0/0 only.
 * Caller owns live readable input and exclusively writable outputs; each range
 * must be within one allocation. No overlap of ANY full declared ranges.
 * Pointer validity cannot be proved by this API. No concurrent mutation.
 * Capacities include NUL: entropy <=65, fingerprint <=9, address <=43.
 * Success writes only entropy hex + NUL, fingerprint + NUL, address + NUL.
 * EVERY returned error leaves ALL output bytes unchanged: caller must invalidate
 * prior success itself. 0 success; -1 null/length/cap bounds/overflow/overlap;
 * -2 insufficient output; -3 derivation; -4 noncanonical text; -5 word count;
 * -6 checksum; -100-index unknown word (zero-based position, no token logging).
 * Zero input length is -1 at ABI (safe Rust validator returns -5).
 * No erasure guarantee; allocation aborts are not returned errors.
 */
int32_t el_mnemonic_run(const uint8_t*,size_t,uint8_t*,size_t,uint8_t*,size_t,uint8_t*,size_t);
#endif
