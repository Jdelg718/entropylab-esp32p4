#pragma once
#include "gui.h"
#include "coin_core.h"
#include <string.h>
/* Single worker only, no LVGL calls. Snapshot remains stable throughout FFI. */
static inline void el_compute(const hex_request_t *q,hex_result_t *r){
 char converted[65]={0};
 memset(r,0,sizeof *r);r->rc=-1;
 if(q->length>256 || q->mode>1)return;
 const uint8_t *src=(const uint8_t*)q->hex;size_t n=q->length;
 if(q->mode==1){
  r->rc=el_coin_to_hex(src,n,q->words,(uint8_t*)converted,sizeof converted);
  if(r->rc){memset(converted,0,sizeof converted);return;}
  src=(const uint8_t*)converted;n/=4;
 }else if(n>64 || q->words!=n*3/8)return;
 r->rc=el_hex_run(src,n,(uint8_t*)r->mnemonic,sizeof r->mnemonic,(uint8_t*)r->fingerprint,sizeof r->fingerprint,(uint8_t*)r->address,sizeof r->address);
 if(r->rc){int32_t rc=r->rc;memset(r,0,sizeof *r);r->rc=rc;}
 memset(converted,0,sizeof converted); /* Best effort, NOT secure erasure. */
}
