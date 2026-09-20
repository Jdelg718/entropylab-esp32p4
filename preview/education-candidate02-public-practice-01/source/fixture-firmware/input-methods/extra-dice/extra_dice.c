#include "extra_dice.h"

static int sep(unsigned char c){return c==' '||c=='\t'||c=='\r'||c=='\n'||c=='\v'||c=='\f'||c==','||c==';'||c=='|';}
static int hexv(unsigned char c){if(c>='0'&&c<='9')return c-'0';if(c>='a'&&c<='f')return c-'a'+10;if(c>='A'&&c<='F')return c-'A'+10;return -1;}

struct byte_range { uintptr_t begin,end; int empty; };

static int make_range(const void *pointer,size_t bytes,struct byte_range *range){
 uintptr_t begin=(uintptr_t)pointer;
 range->begin=begin;range->end=begin;range->empty=bytes==0;
 if(bytes==0)return 1;
 if(!pointer||bytes>UINTPTR_MAX-begin)return 0;
 range->end=begin+bytes;return 1;
}

static int ranges_overlap(const struct byte_range *a,const struct byte_range *b){
 return !a->empty&&!b->empty&&a->begin<b->end&&b->begin<a->end;
}

static int validate_ranges(const char *s,size_t n,uint16_t *out,size_t cap,size_t *count,
                           struct byte_range *input,struct byte_range *output){
 struct byte_range count_range;
 if(cap>SIZE_MAX/sizeof *out)return 0;
 if(!make_range(s,s?n:0,input)||!make_range(out,out?cap*sizeof *out:0,output)||
    !make_range(count,sizeof *count,&count_range))return 0;
 return !ranges_overlap(input,&count_range)&&!ranges_overlap(output,&count_range);
}

static int bitbox_scan(const char *s,size_t n,uint16_t *out,size_t *count){
 size_t used=0,g=0;uint16_t v[5];
 for(size_t i=0;i<n;i++){unsigned char c=(unsigned char)s[i];if(sep(c))continue;if(c<'1'||c>'6')return EL_DICE_INVALID;
  if(g<5){if(c>='5')continue;v[g++]=(uint16_t)(c-'1');}
  else{uint16_t x=0;for(size_t j=0;j<5;j++)x=(uint16_t)(x*4u+v[j]);if(out)out[used]=(uint16_t)(x*2u+(c>='4'));used++;g=0;}
 }
 *count=used;return g?EL_DICE_INCOMPLETE:EL_DICE_OK;
}

int el_bitbox_indices(const char *s,size_t n,uint16_t *out,size_t cap,size_t *count){
 struct byte_range input,output,actual;size_t needed=0;
 if(!count||!validate_ranges(s,n,out,cap,count,&input,&output))return EL_DICE_INVALID;
 *count=0;if(!s||(!out&&cap))return EL_DICE_INVALID;
 int status=bitbox_scan(s,n,NULL,&needed);if(status!=EL_DICE_OK)return status;if(needed>cap)return EL_DICE_CAPACITY;
 if(!make_range(out,needed*sizeof *out,&actual)||ranges_overlap(&input,&actual))return EL_DICE_INVALID;
 status=bitbox_scan(s,n,out,&needed);if(status==EL_DICE_OK)*count=needed;return status;
}

static int dplus_scan(const char *s,size_t n,uint16_t *out,size_t *count){
 unsigned char t[3];size_t p=0,used=0;
 for(size_t i=0;i<n;i++){unsigned char c=(unsigned char)s[i];if(sep(c))continue;t[p++]=c;if(p==3){int a=hexv(t[1]),b=hexv(t[2]);if(t[0]<'1'||t[0]>'8'||a<0||b<0)return EL_DICE_INVALID;if(out)out[used]=(uint16_t)((t[0]-'1')*256+a*16+b);used++;p=0;}}
 *count=used;return p?EL_DICE_INCOMPLETE:EL_DICE_OK;
}

int el_dplus_indices(const char *s,size_t n,uint16_t *out,size_t cap,size_t *count){
 struct byte_range input,output,actual;size_t needed=0;
 if(!count||!validate_ranges(s,n,out,cap,count,&input,&output))return EL_DICE_INVALID;
 *count=0;if(!s||(!out&&cap))return EL_DICE_INVALID;
 int status=dplus_scan(s,n,NULL,&needed);if(status!=EL_DICE_OK)return status;if(needed>cap)return EL_DICE_CAPACITY;
 if(!make_range(out,needed*sizeof *out,&actual)||ranges_overlap(&input,&actual))return EL_DICE_INVALID;
 status=dplus_scan(s,n,out,&needed);if(status==EL_DICE_OK)*count=needed;return status;
}

int el_dplus_final_index(const char *s,size_t n,unsigned words,uint8_t *out){
 static const unsigned char kinds[5][2]={{8,16},{8,8},{16,2},{16,0},{8,0}};unsigned row=words==12?0:words==15?1:words==18?2:words==21?3:words==24?4:99;
 unsigned char t[2];size_t p=0;if(!s||!out||row==99)return EL_DICE_INVALID;for(size_t i=0;i<n;i++){unsigned char c=(unsigned char)s[i];if(sep(c))continue;if(p>=2)return EL_DICE_INVALID;t[p++]=c;}
 size_t need=kinds[row][1]?2:1;if(p<need)return EL_DICE_INCOMPLETE;if(p>need)return EL_DICE_INVALID;unsigned x=0;for(size_t i=0;i<need;i++){unsigned radix=kinds[row][i],v;if(radix==16){int h=hexv(t[i]);if(h<0)return EL_DICE_INVALID;v=(unsigned)h;}else{if(t[i]<'1'||t[i]>'8')return EL_DICE_INVALID;v=radix==8?(unsigned)(t[i]-'1'):(unsigned)(t[i]>='5');}x=x*radix+v;}*out=(uint8_t)x;return EL_DICE_OK;
}
