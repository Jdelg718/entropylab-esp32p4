/* Callable LVGL 9.5 passphrase step. Include once from the GUI translation unit.
 * It owns UI/request snapshots only: no normalization, crypto, queue, or secure-erasure claim. */
#include "passphrase_gui.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
LV_FONT_DECLARE(el_sans_14);
LV_FONT_DECLARE(el_sans_16);
LV_FONT_DECLARE(el_mono_16);
LV_FONT_DECLARE(el_mono_20);
LV_FONT_DECLARE(el_serif_24);

static bool pp_live(const passphrase_ui_t*u){return u && u->live && !u->disposing;}
static bool pp_context_equal(passphrase_ui_context_t a,passphrase_ui_context_t b){return a.mode==b.mode&&a.source_revision==b.source_revision&&a.word_count==b.word_count&&a.derivation_context==b.derivation_context;}
static bool pp_utf8(const uint8_t*s,size_t n){
 size_t i=0;while(i<n){uint32_t c=s[i++];unsigned more=0;uint32_t min=0;
  if(c<0x80){if(!c)return false;continue;}if(c>=0xc2&&c<=0xdf){more=1;min=0x80;c&=31;}else if(c>=0xe0&&c<=0xef){more=2;min=0x800;c&=15;}else if(c>=0xf0&&c<=0xf4){more=3;min=0x10000;c&=7;}else return false;
  if(i+more>n){return false;}while(more--){uint8_t q=s[i++];if((q&0xc0)!=0x80)return false;c=(c<<6)|(q&63);}if(c<min||c>0x10ffff||(c>=0xd800&&c<=0xdfff))return false;
 }return true;
}
static void pp_style(lv_style_t*s,uint32_t bg,int radius,bool border){lv_style_init(s);lv_style_set_bg_color(s,lv_color_hex(bg));lv_style_set_bg_opa(s,LV_OPA_COVER);lv_style_set_radius(s,radius);lv_style_set_border_width(s,border?1:0);lv_style_set_border_color(s,lv_color_hex(0x333333));lv_style_set_pad_all(s,0);lv_style_set_text_color(s,lv_color_hex(0xeeeeee));lv_style_set_text_font(s,&el_sans_16);}
static lv_obj_t*pp_box(lv_obj_t*p,int x,int y,int w,int h,lv_style_t*s){lv_obj_t*o=lv_obj_create(p);lv_obj_remove_style_all(o);lv_obj_add_style(o,s,0);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE);return o;}
static lv_obj_t*pp_text(lv_obj_t*p,int x,int y,int w,const char*t,const lv_font_t*f,uint32_t color){lv_obj_t*o=lv_label_create(p);lv_obj_set_pos(o,x,y);lv_obj_set_width(o,w);lv_obj_set_style_text_font(o,f,0);lv_obj_set_style_text_color(o,lv_color_hex(color),0);lv_label_set_long_mode(o,LV_LABEL_LONG_WRAP);lv_label_set_text(o,t);return o;}
static const char *pp_pages[]={"abcdefghijklmnopqrstuvwxyz", "ABCDEFGHIJKLMNOPQRSTUVWXYZ", "0123456789!\"#$%&'()*+,-./:;<=>?@", "[\\]^_`{|}~"};
static void pp_page(passphrase_ui_t*u){
 const char *p=pp_pages[u->page]; size_t n=strlen(p);
 for(unsigned i=0;i<32;i++){char z[2]={i<n?p[i]:0,0};lv_label_set_text(lv_obj_get_child(u->keys[i],0),z);if(i<n)lv_obj_remove_flag(u->keys[i],LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(u->keys[i],LV_OBJ_FLAG_HIDDEN);}
 const char *next[]={"ABC >","123 >","#+= >","abc >"};lv_label_set_text(lv_obj_get_child(u->keys[34],0),next[u->page]);
 int row=(int)((n+7)/8)*48+4;for(unsigned i=32;i<35;i++)lv_obj_set_y(u->keys[i],row);lv_obj_set_height(u->keyboard,row+52);
 int actions=u->keyboard_open?256+row+64:268;for(unsigned i=1;i<4;i++)lv_obj_set_y(u->controls[i],actions);if(u->result){lv_obj_set_y(u->result,actions+60);int needed=actions+60+lv_obj_get_height(u->result)+16;lv_obj_set_height(u->content,needed>734?needed:734);}
}
static void pp_event(lv_event_t*e);
static lv_obj_t*pp_button(passphrase_ui_t*u,lv_obj_t*p,int x,int y,int w,const char*s){lv_obj_t*o=lv_button_create(p);lv_obj_remove_style_all(o);lv_obj_add_style(o,&u->control,0);lv_obj_add_style(o,&u->primary,LV_STATE_PRESSED);lv_obj_add_style(o,&u->disabled,LV_STATE_DISABLED);lv_obj_add_style(o,&u->focus,LV_STATE_FOCUS_KEY);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,44);lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE);lv_obj_t*l=lv_label_create(o);lv_label_set_text(l,s);lv_obj_center(l);lv_obj_set_user_data(o,u);lv_obj_add_event_cb(o,pp_event,LV_EVENT_CLICKED,NULL);return o;}
static void pp_focus(passphrase_ui_t*u,lv_obj_t*o){if(u->focus_target)lv_obj_remove_state(u->focus_target,LV_STATE_FOCUS_KEY);u->focus_target=o;if(o){lv_obj_add_state(o,LV_STATE_FOCUS_KEY);if(u->group)lv_group_focus_obj(o);}}
static void pp_mask(passphrase_ui_t*u){u->masked=true;u->revealed=false;if(u->controls[PASSPHRASE_UI_CONTROL_REVEAL])lv_label_set_text(lv_obj_get_child(u->controls[PASSPHRASE_UI_CONTROL_REVEAL],0),"Reveal");}
/* Clear all result-owned labels, including integration-added mnemonic/entropy.
 * LVGL may free old allocations: this is not a heap secure-erasure guarantee. */
