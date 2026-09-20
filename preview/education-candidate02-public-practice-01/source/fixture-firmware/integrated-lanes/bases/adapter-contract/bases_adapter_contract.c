#include "bases_adapter_contract.h"
#include "number_bases.h"
#include <string.h>
void el_bases_contract_clear(void *p,size_t n){volatile uint8_t *b=p;while(n--)*b++=0;}
void el_bases_contract_execute(const el_bases_request_v1 *q,el_bases_result_v1 *r){
 size_t written=0; nb_status status;
 memset(r,0,sizeof *r);
 r->version=EL_BASES_CONTRACT_VERSION;
 r->envelope_status=EL_BASES_BAD_ENVELOPE;
 r->core_status=EL_BASES_CORE_NOT_RUN;
 if(!q)return;
 r->request_id=q->request_id;r->revision=q->revision;
 r->operation=q->operation;r->base=q->base;r->words=q->words;
 if(q->version!=EL_BASES_CONTRACT_VERSION || q->input_len>EL_BASES_INPUT_MAX ||
    (q->operation!=EL_BASES_ENCODE && q->operation!=EL_BASES_DECODE))return;
 r->envelope_status=EL_BASES_ACCEPTED;
 if(q->operation==EL_BASES_ENCODE)
  status=nb_encode(q->input,q->input_len,q->base,q->words,(char*)r->output,sizeof r->output,&written);
 else
  status=nb_decode((const char*)q->input,q->input_len,q->base,q->words,r->output,EL_BASES_ENTROPY_MAX,&written);
 r->core_status=(int32_t)status;
 r->written=(uint32_t)written;
}
