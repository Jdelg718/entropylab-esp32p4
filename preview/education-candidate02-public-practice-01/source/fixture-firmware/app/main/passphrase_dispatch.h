#pragma once
#include "gui.h"
#include <string.h>
/* Platform-neutral, NOT thread safe. All calls and clock access require one
 * caller-supplied transition lock. No callbacks, LVGL, queues or crypto here.
 * Clock is application-lifetime zero-initialized stationary storage, NEVER
 * reset/reused while any old identifier may exist (including disposed routes).
 * All dispatchers/routes in a routing domain MUST use that same clock on every
 * open/submit. Changing clock domains or manually resetting counters is invalid.
 * Caller clears its input snapshot after submit on BOTH outcomes, and completion
 * copy after delivery/drop. Dispatcher has no hidden request staging allocation.
 * Dispatcher starts zeroed; never reset/copy/move a live dispatcher. UI owner
 * addresses are not identities. open/detach must precede UI disposal/reinit.
 * Inputs/outputs must be live, stable, fully disjoint from dispatcher and each
 * other. Structures are private to this API; caller must not mutate fields.
 * Submit copies the complete approved compute envelope plus every UI metadata
 * field by value; adapter maps UI raw/context into these neutral types.
 * Claim grants the ONE worker exclusive read ownership of stationary request.
 * Release transition lock before compute; do not hold GUI/display lock. No other
 * API reads/writes request while RUNNING. Worker stops ALL reads before finish,
 * reacquires transition lock, and never uses claim pointer afterwards.
 * ID queue adapter publishes token under the same transition lock; on send
 * failure cancel that token before unlocking/returning false. Queue carries ONLY
 * token; stale IDs cannot claim a later job. No dummy/overwrite queue scrubbing.
 * GUI takes completion by value under transition lock, then unlocks BEFORE UI
 * calls; GUI serialization must keep route valid through delivery. No callbacks
 * under transition lock; no waiting for worker under GUI lock.
 * Best-effort full-byte volatile wiping, NOT forensic/library/stack erasure.
 */
typedef enum { EL_DISPATCH_FREE=0, EL_DISPATCH_QUEUED, EL_DISPATCH_RUNNING, EL_DISPATCH_DONE } el_dispatch_state_t;
typedef struct { uint64_t epoch,token; } el_dispatch_clock_t;
typedef struct {
 uint64_t epoch,request_id,generation,revision;
 uint32_t mode,words,derivation_context,marker;
} el_dispatch_meta_t;
typedef struct { el_dispatch_meta_t meta; int32_t rc; char fingerprint[9],address[43]; } el_dispatch_result_t;
typedef struct {
 el_passphrase_request_t request;
 el_dispatch_meta_t meta;
 el_dispatch_result_t result;
 uint64_t token,route;
 el_dispatch_state_t state;
 bool cancelled;
} el_dispatch_t;
static inline void el_dispatch_clear(void *p,size_t n){volatile uint8_t *b=p;while(n--)*b++=0;}
static inline bool el_dispatch_open(el_dispatch_t *d,el_dispatch_clock_t *c){
 if(!d || !c || d->route || c->epoch==UINT64_MAX)return false;
 d->route=++c->epoch;return true;
}
static inline bool el_dispatch_submit(el_dispatch_t *d,el_dispatch_clock_t *c,const el_passphrase_request_t *q,const el_dispatch_meta_t *m,uint64_t *token){
 if(!d || !c || !q || !m || !token || d->state!=EL_DISPATCH_FREE || !d->route || m->epoch!=d->route || c->token==UINT64_MAX)return false;
 if(q->source.length>1024 || q->passphrase_len>256 || m->mode>=MODE_COUNT || !(m->words==12 || m->words==15 || m->words==18 || m->words==21 || m->words==24) || !m->request_id || !m->generation || m->marker!=(q->passphrase_len?1u:0u) || m->request_id!=q->source.request_id || m->revision!=q->source.revision || m->mode!=q->source.mode || m->words!=q->source.words)return false;
 memcpy(&d->request,q,sizeof *q);d->meta=*m;d->token=++c->token;
 d->state=EL_DISPATCH_QUEUED;*token=d->token;return true;
}
/* Internal release: NEVER call while worker still reads request. */
static inline void el_dispatch_release(el_dispatch_t *d){
 uint64_t route=d->route;
 el_dispatch_clear(d,sizeof *d);d->route=route;
}
static inline bool el_dispatch_equal(const el_dispatch_meta_t *a,const el_dispatch_meta_t *b){
 return a->epoch==b->epoch && a->request_id==b->request_id && a->generation==b->generation && a->revision==b->revision && a->mode==b->mode && a->words==b->words && a->derivation_context==b->derivation_context && a->marker==b->marker;
}
/* Shape validation only, not a Bech32 checksum/derivation oracle. Errors never
 * carry payload bytes. Successful fingerprint/address have exact fixed sizes. */
