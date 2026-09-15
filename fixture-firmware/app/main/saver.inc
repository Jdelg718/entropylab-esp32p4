/* Option A display concealment, LVGL context only; NOT a lock/wipe.
 * Static licensed logo plus fixed 12-second decorative orbit. */
#include "assets/logo_rgb565a8.inc"
static lv_obj_t *app_content,*saver_screen;
static lv_timer_t *saver_timer;
static lv_obj_t *saver_dot,*saver_trail;
static uint32_t saver_motion_last,saver_phase;
/* Immutable logo/ring; only tiny marker and short native arc change. No RNG,
 * per-tick allocation, image transform, canvas, framebuffer, or extra task. */
static void saver_orbit_position(void){
 unsigned angle=(saver_phase*360u/12000u+270u)%360u;
 int x=240+(176*lv_trigo_sin((angle+90u)%360u))/32768;
 int y=360+(176*lv_trigo_sin(angle))/32768;
 lv_obj_set_pos(saver_dot,x-3,y-3);
 lv_arc_set_angles(saver_trail,(angle+345u)%360u,angle);
}
static bool saver_active,wake_latch,saver_eligible_last;
static uint32_t saver_since;
static uintptr_t saver_epoch;
static bool saver_epoch_exhausted;
static void saver_rebind_controls(void);
static bool saver_event_blocked(lv_event_t *e){
 return saver_blocked()||mn_safety_open||saver_epoch_exhausted||((uintptr_t)lv_event_get_user_data(e)>>10)!=saver_epoch;
}
static uintptr_t saver_payload(lv_event_t *e){return (uintptr_t)lv_event_get_user_data(e)&1023u;}
static lv_indev_t *saver_indev;
static lv_indev_read_cb_t saver_read_original;
static bool saver_finger,saver_drained,saver_scrolling;
static void saver_scroll_event(lv_event_t *e){
 if(saver_blocked())return;
 saver_scrolling=lv_event_get_code(e)==LV_EVENT_SCROLL_BEGIN;
}
static void saver_activity(void){saver_eligible_last=false;}
static void saver_screen_event(lv_event_t *e){
 lv_event_code_t code=lv_event_get_code(e);
 if(code==LV_EVENT_SCREEN_UNLOAD_START||code==LV_EVENT_SCREEN_LOAD_START||code==LV_EVENT_SCREEN_LOADED||code==LV_EVENT_SCREEN_UNLOADED)saver_activity();
}
static bool saver_blocked(void){saver_activity();return saver_active||wake_latch;}
/* LVGL 9.5 lv_indev.c: read callback precedes reset query and pointer hit-test.
 * continue_reading is a driver backlog hint, NOT a universal queue-drain API.
 * Keep the latch AFTER terminal release until the next distinct DOWN; all
 * synthetic callbacks in between still fail. Reset removes LVGL press history.
 * Single aggregate pointer only: multi-contact truth belongs to its driver. */
