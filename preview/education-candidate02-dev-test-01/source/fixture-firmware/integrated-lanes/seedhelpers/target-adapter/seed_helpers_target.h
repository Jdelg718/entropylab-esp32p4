#ifndef ENTROPYLAB_SEED_HELPERS_TARGET_H
#define ENTROPYLAB_SEED_HELPERS_TARGET_H

#include <stddef.h>
#include <stdint.h>
#include "../../playingcards/adapter/include/playing_cards_host_v1.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    SH_WORDS_TO_NUMBERS = 1,
    SH_NUMBERS_TO_WORDS = 2,
    SH_FINAL_WORDS = 3
};
enum {
    SH_OK = 0,
    SH_BOUNDS = -1,
    SH_CAPACITY = -2,
    SH_SELECTOR = -3,
    SH_TEXT = -4,
    SH_COUNT = -5,
    SH_NUMBER = -6
};

typedef struct {
    uint32_t text_len;
    uint32_t number_count;
    uint8_t text[216];
    uint16_t numbers[128];
} sh_output_v1;

/* Allocation-free target implementation of the accepted Seed Helpers shape.
 * Output is committed only after complete validation. FINAL_WORDS returns
 * zero-based sorted dictionary indices and performs no selection or KDF. */
int32_t seed_helpers_target_v1(const uint8_t *input, size_t input_len,
                               uint32_t operation, uint32_t total,
                               uint32_t base, sh_output_v1 *output,
                               size_t output_bytes);

/* Read-only view into the existing canonical dictionary. */
const char *seed_helpers_word_at(uint32_t zero_based_index);

/* Full-phrase integration seam. Mapping success is followed by the existing
 * conversion-only BIP39 dictionary/count/checksum gate. Only success returns a
 * canonical mnemonic suitable for the shared exactly-once derivation path.
 * FINAL_WORDS is intentionally not accepted here: candidate selection remains
 * explicit, then the completed phrase is resubmitted as WORDS_TO_NUMBERS. */
int32_t seed_target_convert(uint32_t operation, uint32_t total, uint32_t base,
                            uint32_t context, const uint8_t *input,
                            size_t input_len, pc_result_v1 *output);

#ifdef __cplusplus
}
#endif
#endif
