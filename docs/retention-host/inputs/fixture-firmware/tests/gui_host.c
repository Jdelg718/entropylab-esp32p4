/* Real LVGL renderer/navigation test. Public test strings only. Not hardware evidence. */
#include "gui.h"
#include "lvgl.h"
#include "compute.h"
#include "lifehash_fingerprint.h"
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
    /* Raw layout containment MUST precede viewport clipping, even offscreen.
     * Only a direct vertical scroll container may extend its content in Y;
     * scrolling an ancestor never excuses overflow in a fixed result box. */
    if(lv_obj_check_type(o,&lv_label_class)){
        lv_obj_t *p=lv_obj_get_parent(o);lv_area_t b;lv_obj_get_content_coords(p,&b);
        bool vertical=lv_obj_has_flag(p,LV_OBJ_FLAG_SCROLLABLE)&&lv_obj_get_scroll_dir(p)==LV_DIR_VER;
        if(a.x1<b.x1||a.x2>b.x2||(!vertical&&(a.y1<b.y1||a.y2>b.y2))){
            fprintf(stderr,"RAW_PARENT_FAIL '%s' %d,%d-%d,%d parent %d,%d-%d,%d\n",lv_label_get_text(o),a.x1,a.y1,a.x2,a.y2,b.x1,b.y1,b.x2,b.y2);abort();
        }
    }
    bool visible=true;
    /* Scroll children can be intentionally offscreen; inspect dimensions even
     * then, but viewport geometry only applies to their visible intersection. */
    if(lv_obj_check_type(o,&lv_button_class)){assert(lv_obj_get_width(o)>=44);assert(lv_obj_get_height(o)>=44);}
    for(lv_obj_t *p=lv_obj_get_parent(o);p;p=lv_obj_get_parent(p)){
        if(lv_obj_has_flag(p,LV_OBJ_FLAG_SCROLLABLE)&&lv_obj_get_scroll_dir(p)==LV_DIR_VER){
            lv_area_t clip;lv_obj_get_content_coords(p,&clip);
            if(a.y2<clip.y1||a.y1>clip.y2){visible=false;break;}
            if(a.y1<clip.y1)a.y1=clip.y1;
            if(a.y2>clip.y2)a.y2=clip.y2;
        }
    }
    if(visible&&(a.x1<0||a.y1<0||a.x2>=480||a.y2>=800)){fprintf(stderr,"OOB %d,%d-%d,%d\n",a.x1,a.y1,a.x2,a.y2);abort();}
    if(lv_obj_check_type(o,&lv_button_class))checked++;
    for(uint32_t i=0;i<lv_obj_get_child_count(o);i++)geometry(lv_obj_get_child(o,i));
}
static lv_obj_t *find(lv_obj_t *o,const char *s){
    if(lv_obj_has_flag(o,LV_OBJ_FLAG_HIDDEN))return NULL;
    if(lv_obj_check_type(o,&lv_label_class)&&strcmp(lv_label_get_text(o),s)==0)return lv_obj_get_parent(o);
    for(uint32_t i=0;i<lv_obj_get_child_count(o);i++){lv_obj_t *r=find(lv_obj_get_child(o,i),s);if(r)return r;}return NULL;
}
static lv_obj_t *find_label_object(lv_obj_t *o,const char *s){
    if(lv_obj_has_flag(o,LV_OBJ_FLAG_HIDDEN))return NULL;
    if(lv_obj_check_type(o,&lv_label_class)&&strcmp(lv_label_get_text(o),s)==0)return o;
    for(uint32_t i=0;i<lv_obj_get_child_count(o);i++){lv_obj_t *r=find_label_object(lv_obj_get_child(o,i),s);if(r)return r;}return NULL;
}
static lv_obj_t *find_visible_image(lv_obj_t *o){
    if(lv_obj_has_flag(o,LV_OBJ_FLAG_HIDDEN))return NULL;
    if(lv_obj_check_type(o,&lv_image_class))return o;
    for(uint32_t i=0;i<lv_obj_get_child_count(o);i++){lv_obj_t *r=find_visible_image(lv_obj_get_child(o,i));if(r)return r;}return NULL;
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
static void render(const char *name);
static void click(const char *s){
 const char *routes[]={"Hex","Coins","D6 raw","D6 6->0","Words","Seed","Cards","Bases","Dice+","D++ D8/D16"};
 for(unsigned i=0;i<10;i++)if(strlen(s)==strlen(routes[i])&&!memcmp(s,routes[i],strlen(s))){lv_obj_send_event(nav_family,LV_EVENT_CLICKED,NULL);
  unsigned row=i==0?0:i==1?1:(i==2||i==3||i>=8)?2:i-1;
  lv_obj_send_event(nav_rows[row],LV_EVENT_CLICKED,NULL);
  if(nav_confirming&&nav_open)lv_obj_send_event(nav_rows[1],LV_EVENT_CLICKED,NULL);
  if(i==2||i==3||i>=8){lv_obj_send_event(nav_method,LV_EVENT_CLICKED,NULL);lv_obj_send_event(nav_rows[i==2?0:i==3?1:i==8?2:3],LV_EVENT_CLICKED,NULL);if(nav_confirming&&nav_open)lv_obj_send_event(nav_rows[1],LV_EVENT_CLICKED,NULL);}
  assert(nav_route==i);return;}
 lv_obj_t *b=find_button(lv_screen_active(),s);if(!b||lv_obj_has_state(b,LV_STATE_DISABLED))fprintf(stderr,"click target unavailable: %s (found=%d disabled=%d)\n",s,b!=NULL,b?lv_obj_has_state(b,LV_STATE_DISABLED):-1);assert(b);assert(!lv_obj_has_state(b,LV_STATE_DISABLED));lv_obj_send_event(b,LV_EVENT_CLICKED,NULL);}
static void reset_legacy_input(void){
 lv_obj_t *b=find_button(lv_screen_active(),"Clear");assert(b);
 bool empty=lengths[mode]==0;
 assert(lv_obj_has_state(b,LV_STATE_DISABLED)==empty);
 lv_obj_send_event(b,LV_EVENT_CLICKED,NULL);assert(lengths[mode]==0);
}
static void roll_face(unsigned face){
 assert(el_mode_is_dice(mode)&&face>=1&&face<=6);
 lv_obj_t *b=dicekeys[face-1];assert(b&&!lv_obj_has_state(b,LV_STATE_DISABLED));
 lv_obj_send_event(b,LV_EVENT_CLICKED,NULL);
}
static gui08_request dice_visual_request(void){
 gui08_request r={0};extra_dice_native *n=&extra_dice_ui;
 r.request_id=++n->editor.request_serial;if(!r.request_id)r.request_id=++n->editor.request_serial;
 r.revision=n->editor.revision;r.method=GUI08_DICE;r.words=n->editor.words;
 r.dice_method=n->method;r.dice_final_choice=n->final_choice;r.dice_final_length=n->final_length;
 memcpy(r.dice_final,n->final_roll,n->final_length+1);r.length=n->editor.length;
 memcpy(r.transcript,n->editor.transcript,n->editor.length+1);r.passphrase_marker=GUI08_PASSPHRASE_EMPTY;
 return r;
}
static void publish_dice_visual_result(const char fingerprint[9]){
 gui08_request r=dice_visual_request();static uint8_t rgb[FINGERPRINT_VIEW_RGB_SIZE];
 for(size_t i=0;i<sizeof rgb;++i)rgb[i]=(uint8_t)(i*37u+11u);
 assert(extra_dice_native_pending(&extra_dice_ui,&r));
 assert(extra_dice_native_accept(&extra_dice_ui,&r));
 assert(extra_dice_native_lifehash(&extra_dice_ui,&r,fingerprint,rgb,sizeof rgb));
}
static void d6_indexed_cell_regression(void){
 lv_obj_clean(lv_screen_active());gui_create(request);click("D6 raw");
 assert(d6_cell_for_index(1)==NULL);
 for(size_t expected=1;expected<=40;expected++){
  lv_obj_send_event(dicekeys[5],LV_EVENT_CLICKED,NULL);
  assert(lengths[mode]==expected);assert(d6_cell_for_index(expected)!=NULL);
  assert(d6_cell_for_index(expected+1)==NULL);
  if(expected==1)assert(d6_cell_for_index(expected)==d6_cells[0]);
 }
 size_t roll_count=lengths[mode];assert(roll_count==40);
 d6_make_visible(roll_count);
 assert(d6_cell_for_index(roll_count)!=NULL);assert(d6_cell_for_index(roll_count+1)==NULL);
 size_t first=d6_cell_index[0];assert(first>1);
 assert(d6_cell_for_index(first-1)==NULL);assert(d6_cell_for_index(first)==d6_cells[0]);
 puts("PASS D6 indexed cell lookup: empty, initial pool edges, virtualized first/last boundaries");
}
static void integrated_dice_visuals(void){
 lv_obj_clean(lv_screen_active());gui_create(request);click("Dice+");
 assert(lv_obj_has_state(gui08_routes[3],LV_STATE_CHECKED));
 assert(!lv_obj_has_state(gui08_routes[0],LV_STATE_CHECKED));
 assert(find(lv_screen_active(),"LifeHash unavailable - no published result."));
 render("visual44-integrated-dice-blank.ppm");
 click("Load public zero");click("Use final #1");render("visual44-integrated-bitbox-selection.ppm");
 publish_dice_visual_result("73c5da0a");render("visual44-integrated-bitbox-result.ppm");
 click("Clear");click("D++ D8/D16");click("Load public zero");click("1");click("0");
 render("visual44-integrated-dplus-selection.ppm");publish_dice_visual_result("12345678");
 render("visual44-integrated-dplus-result.ppm");
 click("Clear");click("Load public zero");click("1");click("0");
 gui08_request failed=dice_visual_request();assert(extra_dice_native_pending(&extra_dice_ui,&failed));
 assert(extra_dice_native_cancelled(&extra_dice_ui,failed.request_id));
 assert(find(lv_screen_active(),"LifeHash failed - no image was published."));
 render("visual44-integrated-lifehash-failed.ppm");
}
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
 lv_obj_t *targets[]={mn_clear,mn_validate,mn_slots[0],nav_family};
 for(unsigned i=0;i<4;i++){
  void *oldnav=saved_callback(nav_family,nav_event);
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
  replay_callback(nav_event,oldnav);assert(!nav_open);
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
#include "global_saver_tests.inc"
#include "modal_touch_tests.inc"
#include "education_tests.inc"
#include "cleanup_tests.inc"
#include "safety_cleanup_tests.inc"
#include "explanation_tests.inc"
#include "passphrase_gui_tests.inc"
#include "passphrase_integration_tests.inc"
#include "seed_panel_tests.inc"
static unsigned reflow_cancel_calls;
static uint64_t reflow_cancel_id;
static bool reflow_cancel(uint64_t request_id){reflow_cancel_calls++;reflow_cancel_id=request_id;return true;}
static bool areas_overlap(lv_obj_t *a,lv_obj_t *b){
    if(lv_obj_has_flag(a,LV_OBJ_FLAG_HIDDEN)||lv_obj_has_flag(b,LV_OBJ_FLAG_HIDDEN))return false;
    lv_area_t x,y;lv_obj_get_coords(a,&x);lv_obj_get_coords(b,&y);
    return !(x.x2<y.x1||y.x2<x.x1||x.y2<y.y1||y.y2<x.y1);
}
static void assert_label_unclipped(lv_obj_t *label){int32_t need=lv_obj_get_self_height(label),have=lv_obj_get_content_height(label);if(need>have)fprintf(stderr,"CLIPPED text='%s' need=%d have=%d\n",lv_label_get_text(label),(int)need,(int)have);assert(need<=have);}
static void reflow_contract(unsigned route,const char *state,const char *capture){
    lv_obj_update_layout(lv_screen_active());
    assert(lv_obj_has_state(gui08_routes[route],LV_STATE_CHECKED));
    for(unsigned i=0;i<4;i++)assert(lv_obj_has_state(gui08_routes[i],LV_STATE_CHECKED)==(i==route));
    assert(gui08_native_visible(&gui08_ui));
    assert(find_label_object(lv_screen_active(),"Direct rank / canonical bases"));
    lv_obj_t *notice=find_label_object(lv_screen_active(),"Public TEST input only / NEVER fund. Use the global Safety / About control.");
    assert(!notice&&gui08_ui.result&&gui08_ui.fingerprint.label&&gui08_ui.fingerprint.explanation&&gui08_ui.fingerprint.canvas);
    lv_obj_t *logo=find_visible_image(app_content);assert(logo);
    lv_area_t logo_area,title_area,meta_area,route_area;
    lv_obj_get_coords(logo,&logo_area);lv_obj_get_coords(header_title,&title_area);lv_obj_get_coords(header_meta,&meta_area);lv_obj_get_coords(gui08_routes[0],&route_area);
    assert(lv_obj_get_width(logo)==20&&lv_obj_get_height(logo)==30);
    assert(logo_area.x1>=16&&logo_area.y1>=7&&logo_area.x2<title_area.x1&&logo_area.x2<meta_area.x1&&logo_area.y2<route_area.y1);
    assert_label_unclipped(gui08_ui.status);assert_label_unclipped(gui08_ui.transcript);
    assert_label_unclipped(gui08_ui.result);
    assert_label_unclipped(gui08_ui.fingerprint.label);assert_label_unclipped(gui08_ui.fingerprint.explanation);
    lv_obj_t *lower[]={gui08_ui.result,gui08_ui.fingerprint.label,gui08_ui.fingerprint.explanation,gui08_ui.fingerprint.canvas};
    for(unsigned i=0;i<4;i++)for(unsigned j=i+1;j<4;j++)assert(!areas_overlap(lower[i],lower[j]));
    if(!strcmp(state,"success")){
        assert(!strcmp(lv_label_get_text(gui08_ui.result),"Test result accepted for this exact input."));
        assert(!strncmp(lv_label_get_text(gui08_ui.fingerprint.label),"Fingerprint ",12));
        assert(!strcmp(lv_label_get_text(gui08_ui.fingerprint.explanation),"LifeHash version2 - display only, never input."));
        assert(!lv_obj_has_flag(gui08_ui.fingerprint.canvas,LV_OBJ_FLAG_HIDDEN));
    }else{
        assert(!strcmp(lv_label_get_text(gui08_ui.result),"No result."));
        assert(!strcmp(lv_label_get_text(gui08_ui.fingerprint.label),"Fingerprint --------"));
        assert(lv_obj_has_flag(gui08_ui.fingerprint.canvas,LV_OBJ_FLAG_HIDDEN));
    }
    lv_obj_update_layout(lv_screen_active());geometry(lv_screen_active());render(capture);
    fprintf(stderr,"REFLOW52_GREEN route=%s state=%s one-only=1 native-visible=1 raw-parent=1 controls44=1 lower-nonoverlap=1 unclipped=1\n",route?"Bases":"Cards",state);
}
static void fill_reflow_input(unsigned route){
    const char *symbol=route?"0":"A";
    while(!gui08_ready(&gui08_ui.editor)){
        lv_obj_t *button=NULL;
        for(unsigned i=0;i<26;i++){
            lv_obj_t *candidate=gui08_ui.symbol_buttons[i];
            if(candidate&&!lv_obj_has_flag(candidate,LV_OBJ_FLAG_HIDDEN)&&!strcmp(lv_label_get_text(lv_obj_get_child(candidate,0)),symbol)){button=candidate;break;}
        }
        assert(button&&!lv_obj_has_state(button,LV_STATE_DISABLED));
        lv_obj_send_event(button,LV_EVENT_CLICKED,NULL);
    }
}
static gui08_request start_reflow_request(gui08_passphrase_marker marker){
    gui08_request context=gui08_begin(&gui08_ui.editor);assert(context.request_id);
    context.passphrase_marker=marker;assert(gui_cards_bases_pending(&context));return context;
}
static void cards_bases_reflow_matrix(void){
    static uint8_t rgb[FINGERPRINT_VIEW_RGB_SIZE];for(size_t i=0;i<sizeof rgb;i++)rgb[i]=(uint8_t)(i*29u+7u);
    lv_obj_clean(lv_screen_active());gui_create(request);gui_cards_bases_configure(NULL,reflow_cancel);
    for(unsigned route=0;route<2;route++){
        const char *prefix=route?"reflow52-bases":"reflow52-cards";char capture[96];
        lv_obj_send_event(gui08_routes[route],LV_EVENT_CLICKED,NULL);
        snprintf(capture,sizeof capture,"%s-blank.ppm",prefix);reflow_contract(route,"blank",capture);
        lv_obj_send_event(mn_safety,LV_EVENT_CLICKED,NULL);assert(mn_safety_open);
        snprintf(capture,sizeof capture,"%s-global-safety.ppm",prefix);render(capture);click("Close");
        fill_reflow_input(route);
        gui08_request success=start_reflow_request(route?GUI08_PASSPHRASE_EMPTY:GUI08_PASSPHRASE_ACTIVE);
        assert(strstr(lv_label_get_text(gui08_ui.status),route?"Passphrase Empty":"Passphrase Active"));
        snprintf(capture,sizeof capture,"%s-pending.ppm",prefix);reflow_contract(route,"pending",capture);
        assert(gui_cards_bases_result(&success));assert(gui_cards_bases_lifehash(&success,route?"b45e5a5e":"ca4d5eed",rgb,sizeof rgb));
        snprintf(capture,sizeof capture,"%s-success.ppm",prefix);reflow_contract(route,"success",capture);
        lv_obj_send_event(gui08_ui.clear,LV_EVENT_CLICKED,NULL);
        assert(!gui_cards_bases_result(&success));assert(!gui_cards_bases_lifehash(&success,"deadbeef",rgb,sizeof rgb));
        assert(!strcmp(lv_label_get_text(gui08_ui.fingerprint.explanation),"LifeHash unavailable - no published result."));
        snprintf(capture,sizeof capture,"%s-stale-cleared.ppm",prefix);reflow_contract(route,"stale-cleared",capture);
        fill_reflow_input(route);gui08_request failed=start_reflow_request(GUI08_PASSPHRASE_EMPTY);
        assert(gui_cards_bases_cancelled(failed.request_id));assert(strstr(lv_label_get_text(gui08_ui.status),"failed"));
        snprintf(capture,sizeof capture,"%s-failure.ppm",prefix);reflow_contract(route,"failure",capture);
        fill_reflow_input(route);gui08_request cancelled=start_reflow_request(GUI08_PASSPHRASE_ACTIVE);
        unsigned before=reflow_cancel_calls;lv_obj_send_event(gui08_ui.derive,LV_EVENT_CLICKED,NULL);
        assert(reflow_cancel_calls==before+1&&reflow_cancel_id==cancelled.request_id);
        assert(strstr(lv_label_get_text(gui08_ui.status),"Cancelled"));
        snprintf(capture,sizeof capture,"%s-cancellation.ppm",prefix);reflow_contract(route,"cancellation",capture);
        lv_obj_send_event(gui08_ui.clear,LV_EVENT_CLICKED,NULL);
    }
    puts("PASS reflow52 actual gui_create Cards/Bases native state matrix");
}
int main(void){
    lv_init();lv_display_t *d=lv_display_create(480,800);assert(d);
    lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);lv_display_set_flush_cb(d,flush);
    lv_display_set_buffers(d,buffer,NULL,sizeof buffer,LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_indev_t *dev=lv_indev_create();lv_indev_set_type(dev,LV_INDEV_TYPE_POINTER);lv_indev_set_read_cb(dev,touch_read);
    gui_create(request);if(getenv("EDUCATION_ONLY")){education_matrix(dev);lv_deinit();return 0;}if(getenv("MODAL_TOUCH_ONLY")){modal_touch_tests(dev);lv_deinit();return 0;}if(getenv("CARDS_BASES_REFLOW_ONLY")){cards_bases_reflow_matrix();lv_deinit();return 0;}d6_indexed_cell_regression();if(getenv("D6_CELL_ONLY")){lv_deinit();return 0;}integrated_dice_visuals();lv_obj_clean(lv_screen_active());gui_create(request);passphrase_gui_tests();passphrase_integration_tests();seed_panel_event_test();if(getenv("SEED_PANEL_ONLY")){lv_deinit();return 0;}if(getenv("PASSPHRASE_ONLY")){lv_deinit();return 0;}render("empty-native.ppm");
    explanation_tests(dev);if(getenv("EXPLANATION_ONLY")){lv_deinit();return 0;}
    cleanup_tests(dev);safety_cleanup_tests();if(getenv("CLEANUP_ONLY")){lv_deinit();return 0;}
    global_saver_tests(dev);saver_tests();saver_input_tests(dev);if(getenv("SAVER_ONLY")){lv_deinit();return 0;}
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
    assert(lv_obj_has_state(tabs[1],LV_STATE_DISABLED));lv_obj_send_event(tabs[1],LV_EVENT_CLICKED,NULL);
    assert(lv_obj_has_state(find_button(lv_screen_active(),"Clear"),LV_STATE_DISABLED));
    assert(lv_obj_has_state(tabs[0],LV_STATE_DISABLED));lv_obj_send_event(tabs[0],LV_EVENT_CLICKED,NULL);
    unsigned before=runs;lv_obj_send_event(find_button(lv_screen_active(),"Calculate"),LV_EVENT_CLICKED,NULL);assert(runs==before);
    hex_request_t snapshot=owned;
    const char *blocked[]={"Clear","Delete","A","Load public zero"};
    for(unsigned i=0;i<sizeof blocked/sizeof blocked[0];i++)
        lv_obj_send_event(find_button(lv_screen_active(),blocked[i]),LV_EVENT_CLICKED,NULL);
    assert(!memcmp(&snapshot,&owned,sizeof owned));
    assert(find(lv_screen_active(),"64 / 64 chars  |  256 bits"));
    assert(lv_obj_has_state(tabs[1],LV_STATE_DISABLED));lv_obj_send_event(tabs[1],LV_EVENT_CLICKED,NULL);
    lv_obj_send_event(find_button(lv_screen_active(),"Clear"),LV_EVENT_CLICKED,NULL);
    assert(lv_obj_has_state(tabs[0],LV_STATE_DISABLED));lv_obj_send_event(tabs[0],LV_EVENT_CLICKED,NULL);
    assert(find(lv_screen_active(),"64 / 64 chars  |  256 bits"));
    hex_result_t r=compute();gui_result(&r);render("results-native.ppm");
    assert_words(&r);assert(find(lv_screen_active(),r.fingerprint));
    click("Back to hex input");click("Delete");click("Test results");assert(!find(lv_screen_active(),r.fingerprint));render(NULL);
    click("Back to hex input");reset_legacy_input();
    lv_obj_t *empty_delete=find_button(lv_screen_active(),"Delete");assert(empty_delete&&lv_obj_has_state(empty_delete,LV_STATE_DISABLED));
    lv_obj_send_event(empty_delete,LV_EVENT_CLICKED,NULL);assert(lengths[MODE_HEX]==0);
    assert(find(lv_screen_active(),"0 / 64 chars  |  0 bits"));
    for(int n=32;n<=64;n+=8){
        reset_legacy_input();for(int i=0;i<n;i++)click("F");click("Calculate");r=compute();gui_result(&r);render(NULL);assert_words(&r);assert(find(lv_screen_active(),r.fingerprint));click("Back to hex input");
    }
    click("Load public zero");click("Calculate");r=compute();gui_result(&r);render("zero24-native.ppm");assert(find(lv_screen_active(),"art"));
    click("Clear");click("Test results");assert(!find(lv_screen_active(),r.fingerprint));click("Back to hex input");
    click("Load public zero");reject=true;click("Calculate");assert(!lv_obj_has_state(find(lv_screen_active(),"Calculate"),LV_STATE_DISABLED));reject=false;
    click("Calculate");r=compute();r.rc=-3;gui_result(&r);render("failure-native.ppm");assert(!find(lv_screen_active(),r.fingerprint));click("Back to hex input");click("Calculate");r=compute();gui_result(&r);render(NULL);
    printf("PASS: keys 0-F, boundaries 1..65, owned copy, busy, invalidation/navigation, failure/retry, 12/15/18/21/24 real core words, sentinels; %u geometry checks\n",checked);
    coin_tests();dice_tests();
    prefix_tests();if(getenv("PREFIX_ONLY")){lv_deinit();return 0;}
    click("Words");assert(nav_route==4&&mode==MODE_MNEMONIC);render("mnemonic-empty.ppm");
    for(int i=0;i<11;i++)type_word("abandon");
    type_word("about");
    click("Validate");assert(owned.mode==MODE_MNEMONIC && owned.length==93);
    r=compute();gui_result(&r);render("mnemonic-result.ppm");
    assert(find(lv_screen_active(),r.fingerprint));assert(!find(lv_screen_active(),r.entropy));
    click("View entropy");render("mnemonic-reveal.ppm");assert(find(lv_screen_active(),r.entropy));
    puts("PASS native mnemonic keyboard -> owned worker -> hidden/revealed entropy");
    mnemonic_tests();acceptance_tests();lv_deinit();return 0;
}