static inline bool el_dispatch_result_valid(const el_dispatch_result_t *r){
 if(r->rc<0){
  for(size_t i=0;i<sizeof r->fingerprint;i++)if(r->fingerprint[i])return false;
  for(size_t i=0;i<sizeof r->address;i++)if(r->address[i])return false;
  return true;
 }
 if(r->rc==1 && !el_mode_is_dice(r->meta.mode))return false;
 if(r->rc>1 || r->fingerprint[8] || r->address[42] || memcmp(r->address,"bc1q",4))return false;
 for(size_t i=0;i<8;i++)if(!r->fingerprint[i] || !strchr("0123456789abcdef",r->fingerprint[i]))return false;
 for(size_t i=4;i<42;i++)if(!r->address[i] || !strchr("qpzry9x8gf2tvdw0s3jn54khce6mua7l",r->address[i]))return false;
 return true;
}
/* Worker hands back only bounded public result, then clears its own result
 * staging after this call (also on false). Wrong token cannot release live job. */
static inline bool el_dispatch_finish(el_dispatch_t *d,uint64_t token,const el_dispatch_result_t *r){
 if(!d || !token || d->token!=token || d->state!=EL_DISPATCH_RUNNING)return false;
 if(d->cancelled || !d->route || d->route!=d->meta.epoch || !r || !el_dispatch_equal(&d->meta,&r->meta) || !el_dispatch_result_valid(r)){
  el_dispatch_release(d);return false;
 }
 el_dispatch_clear(&d->request,sizeof d->request);
 el_dispatch_clear(&d->result,sizeof d->result);
 d->result.meta=d->meta;d->result.rc=r->rc;
 memcpy(d->result.fingerprint,r->fingerprint,sizeof r->fingerprint);
 memcpy(d->result.address,r->address,sizeof r->address);
 d->state=EL_DISPATCH_DONE;return true;
}
static inline bool el_dispatch_take(el_dispatch_t *d,uint64_t token,const el_dispatch_meta_t *m,el_dispatch_result_t *r){
 if(!r)return false;
 el_dispatch_clear(r,sizeof *r);
 if(!d || !token || token!=d->token || d->state!=EL_DISPATCH_DONE)return false;
 bool valid=m && d->route && d->route==d->meta.epoch && el_dispatch_equal(m,&d->meta);
 if(valid)memcpy(r,&d->result,sizeof *r);
 el_dispatch_release(d);return valid;
}
static inline bool el_dispatch_cancel(el_dispatch_t *d,uint64_t token){
 if(!d || !token || token!=d->token || d->state==EL_DISPATCH_FREE)return false;
 if(d->state==EL_DISPATCH_RUNNING)d->cancelled=true;
 else el_dispatch_release(d);
 return true;
}
static inline void el_dispatch_detach(el_dispatch_t *d){
 if(!d)return;
 d->route=0;
 if(d->state!=EL_DISPATCH_FREE)(void)el_dispatch_cancel(d,d->token);
}
static inline const el_passphrase_request_t *el_dispatch_claim(el_dispatch_t *d,uint64_t token){
 if(!d || !token || token!=d->token || d->state!=EL_DISPATCH_QUEUED)return NULL;
 d->state=EL_DISPATCH_RUNNING;return &d->request;
}
