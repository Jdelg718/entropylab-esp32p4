/* Additional real LVGL / real core contract tests. */
static void coin_tests(void){
 click("Back to hex input");click("Coins");assert(find_button(lv_screen_active(),"Coin input"));render("coins-empty.ppm");
 assert(find(lv_screen_active(),"0 / 128 encoded bits"));
 click("Heads 0");click("Tails 1");click("Hex");
 assert(find(lv_screen_active(),"64 / 64 chars  |  256 bits"));
 click("Coins");assert(find(lv_screen_active(),"2 / 128 encoded bits"));click("Clear");
 for(unsigned w=12;w<=24;w+=3){
  char selector[4];snprintf(selector,sizeof selector,"%u",w);click(selector);
  unsigned n=w*32/3;click("Clear");
  for(unsigned i=0;i<n;i++)click(i==n-1?"Tails 1":"Heads 0");
  render("coins-input.ppm");click("Calculate");
  assert(owned.mode==1&&owned.words==w&&owned.length==n);
  assert(owned.hex[0]=='0'&&owned.hex[n-1]=='1'&&owned.hex[n]==0);
  const char *blocked[]={"Hex","Coins","12","24","Heads 0","Tails 1","Clear","Undo"};
  for(unsigned i=0;i<sizeof blocked/sizeof blocked[0];i++){
   lv_obj_t *b=find_button(lv_screen_active(),blocked[i]);assert(b&&lv_obj_has_state(b,LV_STATE_DISABLED));lv_obj_send_event(b,LV_EVENT_CLICKED,NULL);
  }
  hex_result_t r=compute();gui_result(&r);render("coins-results.ppm");assert_words(&r);
  click("Back to coin input");click("Undo");click("Test results");assert(!find(lv_screen_active(),r.fingerprint));click("Back to coin input");
  click("Tails 1");
 }
 click("Tails 1");assert(find(lv_screen_active(),"256 / 256 encoded bits"));
 click("12");assert(find(lv_screen_active(),"256 / 128 encoded bits"));assert(lv_obj_has_state(find_button(lv_screen_active(),"Calculate"),LV_STATE_DISABLED));render("coins-excess.ppm");
 click("24");reject=true;click("Calculate");reject=false;click("Calculate");
 hex_result_t r=compute();r.rc=-4;gui_result(&r);render("coins-failure.ppm");assert(find(lv_screen_active(),"--------"));click("Back to coin input");click("Calculate");r=compute();gui_result(&r);assert_words(&r);
 click("Clear");assert(find(lv_screen_active(),"0 / 256 encoded bits"));click("Test results");assert(!find(lv_screen_active(),r.fingerprint));click("Back to coin input");
 click("Hex");assert(find(lv_screen_active(),"64 / 64 chars  |  256 bits"));
 puts("PASS coins: all selectors, MSB/leading zero input, Heads/Tails, per-mode isolation, excess preserved, max256, invalidation, busy guards, retry, actual core outputs/geometry");
}
