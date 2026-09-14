#include "hex_core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(int argc,char **argv){uint8_t m[216],f[9],a[43];if(argc!=2)return 2;memset(m,0xa5,sizeof m);memset(f,0xa5,sizeof f);memset(a,0xa5,sizeof a);int r=el_hex_run((const uint8_t*)argv[1],strlen(argv[1]),m,sizeof m,f,sizeof f,a,sizeof a);if(r)return 1;printf("%s\t%s\t%s\n",m,f,a);return 0;}
