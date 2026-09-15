/* Native fixed-capacity dictionary editor. Public test input only.
 * UI/worker ownership remains the LVGL task/lock convention in gui.c.
 * Ordinary buffer clearing is best effort, NOT secure erasure. */
#include "bip39_dictionary.inc"
static lv_obj_t *mn_panel,*legacy_panel,*mn_list,*mn_draft_label,*mn_status,*mn_output,*mn_fp,*mn_addr,*mn_entropy;
static lv_obj_t *mn_validate,*mn_targets[5],*mn_candidate_buttons[3],*mn_slots[24],*mn_keys[26];
static lv_obj_t *mn_prev,*mn_next,*mn_cancel,*mn_clear,*mn_delete,*mn_hint,*mn_pages;
static lv_obj_t *mn_modal,*mn_modal_text,*mn_no,*mn_yes,*mn_reveal,*mn_back;
static char mnemonic_transcript[216],mn_draft[9],mn_recovered[65];
static uint16_t mn_words[24];
static unsigned mn_count,mn_target=12,mn_page,mn_matches,mn_candidates[3];
static int mn_edit=-1;
/* Pending action only; phrase/draft are never copied into a dialog. */
static unsigned mn_confirm;
static uint64_t mn_revision,mn_id,mn_pending;
static bool mn_valid,mn_reviewing,mn_safety_open;
static lv_obj_t *mn_safety,*mn_safety_close,*mn_safety_modal;
static lv_obj_t *mn_safety_content,*mn_about_content,*mn_about,*mn_about_back,*mn_about_label,*mn_about_body,*mn_about_scroll;
static bool mn_about_open;
static el_mode_t mn_about_mode;
static void mn_rebind(void);
#include "input_explanations.inc"
static lv_obj_t *mn_review,*mn_review_button,*mn_review_back,*mn_review_clear,*mn_review_list,*mn_review_rows[24];
/* Payload is immutable by value, not a pointer to a reused candidate record.
 * 10 action bits + monotonic generation; never wrap, fail closed at exhaustion.
 * A generation covers prefix/page, slot, target, modal and screen lifetime. */
