#include "dice_core.h"
#include "../rust/hex_core.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void) {
 uint8_t h[65],m[216],f[9],a[43]; memset(h,0xa5,sizeof h);
 assert(el_dice_to_hex((const uint8_t*)"6",1,EL_DICE_COLEMAN_6_TO_0_BEFORE_SHA256,12,h,65)==EL_DICE_WEAK_INPUT_LAB_ONLY);
 assert(!strcmp((char*)h,"5feceb66ffc86f38d952786c6d696c79"));
 assert(el_hex_run(h,32,m,216,f,9,a,43)==0);
 assert(!strcmp((char*)m,"garment guard super zebra manage organ grab excuse hockey hero force vessel"));
 assert(el_dice_required_rolls(24)==100);
 memset(h,0xa5,sizeof h);assert(el_dice_to_hex((const uint8_t*)"1230",4,1,12,h,65)==-4);
 for(size_t i=0;i<65;i++)assert(h[i]==0xa5);
 puts("C header ABI + single composed archive + upstream mnemonic: PASS");return 0;
}
