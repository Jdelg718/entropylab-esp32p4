/* Finite public-fixture acceptance; no access to private GUI state. */
static const char *mode_names[]={"Hex","Coins","D6 raw","D6 6->0","Words"};
static void editor(unsigned m){if(m==4){lv_obj_t *b=find_button(lv_screen_active(),"Back to words");if(b)click("Back to words");}else {const char *n=m==0?"Hex input":m==1?"Coin input":"Dice input";click(n);}}
static void seed_mode(unsigned m){
 click(mode_names[m]);editor(m);
 if(m==4){clear_words();click("12");for(int i=0;i<11;i++)type_word("abandon");type_word("about");}
 else {reset_legacy_input();if(m<2){if(m==1)click("12");click("Load public zero");if(m==0){click("Delete");click("F");}else {click("Undo");click("Tails 1");}}
 else {click("12");for(int i=0;i<50;i++)roll_face(m==2?2:6);}}
}
static void submit_mode(unsigned m){click(m==4?"Validate":"Calculate");assert(owned.mode==m);}
static lv_obj_t *panel_of(lv_obj_t *o){while(lv_obj_get_parent(o)!=app_content)o=lv_obj_get_parent(o);return o;}
static unsigned inject_panel(lv_obj_t *o){unsigned n=0;if(lv_obj_check_type(o,&lv_button_class)){lv_obj_send_event(o,LV_EVENT_CLICKED,NULL);n++;}for(uint32_t i=0;i<lv_obj_get_child_count(o);i++)n+=inject_panel(lv_obj_get_child(o,i));return n;}
static void acceptance_tests(void){
 hex_request_t fixtures[5];lv_obj_t *keys[5];
 for(unsigned m=0;m<5;m++){seed_mode(m);keys[m]=find_button(lv_screen_active(),m==0?"F":m==1?"Tails 1":m<4?"6":"z");assert(keys[m]);submit_mode(m);fixtures[m]=owned;hex_result_t r=compute();gui_result(&r);editor(m);}
 unsigned pairs=0,injections=0,stales=0;
 for(unsigned from=0;from<5;from++)for(unsigned to=0;to<5;to++){
  click(mode_names[from]);click(mode_names[to]);
  /* Hidden keypad callbacks cannot write another mode's transcript. D6 shares a keypad. */
  unsigned before=runs;injections+=inject_panel(panel_of(keys[to==4?0:4]));
  for(unsigned h=0;h<5;h++)if(h!=to&&!(h>=2&&h<=3&&to>=2&&to<=3)){lv_obj_send_event(keys[h],LV_EVENT_CLICKED,NULL);injections++;}
  assert(runs==before);submit_mode(to);assert(owned.length==fixtures[to].length&&owned.words==fixtures[to].words&&!memcmp(owned.hex,fixtures[to].hex,sizeof owned.hex));
  hex_request_t save=owned;hex_result_t r=compute();injections+=inject_panel(panel_of(keys[to==4?0:4]));
  for(unsigned h=0;h<5;h++){lv_obj_send_event(find_button(lv_screen_active(),mode_names[h]),LV_EVENT_CLICKED,NULL);lv_obj_send_event(keys[h],LV_EVENT_CLICKED,NULL);injections+=2;}
  assert(runs==before+1&&!memcmp(&save,&owned,sizeof save));
  for(unsigned fault=0;fault<4;fault++){hex_result_t bad=r;if(fault==0)bad.request_id++;if(fault==1)bad.revision++;if(fault==2)bad.mode=(to+1)%5;if(fault==3)bad.words=0;gui_result(&bad);assert(disabled_button(to==4?"Validate":"Calculate"));assert(!find(lv_screen_active(),r.fingerprint));stales++;}
  gui_result(&r);assert(find(lv_screen_active(),r.fingerprint));editor(to);gui_result(&r);assert(!find(lv_screen_active(),r.fingerprint));stales++;
  submit_mode(to);hex_result_t fresh=compute();gui_result(&r);assert(disabled_button(to==4?"Validate":"Calculate"));gui_result(&fresh);editor(to);stales++;pairs++;
 }
 /* Destroy objects before recreation; old results rejected idle and during new work. */
 for(unsigned m=0;m<5;m++){
  seed_mode(m);submit_mode(m);hex_result_t old=compute();lv_obj_clean(lv_screen_active());gui_create(request);gui_result(&old);assert(!find(lv_screen_active(),old.fingerprint));
  seed_mode(m);submit_mode(m);hex_result_t fresh=compute();gui_result(&old);assert(disabled_button(m==4?"Validate":"Calculate"));gui_result(&fresh);assert(find(lv_screen_active(),fresh.fingerprint));editor(m);stales+=2;
 }
 render(NULL);printf("PASS finite GUI acceptance: %u ordered mode pairs (self included), %u idle/busy callback injections, %u stale/duplicate/recreated drops, 5 recreated pending UIs; geometry total %u\n",pairs,injections,stales,checked);
}
