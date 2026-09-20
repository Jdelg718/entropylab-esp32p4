#ifndef ENTROPYLAB_EXTRA_DICE_H
#define ENTROPYLAB_EXTRA_DICE_H
#include <stddef.h>
#include <stdint.h>

enum { EL_DICE_OK=0, EL_DICE_INVALID=-1, EL_DICE_INCOMPLETE=-2, EL_DICE_CAPACITY=-3 };
/*
 * Output values are zero-based BIP39 English word-list indices.
 *
 * On EL_DICE_OK, *count is the number of initialized entries in out.
 * On EL_DICE_INVALID, EL_DICE_INCOMPLETE, or EL_DICE_CAPACITY, *count is zero
 * and out is unchanged. A non-NULL count is required; when count is non-NULL,
 * it is zeroed even if another argument is invalid.
 *
 * If the output entries that would be initialized overlap the non-empty input,
 * the call is rejected with EL_DICE_INVALID before out is written. Overlap with
 * unused output capacity is harmless. The count object must not overlap the
 * non-empty input or declared output capacity. If that unsatisfiable alias
 * configuration or an unrepresentable range is supplied, the call returns
 * EL_DICE_INVALID without modifying any storage, including count. Range checks
 * use integer addresses; unrelated C pointers are not compared relationally.
 */
int el_bitbox_indices(const char *input, size_t length, uint16_t *out, size_t capacity, size_t *count);
int el_dplus_indices(const char *input, size_t length, uint16_t *out, size_t capacity, size_t *count);
/* Returns the zero-based index into the checksum-valid final-word candidates. */
int el_dplus_final_index(const char *input, size_t length, unsigned words, uint8_t *index);
#endif
