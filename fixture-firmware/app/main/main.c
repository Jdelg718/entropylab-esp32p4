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
extern int32_t fixture_run(uint8_t*,size_t,uint8_t*,size_t,uint8_t*,size_t);
// Rust allocator: aligned internal 8-bit heap. NULL invokes Rust allocation abort;
// panic/OOM reset rather than unwind. Public fixtures only, not secret-safe memory.
void *fixture_alloc(size_t n,size_t align) {
    if (align < sizeof(void*)) align=sizeof(void*);
    return heap_caps_aligned_alloc(align,n?n:1,MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
}
void fixture_free(void *p) {heap_caps_free(p);}
typedef struct {char mnemonic[128],fingerprint[9],address[64];int32_t rc;bool pass;} result_t;
static QueueHandle_t requests,results;
static lv_obj_t *button,*status_label,*mnemonic_label,*fingerprint_label,*address_label,*touch_label;
static uint32_t touches;
static void worker(void *unused) {
    (void)unused;
    for (;;) {
        uint8_t request;
        if(xQueueReceive(requests,&request,portMAX_DELAY)!=pdTRUE)continue;
        result_t r={0};
        ESP_LOGI("fixture","compute start internal_free=%u",(unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
        r.rc=fixture_run((uint8_t*)r.mnemonic,sizeof r.mnemonic,(uint8_t*)r.fingerprint,sizeof r.fingerprint,(uint8_t*)r.address,sizeof r.address);
        r.pass=r.rc==0 && strcmp(r.mnemonic,"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about")==0 && strcmp(r.fingerprint,"73c5da0a")==0 && strcmp(r.address,"bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu")==0;
        ESP_LOGI("fixture","compute done rc=%ld pass=%d internal_free=%u stack_watermark=%u",(long)r.rc,r.pass,(unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),(unsigned)uxTaskGetStackHighWaterMark(NULL));
        // Queue depth one, copies all owned bytes; never hands Rust/UI pointers across tasks.
        xQueueOverwrite(results,&r);
    }
}
static void clicked(lv_event_t *e) {
    (void)e;touches++;
    lv_label_set_text_fmt(touch_label,"Touch count: %lu",(unsigned long)touches);
    uint8_t req=1;
    if(xQueueSend(requests,&req,0)==pdTRUE){
        lv_obj_add_state(button,LV_STATE_DISABLED);
        lv_label_set_text(status_label,"Computing public fixture...");
    } else lv_label_set_text(status_label,"Worker busy; try again");
}
static void poll_result(lv_timer_t *timer) {
    (void)timer;result_t r;
    if(xQueueReceive(results,&r,0)==pdTRUE){
        lv_label_set_text(mnemonic_label,r.rc==0?r.mnemonic:"Calculation failed");
        lv_label_set_text_fmt(fingerprint_label,"Fingerprint: %s",r.rc==0?r.fingerprint:"unavailable");
        lv_label_set_text(address_label,r.rc==0?r.address:"Address unavailable");
        lv_label_set_text(status_label,r.pass?"PASS - matches BIP39 / BIP84 fixture":"FAIL - do not use");
        lv_obj_remove_state(button,LV_STATE_DISABLED);
    }
}
static lv_obj_t *label(lv_obj_t *parent,const char *text) {
    lv_obj_t *o=lv_label_create(parent);lv_obj_set_width(o,420);lv_label_set_long_mode(o,LV_LABEL_LONG_WRAP);lv_label_set_text(o,text);return o;
}
void app_main(void) {
    bsp_display_cfg_t cfg={.lv_adapter_cfg=ESP_LV_ADAPTER_DEFAULT_CONFIG(),.rotation=ESP_LV_ADAPTER_ROTATE_0,.tear_avoid_mode=ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL,.touch_flags={.swap_xy=0,.mirror_x=0,.mirror_y=0}};
    if(bsp_display_start_with_config(&cfg)==NULL){ESP_LOGE("fixture","Display initialization failed");return;}
    if(bsp_display_backlight_on()!=ESP_OK){ESP_LOGE("fixture","Backlight initialization failed");return;}
    requests=xQueueCreate(1,sizeof(uint8_t));results=xQueueCreate(1,sizeof(result_t));
    if(!requests||!results){ESP_LOGE("fixture","Queue allocation failed");return;}
    // ESP-IDF task stack size is bytes; explicit INTERNAL prohibits PSRAM stacks.
    if(xTaskCreateWithCaps(worker,"fixture_calc",32768,NULL,3,NULL,MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)!=pdPASS){ESP_LOGE("fixture","Worker allocation failed");return;}
    if(bsp_display_lock(-1)!=ESP_OK){ESP_LOGE("fixture","Display lock failed");return;}
    lv_obj_t *screen=lv_screen_active();
    lv_obj_set_style_pad_all(screen,24,0);lv_obj_set_style_pad_row(screen,18,0);
    lv_obj_set_flex_flow(screen,LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_text_font(screen,&lv_font_montserrat_20,0);
    label(screen,"EntropyLab");label(screen,"DEVELOPMENT TEST");
    label(screen,"Public fixtures only - NEVER fund these addresses");
    label(screen,"Zero entropy / empty passphrase\nm/84'/0'/0'/0/0");
    mnemonic_label=label(screen,"Mnemonic: press Run Test");
    fingerprint_label=label(screen,"Fingerprint: not calculated");
    address_label=label(screen,"Address: not calculated");
    status_label=label(screen,"Ready - computation runs on this device");
    touch_label=label(screen,"Touch count: 0");
    button=lv_button_create(screen);lv_obj_set_size(button,420,64);
    lv_obj_t *t=lv_label_create(button);lv_label_set_text(t,"Run Test");lv_obj_center(t);
    lv_obj_add_event_cb(button,clicked,LV_EVENT_CLICKED,NULL);
    lv_timer_create(poll_result,100,NULL);
    bsp_display_unlock();
}
