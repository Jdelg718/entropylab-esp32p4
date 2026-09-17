#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <initializer_list>
#include <dlfcn.h>
#include <cstdlib>

extern "C" {
#include "bases_target.h"
#include "cards_target.h"
#include "number_bases.h"
#include "passphrase_core.h"
}
#include "all_features_application.h"

static uint64_t queued;
static unsigned locks, converts, kdf_calls, results, cancelled_count;
static bool active_passphrase, cancel_in_convert;
static uint8_t passphrase_source[256];
static size_t passphrase_source_len;
static gui08_request published_request;
static pc_result_v1 published_conversion;
static all_features_derivation_result published_derivation;

extern "C" int32_t __real_el_bip39_passphrase_run(
    const uint8_t*,size_t,const uint8_t*,size_t,uint8_t*,size_t,
    uint8_t*,size_t,uint8_t*,size_t);
extern "C" int32_t __wrap_el_bip39_passphrase_run(
    const uint8_t *m,size_t mn,const uint8_t *p,size_t pn,uint8_t *e,size_t en,
    uint8_t *f,size_t fn,uint8_t *a,size_t an) {
    ++kdf_calls;
    return __real_el_bip39_passphrase_run(m,mn,p,pn,e,en,f,fn,a,an);
}

static void lock_app(){assert(locks++==0);}
static void unlock_app(){assert(locks--==1);}
static bool publish(uint64_t token){
    /* Authoritative serialization erratum: publisher callback is outside the
     * transition lock. This assertion preserves, rather than removes, it. */
    assert(locks==0&&!queued);queued=token;return true;
}
static gui08_passphrase_marker snapshot(const gui08_request*,uint8_t out[256],size_t *n){
    std::memset(out,0,256);*n=active_passphrase?passphrase_source_len:0;
    if(*n)std::memcpy(out,passphrase_source,*n);
    return *n?GUI08_PASSPHRASE_ACTIVE:GUI08_PASSPHRASE_EMPTY;
}
static bool pending(const gui08_request*){return true;}
static bool result(const gui08_request *request,const pc_result_v1 *conversion,
                   const all_features_derivation_result *derivation){
    published_request=*request;published_conversion=*conversion;
    published_derivation=*derivation;++results;return true;
}
static bool cancelled(uint64_t){++cancelled_count;return true;}
static int convert(const gui08_request *request,pc_result_v1 *value){
    ++converts;
    int rc=request->method==GUI08_CARDS?
        (int)cards_target_convert(PC_V1_DIRECT,request->words,(uint32_t)request->request_id,
                                  (const uint8_t*)request->transcript,request->length,value):
        (int)bases_target_convert(request->base,request->words,(uint32_t)request->request_id,
                                  (const uint8_t*)request->transcript,request->length,value);
    if(cancel_in_convert)assert(all_features_application_cancel(request->request_id));
    return rc;
}
static int derive(const gui08_request *request,const pc_result_v1 *conversion,
                  all_features_derivation_result *out){
    return (int)el_bip39_passphrase_run(
        conversion->mnemonic,conversion->mnemonic_len,
        request->passphrase,request->passphrase_len,
        (uint8_t*)out->entropy,sizeof out->entropy,
        (uint8_t*)out->fingerprint,sizeof out->fingerprint,
        (uint8_t*)out->address,sizeof out->address);
}
static gui08_request request_for(uint64_t id,gui08_method method,const char *text,size_t length){
    gui08_request r{};r.request_id=id;r.revision=id;r.method=method;r.words=12;
    r.base=method==GUI08_BASES?32:4;r.length=length;
    std::memcpy(r.transcript,text,length);r.transcript[length]=0;return r;
}
static void run_queued(){assert(queued);const uint64_t token=queued;queued=0;
    all_features_application_work(token,convert);all_features_application_poll();}
