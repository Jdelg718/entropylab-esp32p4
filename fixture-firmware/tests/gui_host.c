/* Real LVGL renderer/navigation test. Public test strings only. Not hardware evidence. */
#include "gui.h"
#include "lvgl.h"
#include "compute.h"
#include "../app/main/gui.c"
#undef hex
#undef length
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
    /* Scroll children can be intentionally offscreen; inspect dimensions even
     * then, but viewport geometry only applies to their visible intersection. */
    if(lv_obj_check_type(o,&lv_button_class)){assert(lv_obj_get_width(o)>=44);assert(lv_obj_get_height(o)>=44);}
    for(lv_obj_t *p=lv_obj_get_parent(o);p;p=lv_obj_get_parent(p)){
        if(lv_obj_has_flag(p,LV_OBJ_FLAG_SCROLLABLE)&&lv_obj_get_scroll_dir(p)==LV_DIR_VER){
            lv_area_t clip;lv_obj_get_content_coords(p,&clip);
            if(a.y2<clip.y1||a.y1>clip.y2)return;
        }
    }
    if(a.x1<0||a.y1<0||a.x2>=480||a.y2>=800){fprintf(stderr,"OOB %d,%d-%d,%d\n",a.x1,a.y1,a.x2,a.y2);abort();}
    if(lv_obj_check_type(o,&lv_button_class)){assert(lv_obj_get_width(o)>=44);assert(lv_obj_get_height(o)>=44);checked++;}
    if(lv_obj_check_type(o,&lv_label_class)){
        lv_obj_t *p=lv_obj_get_parent(o);lv_area_t b;lv_obj_get_content_coords(p,&b);
        if(a.x1<b.x1||a.x2>b.x2||((a.y1<b.y1||a.y2>b.y2)&&!(lv_obj_has_flag(p,LV_OBJ_FLAG_SCROLLABLE)&&lv_obj_get_scroll_dir(p)==LV_DIR_VER))){fprintf(stderr,"CLIPPED '%s' %d,%d-%d,%d parent %d,%d-%d,%d\n",lv_label_get_text(o),a.x1,a.y1,a.x2,a.y2,b.x1,b.y1,b.x2,b.y2);abort();}
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
    /* Hit testing walks foreground siblings first, like the native display. */
    for(uint32_t i=lv_obj_get_child_count(o);i>0;i--){lv_obj_t *r=find_button(lv_obj_get_child(o,i-1),s);if(r)return r;}return NULL;
}
static void click(const char *s){lv_obj_t *b=find_button(lv_screen_active(),s);if(!b)fprintf(stderr,"missing click target: %s\n",s);assert(b);assert(!lv_obj_has_state(b,LV_STATE_DISABLED));lv_obj_send_event(b,LV_EVENT_CLICKED,NULL);}
static void render(const char *name){
    lv_obj_update_layout(lv_screen_active());geometry(lv_screen_active());
    lv_obj_invalidate(lv_screen_active());lv_refr_now(NULL);
    if(name){FILE *f=fopen(name,"wb");assert(f);fprintf(f,"P6\n480 800\n255\n");assert(fwrite(pixels,1,sizeof pixels,f)==sizeof pixels);fclose(f);}
}
#include "coin_tests.inc"
#include "dice_tests.inc"
#include "mnemonic_gui_tests.inc"
#include "prefix_tests.inc"
#include "gui_acceptance.inc"
#include "layout_tests.inc"
#include "saver_tests.inc"
static lv_indev_data_t touch_data,touch_queue[8];
static unsigned touch_queue_count,touch_queue_next;
static void touch_read(lv_indev_t *dev,lv_indev_data_t *data){
 (void)dev;
 if(touch_queue_next<touch_queue_count){*data=touch_queue[touch_queue_next++];data->continue_reading=touch_queue_next<touch_queue_count;touch_data=*data;}
 else *data=touch_data;
}
static void touch_at(lv_indev_t *dev,int x,int y,bool down){touch_data.point.x=x;touch_data.point.y=y;touch_data.state=down?LV_INDEV_STATE_PRESSED:LV_INDEV_STATE_RELEASED;lv_indev_read(dev);}
static void *saved_callback(lv_obj_t *obj,lv_event_cb_t cb){
 for(unsigned n=0;n<lv_obj_get_event_count(obj);n++){lv_event_dsc_t *d=lv_obj_get_event_dsc(obj,n);if(lv_event_dsc_get_cb(d)==cb)return lv_event_dsc_get_user_data(d);}abort();
}
static void replay_callback(lv_event_cb_t cb,void *token){lv_obj_t *o=lv_obj_create(lv_screen_active());lv_obj_add_flag(o,LV_OBJ_FLAG_HIDDEN);lv_obj_add_event_cb(o,cb,LV_EVENT_CLICKED,token);lv_obj_send_event(o,LV_EVENT_CLICKED,NULL);lv_obj_delete(o);}
static void saver_input_tests(lv_indev_t *dev){
 lv_obj_clean(lv_screen_active());gui_create(request);click("Words");
 for(unsigned i=0;i<12;i++)type_word("abandon");
 lv_obj_t *targets[]={mn_clear,mn_validate,mn_slots[0],modes[0]};
 for(unsigned i=0;i<4;i++){
  void *oldmode=saved_callback(modes[0],switch_mode);
  saver_time(100);saver_time(60000);assert(saver_active);
  void *wakemode=saved_callback(modes[0],switch_mode);
  void *old=saved_action(mn_clear);unsigned before=runs;uintptr_t generation=mn_generation;
  touch_at(dev,0,0,false); /* update layout while concealed before actual DOWN */
  int x=i==0?70:i==1?360:i==2?80:40;int y=i<2?728:i==2?206:78;
  touch_at(dev,x,y,true);
  fprintf(stderr,"first DOWN must wake and latch before hit-testing target %u\n",i);
  assert(!saver_active&&wake_latch);assert(!lv_obj_has_flag(app_content,LV_OBJ_FLAG_HIDDEN));
  lv_obj_send_event(targets[i],LV_EVENT_CLICKED,NULL);replay_action(old);
  int orbit_x=lv_obj_get_x(saver_dot),orbit_y=lv_obj_get_y(saver_dot);
  saver_time(1500);assert(lv_obj_get_x(saver_dot)==orbit_x&&lv_obj_get_y(saver_dot)==orbit_y);
  touch_at(dev,x,y,true);touch_at(dev,x,y,false);
  lv_obj_send_event(targets[i],LV_EVENT_CLICKED,NULL);replay_action(old);
  assert(wake_latch&&mode==MODE_MNEMONIC&&!mn_confirm&&mn_edit<0&&runs==before&&mn_count==12);
  touch_at(dev,250,770,true); /* next distinct touch is Safety */
  assert(!wake_latch&&mn_generation>generation);
  touch_at(dev,250,770,false);assert(mn_safety_open);click("Close");
  replay_action(old);assert(!mn_confirm);
  replay_callback(switch_mode,oldmode);replay_callback(switch_mode,wakemode);
  fprintf(stderr,"stale mode callbacks must fail after wake completion\n");assert(mode==MODE_MNEMONIC);
 }
 render("saver-after-wake.ppm");
 lv_obj_clean(lv_screen_active());gui_create(request);click("Words");
 lv_indev_t *extra=lv_indev_create();lv_indev_set_type(extra,LV_INDEV_TYPE_POINTER);lv_indev_set_read_cb(extra,touch_read);
 saver_time(100);saver_time(60000);
 fprintf(stderr,"multiple/unowned input devices must disable saver entry\n");assert(!saver_active);
 lv_indev_delete(extra);
 lv_obj_send_event(mn_list,LV_EVENT_SCROLL_BEGIN,NULL);saver_time(100);saver_time(60000);assert(!saver_active);
 lv_obj_send_event(mn_list,LV_EVENT_SCROLL_END,NULL);saver_time(100);saver_time(59900);assert(!saver_active);
 touch_at(dev,470,500,true);saver_time(60100);assert(!saver_active);touch_at(dev,470,500,false);
 saver_time(100);saver_time(59900);assert(!saver_active);saver_time(100);assert(saver_active);
 /* Backlogged press/hold/release/new press/release in one LVGL read loop.
  * A release with continue_reading is NOT a drained gesture boundary. */
 touch_queue_count=6;touch_queue_next=0;
 for(unsigned q=0;q<touch_queue_count;q++)touch_queue[q]=(lv_indev_data_t){.point={70,728},.state=(q==2||q==5)?LV_INDEV_STATE_RELEASED:LV_INDEV_STATE_PRESSED};
 lv_indev_read(dev);assert(touch_queue_next==touch_queue_count);
 assert(!saver_active&&wake_latch&&!mn_confirm);
 lv_obj_send_event(mn_clear,LV_EVENT_LONG_PRESSED_REPEAT,NULL);lv_obj_send_event(mn_clear,LV_EVENT_CLICKED,NULL);assert(!mn_confirm);
 touch_at(dev,250,770,true);touch_at(dev,250,770,false);assert(mn_safety_open&&!wake_latch);click("Close");
 lv_obj_clean(lv_screen_active());gui_create(request);click("Words");click("24");
 for(unsigned n=0;n<24;n++)type_word("abstract");
 lv_obj_scroll_to_y(mn_list,0,LV_ANIM_OFF);lv_obj_update_layout(lv_screen_active());
 saver_time(100);saver_time(59000);
 lv_obj_set_style_anim_duration(mn_list,2000,0);lv_obj_scroll_to_y(mn_list,240,LV_ANIM_ON);
 assert(lv_obj_is_scrolling(mn_list));saver_time(1100);assert(!saver_active);
 saver_time(3000);saver_time(100);assert(!lv_obj_is_scrolling(mn_list)&&!saver_scrolling);
 assert(lv_obj_get_scroll_y(mn_list)==240);
 saver_time(59800);assert(!saver_active);saver_time(200);assert(saver_active);
 puts("PASS real animated scroll fresh interval and backlogged input burst consumption");
 lv_obj_clean(lv_screen_active());gui_create(request);click("Words");
 saver_time(100);saver_time(59000);
 lv_obj_t *home=lv_screen_active(),*other=lv_obj_create(NULL);
 lv_screen_load_anim(other,LV_SCREEN_LOAD_ANIM_MOVE_LEFT,2000,2000,false);
 saver_time(1100);
 fprintf(stderr,"pending real screen transition must block saver entry\n");assert(!saver_active);
 saver_time(3000);saver_time(3000);assert(!saver_active);
 lv_screen_load(home);lv_obj_delete(other);
 saver_time(100);saver_time(59000);
 other=lv_obj_create(NULL);lv_screen_load(other);lv_screen_load(home);lv_obj_delete(other);
 saver_time(1000);fprintf(stderr,"brief actual screen transition resets idle before next timer\n");assert(!saver_active);
 saver_time(59900);assert(!saver_active);saver_time(100);assert(saver_active);
 lv_obj_clean(lv_screen_active());gui_create(request);
 puts("PASS real LVGL read/hit-test/release pipeline wake over Clear/Validate/slot/mode");
}
#include "cleanup_tests.inc"
#include "safety_cleanup_tests.inc"
int main(void){
    lv_init();lv_display_t *d=lv_display_create(480,800);assert(d);
    lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);lv_display_set_flush_cb(d,flush);
    lv_display_set_buffers(d,buffer,NULL,sizeof buffer,LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_indev_t *dev=lv_indev_create();lv_indev_set_type(dev,LV_INDEV_TYPE_POINTER);lv_indev_set_read_cb(dev,touch_read);
    gui_create(request);render("empty-native.ppm");
    cleanup_tests(dev);safety_cleanup_tests();if(getenv("CLEANUP_ONLY")){lv_deinit();return 0;}
    saver_tests();saver_input_tests(dev);if(getenv("SAVER_ONLY")){lv_deinit();return 0;}
    layout_tests();if(getenv("LAYOUT_ONLY")){lv_deinit();return 0;}
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
    click("Calculate");r=compute();r.rc=-3;gui_result(&r);render("failure-native.ppm");assert(!find(lv_screen_active(),r.fingerprint));click("Back to hex input");click("Calculate");r=compute();gui_result(&r);render(NULL);
    printf("PASS: keys 0-F, boundaries 1..65, owned copy, busy, invalidation/navigation, failure/retry, 12/15/18/21/24 real core words, sentinels; %u geometry checks\n",checked);
    coin_tests();dice_tests();
    prefix_tests();if(getenv("PREFIX_ONLY")){lv_deinit();return 0;}
    click("Words");assert(lv_obj_has_state(find_button(lv_screen_active(),"Words"),LV_STATE_CHECKED));render("mnemonic-empty.ppm");
    for(int i=0;i<11;i++)type_word("abandon");
    type_word("about");
    click("Validate");assert(owned.mode==MODE_MNEMONIC && owned.length==93);
    r=compute();gui_result(&r);render("mnemonic-result.ppm");
    assert(find(lv_screen_active(),r.fingerprint));assert(!find(lv_screen_active(),r.entropy));
    click("View entropy");render("mnemonic-reveal.ppm");assert(find(lv_screen_active(),r.entropy));
    puts("PASS native mnemonic keyboard -> owned worker -> hidden/revealed entropy");
    mnemonic_tests();acceptance_tests();lv_deinit();return 0;
}
