#include "coin_core.h"
#include "../hex-core/hex_core.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(void) {
 uint8_t bits[256], hex[65], m[216], f[9], a[43];
 memset(bits,'0',sizeof bits);
 for (int i=0;i<2;i++) {
  if(i) memset(bits+252,'1',4);
  assert(el_coin_to_hex(bits,256,24,hex,65)==0);
  assert(el_hex_run(hex,64,m,216,f,9,a,43)==0);
  assert(strcmp((char*)f,i?"53f6b5aa":"5436d724")==0);
  assert(strcmp((char*)a,i?"bc1qpyhdk7n9nk30j5z0242kvp0p4m2udjtay4zv20":"bc1qzmtrqsfuaf6l6kkcsseumq26ukaphfj9skkug6")==0);
  printf("PUBLIC same-core integration: %s %s %s\n",m,f,a);
 }
 return 0;
}