static void pp_clear_result_text(lv_obj_t*o){if(lv_obj_check_type(o,&lv_label_class))lv_label_set_text(o,"");for(uint32_t i=0;i<lv_obj_get_child_count(o);i++)pp_clear_result_text(lv_obj_get_child(o,i));}
#ifndef PP_INVALIDATE_OWNER
#define PP_INVALIDATE_OWNER(u) ((void)(u))
#endif
static void pp_invalidate(passphrase_ui_t*u){PP_INVALIDATE_OWNER(u);u->generation++;u->pending=false;u->pending_id=0;u->result_visible=false;if(u->lifehash.canvas)fingerprint_view_blank(&u->lifehash);pp_clear_result_text(u->result);lv_obj_add_flag(u->result,LV_OBJ_FLAG_HIDDEN);}
static void pp_wipe_raw(passphrase_ui_t*u){volatile uint8_t*p=u->raw;for(size_t i=0;i<sizeof u->raw;i++)p[i]=0;u->raw_len=0;}
static void pp_refresh(passphrase_ui_t*u,const char*message){
 char b[128];passphrase_ui_marker_t m=u->raw_len?PASSPHRASE_UI_ACTIVE:PASSPHRASE_UI_EMPTY;
 snprintf(b,sizeof b,"%s - %u / 256 UTF-8 bytes",m==PASSPHRASE_UI_ACTIVE?"Active":"Empty",(unsigned)u->raw_len);lv_label_set_text(u->counter,b);
 if(message)lv_label_set_text(u->status,message);else lv_label_set_text(u->status,m==PASSPHRASE_UI_ACTIVE?"Active - exact case and spaces retained.":"Empty - no passphrase will be used.");
 char shown[270];if(!u->raw_len)strcpy(shown,"(empty)");else if(u->masked){size_t n=u->raw_len<32?u->raw_len:32;memset(shown,'*',n);shown[n]=0;if(u->raw_len>32)strcat(shown,"...");}else{size_t n=u->raw_len<256?u->raw_len:256;memcpy(shown,u->raw,n);shown[n]=0;for(size_t i=0;i<n;i++)if(u->raw[i]<32||u->raw[i]>126){strcpy(shown,"[Non-ASCII: cannot display]");break;}}lv_label_set_text(u->field_text,shown);
 lv_obj_t*clear=u->controls[PASSPHRASE_UI_CONTROL_CLEAR];if(u->pending||(!u->raw_len&&!u->result_visible))lv_obj_add_state(clear,LV_STATE_DISABLED);else lv_obj_remove_state(clear,LV_STATE_DISABLED);
 for(int i=0;i<4;i++){if(u->pending&&i!=PASSPHRASE_UI_CONTROL_BACK)lv_obj_add_state(u->controls[i],LV_STATE_DISABLED);else if(i!=PASSPHRASE_UI_CONTROL_CLEAR||u->raw_len)lv_obj_remove_state(u->controls[i],LV_STATE_DISABLED);}
 lv_label_set_text(lv_obj_get_child(u->controls[PASSPHRASE_UI_CONTROL_CONTINUE],0),u->raw_len?"Continue active":"Continue empty");
}
static void pp_show_modal(passphrase_ui_t*u,bool show){u->modal_open=show;lv_group_remove_all_objs(u->group);if(show){lv_group_add_obj(u->group,u->controls[PASSPHRASE_UI_CONTROL_KEEP_MASKED]);lv_group_add_obj(u->group,u->controls[PASSPHRASE_UI_CONTROL_REVEAL_NOW]);}else{lv_group_add_obj(u->group,u->field);for(unsigned i=0;i<4;i++)lv_group_add_obj(u->group,u->controls[i]);for(unsigned i=0;i<PASSPHRASE_UI_KEY_MAX;i++)lv_group_add_obj(u->group,u->keys[i]);}if(show){lv_obj_remove_flag(u->modal,LV_OBJ_FLAG_HIDDEN);lv_obj_move_foreground(u->modal);pp_focus(u,u->controls[PASSPHRASE_UI_CONTROL_KEEP_MASKED]);}else{lv_obj_add_flag(u->modal,LV_OBJ_FLAG_HIDDEN);pp_focus(u,u->controls[PASSPHRASE_UI_CONTROL_REVEAL]);}}
static void pp_continue(passphrase_ui_t*u){if(u->pending)return;pp_mask(u);pp_invalidate(u);passphrase_ui_request_t q={0};memcpy(q.raw,u->raw,u->raw_len);q.raw_len=u->raw_len;q.marker=u->raw_len?PASSPHRASE_UI_ACTIVE:PASSPHRASE_UI_EMPTY;q.context=u->context;q.request_id=++u->next_id;q.generation=u->generation;u->pending=true;u->pending_id=q.request_id;u->pending_generation=q.generation;u->pending_context=q.context;u->pending_marker=q.marker;pp_refresh(u,"Pending - calculating outside the display lock.");if(!u->submit||!u->submit(&q,u->submit_user)){u->pending=false;u->pending_id=0;pp_refresh(u,"Worker unavailable - Back and retry manually.");}volatile uint8_t*p=(volatile uint8_t*)&q;for(size_t i=0;i<sizeof q;i++)p[i]=0;}
static void pp_event(lv_event_t*e){lv_obj_t*o=lv_event_get_target_obj(e);passphrase_ui_t*u=lv_obj_get_user_data(o);if(!pp_live(u))return;
 for(lv_obj_t*p=o;p;p=lv_obj_get_parent(p))if(lv_obj_has_flag(p,LV_OBJ_FLAG_HIDDEN))return;
 bool modal_control=o==u->controls[PASSPHRASE_UI_CONTROL_KEEP_MASKED]||o==u->controls[PASSPHRASE_UI_CONTROL_REVEAL_NOW];
 if(modal_control!=u->modal_open || (u->pending&&o!=u->controls[PASSPHRASE_UI_CONTROL_BACK]) || lv_obj_has_state(o,LV_STATE_DISABLED))return;
 if(o==u->field){passphrase_ui_open_keyboard(u);return;}if(o==u->controls[PASSPHRASE_UI_CONTROL_REVEAL]){if(u->revealed){pp_mask(u);pp_refresh(u,"Masked again - draft remains Active.");pp_focus(u,u->field);}else pp_show_modal(u,true);return;}
 if(o==u->controls[PASSPHRASE_UI_CONTROL_KEEP_MASKED]){pp_mask(u);pp_show_modal(u,false);pp_refresh(u,"Reveal cancelled - passphrase remains masked.");return;}if(o==u->controls[PASSPHRASE_UI_CONTROL_REVEAL_NOW]){pp_show_modal(u,false);u->masked=false;u->revealed=true;lv_label_set_text(lv_obj_get_child(u->controls[PASSPHRASE_UI_CONTROL_REVEAL],0),"Mask");pp_refresh(u,"Revealed - visible until dismissed or navigation.");pp_focus(u,u->field);return;}
 if(o==u->controls[PASSPHRASE_UI_CONTROL_CLEAR]){passphrase_ui_clear(u);return;}if(o==u->controls[PASSPHRASE_UI_CONTROL_BACK]){passphrase_ui_back(u);return;}if(o==u->controls[PASSPHRASE_UI_CONTROL_CONTINUE]){pp_continue(u);return;}
 for(unsigned i=0;i<PASSPHRASE_UI_KEY_MAX;i++)if(o==u->keys[i]){if(i<32){if(i<strlen(pp_pages[u->page]))passphrase_ui_editor_append(u,(unsigned char)pp_pages[u->page][i]);}else if(i==32)passphrase_ui_editor_append(u,' ');else if(i==33){if(u->raw_len){u->raw_len--;while(u->raw_len&&((u->raw[u->raw_len]&0xc0)==0x80))u->raw_len--;pp_invalidate(u);pp_mask(u);pp_refresh(u,NULL);}}else{u->page=(u->page+1)%4;pp_page(u);}return;}
}
/* DELETE is sent before LVGL deletes children. Delete them explicitly while
 * their owner/styles are intact, then detach the root style before resetting it. */
