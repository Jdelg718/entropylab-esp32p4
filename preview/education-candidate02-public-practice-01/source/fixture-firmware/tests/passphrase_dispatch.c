#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "passphrase_dispatch.h"
int main(void) {
 el_dispatch_t d={0}; el_dispatch_clock_t clock={0};
 el_passphrase_request_t q={0}; el_dispatch_meta_t m={0}; uint64_t t=0;
 q.source.mode=0;q.source.words=12;q.source.request_id=1;q.source.revision=2;
 q.source.length=32;memset(q.source.hex,'0',32);q.passphrase_len=6;memcpy(q.passphrase,"TREZOR",6);
 m.request_id=1;m.generation=1;m.revision=2;m.words=12;m.marker=1;
 assert(el_dispatch_open(&d,&clock));m.epoch=d.route;
 assert(el_dispatch_submit(&d,&clock,&q,&m,&t));assert(t && d.state==EL_DISPATCH_QUEUED);
 memset(&q,0,sizeof q);
 const el_passphrase_request_t *owned=el_dispatch_claim(&d,t);
 assert(owned && owned->passphrase_len==6 && !memcmp(owned->passphrase,"TREZOR",6));
 assert(d.state==EL_DISPATCH_RUNNING);assert(!el_dispatch_claim(&d,t));
 el_dispatch_result_t r={0},out; r.meta=m;r.rc=-7;
 assert(el_dispatch_finish(&d,t,&r));assert(d.state==EL_DISPATCH_DONE);
 for(size_t i=0;i<sizeof d.request;i++)assert(((unsigned char*)&d.request)[i]==0);
 assert(el_dispatch_take(&d,t,&m,&out));assert(out.rc==-7);assert(d.state==EL_DISPATCH_FREE);
 assert(!el_dispatch_take(&d,t,&m,&out));
 q.source.mode=0;q.source.words=12;q.source.request_id=1;q.source.revision=2;q.source.length=32;
 memset(q.source.hex,'0',32);q.passphrase_len=6;memcpy(q.passphrase,"TREZOR",6);
 for(unsigned phase=0;phase<3;phase++){
  uint64_t old=t; assert(el_dispatch_submit(&d,&clock,&q,&m,&t));assert(t>old);
  el_dispatch_t before=d;uint64_t untouched=99;
  assert(!el_dispatch_submit(&d,&clock,&q,&m,&untouched));assert(untouched==99);assert(!memcmp(&before,&d,sizeof d));
  assert(!el_dispatch_claim(&d,old));
  if(phase)assert(el_dispatch_claim(&d,t));
  if(phase==2)assert(el_dispatch_finish(&d,t,&r));
  assert(el_dispatch_cancel(&d,t));
  if(phase==1){assert(d.state==EL_DISPATCH_RUNNING);assert(!memcmp(d.request.passphrase,"TREZOR",6));assert(!el_dispatch_finish(&d,t,&r));}
  assert(d.state==EL_DISPATCH_FREE);
  for(size_t i=0;i<sizeof d.request;i++)assert(((unsigned char*)&d.request)[i]==0);
  for(size_t i=0;i<sizeof d.result;i++)assert(((unsigned char*)&d.result)[i]==0);
 }
 /* Every route identity dimension rejects a matching-token wrong completion. */
 for(unsigned field=0;field<8;field++){
  assert(el_dispatch_submit(&d,&clock,&q,&m,&t));assert(el_dispatch_claim(&d,t));r.meta=m;
  switch(field){case 0:r.meta.epoch++;break;case 1:r.meta.request_id++;break;case 2:r.meta.generation++;break;case 3:r.meta.revision++;break;case 4:r.meta.mode++;break;case 5:r.meta.words++;break;case 6:r.meta.derivation_context++;break;default:r.meta.marker++;}
  assert(!el_dispatch_finish(&d,t,&r));assert(d.state==EL_DISPATCH_FREE);
 }
 r.meta=m;r.rc=0;memset(r.fingerprint,'a',8);r.fingerprint[8]=0;
 memcpy(r.address,"bc1q",4);memset(r.address+4,'q',38);r.address[42]=0;
 assert(el_dispatch_submit(&d,&clock,&q,&m,&t));assert(el_dispatch_claim(&d,t));
 assert(!el_dispatch_finish(&d,t-1,&r));assert(d.state==EL_DISPATCH_RUNNING);
 assert(el_dispatch_finish(&d,t,&r));assert(!el_dispatch_finish(&d,t,&r));assert(el_dispatch_take(&d,t,&m,&out));
 assert(!strcmp(out.fingerprint,"aaaaaaaa"));
 for(unsigned bad=0;bad<4;bad++){
  el_dispatch_result_t malformed=r;
  if(bad==0)malformed.fingerprint[8]='a';
  if(bad==1)malformed.address[42]='q';
  if(bad==2)malformed.fingerprint[0]='z';
  if(bad==3)malformed.address[0]='x';
  assert(el_dispatch_submit(&d,&clock,&q,&m,&t));assert(el_dispatch_claim(&d,t));
  assert(!el_dispatch_finish(&d,t,&malformed));assert(d.state==EL_DISPATCH_FREE);
 }
 assert(el_dispatch_submit(&d,&clock,&q,&m,&t));assert(el_dispatch_claim(&d,t));
 el_dispatch_detach(&d);assert(!d.route && d.state==EL_DISPATCH_RUNNING);
 assert(el_dispatch_open(&d,&clock));assert(d.route!=m.epoch);
 assert(!el_dispatch_finish(&d,t,&r));assert(d.state==EL_DISPATCH_FREE);
 m.epoch=d.route;r.meta=m;
 assert(el_dispatch_submit(&d,&clock,&q,&m,&t));assert(el_dispatch_claim(&d,t));assert(el_dispatch_finish(&d,t,&r));
 el_dispatch_meta_t wrong=m;wrong.generation++;
 memset(&out,0xa5,sizeof out);assert(!el_dispatch_take(&d,t,&wrong,&out));
 for(size_t i=0;i<sizeof out;i++)assert(((unsigned char*)&out)[i]==0);
 assert(d.state==EL_DISPATCH_FREE);
 q.passphrase_len=257;assert(!el_dispatch_submit(&d,&clock,&q,&m,&t));q.passphrase_len=6;
 q.source.length=SIZE_MAX;assert(!el_dispatch_submit(&d,&clock,&q,&m,&t));q.source.length=32;
 /* Reject weak-dice status on a non-dice route. */
 r.rc=1;
 assert(el_dispatch_submit(&d,&clock,&q,&m,&t));assert(el_dispatch_claim(&d,t));
 assert(!el_dispatch_finish(&d,t,&r));assert(d.state==EL_DISPATCH_FREE);r.rc=0;
 /* Invalid submissions do not mutate any slot bytes. */
 for(unsigned bad=0;bad<7;bad++){
  el_dispatch_meta_t invalid=m;el_dispatch_t before=d;
  switch(bad){case 0:invalid.epoch++;break;case 1:invalid.request_id++;break;case 2:invalid.generation=0;break;case 3:invalid.revision++;break;case 4:invalid.mode=99;break;case 5:invalid.words=13;break;default:invalid.marker=0;}
  assert(!el_dispatch_submit(&d,&clock,&q,&invalid,&t));assert(!memcmp(&d,&before,sizeof d));
 }
 for(unsigned phase=0;phase<2;phase++){
  assert(el_dispatch_submit(&d,&clock,&q,&m,&t));
  if(phase){assert(el_dispatch_claim(&d,t));assert(el_dispatch_finish(&d,t,&r));}
  uint64_t old=t;el_dispatch_detach(&d);el_dispatch_detach(&d);
  assert(d.state==EL_DISPATCH_FREE && !d.route);
  for(size_t i=0;i<sizeof d;i++)assert(((unsigned char*)&d)[i]==0);
  assert(el_dispatch_open(&d,&clock));m.epoch=d.route;r.meta=m;
  assert(el_dispatch_submit(&d,&clock,&q,&m,&t));
  assert(!el_dispatch_cancel(&d,old));assert(!el_dispatch_claim(&d,old));
  assert(el_dispatch_cancel(&d,t));
 }
 /* Fresh zeroed storage cannot revive old IDs when using same lifetime clock. */
 el_dispatch_t fresh={0};assert(el_dispatch_open(&fresh,&clock));assert(fresh.route!=d.route);
 assert(!el_dispatch_claim(&fresh,t));el_dispatch_detach(&fresh);
 clock.token=UINT64_MAX-1;
 assert(el_dispatch_submit(&d,&clock,&q,&m,&t));assert(t==UINT64_MAX);assert(el_dispatch_cancel(&d,t));
 assert(!el_dispatch_submit(&d,&clock,&q,&m,&t));
 el_dispatch_detach(&d);clock.epoch=UINT64_MAX;assert(!el_dispatch_open(&d,&clock));
 assert(!el_dispatch_cancel(&d,t));assert(!el_dispatch_claim(&d,0));
 puts("dispatcher transitions, identity, cancellation, bounds and wipes PASS");return 0;
}
