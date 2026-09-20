#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

extern "C" {
#include "cards_target.h"
}
#include "all_features_application.h"

static uint64_t queued;
static unsigned locks;
static unsigned converts;
static unsigned results;
static unsigned errors;
static bool cancel_inside;
static gui08_request published_request;
static pc_result_v1 published_result;

static void lock_app(){assert(locks++==0);}
static void unlock_app(){assert(locks--==1);}
static bool publish(uint64_t token){
    /* CONTRACT-ERRATUM.md: token-only queue publication is outside the
     * transition lock; submit reacquires it for commit or exact rollback. */
    assert(locks==0&&!queued);queued=token;return true;
}
static gui08_passphrase_marker marker(const gui08_request*,uint8_t out[256],size_t *n){std::memset(out,0,256);*n=0;return GUI08_PASSPHRASE_EMPTY;}
static int derive(const gui08_request*,const pc_result_v1*,all_features_derivation_result *out){std::memset(out,0,sizeof *out);std::strcpy(out->entropy,"00");std::strcpy(out->fingerprint,"00000000");std::strcpy(out->address,"bc1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq");return 0;}
static bool pending(const gui08_request*){return true;}
static bool result(const gui08_request *request,const pc_result_v1 *value,const all_features_derivation_result*){published_request=*request;published_result=*value;++results;return true;}
static bool cancelled(uint64_t){++errors;return true;}
static int convert(const gui08_request *request,pc_result_v1 *value){
    ++converts;
    if(cancel_inside)assert(all_features_application_cancel(request->request_id));
#ifdef RED_UNSUPPORTED
    (void)value;
    return PC_V1_MODE_ERROR;
#else
    if(request->method!=GUI08_CARDS)return PC_V1_MODE_ERROR;
    return (int)cards_target_convert(PC_V1_DIRECT,request->words,(uint32_t)request->request_id,
                                    (const uint8_t*)request->transcript,request->length,value);
#endif
}
static gui08_request direct_request(uint64_t id,unsigned words){
    gui08_request r{};r.request_id=id;r.revision=id;r.method=GUI08_CARDS;r.words=words;r.base=4;
    r.length=(words-1u)*4u+(words==12?3u:words==24?1u:2u);
    std::memset(r.transcript,'A',r.length);r.transcript[r.length]=0;return r;
}
static void work_poll(){assert(queued);uint64_t token=queued;queued=0;all_features_application_work(token,convert);all_features_application_poll();}
static void expect_bytes(const uint8_t *actual,const uint8_t *expected,size_t n){assert(std::memcmp(actual,expected,n)==0);}
static void expect_mnemonic_shape(const pc_result_v1 &value){
    assert(value.mnemonic_len<sizeof value.mnemonic);
    assert(std::strlen((const char*)value.mnemonic)==value.mnemonic_len);
    unsigned words=1;for(unsigned i=0;i<value.mnemonic_len;++i)words+=value.mnemonic[i]==' ';
    assert(words==value.words);
}

