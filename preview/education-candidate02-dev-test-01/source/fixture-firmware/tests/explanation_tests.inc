/* Public fixtures only. Navigation must not touch any editor/result state. */
/* Frozen contract copy: independent of the production lookup table. */
static const char *const expected_explanation_names[GUI_MODE_COUNT]={"HEX","Coins","D6 raw","D6 6->0 (Coleman)","Words"};
static const char *const expected_explanation_copy[GUI_MODE_COUNT]={
 "Each hex character encodes 4 bits. Accepted lengths are 32, 40, 48, 56 or 64 characters.\n\n"
 "The displayed bit count is encoded width, not measured randomness. A full-length input can still be predictable.\n\n"
 "This mode uses the entered hex bytes as the BIP39 input. It does not measure how that input was generated.",
 "Heads = 0. Tails = 1. Bits are packed in entry order, most-significant bit first.\n\n"
 "Use exactly 128, 160, 192, 224 or 256 flips for 12, 15, 18, 21 or 24 words, respectively. There is no input hash, padding or truncation.\n\n"
 "The preview shows the latest 64 bits; the full input is retained. Encoded width is not measured randomness. An exact count does not prove fair, independent flips.",
 "Each roll is recorded as its ASCII digit, 1 through 6. SHA-256 hashes ALL entered rolls in order, not just the preview.\n\n"
 "For 12, 15, 18, 21 or 24 words, the first 16, 20, 24, 28 or 32 bytes of the hash are used, respectively.\n\n"
 "Nominal roll counts assume fair, independent dice. WEAK_INPUT_LAB_ONLY means the input is below the nominal count; meeting that count does not prove fairness or randomness quality.\n\n"
 "Hashing cannot manufacture entropy. A predictable roll sequence remains predictable after hashing. Hash output width is not measured randomness.",
 "Each roll is recorded as a digit. Every 6 is mapped to ASCII 0 before hashing; digits 1 through 5 are unchanged. SHA-256 hashes ALL mapped rolls in order, not just the preview.\n\n"
 "For 12, 15, 18, 21 or 24 words, the first 16, 20, 24, 28 or 32 bytes of the hash are used, respectively.\n\n"
 "Nominal roll counts assume fair, independent dice. WEAK_INPUT_LAB_ONLY means the input is below the nominal count; meeting that count does not prove fairness or randomness quality.\n\n"
 "Hashing cannot manufacture entropy. A predictable roll sequence remains predictable after hashing. Hash output width is not measured randomness.",
 "#ff9900 What it does#\nWords accepts typed BIP39 English words and validates the selected count, dictionary membership, and checksum.\n\n"
 "#ff9900 When to choose it#\nUse it to import or review a phrase produced elsewhere. This route does not generate randomness.\n\n"
 "#ff9900 What you need#\nA public practice phrase or an existing phrase from an external process. Never enter a funded phrase in this laboratory build.\n\n"
 "#ff9900 Steps#\n1. Choose 12, 15, 18, 21, or 24 words.\n2. Type a prefix with the letter keys.\n3. Select a displayed dictionary candidate.\n4. Review committed words; replace a word if needed.\n5. Tap Validate when the selected count is complete.\n\n"
 "#ff9900 Where unpredictability comes from#\nOnly from the external process that produced the phrase. Typing and dictionary lookup add none.\n\n"
 "#ff9900 What adds no entropy#\nDictionary checks, checksum validation, hashing, PBKDF2, and LifeHash are deterministic and add no entropy.\n\n"
 "#ff9900 Accepted input, discards, and limits#\nOnly displayed BIP39 English dictionary candidates can be committed. Supported counts are 12, 15, 18, 21, and 24. Words accepts words; Seed accepts word-list numbers and has number preview and a final-candidate helper. Both import existing entropy; neither invents it."
};

