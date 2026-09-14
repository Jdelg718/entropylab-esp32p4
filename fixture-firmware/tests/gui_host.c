/* Real LVGL renderer/navigation test. Public test strings only. Not hardware evidence. */
#include "gui.h"
#include "lvgl.h"
#include "compute.h"
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
static hex_request_t owned;
static bool reject;
static bool request(const hex_request_t *r){runs++;owned=*r;return !reject;}
static hex_result_t compute(void){
    struct {unsigned pre;hex_result_t r;unsigned post;} g={.pre=0x12345678,.post=0xabcdef01};
    el_compute(&owned,&g.r);
    assert(g.pre==0x12345678&&g.post==0xabcdef01);assert(g.r.rc==0);return g.r;
}
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
static unsigned occurrences(lv_obj_t *o,const char *s){
    if(lv_obj_has_flag(o,LV_OBJ_FLAG_HIDDEN))return 0;
    unsigned n=lv_obj_check_type(o,&lv_label_class)&&!strcmp(lv_label_get_text(o),s);
    for(uint32_t i=0;i<lv_obj_get_child_count(o);i++)n+=occurrences(lv_obj_get_child(o,i),s);
    return n;
}
static void assert_words(const hex_result_t *r){
    char copy[216];memcpy(copy,r->mnemonic,sizeof copy);char *tokens[24];unsigned n=0;
    for(char *t=strtok(copy," ");t;t=strtok(NULL," ")){assert(n<24);tokens[n++]=t;}
    assert(n==owned.words);
    for(unsigned i=0;i<n;i++){unsigned expected=0;for(unsigned j=0;j<n;j++)expected+=!strcmp(tokens[i],tokens[j]);assert(occurrences(lv_screen_active(),tokens[i])==expected);}
}
static lv_obj_t *find_button(lv_obj_t *o,const char *s){
    if(lv_obj_has_flag(o,LV_OBJ_FLAG_HIDDEN))return NULL;
    if(lv_obj_check_type(o,&lv_label_class)&&!strcmp(lv_label_get_text(o),s)&&lv_obj_check_type(lv_obj_get_parent(o),&lv_button_class))return lv_obj_get_parent(o);
    for(uint32_t i=0;i<lv_obj_get_child_count(o);i++){lv_obj_t *r=find_button(lv_obj_get_child(o,i),s);if(r)return r;}return NULL;
}
static void click(const char *s){lv_obj_t *b=find_button(lv_screen_active(),s);assert(b);assert(!lv_obj_has_state(b,LV_STATE_DISABLED));lv_obj_send_event(b,LV_EVENT_CLICKED,NULL);}
static void render(const char *name){
    lv_obj_update_layout(lv_screen_active());geometry(lv_screen_active());
    lv_obj_invalidate(lv_screen_active());lv_refr_now(NULL);
    if(name){FILE *f=fopen(name,"wb");assert(f);fprintf(f,"P6\n480 800\n255\n");assert(fwrite(pixels,1,sizeof pixels,f)==sizeof pixels);fclose(f);}
}
#include "coin_tests.inc"
int main(void){
    lv_init();lv_display_t *d=lv_display_create(480,800);assert(d);
    lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);lv_display_set_flush_cb(d,flush);
    lv_display_set_buffers(d,buffer,NULL,sizeof buffer,LV_DISPLAY_RENDER_MODE_PARTIAL);
    gui_create(request);render("empty-native.ppm");
    assert(lv_obj_has_state(find(lv_screen_active(),"Calculate"),LV_STATE_DISABLED));
    for(int i=1;i<=65;i++){
        char k[2]={"0123456789ABCDEF"[(i-1)%16],0};click(k);
        unsigned n=i>64?64:(unsigned)i;char c[64];snprintf(c,sizeof c,"%u / 64 chars  |  %u bits",n,n*4);
        assert(find(lv_screen_active(),c));
        assert(lv_obj_has_state(find(lv_screen_active(),"Calculate"),LV_STATE_DISABLED)!=(n>=32&&n%8==0));
        render(NULL);
    }
    render("input-native.ppm");click("Calculate");assert(owned.length==64);
    assert(strcmp(owned.hex,"0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF")==0);
    assert(lv_obj_has_state(find(lv_screen_active(),"A"),LV_STATE_DISABLED));
    click("Test results");assert(lv_obj_has_state(find_button(lv_screen_active(),"Clear"),LV_STATE_DISABLED));click("Hex input");
    unsigned before=runs;lv_obj_send_event(find_button(lv_screen_active(),"Calculate"),LV_EVENT_CLICKED,NULL);assert(runs==before);
    hex_request_t snapshot=owned;
    const char *blocked[]={"Clear","Delete","A","Load public zero"};
    for(unsigned i=0;i<sizeof blocked/sizeof blocked[0];i++)
        lv_obj_send_event(find_button(lv_screen_active(),blocked[i]),LV_EVENT_CLICKED,NULL);
    assert(!memcmp(&snapshot,&owned,sizeof owned));
    assert(find(lv_screen_active(),"64 / 64 chars  |  256 bits"));
    click("Test results");lv_obj_send_event(find_button(lv_screen_active(),"Clear"),LV_EVENT_CLICKED,NULL);click("Hex input");
    assert(find(lv_screen_active(),"64 / 64 chars  |  256 bits"));
    hex_result_t r=compute();gui_result(&r);render("results-native.ppm");
    assert_words(&r);assert(find(lv_screen_active(),r.fingerprint));
    click("Back to hex input");click("Delete");click("Test results");assert(!find(lv_screen_active(),r.fingerprint));render(NULL);
    click("Back to hex input");click("Clear");click("Delete");
    assert(find(lv_screen_active(),"0 / 64 chars  |  0 bits"));
    for(int n=32;n<=64;n+=8){
        click("Clear");for(int i=0;i<n;i++)click("F");click("Calculate");r=compute();gui_result(&r);render(NULL);assert_words(&r);assert(find(lv_screen_active(),r.fingerprint));click("Back to hex input");
    }
    click("Load public zero");click("Calculate");r=compute();gui_result(&r);render("zero24-native.ppm");assert(find(lv_screen_active(),"art"));
    click("Clear");click("Test results");assert(!find(lv_screen_active(),r.fingerprint));click("Back to hex input");
    click("Load public zero");reject=true;click("Calculate");assert(!lv_obj_has_state(find(lv_screen_active(),"Calculate"),LV_STATE_DISABLED));reject=false;
    click("Calculate");r.rc=-3;gui_result(&r);render("failure-native.ppm");assert(!find(lv_screen_active(),r.fingerprint));click("Back to hex input");click("Calculate");r=compute();gui_result(&r);render(NULL);
    printf("PASS: keys 0-F, boundaries 1..65, owned copy, busy, invalidation/navigation, failure/retry, 12/15/18/21/24 real core words, sentinels; %u geometry checks\n",checked);
    coin_tests();
    lv_deinit();return 0;
}