static uintptr_t mn_generation;
static bool mn_exhausted;
static void mn_refresh(void);
static lv_obj_t *mn_extent;
static int mn_reveal_slot=-1;
static bool mn_settling;
static int mn_scroll_max(void){int extent=(int)((mn_count+2)/3)*48;return extent>144?extent-144:0;}
static void mn_settle(lv_event_t *e){
 (void)e;if(saver_blocked()||mn_settling)return;mn_settling=true;
 int y=lv_obj_get_scroll_y(mn_list);y=((y+24)/48)*48;
 if(y<0)y=0;
 if(y>mn_scroll_max())y=mn_scroll_max();
 lv_obj_scroll_to_y(mn_list,y,LV_ANIM_OFF);mn_settling=false;
}
static void mn_scroll_refresh(void){
 lv_obj_set_y(mn_extent,mn_count?(int)((mn_count+2)/3)*48-1:0);
 lv_obj_update_layout(mn_list);
 int y=lv_obj_get_scroll_y(mn_list);y=((y+24)/48)*48;
 if(mn_reveal_slot>=0){int row=mn_reveal_slot/3*48;if(row<y)y=row;else if(row>=y+144)y=row-96;mn_reveal_slot=-1;}
 if(y<0)y=0;
 if(y>mn_scroll_max())y=mn_scroll_max();
 lv_obj_scroll_to_y(mn_list,y,LV_ANIM_OFF);
}
static void mn_invalidate(void){mn_reviewing=false;lv_obj_add_flag(mn_review,LV_OBJ_FLAG_HIDDEN);for(unsigned i=0;i<24;i++)gui_label_reset(mn_review_rows[i],"");mn_valid=false;mn_pending=0;mn_revision++;gui_owned_zero(mn_recovered,sizeof mn_recovered);gui_label_reset(mn_entropy,"");gui_label_reset(mn_fp,"");gui_label_reset(mn_addr,"");lv_obj_add_flag(mn_output,LV_OBJ_FLAG_HIDDEN);}
static void mn_reset_draft(void){gui_owned_zero(mn_draft,sizeof mn_draft);mn_edit=-1;mn_page=0;}
static void mn_lookup(void){
 mn_matches=0;for(unsigned i=0;i<3;i++)mn_candidates[i]=2048;
 if(!mn_draft[0]){mn_page=0;return;}
 size_t n=strlen(mn_draft);
 for(unsigned i=0;i<2048;i++)if(!strncmp(mn_dictionary[i],mn_draft,n))mn_matches++;
 if(mn_page*3>=mn_matches)mn_page=0;
 unsigned match=0;
 for(unsigned i=0;i<2048;i++)if(!strncmp(mn_dictionary[i],mn_draft,n)){
  if(match>=mn_page*3 && match<mn_page*3+3)mn_candidates[match-mn_page*3]=i;
  match++;
 }
}
/* Rebuild before mutation: 24*8 + 23 spaces + NUL = 216. */
static bool mn_serialize(unsigned replace,unsigned id,unsigned count,char out[216]){
 size_t used=0;memset(out,0,216);
 if(count>24)return false;
 for(unsigned i=0;i<count;i++){
  unsigned w=i==replace?id:mn_words[i];if(w>=2048)return false;
  size_t n=strlen(mn_dictionary[w]);if(used+(i?1:0)+n>=216)return false;
  if(i)out[used++]=' ';
  memcpy(out+used,mn_dictionary[w],n);used+=n;
 }
 return true;
}
static void mn_apply(unsigned k){
 if(k==3){gui_owned_zero(mn_words,sizeof mn_words);gui_owned_zero(mnemonic_transcript,sizeof mnemonic_transcript);mn_count=0;mn_reset_draft();}
 else if(k>=12&&k<=24&&k%3==0){
  char out[216];unsigned n=mn_count>k?k:mn_count;
  if(!mn_serialize(24,0,n,out))return;
  memcpy(mnemonic_transcript,out,sizeof out);gui_owned_zero(out,sizeof out);
  for(unsigned i=n;i<24;i++)mn_words[i]=0;
  mn_count=n;mn_target=k;mn_reset_draft();
 }else if(k>=600&&k<624 && k-600<mn_count){mn_reset_draft();mn_edit=(int)(k-600);}
 mn_invalidate();
}
static void mn_action(lv_event_t *e){
 if(saver_blocked()||busy)return;
 uintptr_t token=(uintptr_t)lv_event_get_user_data(e);
 if(mn_exhausted || (token>>10)!=mn_generation)return;
 unsigned k=token&1023u;
 if(mn_safety_open){
  if(k==801)mn_safety_open=false;
  else if(k==802&&!mn_about_open){
   mn_about_open=true;
   lv_label_set_text(mn_about_label,mn_explanation_names[mn_about_mode]);
   lv_label_set_text(mn_about_body,mn_explanation_copy[mn_about_mode]);
   lv_obj_scroll_to_y(mn_about_scroll,0,LV_ANIM_OFF);
  }else if(k==803&&mn_about_open)mn_about_open=false;
  else return;
  saver_rebind_controls();mn_rebind();return;
 }
 if(k>=801&&k<=803)return;
 if(mode!=MODE_MNEMONIC && k!=800)return;
 if(k==800){if(mn_confirm)return;mn_safety_open=true;mn_about_open=false;mn_about_mode=mode;saver_rebind_controls();mn_rebind();return;}
 if(mn_confirm){
  if(k!=701&&k!=702)return;
  unsigned pending=mn_confirm;mn_confirm=0;
  if(k==702)mn_apply(pending);
  mn_refresh();return;
 }
 if(k==701||k==702)return;
 if(mn_reviewing && k!=301 && k!=3)return;
 if(k==201){
  if(mn_count!=mn_target||mn_draft[0]||mn_edit>=0||mn_valid)return;
  mn_invalidate();mn_reviewing=true;mn_refresh();return;
 }
 if(k==300){if(mn_valid)lv_label_set_text(mn_entropy,mn_recovered);return;}
 if(k==301){mn_reset_draft();mn_invalidate();mn_refresh();return;}
 if(!lv_obj_has_flag(mn_output,LV_OBJ_FLAG_HIDDEN))return;
 if(k==200){
  if(mn_count!=mn_target||mn_draft[0]||mn_edit>=0)return;
  hex_request_t q=snapshot(mnemonic_transcript,strlen(mnemonic_transcript),MODE_MNEMONIC,mn_target);
  mn_invalidate();q.revision=mn_revision;q.request_id=++mn_id;mn_pending=q.request_id;busy=true;mn_refresh();
  if(!request_hex||!request_hex(&q)){busy=false;mn_pending=0;mn_refresh();lv_label_set_text(mn_status,"Worker unavailable. Retry validation.");}
  gui_owned_zero(&q,sizeof q);return;
 }
 if(k==3 || (k>=12&&k<=24&&k%3==0) || (k>=600&&k<624)){
  if(k==mn_target)return;
  if(k>=600&&k-600>=mn_count)return;
  if(k==3||mn_draft[0]||mn_edit>=0||(k<=24&&mn_count>k))mn_confirm=k;
  else mn_apply(k);
  if(!mn_reviewing)mn_invalidate();
  mn_refresh();return;
 }
 if(k==400||k==401){if(k==400&&mn_page)mn_page--;if(k==401&&(mn_page+1)*3<mn_matches)mn_page++;mn_refresh();return;}
 if(k==4){if(mn_edit<0)return;mn_reset_draft();}
 else if(k==2){size_t n=strlen(mn_draft);if(!n)return;mn_draft[n-1]=0;mn_page=0;}
 else if(k>='a'&&k<='z'){
  size_t n=strlen(mn_draft);if(n>=8||(mn_edit<0&&mn_count>=mn_target))return;
  mn_draft[n]=(char)k;mn_draft[n+1]=0;mn_page=0;
 }else if(k>=500&&k<503){
  if(!mn_draft[0]||(mn_edit<0&&mn_count>=mn_target))return;
  unsigned id=mn_candidates[k-500];
  if(id>=2048||strncmp(mn_dictionary[id],mn_draft,strlen(mn_draft)))return;
  unsigned slot=mn_edit<0?mn_count:(unsigned)mn_edit;
  unsigned n=mn_count+(mn_edit<0?1:0);char out[216];
  if(slot>=24||!mn_serialize(slot,id,n,out))return;
  mn_reveal_slot=(int)slot;mn_words[slot]=id;mn_count=n;memcpy(mnemonic_transcript,out,sizeof out);gui_owned_zero(out,sizeof out);mn_reset_draft();
 }else return;
 mn_invalidate();mn_refresh();
}
static void mn_enable(lv_obj_t *o,bool enabled){if(enabled)lv_obj_remove_state(o,LV_STATE_DISABLED);else lv_obj_add_state(o,LV_STATE_DISABLED);}
static void mn_visible(lv_obj_t *o,bool visible){if(visible)lv_obj_remove_flag(o,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(o,LV_OBJ_FLAG_HIDDEN);}
static void mn_bind(lv_obj_t *o,unsigned action){
 lv_obj_remove_event_cb(o,mn_action);
 if(!mn_exhausted)lv_obj_add_event_cb(o,mn_action,LV_EVENT_CLICKED,(void*)((mn_generation<<10)|action));
}
/* Navigation only rebinds callbacks/visibility; never refreshes input or results. */
static void mn_rebind(void){
 saver_activity();
 if(mn_generation==UINTPTR_MAX>>10)mn_exhausted=true;else mn_generation++;
 mn_bind(mn_safety,800);mn_bind(mn_safety_close,801);mn_bind(mn_about,802);mn_bind(mn_about_back,803);
 mn_visible(mn_safety_content,!mn_about_open);mn_visible(mn_about_content,mn_about_open);
 mn_visible(mn_about,!mn_about_open);mn_visible(mn_about_back,mn_about_open);
 mn_visible(mn_safety,true);mn_enable(mn_safety,!busy&&!mn_confirm&&!mn_safety_open);mn_visible(mn_safety_modal,mn_safety_open);
 for(unsigned i=0;i<24;i++)mn_bind(mn_slots[i],600+i);
 for(unsigned i=0;i<26;i++)mn_bind(mn_keys[i],'a'+i);
 for(unsigned i=0;i<5;i++)mn_bind(mn_targets[i],12+3*i);
 for(unsigned i=0;i<3;i++)mn_bind(mn_candidate_buttons[i],500+i);
 mn_bind(mn_review_button,201);mn_bind(mn_review_back,301);mn_bind(mn_review_clear,3);
 mn_bind(mn_prev,400);mn_bind(mn_next,401);mn_bind(mn_cancel,4);
 mn_bind(mn_delete,2);mn_bind(mn_clear,3);mn_bind(mn_validate,200);
 mn_bind(mn_reveal,300);mn_bind(mn_back,301);mn_bind(mn_no,701);mn_bind(mn_yes,702);
}
static void mn_refresh(void){
 mn_rebind();
 char b[96];mn_lookup();
 snprintf(b,sizeof b,"MNEMONIC  %u/%u words  %u/215 bytes",mn_count,mn_target,(unsigned)strlen(mnemonic_transcript));lv_label_set_text(mn_status,b);
 if(busy)lv_label_set_text(mn_status,"Validating outside the display lock...");
 if(mn_edit>=0)snprintf(b,sizeof b,"Replace %02u: %s",(unsigned)mn_edit+1,mn_draft[0]?mn_draft:"type prefix");
 else snprintf(b,sizeof b,"Next: %s",mn_draft[0]?mn_draft:"type prefix");
 gui_label_reset(mn_draft_label,b);
 bool enabled=!busy&&!mn_confirm&&!mn_reviewing;
 mn_visible(mn_review,mn_reviewing);
 for(unsigned i=0;i<24;i++){
  if(mn_reviewing&&i<mn_count)snprintf(b,sizeof b,"%02u  %s",i+1,mn_dictionary[mn_words[i]]);
  else b[0]=0;
  gui_label_reset(mn_review_rows[i],b);
  mn_visible(mn_review_rows[i],mn_reviewing&&i<mn_count);
 }
 mn_enable(mn_review_button,enabled&&mn_count==mn_target&&!mn_draft[0]&&mn_edit<0);
 mn_enable(mn_review_back,!busy&&!mn_confirm);mn_enable(mn_review_clear,!busy&&!mn_confirm);
 for(unsigned i=0;i<24;i++){
  mn_visible(mn_slots[i],i<mn_count);
  if(i<mn_count){snprintf(b,sizeof b,"%02u %s",i+1,mn_dictionary[mn_words[i]]);gui_label_reset(lv_obj_get_child(mn_slots[i],0),b);}
  else gui_label_reset(lv_obj_get_child(mn_slots[i],0),"");
  mn_enable(mn_slots[i],enabled);
  if(mn_edit==(int)i)lv_obj_add_state(mn_slots[i],LV_STATE_CHECKED);else lv_obj_remove_state(mn_slots[i],LV_STATE_CHECKED);
 }
 mn_scroll_refresh();
 for(unsigned i=0;i<3;i++){
  lv_label_set_text(lv_obj_get_child(mn_candidate_buttons[i],0),mn_candidates[i]<2048?mn_dictionary[mn_candidates[i]]:"--");
  mn_enable(mn_candidate_buttons[i],enabled&&mn_candidates[i]<2048);
 }
 for(unsigned i=0;i<26;i++)mn_enable(mn_keys[i],enabled&&strlen(mn_draft)<8&&(mn_edit>=0||mn_count<mn_target));
 mn_enable(mn_prev,enabled&&mn_page>0);mn_enable(mn_next,enabled&&(mn_page+1)*3<mn_matches);
 mn_enable(mn_delete,enabled);mn_enable(mn_clear,enabled);mn_enable(mn_cancel,enabled);mn_visible(mn_cancel,mn_edit>=0);
 mn_enable(mn_validate,enabled&&mn_count==mn_target&&!mn_draft[0]&&mn_edit<0);
 mn_enable(mn_reveal,!busy);mn_enable(mn_back,!busy);
 snprintf(b,sizeof b,"%u matches / page %u of %u",mn_matches,mn_matches?mn_page+1:0,(mn_matches+2)/3);lv_label_set_text(mn_pages,b);
 lv_label_set_text(mn_hint,mn_draft[0]?(mn_matches?"Choose a candidate explicitly":"No matching word"):(mn_edit>=0?"Original kept until selection":"Tap a selected word to replace"));
 for(unsigned i=0;i<5;i++){mn_enable(mn_targets[i],enabled);if(mn_target==12+3*i)lv_obj_add_state(mn_targets[i],LV_STATE_CHECKED);else lv_obj_remove_state(mn_targets[i],LV_STATE_CHECKED);}
 for(int i=0;i<GUI_MODE_COUNT;i++){if(mode==(el_mode_t)i)lv_obj_add_state(modes[i],LV_STATE_CHECKED);else lv_obj_remove_state(modes[i],LV_STATE_CHECKED);mn_enable(modes[i],!busy&&!mn_confirm);}
 mn_visible(mn_modal,mn_confirm!=0);mn_enable(mn_no,!busy);mn_enable(mn_yes,!busy);
 lv_label_set_text(mn_modal_text,!mn_confirm?"":mn_confirm==3?"Clear all selected words and draft?\nBest effort only; not secure erasure.":mn_confirm<=24?"Change target? Trailing words beyond\nthe target and draft will be discarded.":"Discard draft and edit another slot?\nSelected words are kept.");
}
static void mn_create(lv_obj_t *s){
 gui_owned_zero(mnemonic_transcript,sizeof mnemonic_transcript);gui_owned_zero(mn_words,sizeof mn_words);mn_reset_draft();mn_count=0;mn_target=12;mn_pending=0;mn_confirm=0;mn_safety_open=false;mn_about_open=false;mn_about_mode=MODE_HEX;
 mn_panel=box(s,16,104,448,648,&card);
 lv_obj_set_style_border_width(mn_panel,0,0);
 mn_status=text(mn_panel,12,6,424,"MNEMONIC",&el_sans_14,0xff9900);
 for(unsigned i=0;i<5;i++){char b[4];snprintf(b,sizeof b,"%u",12+3*i);mn_targets[i]=button(mn_panel,12+i*85,28,80,b,mn_action,(void*)(uintptr_t)(12+3*i));}
 mn_list=box(mn_panel,12,78,424,144,&well);lv_obj_add_flag(mn_list,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(mn_list,LV_DIR_VER);lv_obj_set_scrollbar_mode(mn_list,LV_SCROLLBAR_MODE_ON);
 lv_obj_remove_flag(mn_list,LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM);lv_obj_add_event_cb(mn_list,mn_settle,LV_EVENT_SCROLL_END,NULL);mn_extent=box(mn_list,0,0,1,1,&well);lv_obj_remove_flag(mn_extent,LV_OBJ_FLAG_CLICKABLE);lv_obj_set_style_border_width(mn_extent,0,0);
 lv_obj_set_style_border_width(mn_list,0,0);lv_obj_set_style_radius(mn_list,0,0);
 lv_obj_set_style_width(mn_list,4,LV_PART_SCROLLBAR);lv_obj_set_style_bg_color(mn_list,lv_color_hex(0xff9900),LV_PART_SCROLLBAR);lv_obj_set_style_bg_opa(mn_list,LV_OPA_COVER,LV_PART_SCROLLBAR);lv_obj_set_style_radius(mn_list,2,LV_PART_SCROLLBAR);lv_obj_set_style_pad_right(mn_list,2,LV_PART_SCROLLBAR);
 for(unsigned i=0;i<24;i++){mn_slots[i]=button(mn_list,2+(i%3)*138,2+(i/3)*48,134,"",mn_action,(void*)(uintptr_t)(600+i));lv_obj_set_style_text_font(mn_slots[i],&el_mono_16,0);}
 mn_draft_label=text(mn_panel,12,230,300,"",&el_mono_16,0xeeeeee);
 mn_delete=button(mn_panel,326,230,110,"Backspace",mn_action,(void*)2);
 mn_hint=text(mn_panel,12,278,424,"",&el_sans_14,0xa3a3a3);
 for(unsigned i=0;i<3;i++)mn_candidate_buttons[i]=button(mn_panel,12+i*142,302,136,"--",mn_action,(void*)(uintptr_t)(500+i));
 mn_prev=button(mn_panel,12,354,66,"Prev",mn_action,(void*)400);mn_next=button(mn_panel,370,354,66,"Next",mn_action,(void*)401);
 mn_pages=text(mn_panel,84,354,280,"",&el_sans_14,0xa3a3a3);
 for(unsigned i=0;i<26;i++){char b[2]={'a'+i,0};mn_keys[i]=button(mn_panel,12+(i%9)*47,406+(i/9)*48,44,b,mn_action,(void*)(uintptr_t)('a'+i));}
 mn_cancel=button(mn_panel,12,550,424,"Cancel replacement",mn_action,(void*)4);
 mn_clear=button(mn_panel,12,602,136,"Clear words",mn_action,(void*)3);
 mn_review_button=button(mn_panel,154,602,140,"Review words",mn_action,(void*)201);
 mn_validate=button(mn_panel,300,602,136,"Validate",mn_action,(void*)200);
 mn_output=box(mn_panel,0,0,448,648,&card);lv_obj_set_style_border_width(mn_output,0,0);lv_obj_set_style_radius(mn_output,0,0);
 text(mn_output,16,16,416,"Checksum valid - not entropy quality\nEnglish / empty passphrase / NEVER fund",&el_sans_14,0xff9900);
 text(mn_output,16,80,416,"Master fingerprint",&el_sans_14,0xa3a3a3);mn_fp=text(mn_output,16,106,416,"",&el_mono_20,0xeeeeee);
 text(mn_output,16,154,416,"Mainnet m/84'/0'/0'/0/0",&el_sans_14,0xa3a3a3);mn_addr=text(mn_output,16,184,416,"",&el_mono_16,0xeeeeee);
 mn_reveal=button(mn_output,16,260,200,"View entropy",mn_action,(void*)300);mn_entropy=text(mn_output,16,318,416,"",&el_mono_16,0xeeeeee);
 mn_back=button(mn_output,16,488,200,"Back to words",mn_action,(void*)301);
 mn_review=box(mn_panel,0,0,448,648,&card);lv_obj_set_style_border_width(mn_review,0,0);lv_obj_set_style_radius(mn_review,0,0);
 text(mn_review,16,16,416,"Review only - not yet validated",&el_sans_14,0xff9900);
 text(mn_review,16,48,416,"Dictionary and count do not prove checksum\nor entropy quality. No derivation performed.",&el_sans_14,0xa3a3a3);
 mn_review_list=box(mn_review,16,100,416,360,&well);
 lv_obj_add_flag(mn_review_list,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(mn_review_list,LV_DIR_VER);lv_obj_set_scrollbar_mode(mn_review_list,LV_SCROLLBAR_MODE_ON);
 lv_obj_set_style_width(mn_review_list,4,LV_PART_SCROLLBAR);lv_obj_set_style_bg_color(mn_review_list,lv_color_hex(0xff9900),LV_PART_SCROLLBAR);lv_obj_set_style_bg_opa(mn_review_list,LV_OPA_COVER,LV_PART_SCROLLBAR);
 for(unsigned i=0;i<24;i++)mn_review_rows[i]=text(mn_review_list,8+(i%2)*200,8+(i/2)*44,192,"",&el_mono_16,0xeeeeee);
 mn_review_back=button(mn_review,16,488,200,"Back to words",mn_action,(void*)301);
 mn_review_clear=button(mn_review,232,488,200,"Clear words",mn_action,(void*)3);
 /* Full panel intercepts touches; guards independently reject underlying events. */
 mn_modal=box(mn_panel,0,0,448,648,&card);lv_obj_set_style_border_width(mn_modal,0,0);lv_obj_set_style_radius(mn_modal,0,0);
 text(mn_modal,16,24,416,"PUBLIC TEST INPUT - NO REAL SECRETS",&el_sans_14,0xff9900);
 mn_modal_text=text(mn_modal,16,100,416,"",&el_sans_16,0xeeeeee);
 mn_no=button(mn_modal,16,260,200,"Cancel",mn_action,(void*)701);mn_yes=button(mn_modal,232,260,200,"Confirm",mn_action,(void*)702);
 lv_obj_set_height(mn_status,16);lv_obj_set_height(mn_draft_label,44);lv_obj_set_height(mn_hint,20);lv_obj_set_height(mn_pages,44);
 mn_safety=button(s,16,756,448,"Public test only / Safety",mn_action,(void*)800);
 mn_safety_modal=box(s,0,0,480,800,&base);
 mn_safety_content=box(mn_safety_modal,0,0,480,616,&base);
 text(mn_safety_content,28,84,424,"PUBLIC TEST INPUT - NO FUNDS",&el_sans_16,0xff9900);
 text(mn_safety_content,28,132,424,"No real secrets or funds. Never fund this address.",&el_sans_16,0xeeeeee);
 text(mn_safety_content,28,196,424,"Checksum validity does not establish entropy quality.",&el_sans_16,0xeeeeee);
 text(mn_safety_content,28,288,424,"Clearing is best effort, not secure erasure.",&el_sans_16,0xeeeeee);
 text(mn_safety_content,28,28,424,"Safety / About",&el_serif_24,0xeeeeee);
 text(mn_safety_content,28,244,424,"Encoded width is not measured randomness.",&el_sans_14,0xa3a3a3);
 text(mn_safety_content,28,336,424,"Owned buffers are cleared best effort. Crypto and\nallocator copies may remain. Saver is not a lock.",&el_sans_14,0xa3a3a3);
 text(mn_safety_content,28,400,424,"Unofficial native adaptation\nEntropyLab - Team Ooga Booga",&el_sans_16,0xeeeeee);
 text(mn_safety_content,28,460,424,"Origin: Mr.Hodl (@mrhodl)\nNo upstream endorsement or affiliation.",&el_sans_14,0xa3a3a3);
 text(mn_safety_content,28,516,424,"https://github.com/OogaBoogaX/entropylab",&el_sans_14,0xa3a3a3);
 text(mn_safety_content,28,564,424,"THE OOGA BOOGA LICENSE retained.\nFull offline notices: THIRD_PARTY_NOTICES.md\nLogo: upstream asset, not sole-author credit.",&el_sans_14,0xa3a3a3);
 mn_about_content=box(mn_safety_modal,0,0,480,616,&base);
 text(mn_about_content,28,28,424,"About this input",&el_serif_24,0xeeeeee);
 mn_about_label=text(mn_about_content,28,84,424,"",&el_sans_16,0xff9900);
 mn_about_scroll=box(mn_about_content,28,120,424,496,&base);
 lv_obj_add_flag(mn_about_scroll,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(mn_about_scroll,LV_DIR_VER);
 lv_obj_set_scrollbar_mode(mn_about_scroll,LV_SCROLLBAR_MODE_AUTO);
 lv_obj_remove_flag(mn_about_scroll,LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM);
 lv_obj_set_style_width(mn_about_scroll,4,LV_PART_SCROLLBAR);lv_obj_set_style_bg_color(mn_about_scroll,lv_color_hex(0xff9900),LV_PART_SCROLLBAR);
 mn_about_body=text(mn_about_scroll,0,0,424,"",&el_sans_16,0xeeeeee);
 mn_about=button(mn_safety_modal,28,632,424,"About this input",mn_action,(void*)802);
 mn_about_back=button(mn_safety_modal,28,632,424,"Back to Safety",mn_action,(void*)803);
 mn_safety_close=button(mn_safety_modal,28,706,424,"Close",mn_action,(void*)801);
 mn_invalidate();mn_refresh();lv_obj_add_flag(mn_panel,LV_OBJ_FLAG_HIDDEN);
}
