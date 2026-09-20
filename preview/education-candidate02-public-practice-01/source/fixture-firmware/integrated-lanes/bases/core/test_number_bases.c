#include "number_bases.h"
#include <assert.h>
#include <string.h>

int main(void) {
  static const unsigned bases[] = {4,8,32,64}, words[] = {12,15,18,21,24};
  uint8_t in[32], out[32]; char text[140], guarded[142]; size_t n, used;
  for (n=0;n<32;n++) in[n]=(uint8_t)n;
  for (size_t w=0;w<5;w++) for(size_t b=0;b<4;b++) {
    size_t bytes=nb_entropy_bytes(words[w]), chars=nb_encoded_length(bases[b],words[w]);
    assert(nb_encode(in,bytes,bases[b],words[w],text,sizeof text,&used)==NB_OK && used==chars);
    memset(out,0xaa,sizeof out);
    assert(nb_decode(text,used,bases[b],words[w],out,bytes,&n)==NB_OK && n==bytes && !memcmp(in,out,bytes));
    memset(guarded,0x5a,sizeof guarded);
    assert(nb_encode(in,bytes,bases[b],words[w],guarded+1,chars,&used)==NB_CAPACITY);
    assert((unsigned char)guarded[0]==0x5a && (unsigned char)guarded[141]==0x5a);
  }
  memset(out,0xa5,sizeof out); assert(nb_decode("",0,8,12,out,16,&n)==NB_EMPTY && out[0]==0xa5);
  assert(nb_encode(NULL,16,4,12,text,sizeof text,&used)==NB_NULL);
  assert(nb_decode(NULL,1,4,12,out,16,&n)==NB_NULL);
  assert(nb_decode("0000000000000000000000000000000000000000004",43,8,12,out,16,&n)==NB_FINAL_DIGIT);
  return 0;
}
