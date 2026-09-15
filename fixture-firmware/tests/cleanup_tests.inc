/* Public fixtures only: per-mode clear must retire completed request identity. */
static void cleanup_tests(lv_indev_t *dev){
 const char *names[]={"Hex","Coins","D6 raw","D6 6->0","Words"};
 lv_obj_clean(lv_screen_active());gui_create(request);
 click("Coins");click("Tails 1");click("Words");type_word("abstract");
 for(unsigned m=0;m<4;m++){
  click(names[m]);click("Clear");
  if(m<2)click("Load public zero");else for(unsigned j=0;j<50;j++)click("6");
  click("Calculate");hex_result_t r=compute();gui_result(&r);assert(valid_result);
  char image[64];snprintf(image,sizeof image,"cleanup-result-%u.ppm",m);render(image);
  click("Clear");
  fprintf(stderr,"Clear retires legacy pending identity mode %u\n",m);assert(legacy_pending==0);
  assert(!lengths[m]&&!valid_result);for(unsigned j=0;j<sizeof transcripts[m];j++)assert(!transcripts[m][j]);
  assert(!strcmp(lv_label_get_text(fp),"--------"));assert(!strcmp(lv_label_get_text(addr),"Not calculated"));
  for(unsigned j=0;j<24;j++)assert(!strcmp(lv_label_get_text(words[j]),"--"));
  gui_result(&r);assert(!valid_result);click("Words");assert(mn_count==1);click(names[m]);assert(!lengths[m]);
  snprintf(image,sizeof image,"cleanup-clear-%u.ppm",m);render(image);
 }
 click("Words");clear_words();for(unsigned j=0;j<11;j++)type_word("abandon");type_word("about");
 click("Validate");hex_result_t r=compute();gui_result(&r);click("View entropy");assert(mn_recovered[0]);
 click("Back to words");click("Review words");click("Clear words");click("Confirm");
 fprintf(stderr,"Clear retires Words pending identity\n");assert(!mn_pending);
 assert(!mn_count&&!mn_draft[0]&&mn_edit==-1&&!mn_reviewing&&!mn_valid);
 for(unsigned j=0;j<sizeof mnemonic_transcript;j++)assert(!mnemonic_transcript[j]);
 for(unsigned j=0;j<sizeof mn_recovered;j++)assert(!mn_recovered[j]);
 for(unsigned j=0;j<24;j++){assert(!mn_words[j]);assert(!*lv_label_get_text(mn_review_rows[j]));assert(!*lv_label_get_text(lv_obj_get_child(mn_slots[j],0)));}
 assert(!*lv_label_get_text(mn_entropy)&&!*lv_label_get_text(mn_fp)&&!*lv_label_get_text(mn_addr));
 gui_result(&r);assert(!mn_valid);click("Hex");click("Words");render("cleanup-clear-words.ppm");
 saver_time(100);saver_time(60000);assert(saver_active);render("cleanup-clear-saver.ppm");
 touch_at(dev,70,728,true);touch_at(dev,70,728,false);assert(wake_latch&&!mn_confirm);
 touch_at(dev,470,790,true);touch_at(dev,470,790,false);assert(!wake_latch);
 if(mn_safety_open)click("Close");
 gui_result(&r);assert(!mn_valid&&!mn_count&&!mn_recovered[0]);render("cleanup-clear-wake.ppm");
 lv_obj_clean(lv_screen_active());gui_create(request);
 puts("PASS per-mode Clear after real derivation, pending identity, owned buffers, labels, switch and saver wake");
}