static void expect_vector(gui08_method method,bool active,uint64_t id,const char *text,size_t length,
                          const char *fingerprint,const char *address){
    active_passphrase=active;
    if(active){std::memcpy(passphrase_source,"TREZOR",6);passphrase_source_len=6;}
    gui08_request request=request_for(id,method,text,length);
    const unsigned before_kdf=kdf_calls,before_results=results;
    assert(all_features_application_submit(&request));
    std::memset(request.transcript,'x',request.length);
    if(active)std::memset(passphrase_source,'x',passphrase_source_len);
    run_queued();
    assert(kdf_calls==before_kdf+1&&results==before_results+1);
    assert(published_request.passphrase_marker==(active?GUI08_PASSPHRASE_ACTIVE:GUI08_PASSPHRASE_EMPTY));
    assert(published_request.passphrase_len==(active?6u:0u));
    if(active)assert(!std::memcmp(published_request.passphrase,"TREZOR",6));
    assert(!std::strcmp(published_derivation.fingerprint,fingerprint));
    assert(!std::strcmp(published_derivation.address,address));
    assert(published_conversion.mnemonic_len==93);
}

using oracle_fn=uint32_t(*)(uint32_t,uint32_t,uint32_t,uint32_t,const uint8_t*,uint32_t,pc_result_v1*,uint32_t);
static void unicode_oracle(){
    const char *oracle_path=std::getenv("PLAYING_CARDS_ORACLE_LIBRARY");
    assert(oracle_path && "set PLAYING_CARDS_ORACLE_LIBRARY to independently built oracle");
    void *lib=dlopen(oracle_path,RTLD_NOW|RTLD_LOCAL);
    assert(lib);oracle_fn oracle=(oracle_fn)dlsym(lib,"pc_convert_v1");assert(oracle);
    const char *cases[]={
        "A♠ 2♥ 3♦ 4♣","9♠ T♥ J♦ Q♣ K♠",
        "2♠ 3♥ 4♦ 5♣ 6♠ 7♥ 8♦"
    };
    for(uint32_t mode: {PC_V1_HASH,PC_V1_COLEMAN})for(const char *text:cases){
        const size_t n=std::strlen(text);pc_result_v1 target{},expected{};
        assert(cards_target_convert(mode,12,77,(const uint8_t*)text,n,&target)==PC_V1_OK);
        assert(oracle(1,mode,12,77,(const uint8_t*)text,(uint32_t)n,&expected,sizeof expected)==PC_V1_OK);
        assert(!std::memcmp(&target,&expected,sizeof target));
    }
    dlclose(lib);
    std::puts("PASS Unicode suits oracle parity paired ranks=A/9/T/J/Q/K and 2-8 modes=hash/Coleman");
}

int main(){
    unicode_oracle();
    assert(all_features_application_configure(lock_app,unlock_app,publish,snapshot,derive,pending,result,cancelled));
    uint8_t zero[16]{};char base[129];size_t base_len=0;
    assert(nb_encode(zero,sizeof zero,32,12,base,sizeof base,&base_len)==NB_OK);
    char cards[48];std::memset(cards,'A',47);cards[47]=0;
    const char *empty_fp="73c5da0a",*empty_addr="bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu";
    const char *active_fp="b4e3f5ed",*active_addr="bc1qv5rmq0kt9yz3pm36wvzct7p3x6mtgehjul0feu";
    expect_vector(GUI08_BASES,false,100,base,base_len,empty_fp,empty_addr);
    expect_vector(GUI08_BASES,true,101,base,base_len,active_fp,active_addr);
    expect_vector(GUI08_CARDS,false,102,cards,47,empty_fp,empty_addr);
    expect_vector(GUI08_CARDS,true,103,cards,47,active_fp,active_addr);

    unsigned before=kdf_calls;
    gui08_request bad=request_for(104,GUI08_BASES,"0",1);
    active_passphrase=false;assert(all_features_application_submit(&bad));run_queued();
    assert(kdf_calls==before); /* parser failure */

    gui08_request queued_cancel=request_for(105,GUI08_BASES,base,base_len);
    assert(all_features_application_submit(&queued_cancel));uint64_t stale_token=queued;
    assert(all_features_application_cancel(queued_cancel.request_id));queued=0;
    all_features_application_work(stale_token,convert);all_features_application_poll();
    assert(kdf_calls==before); /* queued cancel and stale worker token */

    gui08_request running_cancel=request_for(106,GUI08_BASES,base,base_len);
    assert(all_features_application_submit(&running_cancel));cancel_in_convert=true;run_queued();cancel_in_convert=false;
    assert(kdf_calls==before); /* running cancel after parser, before KDF */
    all_features_application_dispose();
    std::printf("PASS actual runtime derivation Cards/Bases empty+active vectors=4 KDF_success=%u parser_cancel_stale_KDF=0 copied-passphrase-owned\n",kdf_calls);
}
