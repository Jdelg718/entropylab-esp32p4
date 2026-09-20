#include "number_bases.h"
#include <stdint.h>

static const char *alphabet(unsigned b){return b==4?"0123":b==8?"01234567":b==32?"qpzry9x8gf2tvdw0s3jn54khce6mua7l":b==64?"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/":0;}
static unsigned width(unsigned b){return b==4?2:b==8?3:b==32?5:b==64?6:0;}
static unsigned entropy_bits(unsigned w){return w==12?128:w==15?160:w==18?192:w==21?224:w==24?256:0;}
size_t nb_entropy_bytes(unsigned w){return entropy_bits(w)/8;}
size_t nb_encoded_length(unsigned b,unsigned w){unsigned n=entropy_bits(w),k=width(b),r;if(!n||!k)return 0;r=n%k;return n/k+(r?(b==64?r:1):0);}
static int ws(unsigned char c){return c==0x09||c==0x0a||c==0x0b||c==0x0c||c==0x0d||c==0x20;}
static int digit(unsigned b,unsigned char c){const char *a=alphabet(b);int i;if(b==32&&c>='A'&&c<='Z')c=(unsigned char)(c+32);if(!a)return -1;for(i=0;a[i];i++)if((unsigned char)a[i]==c)return i;return -1;}
static unsigned getbits(const uint8_t *p,size_t pos,unsigned n){unsigned v=0;while(n--){v=(v<<1)|((p[pos/8]>>(7-(pos%8)))&1u);pos++;}return v;}
static void putbits(uint8_t *p,size_t *pos,unsigned v,unsigned n){while(n--){if((v>>(n))&1u)p[*pos/8]|=(uint8_t)(1u<<(7-(*pos%8)));(*pos)++;}}
static int overlap(const void *a,size_t an,const void *b,size_t bn){uintptr_t x=(uintptr_t)a,y=(uintptr_t)b;return an&&bn&&x<y+bn&&y<x+an;}
nb_status nb_encode(const uint8_t *in,size_t il,unsigned b,unsigned w,char *out,size_t cap,size_t *written){unsigned bits=entropy_bits(w),k=width(b),r,full;size_t need,i,pos=0;const char *a=alphabet(b);if(written&&((in&&overlap(in,il,written,sizeof *written))||(out&&overlap(out,cap,written,sizeof *written))))return NB_LENGTH;if(written)*written=0;if(!in||!out||!written)return NB_NULL;if(!bits||!k)return NB_UNSUPPORTED;if(il!=bits/8)return NB_LENGTH;need=nb_encoded_length(b,w);if(cap<need+1)return NB_CAPACITY;if(overlap(in,il,out,cap))return NB_LENGTH;r=bits%k;full=bits/k;for(i=0;i<full;i++){out[pos++]=a[getbits(in,i*k,k)];}if(r){unsigned v=getbits(in,full*k,r);if(b==64){unsigned j;for(j=0;j<r;j++)out[pos++]=(char)('0'+((v>>(r-1-j))&1u));}else out[pos++]=a[v];}out[pos]=0;*written=pos;return NB_OK;}
nb_status nb_decode(const char *in,size_t il,unsigned b,unsigned w,uint8_t *out,size_t cap,size_t *written){unsigned bits=entropy_bits(w),k=width(b),r,full;size_t need,count=0,i,pos=0;int d;if(written&&((in&&overlap(in,il,written,sizeof *written))||(out&&overlap(out,cap,written,sizeof *written))))return NB_LENGTH;if(written)*written=0;if((!in&&il)||!out||!written)return NB_NULL;if(!bits||!k)return NB_UNSUPPORTED;need=bits/8;if(cap<need)return NB_CAPACITY;if(overlap(in,il,out,cap))return NB_LENGTH;r=bits%k;full=bits/k;
 {int invalid=0,final_invalid=0;
 for(i=0;i<il;i++){unsigned char c=(unsigned char)in[i];size_t expected=nb_encoded_length(b,w);if(ws(c))continue;d=digit(b,c);if(d<0){invalid=1;continue;}if(r&&count<expected&&((b==64&&count>=full&&c!='0'&&c!='1')||(b!=64&&count==expected-1&&(unsigned)d>=(1u<<r))))final_invalid=1;count++;}
 if(!count)return NB_EMPTY;
 if(invalid)return NB_INVALID_CHARACTER;
 if(final_invalid)return NB_FINAL_DIGIT;
 }
 if(count!=nb_encoded_length(b,w))return NB_LENGTH;
 for(i=0;i<need;i++)out[i]=0;
 count=0;
 for(i=0;i<il;i++){unsigned char c=(unsigned char)in[i];if(ws(c))continue;d=digit(b,c);if(b==64&&count>=full)putbits(out,&pos,(unsigned)(c-'0'),1);else putbits(out,&pos,(unsigned)d,(r&&count==nb_encoded_length(b,w)-1)?r:k);count++;}
 *written=need;
 return NB_OK;
}
