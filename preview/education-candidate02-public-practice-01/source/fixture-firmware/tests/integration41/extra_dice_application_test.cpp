#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "all_features_application.h"
#include "extra_dice_target.h"

static unsigned lock_depth;
static uint64_t queued;
static unsigned kdf_calls;
static unsigned result_calls;
static unsigned cancelled_calls;
static bool cancel_during_convert;
static pc_result_v1 published;
static gui08_request published_request;

static void lock_app(){assert(lock_depth++==0);} static void unlock_app(){assert(lock_depth--==1);}
static bool publish(uint64_t token){assert(lock_depth==0);if(queued)return false;queued=token;return true;}
static gui08_passphrase_marker snapshot(const gui08_request*,uint8_t out[256],size_t *n){std::memset(out,0,256);*n=0;return GUI08_PASSPHRASE_EMPTY;}
static int derive(const gui08_request*,const pc_result_v1*,all_features_derivation_result *out){assert(lock_depth==0);++kdf_calls;std::memset(out,0,sizeof *out);std::strcpy(out->entropy,"00");std::strcpy(out->fingerprint,"73c5da0a");std::strcpy(out->address,"bc1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq");return 0;}
static bool pending(const gui08_request*){return true;}
static bool result(const gui08_request *request,const pc_result_v1 *value,const all_features_derivation_result*){++result_calls;published=*value;published_request=*request;return true;}
static bool cancelled(uint64_t){++cancelled_calls;return true;}
static int convert(const gui08_request *request,pc_result_v1 *value){int rc=extra_dice_target_convert(request,(uint32_t)request->request_id,value);if(cancel_during_convert)assert(all_features_application_cancel(request->request_id));return rc;}
static std::string repeated(const char *group,unsigned count){std::string out;for(unsigned i=0;i<count;++i){if(i)out+=' ';out+=group;}return out;}
static gui08_request request(uint64_t id,unsigned method,const std::string &prefix,const char *final_roll,unsigned final_choice=0){gui08_request r{};r.request_id=id;r.revision=id;r.method=GUI08_DICE;r.words=12;r.dice_method=method;r.dice_final_choice=final_choice;r.length=prefix.size();assert(r.length<sizeof r.transcript);std::memcpy(r.transcript,prefix.c_str(),r.length+1);r.dice_final_length=std::strlen(final_roll);assert(r.dice_final_length<sizeof r.dice_final);std::memcpy(r.dice_final,final_roll,r.dice_final_length+1);return r;}
static void work_poll(){assert(queued);uint64_t token=queued;queued=0;all_features_application_work(token,convert);all_features_application_poll();}

int main(){
 assert(all_features_application_configure(lock_app,unlock_app,publish,snapshot,derive,pending,result,cancelled));
 const std::string bitbox=repeated("111111",11), dplus=repeated("100",11);
 gui08_request b=request(4101,GUI08_DICE_BITBOX,bitbox,"",0);
 assert(all_features_application_submit(&b));work_poll();
 assert(kdf_calls==1&&result_calls==1&&published_request.dice_method==GUI08_DICE_BITBOX);
 assert(published.method_len==11&&!std::memcmp(published.method,"dice-bitbox",11));
 const pc_result_v1 bitbox_result=published;
 gui08_request d=request(4102,GUI08_DICE_DPLUS,dplus,"10");
 assert(all_features_application_submit(&d));work_poll();
 assert(kdf_calls==2&&result_calls==2&&published_request.dice_method==GUI08_DICE_DPLUS);
 assert(published.method_len==10&&!std::memcmp(published.method,"dice-dplus",10));
 assert(std::memcmp(bitbox_result.method,published.method,published.method_len)!=0);
 gui08_request d16=request(4103,GUI08_DICE_DPLUS,dplus,"8F");
 assert(all_features_application_submit(&d16));work_poll();
 assert(kdf_calls==3&&result_calls==3&&published.mnemonic_len&&
        std::memcmp(bitbox_result.mnemonic,published.mnemonic,published.mnemonic_len)!=0);
 gui08_request invalid=request(4104,GUI08_DICE_DPLUS,repeated("1G0",11),"10");
 assert(all_features_application_submit(&invalid));work_poll();
 assert(kdf_calls==3&&result_calls==3&&cancelled_calls==1);
 gui08_request stale=request(4105,GUI08_DICE_BITBOX,bitbox,"",0);
 assert(all_features_application_submit(&stale));cancel_during_convert=true;work_poll();cancel_during_convert=false;
 assert(kdf_calls==3&&result_calls==3);
 gui08_request queued_cancel=request(4106,GUI08_DICE_DPLUS,dplus,"10");
 assert(all_features_application_submit(&queued_cancel));uint64_t stale_token=queued;queued=0;
 assert(all_features_application_cancel(queued_cancel.request_id));all_features_application_work(stale_token,convert);all_features_application_poll();
 assert(kdf_calls==3&&result_calls==3);
 all_features_application_dispose();
 std::puts("PASS dice41 BitBox and D++ D8/D16 public vectors, selector output, invalid input, cancel/stale suppression, one KDF per success");
}
