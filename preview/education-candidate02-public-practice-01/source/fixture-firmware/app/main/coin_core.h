#ifndef ENTROPYLAB_COIN_CORE_H
#define ENTROPYLAB_COIN_CORE_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Direct keypad transcript ONLY: Heads=ASCII '0', Tails=ASCII '1'.
 * words must be 12/15/18/21/24; bits_len must be 128/160/192/224/256.
 * No whitespace, separators, H/T aliases, NUL, Unicode, pasted/import policy.
 * Success: writes lowercase hex (bits_len/4 chars) plus NUL, untouched suffix.
 * Capacity must be 1..65 and at least bits_len/4+1 (recommend char out[65]).
 * Returns 0 success; -1 invalid selector/count/null/range/overlap/cap>65 or 0;
 * -2 insufficient capacity; -4 invalid alphabet. This order is intentional.
 * Every error leaves input and output unchanged. Input is never modified.
 * Both declared ranges must be disjoint; integer wrap is rejected.
 * Caller MUST supply live readable input and exclusive writable output allocations
 * of declared lengths, stable throughout the call (no concurrent modification).
 * Non-null bogus pointers cannot be detected; violating this is undefined behavior.
 * No allocation, RNG, hashing, signing, network, persistence or crypto in adapter.
 * On success separately call el_hex_run(out,bits_len/4,...), existing hex_core.h.
 * On failure do NOT derive from stale output. Caller owns invalidation and wiping.
 */
int32_t el_coin_to_hex(const uint8_t *bits, size_t bits_len, uint32_t words,
                       uint8_t *out, size_t capacity);
#ifdef __cplusplus
}
#endif
#endif
