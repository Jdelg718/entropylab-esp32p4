#include "compute.h"
#include <assert.h>
#include <stdio.h>
static void empty(const hex_result_t*r){assert(r->rc<0);const char *p[]={r->mnemonic,r->entropy,r->fingerprint,r->address};size_t n[]={216,65,9,43};for(unsigned i=0;i<4;i++)for(size_t j=0;j<n[i];j++)assert(!p[i][j]);assert(!r->weak);}
int main(void){
 _Static_assert(sizeof(((hex_request_t*)0)->hex)==1025,"owned1025");
 _Static_assert(sizeof(((hex_result_t*)0)->entropy)==65,"entropy65");
 hex_request_t q={.mode=MODE_MNEMONIC,.words=12,.request_id=77,.revision=88};
 strcpy(q.hex,"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about");q.length=strlen(q.hex);
 hex_request_t owned=q;memset(q.hex,'X',q.length);hex_result_t r;
 el_compute(&owned,&r);assert(!r.rc&&!strcmp(r.entropy,"00000000000000000000000000000000")&&!strcmp(r.fingerprint,"73c5da0a"));assert(r.request_id==77&&r.revision==88);
 q=owned;q.hex[0]='A';el_compute(&q,&r);assert(r.rc==-4);empty(&r);
 q=owned;q.hex[0]='z';el_compute(&q,&r);assert(r.rc==-100);empty(&r);
 q=owned;q.hex[7]=0;el_compute(&q,&r);assert(r.rc==-4);empty(&r);
 q=owned;q.words=15;el_compute(&q,&r);assert(r.rc==-5);empty(&r);
 q=owned;q.length=0;el_compute(&q,&r);empty(&r);
 q.words=24;for(unsigned i=0;i<24;i++){memcpy(q.hex+9*i,"abstract",8);if(i<23)q.hex[9*i+8]=' ';}q.hex[215]=0;q.length=215;
 el_compute(&q,&r);assert(r.rc==-6);empty(&r);
 q.length=216;el_compute(&q,&r);empty(&r);q.length=1025;el_compute(&q,&r);empty(&r);
 q=owned;q.mode=5;el_compute(&q,&r);empty(&r);q.mode=UINT32_MAX;el_compute(&q,&r);empty(&r);
 el_compute(&owned,&r);assert(!r.rc&&!r.mnemonic[0]&&!r.weak);
 puts("PASS mnemonic owned snapshot, identities, public BIP84, strict/unknown/count/max215/over216/1025, invalid modes, stale result clear and retry");
}
