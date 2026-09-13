#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
extern int32_t fixture_run(uint8_t*,size_t,uint8_t*,size_t,uint8_t*,size_t);
int main(void){uint8_t m[128],f[9],a[64];memset(m,0xa5,sizeof m);memset(f,0xa5,sizeof f);memset(a,0xa5,sizeof a);
assert(fixture_run(m,93,f,9,a,64)==-2);for(unsigned i=0;i<128;i++)assert(m[i]==0xa5);
assert(fixture_run(m,128,f,9,a,64)==0);
assert(strcmp((char*)m,"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about")==0);
assert(strcmp((char*)f,"73c5da0a")==0);assert(strcmp((char*)a,"bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu")==0);
puts("C ABI public fixture KAT and capacity sentinel PASS");return 0;}
