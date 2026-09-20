#pragma once
#include "gui.h"
#include "coin_core.h"
#include "dice_core.h"
#include "mnemonic_core.h"
#include <string.h>
#include "passphrase_core.h"
/* Best effort owned-buffer cleanup, not forensic erasure. */
static inline void el_compute_clear(void *p,size_t n){volatile uint8_t *b=p;while(n--)*b++=0;}
/* All objects live and disjoint; source/passphrase stable during this call.
 * Destination is stationary writable storage, never an alias of either input. */
static inline bool el_passphrase_snapshot(el_passphrase_request_t *out,const hex_request_t *src,const uint8_t *pass,size_t n){
 if(!out)return false;
 el_compute_clear(out,sizeof *out);
 if(!src || src->length>1024 || n>256 || (!pass && n))return false;
 memcpy(out->source.hex,src->hex,src->length);
 out->source.length=src->length;out->source.mode=src->mode;out->source.words=src->words;
 out->source.request_id=src->request_id;out->source.revision=src->revision;
 if(n)memcpy(out->passphrase,pass,n);
 out->passphrase_len=n;return true;
}
/* Caller owns q for the full call and clears it upon release. r is live,
 * writable and disjoint from q. No borrowed pointers escape; no LVGL/KDF
 * until successful conversion. Fixed capacities cannot be caller reduced. */
static inline void el_compute_passphrase(const el_passphrase_request_t *q,hex_result_t *r){
 char mnemonic[216]={0};hex_result_t staged={0};
 if(!r)return;
 el_compute_clear(r,sizeof *r);r->rc=-1;
 if(!q)return;
 r->mode=q->source.mode;r->words=q->source.words;
 r->request_id=q->source.request_id;r->revision=q->source.revision;
 if(q->source.length>1024 || q->passphrase_len>256)return;
 int32_t converted=el_input_to_mnemonic((const uint8_t*)q->source.hex,q->source.length,q->source.mode,q->source.words,(uint8_t*)mnemonic,sizeof mnemonic);
 if(converted<0){r->rc=converted;goto done;}
 r->rc=el_bip39_passphrase_run((const uint8_t*)mnemonic,strlen(mnemonic),q->passphrase,q->passphrase_len,(uint8_t*)staged.entropy,sizeof staged.entropy,(uint8_t*)staged.fingerprint,sizeof staged.fingerprint,(uint8_t*)staged.address,sizeof staged.address);
 if(r->rc<0)goto done;
 memcpy(r->entropy,staged.entropy,sizeof r->entropy);
 memcpy(r->mnemonic,mnemonic,sizeof r->mnemonic);
 memcpy(r->fingerprint,staged.fingerprint,sizeof r->fingerprint);
 memcpy(r->address,staged.address,sizeof r->address);
 r->weak=converted==1;r->rc=r->weak?1:0;
 done:el_compute_clear(mnemonic,sizeof mnemonic);el_compute_clear(&staged,sizeof staged);
}

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
