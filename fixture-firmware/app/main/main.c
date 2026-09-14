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
// Rust allocator: aligned internal 8-bit heap. NULL invokes Rust allocation abort;
// panic/OOM reset rather than unwind. Public fixtures only, not secret-safe memory.
void *fixture_alloc(size_t n,size_t align) {
    if (align < sizeof(void*)) align=sizeof(void*);
    return heap_caps_aligned_alloc(align,n?n:1,MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
}
void fixture_free(void *p) {heap_caps_free(p);}
static QueueHandle_t requests,results;
static void worker(void *unused) {
    (void)unused;
    for (;;) {
        hex_request_t request={0};
        if(xQueueReceive(requests,&request,portMAX_DELAY)!=pdTRUE)continue;
        hex_result_t r={0};
        /* No LVGL access or lock in the compute task. Full owned queue copies. */
        el_compute(&request,&r);
        xQueueOverwrite(results,&r);
        memset(&request,0,sizeof request);memset(&r,0,sizeof r);
        /* Best effort only: allocator/stack/queue copies are not securely erased. */
    }
}
static bool clicked(const hex_request_t *request) {
    return xQueueSend(requests,request,0)==pdTRUE;
}
static void poll_result(lv_timer_t *timer) {
    (void)timer;hex_result_t r={0};
    if(xQueueReceive(results,&r,0)==pdTRUE)gui_result(&r);
    memset(&r,0,sizeof r);
}
void app_main(void) {
    bsp_display_cfg_t cfg={.lv_adapter_cfg=ESP_LV_ADAPTER_DEFAULT_CONFIG(),.rotation=ESP_LV_ADAPTER_ROTATE_0,.tear_avoid_mode=ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL,.touch_flags={.swap_xy=0,.mirror_x=0,.mirror_y=0}};
    if(bsp_display_start_with_config(&cfg)==NULL){ESP_LOGE("fixture","Display initialization failed");return;}
    if(bsp_display_backlight_on()!=ESP_OK){ESP_LOGE("fixture","Backlight initialization failed");return;}
    requests=xQueueCreate(1,sizeof(hex_request_t));results=xQueueCreate(1,sizeof(hex_result_t));
    if(!requests||!results){ESP_LOGE("fixture","Queue allocation failed");return;}
    // ESP-IDF task stack size is bytes; explicit INTERNAL prohibits PSRAM stacks.
    if(xTaskCreateWithCaps(worker,"fixture_calc",32768,NULL,3,NULL,MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT)!=pdPASS){ESP_LOGE("fixture","Worker allocation failed");return;}
    if(bsp_display_lock(-1)!=ESP_OK){ESP_LOGE("fixture","Display lock failed");return;}
    gui_create(clicked);
    lv_timer_create(poll_result,100,NULL);
    bsp_display_unlock();
}
