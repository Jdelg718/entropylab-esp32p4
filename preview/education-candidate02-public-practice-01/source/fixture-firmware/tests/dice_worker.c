#include "compute.h"
#include <assert.h>
#include <stdio.h>
#include "dice_vectors.h"
static void empty(const hex_result_t*r){assert(r->rc<0);for(size_t i=0;i<sizeof r->mnemonic;i++)assert(!r->mnemonic[i]);for(size_t i=0;i<sizeof r->fingerprint;i++)assert(!r->fingerprint[i]);for(size_t i=0;i<sizeof r->address;i++)assert(!r->address[i]);}
int main(void){unsigned accepted=0,rejected=0;
 for(size_t i=0;i<sizeof vectors/sizeof *vectors;i++){const struct vector*v=vectors+i;hex_request_t q={.mode=v->mode+1,.words=v->words,.length=strlen(v->rolls)};memcpy(q.hex,v->rolls,q.length);struct{unsigned pre;hex_result_t r;unsigned post;}g={.pre=123,.post=456};memset(&g.r,0xa5,sizeof g.r);el_compute(&q,&g.r);assert(g.pre==123&&g.post==456);assert(g.r.rc==v->rc);uint8_t h[65];memset(h,0xa5,65);assert(el_dice_to_hex((uint8_t*)q.hex,q.length,v->mode,q.words,h,65)==v->rc);if(v->rc<0){empty(&g.r);for(int j=0;j<65;j++)assert(h[j]==0xa5);rejected++;}else{assert(!strcmp(g.r.mnemonic,v->mn)&&!strcmp((char*)h,v->hex));assert(g.r.weak==(v->rc==1)&&g.r.mode==q.mode&&g.r.words==q.words);assert(strlen(g.r.fingerprint)==8&&strlen(g.r.address)==42);accepted++;}}
 assert(accepted==14&&rejected==6);
 unsigned thresholds[]={50,62,75,87,100};
 for(unsigned mode=2;mode<=3;mode++)for(unsigned i=0;i<5;i++)for(int delta=-1;delta<=0;delta++){hex_request_t q={.mode=mode,.words=12+3*i,.length=thresholds[i]+delta};memset(q.hex,'6',q.length);hex_result_t r;el_compute(&q,&r);assert(r.rc==(delta<0)&&r.weak==(delta<0)&&r.mnemonic[0]);}
 hex_request_t q={.mode=2,.words=24,.length=1024};memset(q.hex,'6',1024);hex_result_t r;char previous[216]={0};
 for(unsigned mode=2;mode<=3;mode++){q.mode=mode;el_compute(&q,&r);assert(r.rc==0&&!r.weak);uint8_t h[65];assert(!el_dice_to_hex((uint8_t*)q.hex,1024,mode-1,24,h,65));assert(!strcmp((char*)h,fullhex[mode-2]));assert(strcmp(previous,r.mnemonic));strcpy(previous,r.mnemonic);}
 const char bad[]={'0','7','H',' ', '\n',0,(char)0xff};for(unsigned i=0;i<sizeof bad;i++){q.hex[500]=bad[i];el_compute(&q,&r);empty(&r);uint8_t h[65];memset(h,0xa5,65);assert(el_dice_to_hex((uint8_t*)q.hex,1024,1,24,h,65)<0);for(int j=0;j<65;j++)assert(h[j]==0xa5);}q.hex[500]='6';
 q.length=1025;el_compute(&q,&r);empty(&r);q.length=0;el_compute(&q,&r);empty(&r);q.length=1024;q.words=13;el_compute(&q,&r);empty(&r);q.words=24;q.mode=4;el_compute(&q,&r);empty(&r);
 uint8_t over[1025],h[65];memset(over,'6',sizeof over);memset(h,0xa5,65);assert(el_dice_to_hex(over,1025,1,24,h,65)<0);for(int j=0;j<65;j++)assert(h[j]==0xa5);
 puts("PASS actual composed dice+hex worker: upstream20 native14accept6reject, both6 modes, full1024 independent SHA256 oracle, all nominal threshold boundaries incl49/50+99/100, weak metadata, sentinels, negative no-output writes, invalid/empty/overmax, cleared failure outputs");
}