static void explanation_rect(lv_obj_t *o,int x,int y,int w,int h){
 lv_area_t a;lv_obj_get_coords(o,&a);assert(a.x1==x&&a.y1==y);
 assert(lv_obj_get_width(o)==w&&lv_obj_get_height(o)==h);
}
static void explanation_preserved(void){
 char transcript_save[sizeof transcripts],draft_save[sizeof mn_draft],phrase_save[sizeof mnemonic_transcript],recovered_save[sizeof mn_recovered];
 memcpy(transcript_save,transcripts,sizeof transcripts);memcpy(draft_save,mn_draft,sizeof mn_draft);
 memcpy(phrase_save,mnemonic_transcript,sizeof mnemonic_transcript);memcpy(recovered_save,mn_recovered,sizeof mn_recovered);
 size_t lengths_save[GUI_MODE_COUNT];memcpy(lengths_save,lengths,sizeof lengths);
 uint16_t words_save[24];memcpy(words_save,mn_words,sizeof mn_words);
 unsigned candidates_save[3];memcpy(candidates_save,mn_candidates,sizeof mn_candidates);
 unsigned before=runs,count_save=mn_count,target_save=mn_target,page_save=mn_page,matches_save=mn_matches,selector_save=selector;
 int edit_save=mn_edit,scroll_save=lv_obj_get_scroll_y(mn_list),review_scroll=lv_obj_get_scroll_y(mn_review_list);
 uint64_t rev=mn_revision,legacy_rev=legacy_revision,id=mn_id,lid=legacy_id,pending=mn_pending,lpending=legacy_pending;
 bool valid=mn_valid,reviewing=mn_reviewing,legacy_valid=valid_result;
 char fingerprint[32],address[128],entropy[65];strcpy(fingerprint,lv_label_get_text(mn_fp));strcpy(address,lv_label_get_text(mn_addr));strcpy(entropy,lv_label_get_text(mn_entropy));
 const char *legacy_fp=lv_label_get_text(fp),*legacy_addr=lv_label_get_text(addr);
 click(SAFETY_DOCK_TEXT);click("About this input");
 lv_obj_send_event(mn_keys[0],LV_EVENT_CLICKED,NULL);lv_obj_send_event(mn_clear,LV_EVENT_CLICKED,NULL);
 lv_obj_send_event(mn_validate,LV_EVENT_CLICKED,NULL);lv_obj_send_event(mn_targets[4],LV_EVENT_CLICKED,NULL);
 lv_obj_send_event(modes[0],LV_EVENT_CLICKED,NULL);lv_obj_send_event(run,LV_EVENT_CLICKED,NULL);
 click("Back to Safety");click("About this input");click("Close");
 assert(!memcmp(transcript_save,transcripts,sizeof transcripts)&&!memcmp(lengths_save,lengths,sizeof lengths));
 assert(!memcmp(draft_save,mn_draft,sizeof mn_draft)&&!memcmp(phrase_save,mnemonic_transcript,sizeof mnemonic_transcript));
 assert(!memcmp(words_save,mn_words,sizeof mn_words)&&!memcmp(candidates_save,mn_candidates,sizeof mn_candidates));
 assert(!memcmp(recovered_save,mn_recovered,sizeof mn_recovered));
 assert(runs==before&&mn_count==count_save&&mn_target==target_save&&mn_page==page_save&&mn_matches==matches_save&&selector==selector_save&&mn_edit==edit_save);
 assert(mn_revision==rev&&legacy_revision==legacy_rev&&mn_id==id&&legacy_id==lid&&mn_pending==pending&&legacy_pending==lpending);
 assert(mn_valid==valid&&mn_reviewing==reviewing&&valid_result==legacy_valid);
 assert(lv_obj_get_scroll_y(mn_list)==scroll_save&&lv_obj_get_scroll_y(mn_review_list)==review_scroll);
 assert(!strcmp(fingerprint,lv_label_get_text(mn_fp))&&!strcmp(address,lv_label_get_text(mn_addr))&&!strcmp(entropy,lv_label_get_text(mn_entropy)));
 assert(legacy_fp==lv_label_get_text(fp)&&legacy_addr==lv_label_get_text(addr));
}
static void explanation_tests(lv_indev_t *dev){
 const char *names[]={"Hex","Coins","D6 raw","D6 6->0","Words"};
 for(unsigned m=0;m<5;m++){
  click(names[m]);
  void *oldedit=saved_callback(edits[0],edit),*oldmode=saved_callback(modes[(m+1)%5],switch_mode),*oldcalc=saved_callback(run,calculate),*oldkey=saved_action(mn_keys[0]);
  unsigned before=runs;uint64_t rev=mn_revision,lr=legacy_revision;
  click(SAFETY_DOCK_TEXT);
  assert(find_button(mn_safety_modal,"About this input"));
  render(m==0?"explanation-safety-entry.ppm":NULL);
  explanation_rect(mn_about,28,632,424,44);explanation_rect(mn_safety_close,28,706,424,44);
  for(unsigned i=0;i<lv_obj_get_child_count(mn_safety_content);i++){lv_area_t a;lv_obj_get_coords(lv_obj_get_child(mn_safety_content,i),&a);assert(a.y2<632);}
  lv_obj_t *modal=mn_safety_modal;
  void *about=saved_action(mn_about),*close=saved_action(mn_safety_close);
  uintptr_t gen=mn_generation,epoch=saver_epoch;
  click("About this input");
  assert(mn_safety_open&&mn_about_open&&mn_safety_modal==modal&&mn_generation>gen&&saver_epoch>epoch);
  assert(lv_obj_get_style_bg_opa(modal,0)==LV_OPA_COVER);
  assert(!strcmp(lv_label_get_text(mn_about_label),expected_explanation_names[m]));
  assert(!strcmp(lv_label_get_text(mn_about_body),expected_explanation_copy[m]));
  /* Both pages retain their objects; only the selected page is visible. */
  assert(lv_obj_is_visible(mn_about_content)&&!lv_obj_is_visible(mn_safety_content));
  assert(!lv_obj_is_visible(mn_about)&&lv_obj_is_visible(mn_about_back)&&lv_obj_is_visible(mn_safety_close));
  char image[64];snprintf(image,sizeof image,"explanation-%u.ppm",m);render(image);
  explanation_rect(mn_about_label,28,28,424,36);
  explanation_rect(mn_about_back,28,632,424,44);explanation_rect(mn_safety_close,28,706,424,44);
  assert(lv_obj_get_style_text_font(mn_about_body,0)==&el_sans_16);
  assert(lv_obj_get_scroll_dir(mn_about_scroll)==LV_DIR_VER);
  lv_area_t body;lv_obj_get_coords(mn_about_scroll,&body);assert(body.y2<616);
  void *back_token=saved_action(mn_about_back),*about_close=saved_action(mn_safety_close);
  replay_action(about);replay_action(close);assert(mn_about_open&&mn_safety_open);
  replay_callback(edit,oldedit);replay_callback(switch_mode,oldmode);replay_callback(calculate,oldcalc);replay_action(oldkey);
  lv_obj_send_event(edits[0],LV_EVENT_CLICKED,NULL);lv_obj_send_event(run,LV_EVENT_CLICKED,NULL);lv_obj_send_event(modes[(m+1)%5],LV_EVENT_CLICKED,NULL);
  assert(mode==(el_mode_t)m&&runs==before&&mn_revision==rev&&legacy_revision==lr);
  saver_time(100);saver_time(60000);assert(!saver_active);
  click("Back to Safety");replay_action(back_token);replay_action(about_close);assert(mn_safety_open&&!mn_about_open);
  click("About this input");lv_obj_scroll_to_y(mn_about_scroll,10000,LV_ANIM_OFF);lv_obj_update_layout(lv_screen_active());
  explanation_rect(mn_safety_close,28,706,424,44);assert(lv_obj_is_visible(mn_safety_close));click("Close");
  replay_action(about);replay_action(back_token);replay_action(about_close);replay_callback(edit,oldedit);replay_callback(switch_mode,oldmode);replay_callback(calculate,oldcalc);replay_action(oldkey);
  assert(!mn_safety_open&&mode==(el_mode_t)m&&runs==before&&mn_revision==rev&&legacy_revision==lr);
  click(SAFETY_DOCK_TEXT);assert(!mn_about_open);click("About this input");
  assert(lv_obj_get_scroll_y(mn_about_scroll)==0);click("Close");
  click(SAFETY_DOCK_TEXT);assert(find(mn_safety_modal,"Safety / About"));click("Close");
 }
 /* Legacy result and full transcript survive navigation. */
 click("Hex");click("Load public zero");click("Calculate");
 lv_obj_send_event(mn_safety,LV_EVENT_CLICKED,NULL);assert(!mn_safety_open);
 hex_result_t r=compute();gui_result(&r);explanation_preserved();assert(valid_result&&find(lv_screen_active(),r.fingerprint));
 click("Words");click("24");for(unsigned i=0;i<24;i++)type_word("abstract");
 lv_obj_scroll_to_y(mn_list,96,LV_ANIM_OFF);lv_obj_update_layout(lv_screen_active());explanation_preserved();
 click("01 abstract");click("a");click("Next");assert(mn_page>0&&mn_edit==0);explanation_preserved();
 click("Clear words");assert(mn_confirm);lv_obj_send_event(mn_safety,LV_EVENT_CLICKED,NULL);assert(!mn_safety_open);click("Cancel");
 click("Cancel replacement");click("Review words");lv_obj_scroll_to_y(mn_review_list,100,LV_ANIM_OFF);lv_obj_update_layout(lv_screen_active());explanation_preserved();
 click("Back to words");clear_words();click("12");for(unsigned i=0;i<11;i++)type_word("abandon");type_word("about");
 click("Validate");lv_obj_send_event(mn_safety,LV_EVENT_CLICKED,NULL);assert(!mn_safety_open);r=compute();gui_result(&r);
 explanation_preserved();click("View entropy");explanation_preserved();
 /* Real first DOWN at Safety wakes only; next gesture opens the reset page. */
 lv_obj_clean(lv_screen_active());gui_create(request);click("Words");saver_time(100);saver_time(60000);assert(saver_active);
 touch_at(dev,250,770,true);touch_at(dev,250,770,false);assert(!saver_active&&wake_latch&&!mn_safety_open);
 touch_at(dev,250,770,true);touch_at(dev,250,770,false);assert(mn_safety_open&&!mn_about_open);click("About this input");
 void *stale=saved_action(mn_about_back);lv_obj_clean(lv_screen_active());gui_create(request);replay_action(stale);assert(!mn_safety_open&&!mn_about_open);
 click(SAFETY_DOCK_TEXT);assert(!mn_about_open);click("Close");
 /* Synthetic boundary injection, not billions of real UI transitions. Exercise
  * real LVGL controls and matching-generation queued actions after exhaustion;
  * restore the injected counter/flag and bindings before continuing the suite. */
 for(unsigned page=0;page<2;page++){
  click(SAFETY_DOCK_TEXT);if(page)click("About this input");
  uintptr_t generation_save=mn_generation;bool exhausted_save=mn_exhausted;
  assert(!exhausted_save&&generation_save>0);
  mn_generation=(UINTPTR_MAX>>10)-1;mn_rebind();
  void *nav=saved_action(page?mn_about_back:mn_about),*close=saved_action(mn_safety_close);
  assert(mn_generation==(UINTPTR_MAX>>10)&&!mn_exhausted);
  mn_rebind();assert(mn_exhausted&&mn_generation==(UINTPTR_MAX>>10));
  lv_obj_send_event(page?mn_about_back:mn_about,LV_EVENT_CLICKED,NULL);
  replay_action(nav);
  assert(mn_safety_open&&mn_about_open==(page!=0));
  lv_obj_send_event(mn_safety_close,LV_EVENT_CLICKED,NULL);replay_action(close);
  assert(mn_safety_open&&mn_about_open==(page!=0)&&mn_exhausted&&mn_generation==(UINTPTR_MAX>>10));
  mn_generation=generation_save-1;mn_exhausted=exhausted_save;mn_rebind();
  assert(mn_generation==generation_save&&mn_exhausted==exhausted_save);
  click("Close");assert(!mn_safety_open);
 }
 puts("PASS five explanation pages, fixed geometry, state/results/scroll preservation, stale generations/epochs, busy/confirmation/wake/recreate guards, synthetic generation-exhaustion About/Back/Close fail-closed");
}
