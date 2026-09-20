#ifndef PLAYING_CARDS_HOST_V1_H
#define PLAYING_CARDS_HOST_V1_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define PC_V1_VERSION 1u
#define PC_V1_RAW_CAP 4096u
#define PC_V1_RESULT_SIZE 360u
#define PC_V1_HASH 0u
#define PC_V1_COLEMAN 1u
#define PC_V1_DIRECT 2u
#define PC_V1_TEST_ONLY 1u
#define PC_V1_UNDER_RECOMMENDED 2u
enum pc_v1_status {PC_V1_OK=0,PC_V1_VERSION_ERROR=1,PC_V1_MODE_ERROR=2,PC_V1_SIZE_ERROR=3,PC_V1_NULL_ERROR=4,PC_V1_ALIGNMENT_ERROR=5,PC_V1_RANGE_ERROR=6,PC_V1_ALIAS_ERROR=7,PC_V1_UTF8_ERROR=8,PC_V1_NUL_ERROR=9,PC_V1_INTERNAL_CAPACITY_ERROR=10,PC_V1_PANIC_ERROR=11,PC_V1_WORD_COUNT_ERROR=100,PC_V1_INPUT_TOO_LONG=101,PC_V1_INVALID=102,PC_V1_DUPLICATE=103,PC_V1_EMPTY=104,PC_V1_INCOMPLETE=105,PC_V1_EXTRA=106};
typedef struct pc_result_v1 {
 uint32_t version,mode,words,context,entropy_len,mnemonic_len,method_len,flags;
 double source_bits;
 uint8_t entropy[32],mnemonic[256],method[32];
} pc_result_v1;
/* See CONTRACT.md: no output mutation on error; no pointer ownership transfer. */
uint32_t pc_convert_v1(uint32_t version,uint32_t mode,uint32_t words,uint32_t context,const uint8_t *raw,uint32_t len,pc_result_v1 *out,uint32_t out_size);
#ifdef __cplusplus
}
#endif
#endif
