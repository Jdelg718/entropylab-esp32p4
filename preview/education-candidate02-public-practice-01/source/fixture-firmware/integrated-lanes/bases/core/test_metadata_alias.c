#include "number_bases.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
 union {size_t align;unsigned char bytes[160];} in,out;
 unsigned char before_in[160],before_out[160];unsigned mode,where;
 for(mode=0;mode<2;mode++)for(where=0;where<4;where++){
 size_t *used;nb_status status;
 memset(in.bytes,'0',sizeof in.bytes);memset(out.bytes,'Z',sizeof out.bytes);
 memcpy(before_in,in.bytes,160);memcpy(before_out,out.bytes,160);
 used=(size_t *)(void *)((where<2?in.bytes:out.bytes)+(where%2)*sizeof(size_t));
 status=mode?nb_decode((char *)in.bytes,64,4,12,out.bytes,160,used):nb_encode(in.bytes,16,4,12,(char *)out.bytes,160,used);
 if(status!=NB_LENGTH||memcmp(before_in,in.bytes,160)||memcmp(before_out,out.bytes,160)){
 printf("metadata alias failure mode=%u where=%u status=%d\n",mode,where,status);return 1;}
 }
 puts("8 aligned metadata input/output alias rejection cases PASS; all bytes unchanged");return 0;
}
