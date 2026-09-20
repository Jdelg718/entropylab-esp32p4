#include "compute.h"
#include <assert.h>
#include <stdio.h>
int main(void){FILE *f=fopen("vectors/bip39-english.tsv","r");assert(f);char line[2048];unsigned count=0;
 while(fgets(line,sizeof line,f)){
  char *h=strtok(line,"\t"),*mn=strtok(NULL,"\t\n");assert(h&&mn);
  hex_request_t q={.mode=MODE_HEX,.length=strlen(h)};q.words=q.length*3/8;strcpy(q.hex,h);
  hex_result_t r;el_compute(&q,&r);assert(!r.rc&&!strcmp(r.mnemonic,mn)&&strlen(r.fingerprint)==8&&strlen(r.address)==42&&!r.entropy[0]);count++;
 }fclose(f);assert(count==24);puts("PASS actual HEX worker: 24 published entropy/mnemonic pairs");return 0;
}
