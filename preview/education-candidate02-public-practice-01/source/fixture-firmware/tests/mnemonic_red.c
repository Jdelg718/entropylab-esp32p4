#include "compute.h"
#include <assert.h>
int main(void){
 hex_request_t q={.mode=4,.words=12};
 strcpy(q.hex,"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about");q.length=strlen(q.hex);
 hex_result_t r;el_compute(&q,&r);
 assert(r.rc==0); /* RED: current worker rejects mnemonic mode. */
 return 0;
}
