#ifndef ENTROPYLAB_BASES_TARGET_H
#define ENTROPYLAB_BASES_TARGET_H

#include <stddef.h>
#include <stdint.h>
#include "../../../integrated-lanes/playingcards/adapter/include/playing_cards_host_v1.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Decode canonical base-4/8/32/64 GUI text into entropy and BIP39 metadata.
 * Allocation-free; output is committed only on success. */
uint32_t bases_target_convert(uint32_t base, uint32_t words, uint32_t context,
                              const uint8_t *raw, size_t length,
                              pc_result_v1 *output);

#ifdef __cplusplus
}
#endif
#endif