static void pp_root_delete(lv_event_t*e){
 passphrase_ui_t*u=lv_event_get_user_data(e);
 if(!u || u->disposing)return;
 u->disposing=true;u->live=false;u->pending=false;u->pending_id=0;
 u->submit=NULL;u->submit_user=NULL;pp_wipe_raw(u);
 lv_obj_t*root=lv_event_get_target_obj(e);
 while(lv_obj_get_child(root,0))lv_obj_delete(lv_obj_get_child(root,0));
 lv_obj_remove_style_all(root);
 if(u->group)lv_group_delete(u->group);
 lv_style_reset(&u->base);lv_style_reset(&u->card);lv_style_reset(&u->well);
 lv_style_reset(&u->control);lv_style_reset(&u->primary);lv_style_reset(&u->disabled);lv_style_reset(&u->focus);
 uint64_t generation=u->generation,next_id=u->next_id;
 memset(u,0,sizeof *u);u->generation=generation;u->next_id=next_id;
}
void passphrase_ui_destroy(passphrase_ui_t*u){if(pp_live(u))lv_obj_delete(u->root);}
void passphrase_ui_init(passphrase_ui_t*u,lv_obj_t*parent,passphrase_ui_submit_cb submit,void*user){if(!u || u->disposing || u->live)return;uint64_t generation=u->generation,next_id=u->next_id;memset(u,0,sizeof *u);u->generation=generation;u->next_id=next_id;u->live=true;u->submit=submit;u->submit_user=user;u->masked=true;
 pp_style(&u->base,0x000000,0,false);pp_style(&u->card,0x141414,20,true);pp_style(&u->well,0x000000,12,true);pp_style(&u->control,0x202020,8,true);pp_style(&u->primary,0xff9900,8,true);lv_style_set_text_color(&u->primary,lv_color_hex(0));pp_style(&u->disabled,0x202020,8,true);lv_style_set_text_color(&u->disabled,lv_color_hex(0x737373));lv_style_init(&u->focus);lv_style_set_outline_width(&u->focus,3);lv_style_set_outline_color(&u->focus,lv_color_hex(0xff9900));
 u->group=lv_group_create();
 u->root=pp_box(parent,0,0,480,800,&u->base);lv_obj_add_event_cb(u->root,pp_root_delete,LV_EVENT_DELETE,u);lv_obj_remove_flag(u->root,LV_OBJ_FLAG_CLICKABLE);pp_text(u->root,16,12,448,"EntropyLab / Passphrase",&el_sans_16,0xeeeeee);
 u->viewport=pp_box(u->root,16,48,448,736,&u->well);u->content=pp_box(u->viewport,0,0,446,734,&u->base);
 pp_text(u->content,16,14,414,"Optional BIP39 passphrase",&el_sans_16,0xeeeeee);u->context_label=pp_text(u->content,16,44,414,"Context not set",&el_mono_16,0xa3a3a3);u->counter=pp_text(u->content,16,76,414,"",&el_mono_16,0xff9900);
 u->field=pp_button(u,u->content,16,110,310,"(empty)");u->field_text=lv_obj_get_child(u->field,0);lv_obj_set_width(u->field_text,286);lv_label_set_long_mode(u->field_text,LV_LABEL_LONG_SCROLL);lv_obj_set_style_text_font(u->field,&el_mono_16,0);u->controls[PASSPHRASE_UI_CONTROL_REVEAL]=pp_button(u,u->content,334,110,96,"Reveal");pp_text(u->content,16,164,414,"Keyboard/display: printable ASCII. Runtime accepts UTF-8. Exact case and spaces retained.",&el_sans_14,0xa3a3a3);u->status=pp_text(u->content,16,210,414,"",&el_sans_14,0xff9900);
 u->controls[PASSPHRASE_UI_CONTROL_BACK]=pp_button(u,u->content,16,516,92,"Back");u->controls[PASSPHRASE_UI_CONTROL_CLEAR]=pp_button(u,u->content,116,516,92,"Clear");u->controls[PASSPHRASE_UI_CONTROL_CONTINUE]=pp_button(u,u->content,216,516,214,"Continue empty");lv_obj_add_style(u->controls[PASSPHRASE_UI_CONTROL_CONTINUE],&u->primary,0);
 u->keyboard=pp_box(u->content,16,256,414,248,&u->well);for(unsigned i=0;i<32;i++)u->keys[i]=pp_button(u,u->keyboard,4+(i%8)*51,4+(i/8)*48,48,"");u->keys[32]=pp_button(u,u->keyboard,4,196,150,"Space");u->keys[33]=pp_button(u,u->keyboard,160,196,120,"Delete");u->keys[34]=pp_button(u,u->keyboard,286,196,120,"ABC >");pp_page(u);lv_obj_add_flag(u->keyboard,LV_OBJ_FLAG_HIDDEN);
 u->result=pp_box(u->content,16,576,414,142,&u->well);u->result_marker_label=pp_text(u->result,10,8,394,"",&el_sans_16,0xff9900);u->result_fp=pp_text(u->result,10,38,394,"",&el_mono_16,0xeeeeee);lv_obj_add_flag(u->result_fp,LV_OBJ_FLAG_HIDDEN);fingerprint_view_create(&u->lifehash,u->result,10,38);u->result_addr=pp_text(u->result,10,112,394,"",&el_mono_16,0xeeeeee);lv_obj_add_flag(u->result,LV_OBJ_FLAG_HIDDEN);
 u->modal=pp_box(u->root,0,0,480,800,&u->base);lv_obj_add_flag(u->modal,LV_OBJ_FLAG_CLICKABLE);pp_text(u->modal,28,150,424,"Reveal passphrase?",&el_serif_24,0xeeeeee);pp_text(u->modal,28,220,424,"Printable ASCII will be visible, including spaces and case. Non-ASCII bytes cannot be displayed.",&el_sans_16,0xeeeeee);u->controls[PASSPHRASE_UI_CONTROL_KEEP_MASKED]=pp_button(u,u->modal,28,340,200,"Keep masked");u->controls[PASSPHRASE_UI_CONTROL_REVEAL_NOW]=pp_button(u,u->modal,252,340,200,"Reveal now");lv_obj_add_style(u->controls[PASSPHRASE_UI_CONTROL_REVEAL_NOW],&u->primary,0);lv_obj_add_flag(u->modal,LV_OBJ_FLAG_HIDDEN);pp_page(u);pp_show_modal(u,false);pp_focus(u,u->field);pp_refresh(u,NULL);
}
void passphrase_ui_set_context(passphrase_ui_t*u,passphrase_ui_context_t c){if(!pp_live(u))return;bool changed=u->context_set&&!pp_context_equal(u->context,c);if(changed){pp_show_modal(u,false);pp_wipe_raw(u);pp_mask(u);pp_invalidate(u);}u->context=c;u->context_set=true;char b[96];snprintf(b,sizeof b,"Mode %" PRIu32 " / input r%" PRIu64 " / %" PRIu32 " words",c.mode,c.source_revision,c.word_count);lv_label_set_text(u->context_label,b);pp_refresh(u,changed?"Context changed - draft and results cleared.":NULL);}
passphrase_ui_rc_t passphrase_ui_set_raw(passphrase_ui_t*u,const uint8_t*s,size_t n){if(!pp_live(u))return PASSPHRASE_UI_ERR_INSTANCE;if(n>PASSPHRASE_UI_RAW_MAX)return PASSPHRASE_UI_ERR_LIMIT;if(n&&(!s||!pp_utf8(s,n)))return PASSPHRASE_UI_ERR_UTF8;uint8_t saved[PASSPHRASE_UI_RAW_MAX];if(n)memcpy(saved,s,n);pp_wipe_raw(u);if(n)memcpy(u->raw,saved,n);volatile uint8_t*wipe=saved;for(size_t i=0;i<sizeof saved;i++)wipe[i]=0;u->raw_len=n;pp_mask(u);pp_invalidate(u);pp_refresh(u,NULL);return PASSPHRASE_UI_OK;}
passphrase_ui_rc_t passphrase_ui_editor_append(passphrase_ui_t*u,uint32_t c){if(!pp_live(u))return PASSPHRASE_UI_ERR_INSTANCE;if(u->pending||u->modal_open)return PASSPHRASE_UI_ERR_BUSY;if(c<0x20||c>0x7e)return PASSPHRASE_UI_ERR_EDITOR_UNSUPPORTED;if(u->raw_len==PASSPHRASE_UI_RAW_MAX)return PASSPHRASE_UI_ERR_LIMIT;u->raw[u->raw_len++]=(uint8_t)c;pp_mask(u);pp_invalidate(u);pp_refresh(u,NULL);return PASSPHRASE_UI_OK;}
void passphrase_ui_clear(passphrase_ui_t*u){if(!pp_live(u))return;pp_show_modal(u,false);pp_wipe_raw(u);pp_mask(u);pp_invalidate(u);pp_refresh(u,"Cleared - Empty; no passphrase will be used.");pp_focus(u,u->field);}
void passphrase_ui_cancel(passphrase_ui_t*u){if(!pp_live(u))return;passphrase_ui_clear(u);pp_refresh(u,"Cancelled - draft and results cleared.");}
void passphrase_ui_back(passphrase_ui_t*u){if(!pp_live(u))return;pp_show_modal(u,false);pp_mask(u);if(!u->result_visible)pp_invalidate(u);pp_refresh(u,u->result_visible?"Back - current result retained; Cancel clears it.":"Back - pending calculation cancelled; no result is current.");pp_focus(u,u->field);}
bool passphrase_ui_complete(passphrase_ui_t*u,const passphrase_ui_completion_t*r){if(!pp_live(u)||!r||!u->pending||r->request_id!=u->pending_id||r->generation!=u->pending_generation||!pp_context_equal(r->context,u->pending_context)||r->marker!=u->pending_marker)return false;u->pending=false;u->pending_id=0;if(r->rc!=0){u->result_visible=false;lv_obj_add_flag(u->result,LV_OBJ_FLAG_HIDDEN);pp_refresh(u,"Calculation failed - Back and retry manually.");return true;}if(!memchr(r->fingerprint,0,sizeof r->fingerprint)||!memchr(r->address,0,sizeof r->address)){pp_refresh(u,"Malformed completion dropped - no result is current.");return true;}u->result_marker=r->marker;u->result_visible=true;lv_label_set_text(u->result_marker_label,r->marker==PASSPHRASE_UI_ACTIVE?"Snapshot: Active passphrase":"Snapshot: Empty passphrase");lv_label_set_text(u->result_fp,r->fingerprint);lv_label_set_text(u->result_addr,r->address);lv_obj_remove_flag(u->result,LV_OBJ_FLAG_HIDDEN);pp_refresh(u,r->marker==PASSPHRASE_UI_ACTIVE?"Result snapshot - Active.":"Result snapshot - Empty.");return true;}
bool passphrase_ui_lifehash_publish(passphrase_ui_t*u,const char fingerprint[9],const uint8_t*rgb,size_t rgb_size){return pp_live(u)&&u->result_visible&&fingerprint_view_publish(&u->lifehash,fingerprint,rgb,rgb_size);}
void passphrase_ui_lifehash_rendering(passphrase_ui_t*u){if(pp_live(u))fingerprint_view_rendering(&u->lifehash);}
void passphrase_ui_lifehash_failed(passphrase_ui_t*u){if(pp_live(u)&&u->result_visible)fingerprint_view_failed(&u->lifehash);}
bool passphrase_ui_lifehash_visible(const passphrase_ui_t*u){return pp_live(u)&&u->result_visible&&u->lifehash.visible;}
void passphrase_ui_open_keyboard(passphrase_ui_t*u){if(!pp_live(u))return;if(u->pending||u->modal_open)return;u->keyboard_open=true;pp_page(u);lv_obj_remove_flag(u->keyboard,LV_OBJ_FLAG_HIDDEN);lv_obj_update_layout(u->root);pp_focus(u,u->field);}
const uint8_t*passphrase_ui_raw(const passphrase_ui_t*u){return pp_live(u)?u->raw:NULL;}size_t passphrase_ui_raw_len(const passphrase_ui_t*u){return pp_live(u)?u->raw_len:0;}passphrase_ui_marker_t passphrase_ui_snapshot_marker(const passphrase_ui_t*u){return pp_live(u)&&u->raw_len?PASSPHRASE_UI_ACTIVE:PASSPHRASE_UI_EMPTY;}bool passphrase_ui_is_masked(const passphrase_ui_t*u){return !pp_live(u)||u->masked;}bool passphrase_ui_pending(const passphrase_ui_t*u){return pp_live(u)&&u->pending;}bool passphrase_ui_result_visible(const passphrase_ui_t*u){return pp_live(u)&&u->result_visible;}passphrase_ui_marker_t passphrase_ui_result_marker(const passphrase_ui_t*u){return pp_live(u)?u->result_marker:PASSPHRASE_UI_EMPTY;}bool passphrase_ui_modal_open(const passphrase_ui_t*u){return pp_live(u)&&u->modal_open;}bool passphrase_ui_keyboard_open(const passphrase_ui_t*u){return pp_live(u)&&u->keyboard_open;}const char*passphrase_ui_status(const passphrase_ui_t*u){return pp_live(u)?lv_label_get_text(u->status):NULL;}lv_obj_t*passphrase_ui_root(const passphrase_ui_t*u){return pp_live(u)?u->root:NULL;}lv_obj_t*passphrase_ui_control(const passphrase_ui_t*u,passphrase_ui_control_t c){return pp_live(u)&&(unsigned)c<PASSPHRASE_UI_CONTROL_COUNT?u->controls[c]:NULL;}lv_obj_t*passphrase_ui_focus_target(const passphrase_ui_t*u){return pp_live(u)?u->focus_target:NULL;}unsigned passphrase_ui_keyboard_key_count(const passphrase_ui_t*u){return pp_live(u)?PASSPHRASE_UI_KEY_MAX:0;}lv_obj_t*passphrase_ui_keyboard_key(const passphrase_ui_t*u,unsigned i){return pp_live(u)&&i<PASSPHRASE_UI_KEY_MAX?u->keys[i]:NULL;}
