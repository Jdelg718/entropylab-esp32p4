#include "cards_target.h"

#include <math.h>
#include <stdbool.h>
#include <string.h>

#include "../../../app/main/bip39_dictionary.inc"

typedef struct {
    uint32_t h[8];
    uint64_t bits;
    uint8_t block[64];
    size_t used;
} sha256_state;

static uint32_t rotr(uint32_t x, unsigned n) { return (x >> n) | (x << (32u - n)); }
static uint32_t load_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}
static void store_be32(uint8_t *p, uint32_t x) {
    p[0] = (uint8_t)(x >> 24); p[1] = (uint8_t)(x >> 16);
    p[2] = (uint8_t)(x >> 8); p[3] = (uint8_t)x;
}
static void sha256_compress(sha256_state *s, const uint8_t block[64]) {
    static const uint32_t k[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    };
    uint32_t w[64];
    for (unsigned i = 0; i < 16; ++i) w[i] = load_be32(block + i * 4u);
    for (unsigned i = 16; i < 64; ++i) {
        uint32_t a = w[i-15], b = w[i-2];
        uint32_t s0 = rotr(a,7)^rotr(a,18)^(a>>3);
        uint32_t s1 = rotr(b,17)^rotr(b,19)^(b>>10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }
    uint32_t a=s->h[0],b=s->h[1],c=s->h[2],d=s->h[3],e=s->h[4],f=s->h[5],g=s->h[6],h=s->h[7];
    for (unsigned i = 0; i < 64; ++i) {
        uint32_t s1=rotr(e,6)^rotr(e,11)^rotr(e,25), ch=(e&f)^(~e&g);
        uint32_t t1=h+s1+ch+k[i]+w[i];
        uint32_t s0=rotr(a,2)^rotr(a,13)^rotr(a,22), maj=(a&b)^(a&c)^(b&c);
        uint32_t t2=s0+maj;
        h=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
    }
    s->h[0]+=a;s->h[1]+=b;s->h[2]+=c;s->h[3]+=d;
    s->h[4]+=e;s->h[5]+=f;s->h[6]+=g;s->h[7]+=h;
}
static void sha256_init(sha256_state *s) {
    static const uint32_t initial[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    memset(s,0,sizeof *s); memcpy(s->h,initial,sizeof initial);
}
static void sha256_update(sha256_state *s, const uint8_t *data, size_t length) {
    s->bits += (uint64_t)length * 8u;
    while (length) {
        size_t room=64u-s->used, take=length<room?length:room;
        memcpy(s->block+s->used,data,take);s->used+=take;data+=take;length-=take;
        if (s->used==64u) { sha256_compress(s,s->block);s->used=0; }
    }
}
static void sha256_final(sha256_state *s, uint8_t out[32]) {
    uint64_t bits=s->bits;s->block[s->used++]=0x80;
    if (s->used>56u) { memset(s->block+s->used,0,64u-s->used);sha256_compress(s,s->block);s->used=0; }
    memset(s->block+s->used,0,56u-s->used);
    for (unsigned i=0;i<8;++i) s->block[63u-i]=(uint8_t)(bits>>(i*8u));
    sha256_compress(s,s->block);for(unsigned i=0;i<8;++i)store_be32(out+i*4u,s->h[i]);
    memset(s,0,sizeof *s);
}
static void sha256(const uint8_t *data,size_t length,uint8_t out[32]) {
    sha256_state s;sha256_init(&s);sha256_update(&s,data,length);sha256_final(&s,out);
}

static size_t entropy_size(uint32_t words) {
    return (words==12||words==15||words==18||words==21||words==24)?words/3u*4u:0u;
}
static bool separator(uint8_t c) {
    return (c>=9&&c<=13)||c==' '||c==','||c=='.'||c==';'||c==':'||c=='_'||c=='|'||c=='/'||c=='-';
}
static uint8_t upper_ascii(uint8_t c) { return c>='a'&&c<='z'?(uint8_t)(c-'a'+'A'):c; }
static bool normalize_card(const uint8_t *p,size_t n,uint8_t card[2]) {
    if (n==3&&p[0]=='1'&&p[1]=='0') { card[0]='T';card[1]=upper_ascii(p[2]); }
    else if(n==2) { card[0]=upper_ascii(p[0]);card[1]=upper_ascii(p[1]); }
    else if(n==4&&p[1]==0xe2&&p[2]==0x99) {
        card[0]=upper_ascii(p[0]); card[1]=p[3]==0xa0?'S':p[3]==0xa5?'H':p[3]==0xa6?'D':p[3]==0xa3?'C':0;
    } else return false;
    return strchr("A23456789TJQK",card[0])&&strchr("CDHS",card[1]);
}
static bool duplicate(uint8_t cards[104][2],size_t begin,size_t end,const uint8_t card[2]) {
    for(size_t i=begin;i<end;++i)if(cards[i][0]==card[0]&&cards[i][1]==card[1])return true;
    return false;
}
static uint32_t hashed(const uint8_t *raw,size_t length,uint32_t words,bool coleman,
                       uint8_t entropy[32],double *source_bits,uint32_t *flags) {
    uint8_t cards[104][2];size_t count=0,start=0,first=words==12?25:words==15?31:words==18?39:words==21?50:52;
    bool invalid=false,dup=false;
    for(size_t i=0;i<=length;++i) {
        bool sep=i==length||(i<length&&separator(raw[i]));
        if(!sep)continue;
        if(i>start) {
            uint8_t card[2];
            if(!normalize_card(raw+start,i-start,card)) invalid=true;
            else { size_t pool=count<first?0:first; if(duplicate(cards,pool,count,card))dup=true;else if(count<104){cards[count][0]=card[0];cards[count][1]=card[1];++count;}else invalid=true; }
        }
        start=i+1;
    }
    if(invalid)return PC_V1_INVALID;
    if(dup)return PC_V1_DUPLICATE;
    if(!count)return PC_V1_EMPTY;
    uint8_t transcript[520];size_t used=0;
    for(size_t i=0;i<count;++i) {
        if(i)transcript[used++]=' ';
        transcript[used++]=cards[i][0];
        if(!coleman) transcript[used++]=(uint8_t)(cards[i][1]-'A'+'a');
        else { transcript[used++]=0xe2;transcript[used++]=0x99;transcript[used++]=cards[i][1]=='S'?0xa0:cards[i][1]=='H'?0xa5:cards[i][1]=='D'?0xa6:0xa3; }
    }
    uint8_t digest[32];sha256(transcript,used,digest);memcpy(entropy,digest,entropy_size(words));
    double bits=0.0;size_t a=count<first?count:first,b=count>first?count-first:0;
    for(size_t i=0;i<a&&i<52;++i)bits+=log2((double)(52u-i));
    for(size_t i=0;i<b&&i<52;++i)bits+=log2((double)(52u-i));
    *source_bits=bits;*flags=1u|((count<(first+(words==24?6u:0u)))?PC_V1_UNDER_RECOMMENDED:0u);
    return PC_V1_OK;
}
static uint32_t direct(const uint8_t *raw,size_t length,uint32_t words,uint8_t entropy[32],double *source_bits,uint32_t *flags) {
    size_t need=(words-1u)*4u+(words==12?3u:words==24?1u:2u),position=0,bit=0;
    memset(entropy,0,32);
    for(size_t i=0;i<length;++i) {
        if(separator(raw[i]))continue;
        uint8_t c=upper_ascii(raw[i]);
        unsigned value=c=='A'?0u:(c>='2'&&c<='8'?(unsigned)(c-'1'):8u);
        if(position>=need)return PC_V1_EXTRA;
        size_t partial=(words-1u)*4u;unsigned radix;
        if(position<partial)radix=position%4u==3u?4u:8u;
        else { size_t final=position-partial;radix=words==12?(unsigned[]){8,8,2}[final]:words==18?(unsigned[]){8,4}[final]:words==21?(unsigned[]){8,2}[final]:8u; }
        if(value>=radix)return PC_V1_INVALID;
        unsigned width=radix==8?3u:radix==4?2u:1u;
        for(unsigned shift=width;shift--;) { entropy[bit/8u]|=(uint8_t)(((value>>shift)&1u)<<(7u-bit%8u));++bit; }
        ++position;
    }
    if(position<need)return PC_V1_INCOMPLETE;
    *source_bits=(double)(entropy_size(words)*8u);*flags=1u;return PC_V1_OK;
}
static uint32_t mnemonic(const uint8_t entropy[32],size_t bytes,uint8_t out[256],uint32_t *out_len) {
    uint8_t check[32];sha256(entropy,bytes,check);size_t total=bytes*8u+bytes/4u,used=0;
    for(size_t pos=0;pos<total;pos+=11u) {
        unsigned index=0;
        for(unsigned j=0;j<11u;++j) { size_t bit=pos+j;unsigned value=bit<bytes*8u?((entropy[bit/8u]>>(7u-bit%8u))&1u):((check[0]>>(7u-(bit-bytes*8u)))&1u);index=(index<<1)|value; }
        const char *word=mn_dictionary[index];size_t n=strlen(word);
        if(used+(used?1u:0u)+n>256u)return PC_V1_INTERNAL_CAPACITY_ERROR;
        if(used)out[used++]=' ';
        memcpy(out+used,word,n);used+=n;
    }
    *out_len=(uint32_t)used;return PC_V1_OK;
}
uint32_t cards_target_result_from_entropy(uint32_t words,uint32_t context,
                                          const uint8_t *entropy,size_t entropy_len,
                                          const char *method,size_t method_len,
                                          double source_bits,uint32_t flags,
                                          pc_result_v1 *output) {
    const size_t bytes=entropy_size(words);
    if(!bytes)return PC_V1_WORD_COUNT_ERROR;
    if(!output||!entropy||!method)return PC_V1_NULL_ERROR;
    if(entropy_len!=bytes)return PC_V1_SIZE_ERROR;
    if(!method_len||method_len>sizeof output->method)return PC_V1_INTERNAL_CAPACITY_ERROR;
    pc_result_v1 result;memset(&result,0,sizeof result);
    memcpy(result.entropy,entropy,bytes);
    uint32_t status=mnemonic(result.entropy,bytes,result.mnemonic,&result.mnemonic_len);
    if(status!=PC_V1_OK)return status;
    result.version=PC_V1_VERSION;result.mode=PC_V1_DIRECT;result.words=words;
    result.context=context;result.entropy_len=(uint32_t)bytes;
    result.method_len=(uint32_t)method_len;result.flags=flags;
    result.source_bits=source_bits;memcpy(result.method,method,method_len);
    *output=result;return PC_V1_OK;
}
uint32_t cards_target_convert(uint32_t mode,uint32_t words,uint32_t context,const uint8_t *raw,size_t length,pc_result_v1 *output) {
    size_t bytes=entropy_size(words);if(!bytes)return PC_V1_WORD_COUNT_ERROR;
    if(mode>PC_V1_DIRECT)return PC_V1_MODE_ERROR;
    if(length>4096u)return PC_V1_INPUT_TOO_LONG;
    if(!output||(length&&!raw))return PC_V1_NULL_ERROR;
    for(size_t i=0;i<length;++i)if(raw[i]==0)return PC_V1_NUL_ERROR;
    uint8_t entropy[32];double source_bits=0.0;uint32_t flags=0;
    uint32_t status=mode==PC_V1_DIRECT?direct(raw,length,words,entropy,&source_bits,&flags):hashed(raw,length,words,mode==PC_V1_COLEMAN,entropy,&source_bits,&flags);
    if(status!=PC_V1_OK)return status;
    const char *method=mode==PC_V1_HASH?"cards-sha256":mode==PC_V1_COLEMAN?"ian-coleman-cards-sha256":"cards-direct";
    status=cards_target_result_from_entropy(words,context,entropy,bytes,method,strlen(method),source_bits,flags,output);
    if(status==PC_V1_OK)output->mode=mode;
    memset(entropy,0,sizeof entropy);return status;
}
