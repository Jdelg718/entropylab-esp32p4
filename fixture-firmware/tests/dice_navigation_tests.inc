/* Approved successor to Back-discard characterization, preserved in tests.patch
 * and characterization.log in the immutable investigation. Public 100-roll fixture.
 * No mnemonic/address printing or screenshots. Real handlers + dispatcher + Rust. */
static void dn_continue(void){
 lv_obj_send_event(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_CONTINUE),LV_EVENT_CLICKED,NULL);
 assert(pi_dispatch.state==EL_DISPATCH_QUEUED);
 gui_passphrase_work(pi_test_token);gui_passphrase_poll();
 assert(passphrase_ui_result_visible(&pi_ui));
 assert(strlen(lv_label_get_text(pi_mnemonic_label))>0);
}
static void dn_back(void){lv_obj_send_event(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_BACK),LV_EVENT_CLICKED,NULL);}
static void dice_navigation_tests(void){
 gui_passphrase_configure(pi_test_lock,pi_test_unlock,pi_test_publish);
 pi_lifehash_render_fail=false;gui_passphrase_lifehash_configure(pi_lifehash_render_probe);
 lv_obj_clean(lv_screen_active());gui_create(request);click("D6 raw");
 lv_obj_send_event(dicechoices[4],LV_EVENT_CLICKED,NULL);
 for(unsigned i=0;i<100;i++)lv_obj_send_event(dicekeys[i%6],LV_EVENT_CLICKED,NULL);
 assert(mode==MODE_D6_RAW&&lengths[mode]==100&&selector==24);
 click("Calculate");dn_continue();
 char saved_mn[sizeof ((hex_result_t*)0)->mnemonic],saved_en[sizeof ((hex_result_t*)0)->entropy],saved_fp[9],saved_addr[sizeof ((hex_result_t*)0)->address];
 strcpy(saved_mn,lv_label_get_text(pi_mnemonic_label));strcpy(saved_en,lv_label_get_text(pi_entropy_label));strcpy(saved_fp,lv_label_get_text(pi_ui.result_fp));strcpy(saved_addr,lv_label_get_text(pi_ui.result_addr));
 assert(pi_completed_matches());
 unsigned char saved_owner[sizeof pi_completed];memcpy(saved_owner,&pi_completed,sizeof saved_owner);
 uint32_t saved_pixels[32*32];memcpy(saved_pixels,pi_ui.lifehash.pixels,sizeof saved_pixels);
 assert(pi_ui.lifehash.visible);unsigned render_calls=pi_lifehash_render_calls;
 uint64_t original_gen=pi_ui.generation,original_id=pi_ui.next_id;
 dn_back();pp_clear_result_text(pi_ui.result);fingerprint_view_blank(&pi_ui.lifehash);
 assert(!memcmp(saved_owner,&pi_completed,sizeof saved_owner));
 assert(pi_ui.result_visible);lv_obj_send_event(tabs[1],LV_EVENT_CLICKED,NULL);
 assert(!strcmp(saved_mn,lv_label_get_text(pi_mnemonic_label)));
 assert(!strcmp(saved_en,lv_label_get_text(pi_entropy_label)));
 assert(!strcmp(saved_fp,lv_label_get_text(pi_ui.result_fp)));
 assert(!strcmp(saved_addr,lv_label_get_text(pi_ui.result_addr)));
 assert(!memcmp(saved_owner,&pi_completed,sizeof saved_owner));
 assert(pi_ui.lifehash.visible&&!memcmp(saved_pixels,pi_ui.lifehash.pixels,sizeof saved_pixels));
 assert(pi_lifehash_render_calls==render_calls&&pi_ui.generation==original_gen&&pi_ui.next_id==original_id);
 gui_passphrase_work(pi_test_token);gui_passphrase_poll();
 assert(!memcmp(saved_owner,&pi_completed,sizeof saved_owner));
 puts("PASS explicit owner survives erased widgets; full text and LifeHash byte-identical without computation; duplicate completion unchanged");
 uint64_t rev=pi_revision,gen=pi_ui.generation,id=pi_ui.next_id;
 for(unsigned i=0;i<3;i++){
  dn_back();assert(lv_obj_has_flag(pi_ui.root,LV_OBJ_FLAG_HIDDEN));
  lv_obj_send_event(tabs[1],LV_EVENT_CLICKED,NULL);
  assert(!lv_obj_has_flag(pi_ui.root,LV_OBJ_FLAG_HIDDEN));
  assert(passphrase_ui_result_visible(&pi_ui));
  assert(pi_revision==rev&&pi_ui.generation==gen&&pi_ui.next_id==id);
  assert(!valid_result&&!legacy_pending);
  assert(strstr(lv_label_get_text(pi_ui.result_marker_label),"Empty"));
 }
 puts("PASS retained Back/reopen repeated, same generation/request, no legacy secret cache or recalculation");
 assert(passphrase_ui_set_raw(&pi_ui,(const uint8_t*)"PUBLIC",6)==0);
 assert(!passphrase_ui_result_visible(&pi_ui));
 assert(!strlen(lv_label_get_text(pi_mnemonic_label)));
 assert(!strlen(lv_label_get_text(pi_entropy_label)));
 assert(!strlen(lv_label_get_text(pi_ui.result_fp)));
 assert(!strlen(lv_label_get_text(pi_ui.result_addr)));
 dn_continue();dn_back();lv_obj_send_event(tabs[1],LV_EVENT_CLICKED,NULL);
 assert(passphrase_ui_result_visible(&pi_ui));
 assert(strstr(lv_label_get_text(pi_ui.result_marker_label),"Active"));
 puts("PASS passphrase change clears owned labels; Active result retained");
 lv_obj_send_event(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_CONTINUE),LV_EVENT_CLICKED,NULL);
 assert(pi_ui.pending);
 assert(!lv_obj_has_state(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_BACK),LV_STATE_DISABLED));
 gui_passphrase_work(pi_test_token);dn_back();gui_passphrase_poll();
 assert(!pi_ui.pending&&!pi_ui.result_visible);
 assert(pi_full_token==0);
 for(size_t i=0;i<sizeof pi_full_result;i++)assert(((unsigned char*)&pi_full_result)[i]==0);
 assert(!strlen(lv_label_get_text(pi_mnemonic_label)));
 lv_obj_send_event(tabs[1],LV_EVENT_CLICKED,NULL);assert(!pi_ui.result_visible);
 puts("PASS pending Back cancels finished-but-unpolled worker without promotion");
 for(unsigned change=0;change<7;change++){
  dn_continue();
  if(change==0)click("Cancel");
  if(change==1)passphrase_ui_clear(&pi_ui);
  if(change==2){dn_back();lv_obj_send_event(dicekeys[0],LV_EVENT_CLICKED,NULL);lv_obj_send_event(edits[16],LV_EVENT_CLICKED,NULL);assert(lengths[mode]==100);}
  if(change==3){dn_back();lv_obj_send_event(dicechoices[0],LV_EVENT_CLICKED,NULL);lv_obj_send_event(dicechoices[4],LV_EVENT_CLICKED,NULL);}
  if(change==4){dn_back();click("Coins");click("D6 raw");}
  if(change==5){passphrase_ui_context_t c=pi_ui.context;c.derivation_context++;passphrase_ui_set_context(&pi_ui,c);}
  if(change==6){passphrase_ui_set_raw(&pi_ui,(const uint8_t*)"OTHER",5);passphrase_ui_set_raw(&pi_ui,NULL,0);}
  assert(!pi_ui.result_visible);
  for(size_t i=0;i<sizeof pi_completed;i++)assert(((unsigned char*)&pi_completed)[i]==0);
  assert(!strlen(lv_label_get_text(pi_mnemonic_label))&&!strlen(lv_label_get_text(pi_entropy_label)));
  assert(!strlen(lv_label_get_text(pi_ui.result_fp))&&!strlen(lv_label_get_text(pi_ui.result_addr)));
  lv_obj_send_event(tabs[1],LV_EVENT_CLICKED,NULL);assert(!pi_ui.result_visible);
 }
 lv_obj_send_event(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_CONTINUE),LV_EVENT_CLICKED,NULL);
 uint64_t old_token=pi_test_token;dn_back();gui_passphrase_work(old_token);gui_passphrase_poll();assert(!pi_ui.result_visible);
 lv_obj_send_event(tabs[1],LV_EVENT_CLICKED,NULL);dn_continue();
 memcpy(saved_owner,&pi_completed,sizeof saved_owner);
 gui_passphrase_work(old_token);gui_passphrase_poll();assert(pi_ui.result_visible);
 assert(!memcmp(saved_owner,&pi_completed,sizeof saved_owner));
 lv_obj_send_event(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_CONTINUE),LV_EVENT_CLICKED,NULL);
 gui_passphrase_work(pi_test_token);pi_source_changed();gui_passphrase_poll();assert(!pi_ui.result_visible);
 puts("PASS Cancel/Clear/input undo/wordcount restore/mode restore/context/passphrase restore invalidate and clear labels; queued Back and stale request rejected");
 dn_back();lv_obj_send_event(tabs[1],LV_EVENT_CLICKED,NULL);dn_continue();
 assert(!lv_obj_has_state(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_CLEAR),LV_STATE_DISABLED));
 lv_obj_send_event(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_CLEAR),LV_EVENT_CLICKED,NULL);
 assert(!pi_ui.result_visible&&!strlen(lv_label_get_text(pi_mnemonic_label)));
 dn_continue();
 gui_passphrase_dispose();
 for(size_t i=0;i<sizeof pi_completed;i++)assert(((unsigned char*)&pi_completed)[i]==0);
 gui_passphrase_configure(NULL,NULL,NULL);
}
