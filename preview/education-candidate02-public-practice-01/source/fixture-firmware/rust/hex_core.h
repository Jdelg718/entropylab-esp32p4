#ifndef ENTROPYLAB_HEX_CORE_H
#define ENTROPYLAB_HEX_CORE_H
#include <stddef.h>
#include <stdint.h>
#define EL_HEX_MNEMONIC_CAP 216
#define EL_HEX_FINGERPRINT_CAP 9
#define EL_HEX_ADDRESS_CAP 43
#ifdef __cplusplus
extern "C" {
#endif
/* English BIP39; EMPTY passphrase only; Bitcoin mainnet BIP84 m/84'/0'/0'/0/0.
 * Raw ASCII hex only, length exactly 32/40/48/56/64; either case accepted.
 * No NUL in input length. No whitespace/prefix/normalization/truncation/hashing.
 * Caller owns all buffers. All four full ranges must be live, nonoverlapping;
 * input readable, outputs exclusively writable for call duration. No retention.
 * Capacities include NUL and must be 1..216, 1..9, 1..43 respectively.
 * Return 0: NUL-terminated outputs; bytes beyond NUL unchanged.
 * -1: invalid length/pointer/capacity/range/overlap; -2: insufficient capacity;
 * -3: derivation failure; -4: invalid hex byte. Every error leaves outputs intact.
 * Valid lengths/range arithmetic checked before slices. Pointer liveness and
 * actual allocation sizes CANNOT be verified: caller violating them causes UB.
 * OOM/panic aborts (no error return guarantee). Not a production wallet API.
 */
int32_t el_hex_run(const uint8_t *hex, size_t hex_len,
                   uint8_t *mnemonic, size_t mnemonic_cap,
                   uint8_t *fingerprint, size_t fingerprint_cap,
                   uint8_t *address, size_t address_cap);
/* ESP-IDF integrator supplies existing fixture allocator ABI and libc abort.
 * Allocator must honor alignment, return NULL on failure; free matching blocks.
 */
void *fixture_alloc(size_t size, size_t align);
void fixture_free(void *ptr);
#ifdef __cplusplus
}
#endif
#endif
