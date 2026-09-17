#ifndef NUMBER_BASES_H
#define NUMBER_BASES_H
#include <stddef.h>
#include <stdint.h>
typedef enum { NB_OK=0, NB_NULL, NB_UNSUPPORTED, NB_LENGTH, NB_CAPACITY, NB_EMPTY, NB_INVALID_CHARACTER, NB_FINAL_DIGIT } nb_status;
size_t nb_entropy_bytes(unsigned words);
size_t nb_encoded_length(unsigned base, unsigned words);
/* Buffers must designate valid objects of the declared sizes. written must be
 * aligned and disjoint from input/output spans. Metadata overlap returns
 * NB_LENGTH before any write (including written); other errors zero written
 * when non-NULL. Input/output overlap is rejected; output is unchanged on error.
 */
nb_status nb_encode(const uint8_t *input,size_t input_len,unsigned base,unsigned words,char *output,size_t capacity,size_t *written);
nb_status nb_decode(const char *input,size_t input_len,unsigned base,unsigned words,uint8_t *output,size_t capacity,size_t *written);
#endif
