#ifndef ENTROPYLAB_CARDS_TARGET_H
#define ENTROPYLAB_CARDS_TARGET_H

#include <stddef.h>
#include <stdint.h>
#include "../adapter/include/playing_cards_host_v1.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Allocation-free target conversion. Output is committed only on success. */
uint32_t cards_target_convert(uint32_t mode, uint32_t words, uint32_t context,
                              const uint8_t *raw, size_t length,
                              pc_result_v1 *output);

/* Shared bounded result construction for target adapters that already produced
 * exact entropy bytes. Method is copied as metadata; output is transactional. */
uint32_t cards_target_result_from_entropy(uint32_t words, uint32_t context,
                                          const uint8_t *entropy, size_t entropy_len,
                                          const char *method, size_t method_len,
                                          double source_bits, uint32_t flags,
                                          pc_result_v1 *output);

#ifdef __cplusplus
}
#endif
#endif
