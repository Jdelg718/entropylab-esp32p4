#ifndef EL_BASES_ADAPTER_CONTRACT_H
#define EL_BASES_ADAPTER_CONTRACT_H
#include <stddef.h>
#include <stdint.h>
/* Native in-process objects, NOT a wire ABI. No GUI modes allocated here. */
#define EL_BASES_CONTRACT_VERSION 1u
#define EL_BASES_INPUT_MAX 1024u
#define EL_BASES_ENTROPY_MAX 32u
#define EL_BASES_TEXT_CAP 129u
#define EL_BASES_ENCODE 1u
#define EL_BASES_DECODE 2u
/* Envelope status is separate from the unmodified core nb_status. */
#define EL_BASES_ACCEPTED 0u
#define EL_BASES_BAD_ENVELOPE 1u
#define EL_BASES_CORE_NOT_RUN (-1)
typedef struct {
 uint64_t request_id, revision;
 uint32_t version, operation, base, words, input_len;
 uint8_t input[EL_BASES_INPUT_MAX];
} el_bases_request_v1;
typedef struct {
 uint64_t request_id, revision;
 uint32_t version, operation, base, words, envelope_status;
 int32_t core_status;
 uint32_t written;
 uint8_t output[EL_BASES_TEXT_CAP];
} el_bases_result_v1;
/* q/r live, stable, fully disjoint objects; r non-NULL. q may be NULL.
 * Full by-value snapshot before enqueue; caller clears staging on both outcomes.
 * Request has raw bytes + explicit length, no NUL requirement or normalization.
 * Request ID/revision are opaque echoes, not dispatcher identities.
 * Execute has no GUI/queue/crypto/logging side effects. */
void el_bases_contract_execute(const el_bases_request_v1 *q, el_bases_result_v1 *r);
void el_bases_contract_clear(void *p, size_t n);
#endif
