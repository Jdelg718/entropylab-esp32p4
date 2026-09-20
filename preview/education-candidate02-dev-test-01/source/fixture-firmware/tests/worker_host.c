#include "compute.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static void empty(const hex_result_t *r){assert(r->rc);for(size_t i=0;i<sizeof r->mnemonic;i++)assert(!r->mnemonic[i]);for(size_t i=0;i<sizeof r->fingerprint;i++)assert(!r->fingerprint[i]);for(size_t i=0;i<sizeof r->address;i++)assert(!r->address[i]);}
int main(int argc, char **argv){
 FILE *f=fopen(argc == 2 ? argv[1] : "coin/vectors/raw-binary.tsv","r");assert(f);char line[1024];unsigned cases=0;
 while(fgets(line,sizeof line,f)){
  char *id=strtok(line,"\t"),*w=strtok(NULL,"\t"),*bits=strtok(NULL,"\t"),*hex=strtok(NULL,"\t"),*mn=strtok(NULL,"\n");assert(id&&w&&bits&&hex&&mn);
  hex_request_t q={.mode=1,.words=(uint32_t)atoi(w),.length=strlen(bits)};strcpy(q.hex,bits);
  hex_result_t r;memset(&r,0xa5,sizeof r);el_compute(&q,&r);assert(!r.rc&&!strcmp(r.mnemonic,mn));
  char encoded[65]={0};assert(!el_coin_to_hex((const uint8_t*)q.hex,q.length,q.words,(uint8_t*)encoded,65));assert(!strcmp(encoded,hex));
  for(int delta=-1;delta<=1;delta+=2){hex_request_t bad=q;bad.length+=delta;el_compute(&bad,&r);empty(&r);}
  q.hex[0]='H';el_compute(&q,&r);empty(&r);q.hex[0]=bits[0];el_compute(&q,&r);assert(!r.rc&&!strcmp(r.mnemonic,mn));cases++;
 }
 fclose(f);assert(cases==22);
 hex_request_t q={.mode=2,.length=128,.words=12};hex_result_t r;el_compute(&q,&r);empty(&r);
 q.mode=1;q.length=257;el_compute(&q,&r);empty(&r);
 q.length=128;q.words=13;memset(q.hex,'0',128);el_compute(&q,&r);empty(&r);
 q.words=12;el_compute(&q,&r);assert(!r.rc&&!strcmp(r.fingerprint,"73c5da0a"));
 puts("PASS worker with actual archived cores: 22 upstream cases, exact hex+mnemonic, all lengths under/over, alphabet failure, stale-output clearing, retry, invalid mode/selector, max length, public BIP84 fingerprint");
}
