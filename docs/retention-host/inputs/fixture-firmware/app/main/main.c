#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "lvgl.h"
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "gui.h"
#include "compute.h"
#include "all_features_application.h"
#include "cards_target.h"
#include "bases_target.h"
#include "seed_helpers_target.h"
#include "passphrase_core.h"
#include "lifehash_fingerprint.h"
#include "extra_dice_target.h"
// Rust allocator: aligned internal 8-bit heap. NULL invokes Rust allocation abort;
// panic/OOM reset rather than unwind. Public fixtures only, not secret-safe memory.
void *fixture_alloc(size_t n,size_t align) {
    if (align < sizeof(void*)) align=sizeof(void*);
    return heap_caps_aligned_alloc(align,n?n:1,MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
}
void fixture_free(void *p) {heap_caps_free(p);}
static QueueHandle_t requests;
static QueueHandle_t cards_bases_requests;
static portMUX_TYPE transition_lock=portMUX_INITIALIZER_UNLOCKED;
static void dispatch_lock(void){portENTER_CRITICAL(&transition_lock);}
static void dispatch_unlock(void){portEXIT_CRITICAL(&transition_lock);}
static bool publish_token(uint64_t token){return xQueueSend(requests,&token,0)==pdTRUE;}
static bool publish_cards_bases_token(uint64_t token){return xQueueSend(cards_bases_requests,&token,0)==pdTRUE;}
static gui08_passphrase_marker cards_bases_passphrase_snapshot(
 const gui08_request *request,uint8_t passphrase[256],size_t *passphrase_len){
 if(!request||!passphrase||!passphrase_len||request->passphrase_len>256)return GUI08_PASSPHRASE_UNSPECIFIED;
 const bool empty=request->passphrase_marker==GUI08_PASSPHRASE_EMPTY&&request->passphrase_len==0;
 const bool active=request->passphrase_marker==GUI08_PASSPHRASE_ACTIVE&&request->passphrase_len!=0;
 if(!empty&&!active)return GUI08_PASSPHRASE_UNSPECIFIED;
 memset(passphrase,0,256);*passphrase_len=request->passphrase_len;
 if(*passphrase_len)memcpy(passphrase,request->passphrase,*passphrase_len);
 return request->passphrase_marker;
}
static int cards_bases_convert(const gui08_request *request,pc_result_v1 *result){
    if(!request||!result)return PC_V1_NULL_ERROR;
    uint32_t context=(uint32_t)request->request_id^(uint32_t)(request->request_id>>32);
    if(request->method==GUI08_CARDS)
        return (int)cards_target_convert(PC_V1_DIRECT,request->words,context,
                                        (const uint8_t*)request->transcript,
                                        request->length,result);
    if(request->method==GUI08_BASES)
        return (int)bases_target_convert(request->base,request->words,context,
                                        (const uint8_t*)request->transcript,
                                        request->length,result);
    if(request->method==GUI08_SEED)
        return (int)seed_target_convert(request->seed_operation,request->words,
                                        request->base,context,
                                        (const uint8_t*)request->transcript,
                                        request->length,result);
    if(request->method==GUI08_DICE)
        return extra_dice_target_convert(request,context,result);
    return PC_V1_MODE_ERROR;
}
static int cards_bases_derive(const gui08_request *request,const pc_result_v1 *conversion,
                             all_features_derivation_result *result){
 if(!request||!conversion||!result||!conversion->mnemonic_len||
    conversion->mnemonic_len>sizeof conversion->mnemonic)return -1;
 return (int)el_bip39_passphrase_run(
    conversion->mnemonic,conversion->mnemonic_len,
    request->passphrase,request->passphrase_len,
    (uint8_t*)result->entropy,sizeof result->entropy,
    (uint8_t*)result->fingerprint,sizeof result->fingerprint,
    (uint8_t*)result->address,sizeof result->address);
}
static int legacy_lifehash_render(const uint8_t raw4[4],uint8_t rgb[32*32*3]){
 return (int)lifehash_fingerprint_render(raw4,4,LIFEHASH_FINGERPRINT_VERSION_2,
                                         rgb,LIFEHASH_FINGERPRINT_RGB_SIZE);
}
static int lifehash_render(const all_features_lifehash_request *request,
                           all_features_lifehash_result *result){
 if(!request||!result)return LIFEHASH_FINGERPRINT_NULL_INPUT;
 result->route_epoch=request->route_epoch;
 result->request_id=request->request_id;
 result->revision=request->revision;
 return (int)lifehash_fingerprint_render(request->raw4,sizeof request->raw4,
    LIFEHASH_FINGERPRINT_VERSION_2,result->rgb,sizeof result->rgb);
}
static bool cards_bases_result(const gui08_request *request,const pc_result_v1 *result,
                               const all_features_derivation_result *derivation){
    const char *method=request&&request->method==GUI08_CARDS?"cards-direct":
                       request&&request->method==GUI08_BASES&&request->base==4?"base4":
                       request&&request->method==GUI08_BASES&&request->base==8?"base8":
                       request&&request->method==GUI08_BASES&&request->base==32?"base32":
                       request&&request->method==GUI08_BASES&&request->base==64?"base64":
                       request&&request->method==GUI08_SEED&&request->seed_operation==SH_WORDS_TO_NUMBERS?"seed-words":
                       request&&request->method==GUI08_SEED&&request->seed_operation==SH_NUMBERS_TO_WORDS?"seed-numbers":
                       request&&request->method==GUI08_DICE&&request->dice_method==GUI08_DICE_BITBOX?"dice-bitbox":
                       request&&request->method==GUI08_DICE&&request->dice_method==GUI08_DICE_DPLUS?"dice-dplus":NULL;
    if(!request||!result||!derivation||!method||result->version!=PC_V1_VERSION||
       result->mode!=PC_V1_DIRECT||result->words!=request->words||
       !result->mnemonic_len||result->method_len!=strlen(method)||
       memcmp(result->method,method,result->method_len)||
       !memchr(derivation->entropy,0,sizeof derivation->entropy)||
       !memchr(derivation->fingerprint,0,sizeof derivation->fingerprint)||
       !memchr(derivation->address,0,sizeof derivation->address))return false;
    return gui_cards_bases_result(request);
}
static void worker(void *unused){
 (void)unused;for(;;){uint64_t token=0;if(xQueueReceive(requests,&token,portMAX_DELAY)==pdTRUE)gui_passphrase_work(token);}
}
static void cards_bases_worker(void *unused){
 (void)unused;for(;;){uint64_t token=0;if(xQueueReceive(cards_bases_requests,&token,portMAX_DELAY)==pdTRUE)all_features_application_work(token,cards_bases_convert);}
}
/* BSP LVGL timer executes serialized with GUI events; no worker wait here. */
static void poll_result(lv_timer_t *timer){(void)timer;gui_passphrase_poll();all_features_application_poll();}
void app_main(void) {
    bsp_display_cfg_t cfg={.lv_adapter_cfg=ESP_LV_ADAPTER_DEFAULT_CONFIG(),.rotation=ESP_LV_ADAPTER_ROTATE_0,.tear_avoid_mode=ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL,.touch_flags={.swap_xy=0,.mirror_x=0,.mirror_y=0}};
    if(bsp_display_start_with_config(&cfg)==NULL){ESP_LOGE("fixture","Display initialization failed");return;}
    if(bsp_display_backlight_on()!=ESP_OK){ESP_LOGE("fixture","Backlight initialization failed");return;}
    requests=xQueueCreate(1,sizeof(uint64_t));
    if(!requests){ESP_LOGE("fixture","Queue allocation failed");return;}
    cards_bases_requests=xQueueCreate(1,sizeof(uint64_t));
    if(!cards_bases_requests){ESP_LOGE("fixture","Cards/Bases queue allocation failed");return;}
    gui_passphrase_configure(dispatch_lock,dispatch_unlock,publish_token);
    gui_passphrase_lifehash_configure(legacy_lifehash_render);
    if(!all_features_application_configure(dispatch_lock,dispatch_unlock,publish_cards_bases_token,cards_bases_passphrase_snapshot,cards_bases_derive,gui_cards_bases_pending,cards_bases_result,gui_cards_bases_cancelled)){ESP_LOGE("fixture","Cards/Bases dispatcher initialization failed");return;}
    if(!all_features_application_lifehash_configure(lifehash_render,gui_cards_bases_lifehash,gui_cards_bases_lifehash_blank)){ESP_LOGE("fixture","LifeHash initialization failed");return;}
    gui_cards_bases_configure(all_features_application_submit,all_features_application_cancel);
    // ESP-IDF task stack size is bytes; explicit INTERNAL prohibits PSRAM stacks.
    if(xTaskCreateWithCaps(worker,"fixture_calc",32768,NULL,3,NULL,MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)!=pdPASS){ESP_LOGE("fixture","Worker allocation failed");return;}
    if(xTaskCreateWithCaps(cards_bases_worker,"cards_bases_calc",16384,NULL,3,NULL,MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)!=pdPASS){ESP_LOGE("fixture","Cards/Bases worker allocation failed");return;}
    if(bsp_display_lock(-1)!=ESP_OK){ESP_LOGE("fixture","Display lock failed");return;}
    gui_create(NULL);
    lv_timer_create(poll_result,100,NULL);
    bsp_display_unlock();
}
