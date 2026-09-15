#pragma once
#include "gui.h"
#include "coin_core.h"
#include "dice_core.h"
#include "mnemonic_core.h"
#include <string.h>
/* Single worker, disjoint owned buffers, no LVGL, storage or logging. */
static inline void el_compute(const hex_request_t *q,hex_result_t *r){
 char converted[65]={0};
 memset(r,0,sizeof *r);r->rc=-1;
 r->mode=q->mode;r->words=q->words;r->request_id=q->request_id;r->revision=q->revision;
 if(q->length>1024)return;
 const uint8_t *src=(const uint8_t*)q->hex;size_t n=q->length;
 switch(q->mode){
 case MODE_MNEMONIC: {
  if(n==0 || n>215)return;
  unsigned count=1;
  for(size_t i=0;i<n;i++){
   if(src[i]==' '){if(i==0 || i+1==n || src[i-1]==' '){r->rc=-4;return;}count++;}
   else if(src[i]<'a'||src[i]>'z'){r->rc=-4;return;}
  }
  if(!(q->words==12||q->words==15||q->words==18||q->words==21||q->words==24)||count!=q->words){r->rc=-5;return;}
  r->rc=el_mnemonic_run(src,n,(uint8_t*)r->entropy,sizeof r->entropy,(uint8_t*)r->fingerprint,sizeof r->fingerprint,(uint8_t*)r->address,sizeof r->address);
  break;
 }
 case MODE_D6_RAW: case MODE_D6_COLEMAN:
  r->rc=el_dice_to_hex(src,n,q->mode==MODE_D6_RAW?1:2,q->words,(uint8_t*)converted,sizeof converted);
  if(r->rc<0)break;
  r->weak=r->rc==1;src=(const uint8_t*)converted;n=strlen(converted);
  r->rc=el_hex_run(src,n,(uint8_t*)r->mnemonic,sizeof r->mnemonic,(uint8_t*)r->fingerprint,sizeof r->fingerprint,(uint8_t*)r->address,sizeof r->address);
  break;
 case MODE_COINS:
  r->rc=el_coin_to_hex(src,n,q->words,(uint8_t*)converted,sizeof converted);
  if(r->rc)break;
  src=(const uint8_t*)converted;n/=4;
  r->rc=el_hex_run(src,n,(uint8_t*)r->mnemonic,sizeof r->mnemonic,(uint8_t*)r->fingerprint,sizeof r->fingerprint,(uint8_t*)r->address,sizeof r->address);
  break;
 case MODE_HEX:
  if(n>64 || q->words!=n*3/8)return;
  r->rc=el_hex_run(src,n,(uint8_t*)r->mnemonic,sizeof r->mnemonic,(uint8_t*)r->fingerprint,sizeof r->fingerprint,(uint8_t*)r->address,sizeof r->address);
  break;
 default:return;
 }
 if(r->rc<0){memset(r->mnemonic,0,sizeof r->mnemonic);memset(r->entropy,0,sizeof r->entropy);memset(r->fingerprint,0,sizeof r->fingerprint);memset(r->address,0,sizeof r->address);r->weak=false;}
 if(!r->rc && r->weak)r->rc=1;
 memset(converted,0,sizeof converted); /* Best effort, NOT secure erasure. */
}
