/* Copy the immutable callback payload, then deliver it through LVGL later. */
static void *saved_action(lv_obj_t *b){
 for(unsigned i=0;i<lv_obj_get_event_count(b);i++){
  lv_event_dsc_t *d=lv_obj_get_event_dsc(b,i);
  if(lv_event_dsc_get_cb(d)==mn_action)return lv_event_dsc_get_user_data(d);
 }
 abort();
}
static void replay_action(void *token){
 lv_obj_t *b=lv_button_create(lv_screen_active());lv_obj_add_flag(b,LV_OBJ_FLAG_HIDDEN);
 lv_obj_add_event_cb(b,mn_action,LV_EVENT_CLICKED,token);lv_obj_send_event(b,LV_EVENT_CLICKED,NULL);lv_obj_delete(b);
}
/* First tracer: real native candidates, never autoaccept. */
static void prefix_tests(void){
 click("Words");click("a");click("b");
 assert(mn_count==0 && !strcmp(mn_draft,"ab"));
 assert(find_button(lv_screen_active(),"abandon"));
 assert(find_button(lv_screen_active(),"ability"));
 assert(find_button(lv_screen_active(),"able"));
 click("Next");assert(find_button(lv_screen_active(),"about"));
 assert(mn_count==0);click("about");
 assert(mn_count==1 && !strcmp(mnemonic_transcript,"about") && !mn_draft[0]);
 puts("PASS native prefix paging and explicit selection");
 click("Backspace");assert(mn_count==1 && !strcmp(mnemonic_transcript,"about"));
 click("01 about");click("a");click("b");
 assert(!strcmp(mnemonic_transcript,"about"));
 click("Cancel replacement");assert(!strcmp(mnemonic_transcript,"about") && !mn_draft[0]);
 click("01 about");click("a");click("b");click("abandon");
 assert(mn_count==1 && !strcmp(mnemonic_transcript,"abandon"));
 click("Clear words");assert(mn_count==1);click("Cancel");assert(mn_count==1);
 click("Clear words");click("Confirm");assert(mn_count==0 && !mnemonic_transcript[0]);
 puts("PASS native transactional replacement, draft-only backspace, confirmed clear");
 click("a");click("b");void *stale=saved_action(find_button(lv_screen_active(),"abandon"));
 click("Next");replay_action(stale);assert(mn_count==0 && !strcmp(mn_draft,"ab"));
 stale=saved_action(find_button(lv_screen_active(),"about"));click("Backspace");replay_action(stale);assert(mn_count==0);
 stale=saved_action(find_button(lv_screen_active(),"a"));click("Hex");replay_action(stale);click("Words");
 assert(!mn_draft[0]);replay_action(stale);assert(!mn_draft[0]);
 click("Clear words");void *confirm=saved_action(find_button(lv_screen_active(),"Confirm"));click("Cancel");
 click("a");click("Clear words");replay_action(confirm);assert(mn_draft[0]=='a');click("Cancel");
 click("Clear words");click("Confirm");
 puts("PASS stale page/prefix/mode/dialog callback payloads fail closed");
 for(unsigned i=0;i<11;i++) {
  type_word("abandon");
 }
 type_word("about");
 unsigned before=runs;click("Review words");assert(runs==before);
 assert(find(lv_screen_active(),"Review only - not yet validated"));
 render("words-review.ppm");click("Back to words");assert(!mn_valid);
 clear_words();
 /* All one-letter pages expose every dictionary entry exactly once. */
 unsigned visited=0;
 for(unsigned letter=0;letter<26;letter++){
  char key[2]={(char)('a'+letter),0};click(key);
  unsigned expected=0;
  for(unsigned i=0;i<2048;i++)if(mn_dictionary[i][0]==key[0])expected++;
  unsigned seen=0,last=0;
  do {
   for(unsigned i=0;i<3;i++)if(mn_candidates[i]<2048){
    unsigned id=mn_candidates[i];assert(mn_dictionary[id][0]==key[0]);
    assert(!seen||id>last);last=id;seen++;visited++;
   }
   if(disabled_button("Next"))break;
   click("Next");assert(mn_count==0);
  }while(true);
  assert(seen==expected);click("Backspace");
 }
 assert(visited==2048);
 click("a");click("c");click("t");assert(mn_matches==5&&mn_count==0);
 click("act");assert(!strcmp(mnemonic_transcript,"act"));clear_words();
 click("a");click("b");click("o");click("u");assert(mn_matches==1&&mn_count==0);
 render("words-unique-prefix.ppm");click("about");clear_words();
 click("z");click("z");assert(!mn_matches&&disabled_button("--"));render("words-no-match.ppm");clear_words();
 puts("PASS all 2048 dictionary entries paged, exact act amid five matches, unique/no-match never autoaccept");
 for(unsigned n=12;n<=24;n+=3){
  char target[4];snprintf(target,sizeof target,"%u",n);click(target);
  assert(disabled_button("Review words"));
  for(unsigned i=0;i<n;i++)type_word("abandon");
  before=runs;void *entry=saved_action(mn_slots[0]);click("Review words");
  assert(mn_reviewing&&!mn_valid&&runs==before);
  for(unsigned i=0;i<n;i++){
   char row[32];snprintf(row,sizeof row,"%02u  abandon",i+1);assert(find(mn_review,row));
  }
  replay_action(entry);lv_obj_send_event(mn_validate,LV_EVENT_CLICKED,NULL);
  lv_obj_send_event(mn_slots[0],LV_EVENT_CLICKED,NULL);
  assert(mn_reviewing&&mn_edit<0&&runs==before);
  char image[48];snprintf(image,sizeof image,"words-review-%u.ppm",n);render(image);
  if(n==24){lv_obj_scroll_to_y(mn_review_list,1000,LV_ANIM_OFF);render("words-review-24-bottom.ppm");assert(lv_obj_get_scroll_y(mn_review_list)>0);}
  click("Clear words");render(n==24?"words-review-clear-dialog.ppm":NULL);click("Cancel");assert(mn_reviewing&&mn_count==n);
  void *oldback=saved_action(mn_review_back);click("Back to words");
  assert(!mn_reviewing&&!mn_valid&&runs==before);
  for(unsigned i=0;i<24;i++)assert(!lv_label_get_text(mn_review_rows[i])[0]);
  click("01 abandon");assert(disabled_button("Review words"));click("z");click("z");
  render(n==24?"words-replacement-24.ppm":NULL);click("Cancel replacement");assert(mn_count==n);
  click("Review words");replay_action(oldback);assert(mn_reviewing);
  click("Clear words");click("Confirm");assert(!mn_reviewing&&!mn_count&&!mnemonic_transcript[0]);
  assert(runs==before);
 }
 click("12");
 type_word("about");click("01 about");click("a");click("15");click("Cancel");
 assert(mn_target==12&&mn_edit==0&&!strcmp(mn_draft,"a")&&!strcmp(mnemonic_transcript,"about"));
 click("Cancel replacement");clear_words();
 puts("PASS review-only all five counts: no worker/derived validity, guarded overlays, clear cancel/confirm, stale back, label cleanup, replacement/count cancel");
}
