static void safety_cleanup_tests(void){
 const char *names[]={"Hex","Coins","D6 raw","D6 6->0","Words"};
 for(unsigned m=0;m<5;m++){
  click(names[m]);render(NULL);
  assert(lv_obj_get_y(modes[m])==56&&lv_obj_get_y(header_title)==8);
  assert(!find(lv_screen_active(),"PUBLIC TEST INPUT - NO FUNDS"));
  assert(lv_obj_is_visible(mn_safety)&&lv_obj_get_height(mn_safety)==44);
  char image[64];snprintf(image,sizeof image,"cleanup-mode-%u.ppm",m);render(image);
  void *oldedit=saved_callback(edits[0],edit),*oldmode=saved_callback(modes[(m+1)%5],switch_mode);
  void *oldopen=saved_action(mn_safety);
  click("Public test only / Safety");assert(mn_safety_open);
  assert(lv_obj_get_width(mn_safety_modal)==480&&lv_obj_get_height(mn_safety_modal)==800);
  assert(lv_obj_get_style_bg_opa(mn_safety_modal,0)==LV_OPA_COVER);
  unsigned before=runs;size_t len=lengths[m];
  replay_callback(edit,oldedit);replay_callback(switch_mode,oldmode);lv_obj_send_event(run,LV_EVENT_CLICKED,NULL);
  assert(mode==(el_mode_t)m&&runs==before&&lengths[m]==len);
  click("Close");replay_callback(edit,oldedit);replay_callback(switch_mode,oldmode);
  fprintf(stderr,"Safety close rejects old legacy callbacks in mode %u\n",m);
  assert(mode==(el_mode_t)m&&lengths[m]==len);
  replay_action(oldopen);assert(!mn_safety_open);
  click("Public test only / Safety");
  assert(find(lv_screen_active(),"Safety / About"));
  assert(find(lv_screen_active(),"Unofficial native adaptation\nEntropyLab - Team Ooga Booga"));
  assert(find(lv_screen_active(),"Origin: Mr.Hodl (@mrhodl)\nNo upstream endorsement or affiliation."));
  snprintf(image,sizeof image,"cleanup-safety-%u.ppm",m);render(image);click("Close");
 }
 click("Hex");click("Load public zero");click("Calculate");
 assert(lv_obj_has_state(mn_safety,LV_STATE_DISABLED));lv_obj_send_event(mn_safety,LV_EVENT_CLICKED,NULL);assert(!mn_safety_open);
 hex_result_t r=compute();gui_result(&r);assert(!lv_obj_has_state(mn_safety,LV_STATE_DISABLED));click("Clear");
 puts("PASS all-mode compact layout, opaque Safety/About, stale and busy guards");
}
