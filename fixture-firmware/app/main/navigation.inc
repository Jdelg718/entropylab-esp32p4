/* UI-only route IDs. Never serialized into crypto requests. */
static lv_obj_t *nav_family,*nav_method,*nav_sheet,*nav_panel,*nav_rows[7],*nav_close,*nav_title;
static lv_obj_t *nav_context_target,*nav_context_allowed;
static gui08_editor *nav_context_owner;
static uint64_t nav_context_revision;
static unsigned nav_context_route;
static unsigned nav_route,nav_dice=2,nav_pending;
static bool nav_methods,nav_confirming;
static const char *nav_names[]={"Hex","Coins","D6 raw · hashed rolls","D6 Coleman · hashed rolls","Words","Seed","Cards","Bases","BitBox diceware","D++ · D8 / D16"};
static const char *nav_family_descriptions[]={
 "0–F · original hexadecimal input","Heads / tails · raw encoded bits","D6 raw, Coleman, BitBox and D++",
 "English mnemonic · dictionary entry","BIP39 word numbers · checksum picker","Direct card-rank mapping","Canonical base 4, 8, 32 and 64"
};
static const char *nav_method_descriptions[]={
 "Original 1–6 digits · SHA-256","6 → 0 before SHA-256","Five D4 rolls + D6 coin per prefix word","D8 + D16 + D16; size-specific final rolls"
};
static const lv_point_precise_t nav_check_short[]={{0,8},{6,14}};
static const lv_point_precise_t nav_check_long[]={{6,14},{19,1}};
static const unsigned nav_families[]={0,1,2,4,5,6,7};
static const unsigned nav_dices[]={2,3,8,9};
static void nav_confirmation(const char *source,const char *target){
 char caption[144];snprintf(caption,sizeof caption,"%s -> %s\nInput and result will be cleared.",source,target);
 lv_label_set_text(nav_title,caption);
}
static void nav_context_confirmation(const char *owner,const char *kind,unsigned before,unsigned after){
 char caption[144];snprintf(caption,sizeof caption,"%s: %s%u -> %s%u%s\nInput and result will be cleared.",
  owner,kind,before,kind,after,*kind?"":" words");lv_label_set_text(nav_title,caption);
}
static bool nav_busy(void){return busy||mn_confirm||gui08_ui.editor.pending_id||seed_ui.editor.pending_id||extra_dice_ui.editor.pending_id||(pi_owner!=PI_OWNER_NONE&&pi_ui.root&&!lv_obj_has_flag(pi_ui.root,LV_OBJ_FLAG_HIDDEN));}
static void nav_event(lv_event_t *e);
static void nav_refresh(void){if(nav_family){if(nav_busy()){lv_obj_add_state(nav_family,LV_STATE_DISABLED);lv_obj_add_state(nav_method,LV_STATE_DISABLED);}else{lv_obj_remove_state(nav_family,LV_STATE_DISABLED);lv_obj_remove_state(nav_method,LV_STATE_DISABLED);}}}
static void nav_bind(void){
 if(!nav_family)return;
 saver_bind_legacy(nav_family,nav_event,100);saver_bind_legacy(nav_method,nav_event,101);
 saver_bind_legacy(nav_close,nav_event,102);saver_bind_legacy(nav_sheet,nav_event,103);
 for(unsigned i=0;i<7;i++)saver_bind_legacy(nav_rows[i],nav_event,i);
}
static void nav_hide(void){nav_open=false;nav_context_target=NULL;lv_obj_add_flag(nav_sheet,LV_OBJ_FLAG_HIDDEN);saver_rebind_controls();mn_rebind();}
static void nav_labels(void){
 char caption[80];unsigned family=nav_route==3||nav_route>=8?2:nav_route;
 snprintf(caption,sizeof caption,"Input family   %s   v",family==2?"Dice":nav_names[family]);lv_label_set_text(lv_obj_get_child(nav_family,0),caption);
 snprintf(caption,sizeof caption,"%s v",nav_names[nav_dice]);lv_label_set_text(lv_obj_get_child(nav_method,0),caption);
 lv_obj_set_pos(nav_family,16,56);lv_obj_set_size(nav_family,448,48);
 if(family==2){
  /* D6's finalized contract keeps the legacy panel at its fixed absolute
   * coordinates. The in-card dicemethod label already discloses RAW/Coleman. */
  lv_obj_add_flag(nav_method,LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_y(tabs[0],35);lv_obj_set_y(tabs[1],35);lv_obj_set_y(input,88);lv_obj_set_y(output,89);lv_obj_set_height(input,545);lv_obj_set_height(output,545);
 }else {
  lv_obj_add_flag(nav_method,LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_y(tabs[0],35);lv_obj_set_y(tabs[1],35);lv_obj_set_y(input,89);lv_obj_set_y(output,89);lv_obj_set_height(input,545);lv_obj_set_height(output,545);
 }
}
static void nav_commit(unsigned route){
 nav_hide();if(route==nav_route)return;
 if(route<=4)lv_obj_send_event(modes[route],LV_EVENT_CLICKED,NULL);
 else {unsigned i=route==5?2:route==6?0:route==7?1:3;lv_obj_send_event(gui08_routes[i],LV_EVENT_CLICKED,NULL);
 if(route>=8)lv_obj_send_event(extra_dice_ui.method_buttons[route-8],LV_EVENT_CLICKED,NULL);}
 nav_route=route;if(route==2||route==3||route>=8)nav_dice=route;nav_labels();
}
static void nav_show_action_rows(void){
 static const char *titles[]={"Keep input","Change and clear"};
 for(unsigned i=0;i<7;i++){
  if(i>=2){lv_obj_add_flag(nav_rows[i],LV_OBJ_FLAG_HIDDEN);continue;}
  lv_obj_t *primary=lv_obj_get_child(nav_rows[i],0),*secondary=lv_obj_get_child(nav_rows[i],1),*check=lv_obj_get_child(nav_rows[i],2);
  lv_label_set_text(primary,titles[i]);lv_obj_set_style_text_color(primary,lv_color_hex(0xeeeeee),0);
  lv_label_set_text(secondary,"");lv_obj_add_flag(secondary,LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(check,LV_OBJ_FLAG_HIDDEN);
  lv_obj_remove_state(nav_rows[i],LV_STATE_CHECKED);lv_obj_remove_flag(nav_rows[i],LV_OBJ_FLAG_HIDDEN);
 }
}
static void nav_show(bool methods){
 nav_context_target=NULL;nav_methods=methods;nav_confirming=false;nav_open=true;saver_rebind_controls();mn_rebind();
 lv_obj_set_height(nav_panel,methods?374:590);
 lv_label_set_text(nav_title,methods?"Dice method":"Input family");
 for(unsigned i=0;i<7;i++){
  if(methods&&i>=4){lv_obj_add_flag(nav_rows[i],LV_OBJ_FLAG_HIDDEN);continue;}
  unsigned r=methods?nav_dices[i]:nav_families[i];bool selected=methods?r==nav_dice:r==(nav_route==3||nav_route>=8?2:nav_route);
  lv_obj_t *primary=lv_obj_get_child(nav_rows[i],0),*secondary=lv_obj_get_child(nav_rows[i],1),*check=lv_obj_get_child(nav_rows[i],2);
  lv_label_set_text(primary,!methods&&r==2?"Dice":nav_names[r]);
  lv_label_set_text(secondary,methods?nav_method_descriptions[i]:nav_family_descriptions[i]);
  lv_obj_remove_flag(secondary,LV_OBJ_FLAG_HIDDEN);
  lv_obj_set_style_text_color(primary,lv_color_hex(selected?0x000000:0xeeeeee),0);
  lv_obj_set_style_text_color(secondary,lv_color_hex(selected?0x333333:0xa3a3a3),0);
  if(selected)lv_obj_remove_flag(check,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(check,LV_OBJ_FLAG_HIDDEN);
  lv_obj_remove_flag(nav_rows[i],LV_OBJ_FLAG_HIDDEN);
  if(selected)lv_obj_add_state(nav_rows[i],LV_STATE_CHECKED);else lv_obj_remove_state(nav_rows[i],LV_STATE_CHECKED);
 }
 lv_obj_remove_flag(nav_sheet,LV_OBJ_FLAG_HIDDEN);lv_obj_move_foreground(nav_sheet);
}
static void nav_event(lv_event_t *e){
 if(saver_blocked()||mn_safety_open||saver_epoch_exhausted||((uintptr_t)lv_event_get_user_data(e)>>10)!=saver_epoch||nav_busy())return;
 unsigned action=saver_payload(e);if(action==102||action==103){nav_hide();return;}
 if(action>=100){if(!nav_open)nav_show(action==101);return;}if(!nav_open)return;
 if(nav_confirming){if(action==0)nav_hide();else if(action==1){
 if(nav_context_target){lv_obj_t *target=nav_context_target;bool valid=nav_route==nav_context_route&&nav_context_owner->revision==nav_context_revision;
 nav_hide();if(valid){nav_context_allowed=target;lv_obj_send_event(target,LV_EVENT_CLICKED,NULL);nav_context_allowed=NULL;}}
 else nav_commit(nav_pending);}return;}
 if(action>=(nav_methods?4u:7u))return;
 /* The fixed D6 panel has no spare row for nav_method. Keep all four dice
  * methods pointer-reachable through the visible family sheet instead. Merely
  * opening this chooser must not commit a route or discard owned input. */
 if(!nav_methods&&nav_families[action]==2){nav_show(true);return;}
 unsigned route=nav_methods?nav_dices[action]:nav_families[action];
 bool destructive=(route!=nav_route)&&((route==6||route==7)&&gui08_ui.editor.method!=(route==6?GUI08_CARDS:GUI08_BASES)&&(gui08_ui.editor.length||gui08_ui.editor.result_visible));
 destructive|=route>=8&&extra_dice_ui.method!=route-7&&(extra_dice_ui.editor.length||extra_dice_ui.final_length||extra_dice_ui.editor.result_visible);
 destructive|=route<=4&&route!=nav_route&&(mn_draft[0]||mn_edit>=0);
 if(!destructive){nav_commit(route);return;}
 nav_pending=route;nav_confirming=true;nav_confirmation(nav_names[nav_route],nav_names[route]);
 nav_show_action_rows();saver_rebind_controls();
}
/* Intercept only destructive UI context controls, before owner callbacks mutate.
 * Confirmation retains object identity + owner revision, never a crypto request. */
static bool nav_context_guard(lv_event_t *e){
 lv_obj_t *target=lv_event_get_target(e);
 if(target==nav_context_allowed){nav_context_allowed=NULL;return false;}
 static const unsigned counts[]={12,15,18,21,24},bases[]={4,8,32,64};
 gui08_editor *owner=NULL;bool same=false,meaningful=false;unsigned next_words=0,next_base=0;
 if(gui08_native_visible(&gui08_ui)){
  for(unsigned i=0;i<5;i++)if(target==gui08_ui.word_buttons[i]){owner=&gui08_ui.editor;next_words=counts[i];same=owner->words==counts[i];}
  for(unsigned i=0;i<4;i++)if(target==gui08_ui.base_buttons[i]){owner=&gui08_ui.editor;next_base=bases[i];same=owner->base==bases[i];}
  meaningful=gui08_ui.editor.length||gui08_ui.editor.result_visible;
 }else if(seed_native_visible(&seed_ui)){
  for(unsigned i=0;i<5;i++)if(target==seed_ui.word_buttons[i]){owner=&seed_ui.editor;next_words=counts[i];same=owner->words==counts[i];}
  meaningful=seed_ui.draft_len||seed_ui.committed||seed_ui.editor.length||seed_ui.editor.result_visible;
 }else if(extra_dice_native_visible(&extra_dice_ui)){
  for(unsigned i=0;i<5;i++)if(target==extra_dice_ui.word_buttons[i]){owner=&extra_dice_ui.editor;next_words=counts[i];same=owner->words==counts[i];}
  meaningful=extra_dice_ui.editor.length||extra_dice_ui.final_length||extra_dice_ui.final_selected||extra_dice_ui.editor.result_visible;
 }
 if(!owner)return false;
 if(same||nav_busy())return true;
 if(!meaningful)return false;
 nav_show(false);nav_confirming=true;nav_context_target=target;nav_context_owner=owner;nav_context_revision=owner->revision;nav_context_route=nav_route;
 if(next_base)nav_context_confirmation(nav_names[nav_route],"base ",owner->base,next_base);
 else nav_context_confirmation(nav_names[nav_route],"",owner->words,next_words);
 nav_show_action_rows();
 saver_rebind_controls();return true;
}
static void nav_create(lv_obj_t *parent){
 nav_route=0;nav_dice=2;nav_open=false;
 for(unsigned i=0;i<GUI_MODE_COUNT;i++)lv_obj_add_flag(modes[i],LV_OBJ_FLAG_HIDDEN);
 for(unsigned i=0;i<4;i++)lv_obj_add_flag(gui08_routes[i],LV_OBJ_FLAG_HIDDEN);
 for(unsigned i=0;i<2;i++){lv_obj_add_flag(extra_dice_ui.method_buttons[i],LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(gui08_ui.method_buttons[i],LV_OBJ_FLAG_HIDDEN);}
 nav_family=button(parent,16,56,448,"",NULL,NULL);lv_obj_set_height(nav_family,48);
 nav_method=button(parent,32,132,416,"",NULL,NULL);lv_obj_set_height(nav_method,44);
 nav_sheet=box(parent,0,0,480,800,&base);lv_obj_set_style_bg_opa(nav_sheet,LV_OPA_70,0);lv_obj_remove_flag(nav_sheet,LV_OBJ_FLAG_SCROLLABLE);
 nav_panel=box(nav_sheet,16,116,448,590,&card);lv_obj_set_style_pad_all(nav_panel,0,0);lv_obj_set_style_radius(nav_panel,20,0);lv_obj_remove_flag(nav_panel,LV_OBJ_FLAG_SCROLLABLE);
 nav_title=text(nav_panel,16,16,316,"Input family",&el_sans_16,0xeeeeee);lv_obj_set_height(nav_title,44);
 nav_close=button(nav_panel,352,16,80,"Close",NULL,NULL);
 for(unsigned i=0;i<7;i++){
  nav_rows[i]=button(nav_panel,16,76+72*i,416,"",NULL,NULL);lv_obj_set_height(nav_rows[i],64);
  lv_obj_t *primary=lv_obj_get_child(nav_rows[i],0);lv_obj_set_align(primary,LV_ALIGN_TOP_LEFT);lv_obj_set_pos(primary,16,7);lv_obj_set_size(primary,350,20);lv_obj_set_style_text_font(primary,&el_sans_16,0);
  lv_obj_t *secondary=text(nav_rows[i],16,32,350,"",&el_sans_14,0xa3a3a3);lv_obj_set_height(secondary,20);
  lv_obj_t *check=lv_obj_create(nav_rows[i]);lv_obj_remove_style_all(check);lv_obj_set_pos(check,380,21);lv_obj_set_size(check,20,20);lv_obj_remove_flag(check,LV_OBJ_FLAG_SCROLLABLE|LV_OBJ_FLAG_CLICKABLE);
  lv_obj_t *short_line=lv_line_create(check);lv_line_set_points(short_line,nav_check_short,2);lv_obj_set_style_line_color(short_line,lv_color_hex(0),0);lv_obj_set_style_line_width(short_line,3,0);lv_obj_set_style_line_rounded(short_line,true,0);
  lv_obj_t *long_line=lv_line_create(check);lv_line_set_points(long_line,nav_check_long,2);lv_obj_set_style_line_color(long_line,lv_color_hex(0),0);lv_obj_set_style_line_width(long_line,3,0);lv_obj_set_style_line_rounded(long_line,true,0);
  lv_obj_add_flag(check,LV_OBJ_FLAG_HIDDEN);
 }
 lv_obj_add_flag(nav_sheet,LV_OBJ_FLAG_HIDDEN);nav_labels();nav_bind();
}
static const education_document *active_education_document(void){
 if(gui08_native_visible(&gui08_ui)){
  if(gui08_ui.editor.method==GUI08_CARDS)return &education_cards;
  if(gui08_ui.editor.method==GUI08_BASES)return &education_bases;
  return NULL;
 }
 if(seed_native_visible(&seed_ui))return seed_ui.editor.method==GUI08_SEED?&education_seed:NULL;
 if(extra_dice_native_visible(&extra_dice_ui)){
  if(extra_dice_ui.editor.method!=GUI08_DICE)return NULL;
  if(extra_dice_ui.method==GUI08_DICE_BITBOX)return &education_bitbox;
  if(extra_dice_ui.method==GUI08_DICE_DPLUS)return &education_dplus;
  return NULL;
 }
 if(mode==MODE_MNEMONIC)return &education_words;
 return NULL;
}
static bool active_about_legacy(void){return !gui08_native_visible(&gui08_ui)&&!seed_native_visible(&seed_ui)&&!extra_dice_native_visible(&extra_dice_ui)&&(unsigned)mode<GUI_MODE_COUNT;}
static const char *active_about_name(void){const education_document *doc=active_education_document();if(doc)return doc->title;return active_about_legacy()?mn_explanation_names[mode]:"Explanation unavailable.";}
static const char *active_about_copy(void){return active_about_legacy()?mn_explanation_copy[mode]:"Explanation unavailable.";}
