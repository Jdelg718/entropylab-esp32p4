#ifndef ENTROPYLAB_DICE_CORE_H
#define ENTROPYLAB_DICE_CORE_H
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define EL_DICE_RAW_DIGITS_COLDCARD_STYLE 1u
#define EL_DICE_COLEMAN_6_TO_0_BEFORE_SHA256 2u
#define EL_DICE_MAX_ROLLS 1024u
#define EL_DICE_HEX_CAP 65u
#define EL_DICE_COUNT_ONLY_NOT_QUALITY_PROOF 0
#define EL_DICE_WEAK_INPUT_LAB_ONLY 1
/* Returns strict nominal threshold 50/62/75/87/100 for 12/15/18/21/24;
 * -1 for unknown selector. Assumes fair independent D6, NEVER proves quality. */
int32_t el_dice_required_rolls(uint32_t words);
/* ONLY bounded ASCII keypad digits 1..6, no separators, NUL, normalization,
 * import filtering or implicit mode. 1..1024 rolls; all enter SHA256, first
 * 16/20/24/28/32 bytes selected. Coleman maps each 6 to ASCII 0 BEFORE hashing.
 * Output lowercase hex + NUL, cap 1..65 (must fit encoded hex + NUL).
 * Return 1: valid output BUT WEAK_INPUT_LAB_ONLY, below strict nominal count.
 * Return 0: count met ONLY, NOT randomness certification. Callers MUST surface
 * warning 1, never interpret any nonnegative return as a quality certificate.
 * Errors -1 invalid selector/mode/length/pointers/ranges/cap, -2 insufficient
 * cap, -4 invalid digit. ALL negative results leave output entirely unchanged.
 * Output suffix beyond terminating NUL unchanged. No truncation or padding.
 * Caller owns live disjoint readable input and exclusive writable output;
 * keep input stable during call, no concurrent mutation; no retained pointers.
 * Bounds/overflow/overlap are checked; pointer liveness is caller obligation.
 * Not a C string input. No RNG, network, signing, or storage in this adapter.
 */
int32_t el_dice_to_hex(const uint8_t *rolls, size_t len, uint32_t mode,
                       uint32_t words, uint8_t *out, size_t cap);
#ifdef __cplusplus
}
#endif
#endif
