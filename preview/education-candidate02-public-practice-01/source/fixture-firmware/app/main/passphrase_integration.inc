/* Stationary application-lifetime slot/clock. GUI APIs require display serialization.
 * Worker entry uses only transition locking and compute, never LVGL. */
#include "passphrase_dispatch.h"
#include "compute.h"
#include "passphrase_gui.h"
static void pi_completed_invalidate(passphrase_ui_t *u);
#define PP_INVALIDATE_OWNER(u) pi_completed_invalidate(u)
#include "passphrase_gui.inc"
#undef PP_INVALIDATE_OWNER
static passphrase_ui_t pi_ui;
/* GUI/display-serialized owner, never aliased by worker transfer storage. */
static struct {
 hex_result_t full;
 passphrase_ui_completion_t identity;
 uint8_t rgb[FINGERPRINT_VIEW_RGB_SIZE];
 int lifehash_status;
 bool valid;
} pi_completed;
static void pi_completed_invalidate(passphrase_ui_t *u){
 if(u==&pi_ui)el_dispatch_clear(&pi_completed,sizeof pi_completed);
}
static lv_obj_t *pi_safety_dock;
static el_dispatch_t pi_dispatch;
static el_dispatch_clock_t pi_clock;
static void (*pi_lock)(void),(*pi_unlock)(void);
static bool (*pi_publish)(uint64_t);
static el_dispatch_meta_t pi_pending;
static uint64_t pi_token,pi_revision;
static bool pi_exhausted;
/* Full result sidecar shares locked slot/token ownership. */
static hex_result_t pi_full_result;
static uint64_t pi_full_token;
static gui_passphrase_lifehash_render_fn pi_lifehash_render;
static uint8_t pi_lifehash_rgb[FINGERPRINT_VIEW_RGB_SIZE];
static uint64_t pi_lifehash_token;
static int pi_lifehash_status=-1;
static lv_obj_t *pi_mnemonic_label,*pi_entropy_label;
typedef enum { PI_OWNER_NONE=0, PI_OWNER_LEGACY, PI_OWNER_GUI08 } pi_owner_t;
static pi_owner_t pi_owner;
static gui08_request pi_gui08_source;
static bool pi_gui08_source_matches(void){
 return feature_source_matches(&pi_gui08_source);
}
static passphrase_ui_context_t pi_gui08_context(const gui08_request *r){
 passphrase_ui_context_t c={0x80000000u|(uint32_t)r->method,r->revision,r->words,
  r->method==GUI08_SEED?r->seed_operation:r->base};
 return c;
}
static void pi_lifehash_clear(void){
 el_dispatch_clear(pi_lifehash_rgb,sizeof pi_lifehash_rgb);pi_lifehash_token=0;pi_lifehash_status=-1;
}
static int pi_hex_nibble(char c){
 if(c>='0'&&c<='9')return c-'0';
 if(c>='a'&&c<='f')return c-'a'+10;
 if(c>='A'&&c<='F')return c-'A'+10;
 return -1;
}
static bool pi_fingerprint_raw4(const char fingerprint[9],uint8_t raw4[4]){
 if(!fingerprint||!raw4||fingerprint[8]!='\0')return false;
 for(size_t i=0;i<4;i++){int hi=pi_hex_nibble(fingerprint[i*2]),lo=pi_hex_nibble(fingerprint[i*2+1]);if(hi<0||lo<0)return false;raw4[i]=(uint8_t)((hi<<4)|lo);}return true;
}
static void pi_source_changed(void){
 if(!pi_lock)return;
 pi_lock();el_dispatch_detach(&pi_dispatch);el_dispatch_clear(&pi_full_result,sizeof pi_full_result);pi_full_token=0;pi_lifehash_clear();(void)el_dispatch_open(&pi_dispatch,&pi_clock);pi_unlock();
 if(pi_revision==UINT64_MAX)pi_exhausted=true;else pi_revision++;
 if(pi_ui.live)passphrase_ui_cancel(&pi_ui);
}
static void pi_safety_event(lv_event_t *e){
 (void)e;if(!pi_ui.live||mn_safety_open)return;
 mn_safety_open=true;mn_about_open=false;mn_about_mode=mode;saver_rebind_controls();mn_rebind();
}
void gui_passphrase_configure(void (*lock)(void),void (*unlock)(void),bool (*publish)(uint64_t)){
 /* Configure only before workers start, or after they have been joined. */
 pi_lock=lock;pi_unlock=unlock;pi_publish=publish;
}
void gui_passphrase_lifehash_configure(gui_passphrase_lifehash_render_fn render){
 /* Configure only before workers start, or after they have been joined. */
 pi_lifehash_render=render;
}
void gui_passphrase_dispose(void){
 if(pi_lock){pi_lock();el_dispatch_detach(&pi_dispatch);el_dispatch_clear(&pi_full_result,sizeof pi_full_result);pi_full_token=0;pi_lifehash_clear();pi_unlock();}
 pi_completed_invalidate(&pi_ui);
 pi_token=0;el_dispatch_clear(&pi_pending,sizeof pi_pending);
 el_dispatch_clear(&pi_gui08_source,sizeof pi_gui08_source);pi_owner=PI_OWNER_NONE;
 passphrase_ui_destroy(&pi_ui);pi_safety_dock=NULL;
}
gui08_passphrase_marker gui_passphrase_snapshot(uint8_t passphrase[256],size_t *passphrase_len){
 if(!passphrase||!passphrase_len)return GUI08_PASSPHRASE_UNSPECIFIED;
 memset(passphrase,0,256);*passphrase_len=passphrase_ui_raw_len(&pi_ui);
 if(*passphrase_len)memcpy(passphrase,passphrase_ui_raw(&pi_ui),*passphrase_len);
 return *passphrase_len?GUI08_PASSPHRASE_ACTIVE:GUI08_PASSPHRASE_EMPTY;
}
static void pi_deleted(lv_event_t *e){
 lv_obj_t *target=lv_event_get_target_obj(e);
 if(target!=app_content&&target!=pi_ui.root)return;
 if(target==pi_ui.root)pi_safety_dock=NULL;
 if(pi_lock){pi_lock();el_dispatch_detach(&pi_dispatch);el_dispatch_clear(&pi_full_result,sizeof pi_full_result);pi_full_token=0;pi_lifehash_clear();pi_unlock();}
 pi_completed_invalidate(&pi_ui);
 pi_token=0;el_dispatch_clear(&pi_pending,sizeof pi_pending);
}
static bool pi_submit(const passphrase_ui_request_t *r,void *unused){
 (void)unused;el_passphrase_request_t q={0};el_dispatch_meta_t m={0};uint64_t token=0;
 if(pi_owner==PI_OWNER_GUI08){
  gui08_request owned={0};bool ok=false;
  if(!r||!request_gui08||!pi_gui08_source_matches()||
     r->raw_len>sizeof owned.passphrase||
     !pp_context_equal(r->context,pi_gui08_context(&pi_gui08_source)))return false;
  owned=pi_gui08_source;
  memcpy(owned.passphrase,r->raw,r->raw_len);owned.passphrase_len=r->raw_len;
  owned.passphrase_marker=r->marker==PASSPHRASE_UI_ACTIVE?
      GUI08_PASSPHRASE_ACTIVE:GUI08_PASSPHRASE_EMPTY;
  ok=request_gui08(&owned);
  el_dispatch_clear(&owned,sizeof owned);
  if(ok){passphrase_ui_back(&pi_ui);lv_obj_add_flag(pi_ui.root,LV_OBJ_FLAG_HIDDEN);}
  return ok;
 }
 if(!pi_lock||!pi_publish||pi_exhausted)return false;
 const char *src=mode==MODE_MNEMONIC?mnemonic_transcript:transcripts[mode];
 size_t n=mode==MODE_MNEMONIC?strlen(mnemonic_transcript):lengths[mode];
 q.source=snapshot(src,n,mode,mode==MODE_MNEMONIC?mn_target:mode==MODE_HEX?(uint32_t)(lengths[mode]*3/8):selector);
 q.source.request_id=r->request_id;q.source.revision=pi_revision;
 memcpy(q.passphrase,r->raw,sizeof q.passphrase);q.passphrase_len=r->raw_len;
 m.request_id=r->request_id;m.generation=r->generation;m.revision=r->context.source_revision;
 m.mode=r->context.mode;m.words=r->context.word_count;m.derivation_context=r->context.derivation_context;m.marker=r->marker;
 pi_lock();m.epoch=pi_dispatch.route;
 bool ok=el_dispatch_submit(&pi_dispatch,&pi_clock,&q,&m,&token);
 pi_unlock();
 /* Publish only after leaving the transition critical section. The GUI holds
  * display serialization, so no GUI cancel/re-submit can overtake this call.
  * A worker may finish immediately; token-bound polling still runs later. */
 if(ok&&!pi_publish(token)){pi_lock();el_dispatch_cancel(&pi_dispatch,token);pi_unlock();ok=false;}
 el_dispatch_clear(&q,sizeof q);
 if(ok){pi_pending=m;pi_token=token;passphrase_ui_lifehash_rendering(&pi_ui);}return ok;
}
static void pi_back_event(lv_event_t *e){
 (void)e;if(pi_owner==PI_OWNER_LEGACY&&pi_lock){pi_lock();el_dispatch_cancel(&pi_dispatch,pi_token);el_dispatch_clear(&pi_full_result,sizeof pi_full_result);pi_full_token=0;pi_lifehash_clear();pi_unlock();}
 pi_token=0;if(pi_ui.root)lv_obj_add_flag(pi_ui.root,LV_OBJ_FLAG_HIDDEN);
}
static void pi_cancel_event(lv_event_t *e){passphrase_ui_cancel(&pi_ui);pi_back_event(e);}
static bool pi_ensure_ui(void){
 if(!pi_ui.live){
  /* Parent DELETE fires before recursive child cleanup, thus route detaches
   * before the frozen owner's root DELETE callback touches children. */
  passphrase_ui_init(&pi_ui,app_content,pi_submit,NULL);
  pi_mnemonic_label=pp_text(pi_ui.result,10,160,394,"",&el_mono_16,0xeeeeee);
  pi_entropy_label=pp_text(pi_ui.result,10,360,394,"",&el_mono_16,0xa3a3a3);
  lv_obj_set_height(pi_ui.result,480);lv_obj_update_layout(pi_ui.root);pp_page(&pi_ui);
  lv_obj_set_height(pi_ui.viewport,696);lv_obj_add_flag(pi_ui.viewport,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(pi_ui.viewport,LV_DIR_VER);
  lv_obj_set_scrollbar_mode(pi_ui.viewport,LV_SCROLLBAR_MODE_ON);lv_obj_set_style_width(pi_ui.viewport,4,LV_PART_SCROLLBAR);lv_obj_set_style_bg_color(pi_ui.viewport,lv_color_hex(0xff9900),LV_PART_SCROLLBAR);lv_obj_set_style_bg_opa(pi_ui.viewport,LV_OPA_COVER,LV_PART_SCROLLBAR);lv_obj_set_style_radius(pi_ui.viewport,2,LV_PART_SCROLLBAR);
  lv_obj_add_event_cb(pi_ui.root,pi_deleted,(lv_event_code_t)(LV_EVENT_DELETE|LV_EVENT_PREPROCESS),NULL);
  lv_obj_add_event_cb(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_BACK),pi_back_event,LV_EVENT_CLICKED,NULL);
  button(pi_ui.root,340,4,124,"Cancel",pi_cancel_event,NULL);
  pi_safety_dock=button(pi_ui.root,16,752,448,SAFETY_DOCK_TEXT,pi_safety_event,NULL);
  lv_obj_set_style_text_font(lv_obj_get_child(pi_safety_dock,0),&el_sans_14,0);
 }
 return pi_ui.live;
}
static bool pi_completed_matches(void){
 const passphrase_ui_completion_t *c=&pi_completed.identity;
 return pi_completed.valid&&pi_ui.live&&!pi_ui.pending&&pi_ui.result_visible&&
  c->request_id==pi_ui.next_id&&c->generation==pi_ui.generation&&
  pp_context_equal(c->context,pi_ui.context)&&c->marker==passphrase_ui_snapshot_marker(&pi_ui);
}
static void pi_completed_render(void){
 if(!pi_completed_matches()){pi_completed_invalidate(&pi_ui);return;}
 const hex_result_t *full=&pi_completed.full;
 lv_label_set_text(pi_ui.result_marker_label,pi_completed.identity.marker==PASSPHRASE_UI_ACTIVE?"Snapshot: Active passphrase":"Snapshot: Empty passphrase");
 lv_label_set_text(pi_ui.result_fp,full->fingerprint);lv_label_set_text(pi_ui.result_addr,full->address);
 lv_label_set_text(pi_mnemonic_label,full->mnemonic);lv_label_set_text(pi_entropy_label,full->entropy);
 lv_obj_remove_flag(pi_ui.result,LV_OBJ_FLAG_HIDDEN);
 if(full->weak)lv_label_set_text(pi_ui.status,"WEAK_INPUT_LAB_ONLY - count is not randomness quality. NEVER fund.");
 if(pi_completed.lifehash_status==0){if(!passphrase_ui_lifehash_publish(&pi_ui,full->fingerprint,pi_completed.rgb,sizeof pi_completed.rgb))passphrase_ui_lifehash_failed(&pi_ui);}else passphrase_ui_lifehash_failed(&pi_ui);
}
static void pi_enter(void){
 if(!pi_lock||pi_exhausted)return;
 pi_lock();if(!pi_dispatch.route)(void)el_dispatch_open(&pi_dispatch,&pi_clock);pi_unlock();
 if(!pi_ensure_ui())return;
 pi_owner=PI_OWNER_LEGACY;el_dispatch_clear(&pi_gui08_source,sizeof pi_gui08_source);
 passphrase_ui_context_t c={mode,pi_revision,mode==MODE_MNEMONIC?mn_target:mode==MODE_HEX?(uint32_t)(lengths[mode]*3/8):selector,0};
 passphrase_ui_set_context(&pi_ui,c);pi_completed_render();lv_obj_remove_flag(pi_ui.root,LV_OBJ_FLAG_HIDDEN);lv_obj_move_foreground(pi_ui.root);
}
static bool pi_enter_gui08(const gui08_request *source){
 if(!source||!source->request_id||!feature_ready(source->method)||
    source->passphrase_marker!=GUI08_PASSPHRASE_UNSPECIFIED)return false;
 pi_gui08_source=*source;
 if(!pi_gui08_source_matches()||!pi_ensure_ui()){
  el_dispatch_clear(&pi_gui08_source,sizeof pi_gui08_source);return false;
 }
 pi_owner=PI_OWNER_GUI08;
 passphrase_ui_set_context(&pi_ui,pi_gui08_context(source));
 lv_obj_remove_flag(pi_ui.root,LV_OBJ_FLAG_HIDDEN);lv_obj_move_foreground(pi_ui.root);
 return true;
}
void gui_passphrase_work(uint64_t token){
 el_dispatch_meta_t m={0};hex_result_t result={0};el_dispatch_result_t out={0};
 uint8_t raw4[4]={0},rgb[FINGERPRINT_VIEW_RGB_SIZE]={0};int lifehash_status=-1;
 pi_lock();const el_passphrase_request_t *q=el_dispatch_claim(&pi_dispatch,token);if(q)m=pi_dispatch.meta;pi_unlock();
 if(!q)return;
 el_compute_passphrase(q,&result); /* sole request reader, outside ALL locks */
 out.meta=m;out.rc=result.rc;
 if(result.rc>=0){
  memcpy(out.fingerprint,result.fingerprint,sizeof out.fingerprint);memcpy(out.address,result.address,sizeof out.address);
  if(pi_lifehash_render&&pi_fingerprint_raw4(result.fingerprint,raw4))lifehash_status=pi_lifehash_render(raw4,rgb);
 }
 pi_lock();if(el_dispatch_finish(&pi_dispatch,token,&out)){
  pi_full_result=result;pi_full_token=token;memcpy(pi_lifehash_rgb,rgb,sizeof pi_lifehash_rgb);pi_lifehash_token=token;pi_lifehash_status=lifehash_status;
 }pi_unlock();
 el_dispatch_clear(raw4,sizeof raw4);el_dispatch_clear(rgb,sizeof rgb);
 el_dispatch_clear(&result,sizeof result);el_dispatch_clear(&out,sizeof out);
}
void gui_passphrase_poll(void){
 if(!pi_lock)return;
 el_dispatch_result_t r={0};hex_result_t full={0};uint8_t rgb[FINGERPRINT_VIEW_RGB_SIZE]={0};int lifehash_status=-1;
 pi_lock();bool ok=el_dispatch_take(&pi_dispatch,pi_token,&pi_pending,&r);
 if(ok){
  if(pi_full_token==pi_token){full=pi_full_result;if(pi_lifehash_token==pi_token){memcpy(rgb,pi_lifehash_rgb,sizeof rgb);lifehash_status=pi_lifehash_status;}}else ok=false;
  el_dispatch_clear(&pi_full_result,sizeof pi_full_result);pi_full_token=0;pi_lifehash_clear();
 }pi_unlock();
 if(ok&&pi_ui.live){
  passphrase_ui_completion_t c={0};c.request_id=r.meta.request_id;c.generation=r.meta.generation;
  c.context=(passphrase_ui_context_t){r.meta.mode,r.meta.revision,r.meta.words,r.meta.derivation_context};c.marker=r.meta.marker;c.rc=(r.rc==1&&full.weak&&el_mode_is_dice(r.meta.mode))?0:r.rc;
  memcpy(c.fingerprint,r.fingerprint,sizeof c.fingerprint);memcpy(c.address,r.address,sizeof c.address);
  if(passphrase_ui_complete(&pi_ui,&c)&&passphrase_ui_result_visible(&pi_ui)){
   pi_completed.full=full;pi_completed.identity=c;
   memcpy(pi_completed.rgb,rgb,sizeof pi_completed.rgb);
   pi_completed.lifehash_status=lifehash_status;pi_completed.valid=true;
   pi_completed_render();
  }
  el_dispatch_clear(&c,sizeof c);
 }
 el_dispatch_clear(rgb,sizeof rgb);el_dispatch_clear(&r,sizeof r);el_dispatch_clear(&full,sizeof full);
}