static void converter_vectors(){
    static const unsigned counts[]={12u,15u,18u,21u,24u};
    for(unsigned words: counts){
        gui08_request r=direct_request(words,words);pc_result_v1 out{};
        assert(cards_target_convert(PC_V1_DIRECT,words,7,(const uint8_t*)r.transcript,r.length,&out)==PC_V1_OK);
        assert(out.entropy_len==words/3u*4u&&out.mnemonic_len>0&&out.mnemonic_len<=256);
        expect_mnemonic_shape(out);
        for(unsigned i=0;i<out.entropy_len;++i)assert(out.entropy[i]==0);
    }
    pc_result_v1 zero{};gui08_request r=direct_request(12,12);
    assert(cards_target_convert(PC_V1_DIRECT,12,9,(const uint8_t*)r.transcript,r.length,&zero)==PC_V1_OK);
    static const char mnemonic[]="abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about";
    assert(zero.mnemonic_len==sizeof(mnemonic)-1);expect_bytes(zero.mnemonic,(const uint8_t*)mnemonic,sizeof(mnemonic)-1);

    static const uint8_t hash_expected[16]={0xe3,0x20,0x55,0x61,0xf5,0x1a,0x3e,0xe5,0x5c,0xa1,0x23,0xdb,0xfb,0xb7,0xea,0x11};
    static const uint8_t coleman_expected[16]={0x65,0x2d,0xc6,0xcf,0x0c,0xa8,0x51,0xf5,0x6d,0xe0,0x32,0x00,0x84,0xd5,0x91,0x4e};
    static const uint8_t cards[]="AS 2S 3S 4S";pc_result_v1 hash{},coleman{};
    assert(cards_target_convert(PC_V1_HASH,12,10,cards,sizeof(cards)-1,&hash)==PC_V1_OK);
    assert(cards_target_convert(PC_V1_COLEMAN,12,11,cards,sizeof(cards)-1,&coleman)==PC_V1_OK);
    expect_bytes(hash.entropy,hash_expected,16);expect_bytes(coleman.entropy,coleman_expected,16);
    assert(hash.entropy_len==16&&coleman.entropy_len==16&&hash.mnemonic_len>0&&coleman.mnemonic_len>0);
    expect_mnemonic_shape(hash);expect_mnemonic_shape(coleman);
    assert(hash.method_len==12&&coleman.method_len==24);

    pc_result_v1 sentinel{};std::memset(&sentinel,0xa5,sizeof sentinel);pc_result_v1 before=sentinel;
    assert(cards_target_convert(PC_V1_HASH,12,1,(const uint8_t*)"AS AS",5,&sentinel)==PC_V1_DUPLICATE);
    assert(std::memcmp(&sentinel,&before,sizeof sentinel)==0);
    assert(cards_target_convert(PC_V1_HASH,12,1,(const uint8_t*)"AS !",4,&sentinel)==PC_V1_INVALID);
    assert(std::memcmp(&sentinel,&before,sizeof sentinel)==0);
    assert(cards_target_convert(PC_V1_HASH,12,1,nullptr,0,&sentinel)==PC_V1_EMPTY);
    assert(cards_target_convert(PC_V1_DIRECT,12,1,(const uint8_t*)"A",1,&sentinel)==PC_V1_INCOMPLETE);
    assert(cards_target_convert(PC_V1_DIRECT,12,1,(const uint8_t*)"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",48,&sentinel)==PC_V1_EXTRA);
    std::puts("PASS converter direct-counts=5 hash-coleman-public-vectors=2 output-bounds invalid-duplicate-transactional");
}

int main(){
    converter_vectors();
    assert(all_features_application_configure(lock_app,unlock_app,publish,marker,derive,pending,result,cancelled));
    gui08_request request=direct_request(100,12);gui08_request original=request;
    assert(all_features_application_submit(&request));
    std::memset(request.transcript,'8',request.length);
    work_poll();
    assert(converts==1&&results==1&&errors==0);
    assert(published_request.request_id==original.request_id&&published_request.revision==original.revision);
    assert(published_request.passphrase_marker==GUI08_PASSPHRASE_EMPTY);
    assert(published_request.length==original.length);
    assert(std::memcmp(published_request.transcript,original.transcript,original.length+1)==0);
    for(unsigned i=0;i<published_result.entropy_len;++i)assert(published_result.entropy[i]==0);

    request=direct_request(101,15);assert(all_features_application_submit(&request));
    assert(all_features_application_cancel(request.request_id));work_poll();
    assert(converts==1&&results==1);

    request=direct_request(102,18);assert(all_features_application_submit(&request));cancel_inside=true;work_poll();cancel_inside=false;
    assert(converts==2&&results==1&&errors==0);

    request=direct_request(103,12);request.method=GUI08_BASES;assert(all_features_application_submit(&request));work_poll();
    assert(converts==3&&results==1&&errors==1);
    all_features_application_dispose();
    std::puts("PASS application converter copied-ownership queued-running-cancellation stale-suppression Bases-unsupported");
}
