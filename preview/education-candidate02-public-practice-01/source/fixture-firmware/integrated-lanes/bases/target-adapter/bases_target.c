#include "bases_target.h"

#include <string.h>

#include "../core/number_bases.h"
#include "../../playingcards/target-adapter/cards_target.h"

static uint32_t map_status(nb_status status) {
    switch(status) {
        case NB_OK: return PC_V1_OK;
        case NB_NULL: return PC_V1_NULL_ERROR;
        case NB_UNSUPPORTED: return PC_V1_MODE_ERROR;
        case NB_LENGTH: return PC_V1_SIZE_ERROR;
        case NB_CAPACITY: return PC_V1_INTERNAL_CAPACITY_ERROR;
        case NB_EMPTY: return PC_V1_EMPTY;
        case NB_INVALID_CHARACTER: return PC_V1_INVALID;
        case NB_FINAL_DIGIT: return PC_V1_RANGE_ERROR;
    }
    return PC_V1_PANIC_ERROR;
}

uint32_t bases_target_convert(uint32_t base,uint32_t words,uint32_t context,
                              const uint8_t *raw,size_t length,
                              pc_result_v1 *output) {
    const size_t bytes=nb_entropy_bytes(words);
    const size_t encoded=nb_encoded_length(base,words);
    if(!bytes)return PC_V1_WORD_COUNT_ERROR;
    if(base!=4u&&base!=8u&&base!=32u&&base!=64u)return PC_V1_MODE_ERROR;
    if(length>1024u)return PC_V1_INPUT_TOO_LONG;
    if(!output||(length&&!raw))return PC_V1_NULL_ERROR;
    if(length!=encoded)return length?PC_V1_SIZE_ERROR:PC_V1_EMPTY;
    for(size_t i=0;i<length;++i)if(raw[i]==0)return PC_V1_NUL_ERROR;

    uint8_t entropy[32];size_t written=0;
    const nb_status decoded=nb_decode((const char*)raw,length,base,words,
                                      entropy,sizeof entropy,&written);
    if(decoded!=NB_OK){memset(entropy,0,sizeof entropy);return map_status(decoded);}
    const char *method=base==4u?"base4":base==8u?"base8":base==32u?"base32":"base64";
    const uint32_t status=cards_target_result_from_entropy(
        words,context,entropy,written,method,strlen(method),(double)(bytes*8u),1u,output);
    memset(entropy,0,sizeof entropy);
    return status;
}