static void saver_read(lv_indev_t *dev,lv_indev_data_t *data){
 saver_read_original(dev,data);
 bool down=data->state==LV_INDEV_STATE_PRESSED;
 if(down||saver_finger)saver_activity();
 saver_finger=down;
 if(saver_active&&down){
  wake_latch=true;saver_drained=false;saver_active=false;
  lv_obj_add_flag(saver_screen,LV_OBJ_FLAG_HIDDEN);
  lv_obj_remove_flag(app_content,LV_OBJ_FLAG_HIDDEN);
 }
 if(wake_latch){
  if(down&&saver_drained){
   lv_indev_reset(dev,NULL);saver_rebind_controls();mn_refresh();wake_latch=false;saver_drained=false;
  }else{
   saver_drained=!down&&!data->continue_reading;
   lv_indev_reset(dev,NULL);data->state=LV_INDEV_STATE_RELEASED;
  }
 }
}
static void saver_tick(lv_timer_t *timer){
 (void)timer;
 if(saver_active){
  uint32_t now=lv_tick_get(),elapsed=(uint32_t)(now-saver_motion_last);
  if(elapsed>=100){saver_phase=(saver_phase+elapsed%12000u)%12000u;saver_motion_last=now;saver_orbit_position();}
  return;
 }
 if(wake_latch)return;
 lv_indev_t *device=lv_indev_get_next(NULL);
 if(lv_screen_active()!=lv_obj_get_screen(app_content)||lv_display_get_screen_loading(NULL)||lv_display_get_screen_prev(NULL)||!device||device!=saver_indev||lv_indev_get_next(device)||lv_indev_get_read_cb(device)!=saver_read||saver_scrolling||lv_obj_is_scrolling(mn_list)||saver_finger||mode!=MODE_MNEMONIC||busy||mn_confirm||mn_safety_open||mn_reviewing||mn_valid||mn_draft[0]||mn_edit>=0||mn_settling||!lv_obj_has_flag(mn_output,LV_OBJ_FLAG_HIDDEN)){saver_eligible_last=false;return;}
 uint32_t now=lv_tick_get();
 if(!saver_eligible_last){saver_since=now;saver_eligible_last=true;return;}
 if((uint32_t)(now-saver_since)<60000)return;
 saver_active=true;saver_rebind_controls();mn_refresh();
 saver_phase=0;saver_motion_last=now;saver_orbit_position();
 lv_obj_add_flag(app_content,LV_OBJ_FLAG_HIDDEN);
 lv_obj_remove_flag(saver_screen,LV_OBJ_FLAG_HIDDEN);
}
static void saver_create(lv_obj_t *root){
 lv_obj_remove_event_cb(root,saver_screen_event);lv_obj_add_event_cb(root,saver_screen_event,LV_EVENT_ALL,NULL);
 saver_active=false;wake_latch=false;saver_eligible_last=false;saver_finger=false;saver_drained=false;saver_scrolling=false;
 lv_indev_t *dev=lv_indev_get_next(NULL);
 if(dev&&!lv_indev_get_next(dev)&&lv_indev_get_type(dev)==LV_INDEV_TYPE_POINTER&&lv_indev_get_read_cb(dev)){
  if(lv_indev_get_read_cb(dev)!=saver_read){saver_indev=dev;saver_read_original=lv_indev_get_read_cb(dev);lv_indev_set_read_cb(dev,saver_read);}
 }

 lv_obj_add_event_cb(mn_list,saver_scroll_event,LV_EVENT_SCROLL_BEGIN,NULL);
 lv_obj_add_event_cb(mn_list,saver_scroll_event,LV_EVENT_SCROLL_END,NULL);
 saver_screen=box(root,0,0,480,800,&base);
 lv_obj_t *ring=box(saver_screen,64,184,352,352,&base);lv_obj_remove_flag(ring,LV_OBJ_FLAG_CLICKABLE);lv_obj_set_style_radius(ring,LV_RADIUS_CIRCLE,0);lv_obj_set_style_border_width(ring,1,0);lv_obj_set_style_border_color(ring,lv_color_hex(0x282828),0);
 saver_trail=lv_arc_create(saver_screen);lv_obj_remove_style_all(saver_trail);lv_obj_set_pos(saver_trail,64,184);lv_obj_set_size(saver_trail,352,352);lv_obj_remove_flag(saver_trail,LV_OBJ_FLAG_CLICKABLE);lv_obj_set_style_arc_opa(saver_trail,LV_OPA_TRANSP,LV_PART_MAIN);lv_obj_set_style_arc_opa(saver_trail,LV_OPA_COVER,LV_PART_INDICATOR);lv_obj_set_style_arc_color(saver_trail,lv_color_hex(0x865b27),LV_PART_INDICATOR);lv_obj_set_style_arc_width(saver_trail,2,LV_PART_INDICATOR);
 saver_dot=box(saver_screen,237,181,6,6,&base);lv_obj_remove_flag(saver_dot,LV_OBJ_FLAG_CLICKABLE);lv_obj_set_style_radius(saver_dot,LV_RADIUS_CIRCLE,0);lv_obj_set_style_bg_color(saver_dot,lv_color_hex(0xd8892b),0);
 lv_obj_t *logo=lv_image_create(saver_screen);lv_image_set_src(logo,&saver_logo);lv_obj_set_pos(logo,150,222);lv_obj_remove_flag(logo,LV_OBJ_FLAG_CLICKABLE);
 lv_obj_t *idle=text(saver_screen,40,144,400,"DISPLAY IDLE",&el_sans_14,0x737373);lv_obj_set_style_text_align(idle,LV_TEXT_ALIGN_CENTER,0);lv_obj_set_style_text_letter_space(idle,2,0);
 lv_obj_t *brand=text(saver_screen,40,560,400,"EntropyLab",&el_serif_24,0xeeeeee);lv_obj_set_style_text_align(brand,LV_TEXT_ALIGN_CENTER,0);
 lv_obj_t *hint=text(saver_screen,40,704,400,"Touch to return",&el_sans_14,0xa3a3a3);lv_obj_set_style_text_align(hint,LV_TEXT_ALIGN_CENTER,0);
 lv_obj_add_flag(saver_screen,LV_OBJ_FLAG_HIDDEN);
 saver_rebind_controls();saver_timer=lv_timer_create(saver_tick,100,NULL);
}
