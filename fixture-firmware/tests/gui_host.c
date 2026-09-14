/* Real LVGL renderer/navigation test. Public test strings only. Not hardware evidence. */
#include "gui.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned char pixels[480*800*3];
static uint16_t buffer[480*64];
static unsigned runs,checked;
static void flush(lv_display_t *d,const lv_area_t *a,uint8_t *data){
    uint16_t *p=(uint16_t*)data;
    for(int y=a->y1;y<=a->y2;y++)for(int x=a->x1;x<=a->x2;x++){
        uint16_t c=*p++;assert(x>=0&&x<480&&y>=0&&y<800);
        unsigned char *q=pixels+3*(y*480+x);
        q[0]=((c>>11)&31)*255/31;q[1]=((c>>5)&63)*255/63;q[2]=(c&31)*255/31;
    }lv_display_flush_ready(d);
}
static void request(void){runs++;gui_busy();}
static void geometry(lv_obj_t *o){
    if(lv_obj_has_flag(o,LV_OBJ_FLAG_HIDDEN))return;
    lv_area_t a;lv_obj_get_coords(o,&a);
    if(a.x1<0||a.y1<0||a.x2>=480||a.y2>=800){fprintf(stderr,"OOB %d,%d-%d,%d\n",a.x1,a.y1,a.x2,a.y2);abort();}
    if(lv_obj_check_type(o,&lv_button_class)){assert(lv_obj_get_width(o)>=44);assert(lv_obj_get_height(o)>=44);checked++;}
    if(lv_obj_check_type(o,&lv_label_class)){
        lv_obj_t *p=lv_obj_get_parent(o);lv_area_t b;lv_obj_get_content_coords(p,&b);
        if(a.x1<b.x1||a.x2>b.x2||a.y1<b.y1||a.y2>b.y2){fprintf(stderr,"CLIPPED '%s' %d,%d-%d,%d parent %d,%d-%d,%d\n",lv_label_get_text(o),a.x1,a.y1,a.x2,a.y2,b.x1,b.y1,b.x2,b.y2);abort();}
    }
    for(uint32_t i=0;i<lv_obj_get_child_count(o);i++)geometry(lv_obj_get_child(o,i));
}
static lv_obj_t *find(lv_obj_t *o,const char *s){
    if(lv_obj_has_flag(o,LV_OBJ_FLAG_HIDDEN))return NULL;
    if(lv_obj_check_type(o,&lv_label_class)&&strcmp(lv_label_get_text(o),s)==0)return lv_obj_get_parent(o);
    for(uint32_t i=0;i<lv_obj_get_child_count(o);i++){lv_obj_t *r=find(lv_obj_get_child(o,i),s);if(r)return r;}return NULL;
}
static void click(const char *s){lv_obj_t *b=find(lv_screen_active(),s);assert(b);assert(!lv_obj_has_state(b,LV_STATE_DISABLED));lv_obj_send_event(b,LV_EVENT_CLICKED,NULL);}
static void render(const char *name){
    lv_obj_update_layout(lv_screen_active());geometry(lv_screen_active());
    lv_obj_invalidate(lv_screen_active());lv_refr_now(NULL);
    if(name){FILE *f=fopen(name,"wb");assert(f);fprintf(f,"P6\n480 800\n255\n");assert(fwrite(pixels,1,sizeof pixels,f)==sizeof pixels);fclose(f);}
}
int main(void){
    lv_init();lv_display_t *d=lv_display_create(480,800);assert(d);
    lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);lv_display_set_flush_cb(d,flush);
    lv_display_set_buffers(d,buffer,NULL,sizeof buffer,LV_DISPLAY_RENDER_MODE_PARTIAL);
    gui_create(request);render("input-native.ppm");
    for(const char *p="0123456789ABCDEF";*p;p++){char k[2]={*p,0};lv_obj_t *b=find(lv_screen_active(),k);assert(b&&lv_obj_has_state(b,LV_STATE_DISABLED));}
    click("Fixture results");render("empty-native.ppm");click("Back to hex input");
    click("Run public fixture");assert(runs==1);assert(lv_obj_has_state(find(lv_screen_active(),"Run public fixture"),LV_STATE_DISABLED));render("busy-native.ppm");
    gui_result(true,"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about","73c5da0a","bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu");render("results-native.ppm");
    assert(find(lv_screen_active(),"about"));assert(find(lv_screen_active(),"73c5da0a"));
    click("Back to hex input");click("Run public fixture");assert(runs==2);
    gui_result(false,"INVALID","INVALID","INVALID");render("failure-native.ppm");
    assert(!find(lv_screen_active(),"73c5da0a"));assert(find(lv_screen_active(),"Unavailable"));
    click("Hex input");assert(!lv_obj_has_state(find(lv_screen_active(),"Run public fixture"),LV_STATE_DISABLED));
    printf("PASS: navigation, disabled keypad, busy/retry/failure, bounded labels, >=44px targets; %u controls checked across states\n",checked);
    lv_deinit();return 0;
}
