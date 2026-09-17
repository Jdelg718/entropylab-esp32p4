#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

extern "C" {
#include "bases_target.h"
#include "cards_target.h"
#include "number_bases.h"
}
#include "all_features_application.h"

static uint64_t queued;
static unsigned locks, converts, results, errors;
static bool cancel_inside;
static gui08_request converted_request, published_request;
static pc_result_v1 published_result;

static void lock_app(){assert(locks++==0);}
static void unlock_app(){assert(locks--==1);}
static bool publish(uint64_t token){
    /* CONTRACT-ERRATUM.md: only the nonblocking token publication is outside
     * the transition lock; dispatcher reservation/rollback stays locked. */
    assert(locks==0&&!queued);queued=token;return true;
}
static gui08_passphrase_marker marker(const gui08_request*,uint8_t out[256],size_t *n){std::memset(out,0,256);*n=0;return GUI08_PASSPHRASE_EMPTY;}
static int derive(const gui08_request*,const pc_result_v1*,all_features_derivation_result *out){std::memset(out,0,sizeof *out);std::strcpy(out->entropy,"00");std::strcpy(out->fingerprint,"00000000");std::strcpy(out->address,"bc1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq");return 0;}
static bool pending(const gui08_request*){return true;}
static bool result(const gui08_request *request,const pc_result_v1 *value,const all_features_derivation_result*){published_request=*request;published_result=*value;++results;return true;}
static bool cancelled(uint64_t){++errors;return true;}
static int convert(const gui08_request *request,pc_result_v1 *value){
    converted_request=*request;++converts;
    if(cancel_inside)assert(all_features_application_cancel(request->request_id));
    if(request->method==GUI08_CARDS)
        return (int)cards_target_convert(PC_V1_DIRECT,request->words,(uint32_t)request->request_id,
                                        (const uint8_t*)request->transcript,request->length,value);
#ifdef RED_UNSUPPORTED
    (void)value;return PC_V1_MODE_ERROR;
#else
    if(request->method==GUI08_BASES)
        return (int)bases_target_convert(request->base,request->words,(uint32_t)request->request_id,
                                        (const uint8_t*)request->transcript,request->length,value);
    return PC_V1_MODE_ERROR;
#endif
}
static void work_poll(){assert(queued);uint64_t token=queued;queued=0;all_features_application_work(token,convert);all_features_application_poll();}
static gui08_request request_for(uint64_t id,gui08_method method,unsigned words,unsigned base,const char *text,size_t length){
    gui08_request r{};r.request_id=id;r.revision=id;r.method=method;r.words=words;r.base=base;r.length=length;
    assert(length<sizeof r.transcript);std::memcpy(r.transcript,text,length);r.transcript[length]=0;return r;
}
static void expect_shape(const pc_result_v1 &value,unsigned words,const char *method){
    assert(value.version==PC_V1_VERSION&&value.mode==PC_V1_DIRECT&&value.words==words);
    assert(value.entropy_len==words/3u*4u&&value.mnemonic_len>0&&value.mnemonic_len<sizeof value.mnemonic);
    assert(value.method_len==std::strlen(method)&&!std::memcmp(value.method,method,value.method_len));
    unsigned count=1;for(unsigned i=0;i<value.mnemonic_len;++i)count+=value.mnemonic[i]==' ';assert(count==words);
}
static void converter_vectors(){
    static const unsigned counts[]={12,15,18,21,24},bases[]={4,8,32,64};
    static const char *methods[]={"base4","base8","base32","base64"};
    uint8_t entropy[32];for(unsigned i=0;i<sizeof entropy;++i)entropy[i]=(uint8_t)i;
    for(unsigned words:counts)for(unsigned bi=0;bi<4;++bi){
        char text[129];size_t written=0;const size_t bytes=nb_entropy_bytes(words);
        assert(nb_encode(entropy,bytes,bases[bi],words,text,sizeof text,&written)==NB_OK);
        pc_result_v1 out{};assert(bases_target_convert(bases[bi],words,0x1234,(const uint8_t*)text,written,&out)==PC_V1_OK);
        expect_shape(out,words,methods[bi]);assert(out.context==0x1234&&out.source_bits==(double)(bytes*8u));
        assert(!std::memcmp(out.entropy,entropy,bytes));
    }
    char zero_text[129];size_t zero_len=0;uint8_t zero_entropy[16]{};
    assert(nb_encode(zero_entropy,16,32,12,zero_text,sizeof zero_text,&zero_len)==NB_OK);
    pc_result_v1 zero{};assert(bases_target_convert(32,12,7,(const uint8_t*)zero_text,zero_len,&zero)==PC_V1_OK);
    static const char mnemonic[]="abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about";
    assert(zero.mnemonic_len==sizeof mnemonic-1&&!std::memcmp(zero.mnemonic,mnemonic,sizeof mnemonic-1));

    pc_result_v1 sentinel;std::memset(&sentinel,0xa5,sizeof sentinel);const pc_result_v1 before=sentinel;
    assert(bases_target_convert(3,12,0,(const uint8_t*)"0",1,&sentinel)==PC_V1_MODE_ERROR);
    assert(bases_target_convert(4,13,0,(const uint8_t*)"0",1,&sentinel)==PC_V1_WORD_COUNT_ERROR);
    assert(bases_target_convert(4,12,0,nullptr,1,&sentinel)==PC_V1_NULL_ERROR);
    assert(bases_target_convert(4,12,0,(const uint8_t*)"",0,&sentinel)==PC_V1_EMPTY);
    assert(bases_target_convert(4,12,0,(const uint8_t*)"0",1,&sentinel)==PC_V1_SIZE_ERROR);
    char over[1025]{};assert(bases_target_convert(4,12,0,(const uint8_t*)over,sizeof over,&sentinel)==PC_V1_INPUT_TOO_LONG);
    char invalid[65];std::memset(invalid,'0',64);invalid[1]='x';invalid[64]=0;
    assert(bases_target_convert(4,12,0,(const uint8_t*)invalid,64,&sentinel)==PC_V1_INVALID);
    char bad_final[44];std::memset(bad_final,'0',43);bad_final[42]='4';bad_final[43]=0;
    assert(bases_target_convert(8,12,0,(const uint8_t*)bad_final,43,&sentinel)==PC_V1_RANGE_ERROR);
    assert(!std::memcmp(&sentinel,&before,sizeof sentinel));
    std::puts("PASS bases converter public-vectors=21 bases=4/8/32/64 word-counts=5 strict-bounds invalid transactional canonical-metadata");
}
int main(){
    converter_vectors();
    assert(all_features_application_configure(lock_app,unlock_app,publish,marker,derive,pending,result,cancelled));
    char text[129];size_t length=0;uint8_t zero[16]{};assert(nb_encode(zero,16,32,12,text,sizeof text,&length)==NB_OK);
    gui08_request request=request_for(100,GUI08_BASES,12,32,text,length),original=request;
    assert(all_features_application_submit(&request));std::memset(request.transcript,'x',request.length);work_poll();
    assert(converts==1&&results==1&&errors==0); /* RED_UNSUPPORTED intentionally aborts here. */
    assert(!std::memcmp(converted_request.transcript,original.transcript,original.length+1));
    assert(published_request.method==GUI08_BASES&&published_request.base==32);expect_shape(published_result,12,"base32");

    request=request_for(101,GUI08_BASES,12,32,text,length);assert(all_features_application_submit(&request));
    assert(all_features_application_cancel(request.request_id));work_poll();assert(converts==1&&results==1);
    request=request_for(102,GUI08_BASES,12,32,text,length);assert(all_features_application_submit(&request));
    cancel_inside=true;work_poll();cancel_inside=false;assert(converts==2&&results==1&&errors==0);

    char cards[48];std::memset(cards,'A',47);cards[47]=0;
    request=request_for(103,GUI08_CARDS,12,4,cards,47);assert(all_features_application_submit(&request));work_poll();
    assert(converts==3&&results==2);expect_shape(published_result,12,"cards-direct");
    all_features_application_dispose();
    std::puts("PASS application Bases copied-ownership queued-running-cancellation stale-suppression Cards-preserved");
}
