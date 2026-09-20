#include "compute.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
/* HOST PUBLIC FIXTURE HARNESS ONLY: stdin/stdout is not production logging. */
int main(void){char line[2048];
 while(fgets(line,sizeof line,stdin)){
  size_t n=strlen(line);if(n&&line[n-1]=='\n')line[--n]=0;
  hex_request_t q={.mode=MODE_MNEMONIC,.request_id=7,.revision=11};
  assert(n<=1024);memcpy(q.hex,line,n);q.length=n;q.words=1;
  for(size_t i=0;i<n;i++)if(line[i]==' ')q.words++;
  struct {unsigned pre;hex_result_t result;unsigned post;} guard={.pre=0x1234,.post=0x5678};
  memset(&guard.result,0xa5,sizeof guard.result);
  el_compute(&q,&guard.result);hex_result_t *r=&guard.result;
  assert(guard.pre==0x1234&&guard.post==0x5678);
  assert(r->request_id==7&&r->revision==11&&r->mode==MODE_MNEMONIC&&!r->weak);
  for(size_t i=0;i<sizeof r->mnemonic;i++)assert(r->mnemonic[i]==0);
  if(r->rc){
   for(size_t i=0;i<sizeof r->entropy;i++)assert(r->entropy[i]==0);
   for(size_t i=0;i<sizeof r->fingerprint;i++)assert(r->fingerprint[i]==0);
   for(size_t i=0;i<sizeof r->address;i++)assert(r->address[i]==0);
   printf("%d\t1\n",r->rc);
  }else printf("0\t%s\t%s\t%s\n",r->entropy,r->fingerprint,r->address);
 }
 return ferror(stdin)?2:0;
}
