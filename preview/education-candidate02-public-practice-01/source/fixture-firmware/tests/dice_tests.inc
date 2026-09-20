static void dice_tests(void){
 click("D6 raw");render("dice-empty.ppm");assert(lv_obj_has_state(find_button(lv_screen_active(),"Calculate"),LV_STATE_DISABLED));
 roll_face(6);render("dice-weak-input.ppm");assert(find(lv_screen_active(),"WEAK_INPUT_LAB_ONLY"));click("Calculate");assert(find(lv_screen_active(),"WEAK_INPUT_LAB_ONLY / Calculating..."));assert(owned.mode==2&&owned.length==1);hex_result_t r;el_compute(&owned,&r);assert(r.rc==1&&r.weak);gui_result(&r);assert_words(&r);render("dice-weak-results.ppm");assert(find(lv_screen_active(),"D6 raw COLDCARD-style\nWEAK_INPUT_LAB_ONLY / NEVER fund"));
 click("Back to dice input");click("D6 6->0");assert(lv_obj_has_state(find_button(lv_screen_active(),"Calculate"),LV_STATE_DISABLED));roll_face(6);click("Calculate");el_compute(&owned,&r);gui_result(&r);render("dice-coleman-results.ppm");click("Back to dice input");click("D6 raw");click("24");
 assert(mode==MODE_D6_RAW&&nav_route==2&&selector==24);
 assert(lengths[MODE_D6_RAW]==1&&lengths[MODE_D6_COLEMAN]==1&&d6_selection[MODE_D6_RAW]==1);
 assert(find(lv_screen_active(),"1/1 selected | 100 required | max 1024"));
 for(int i=1;i<1024;i++){roll_face(6);}render("dice-full-input.ppm");click("Calculate");assert(owned.length==1024&&owned.hex[1024]==0&&owned.words==24);
 const char *blocked[]={"Input family   Dice   v","12","24","Undo","Clear"};for(unsigned i=0;i<sizeof blocked/sizeof *blocked;i++){lv_obj_t*b=find_button(lv_screen_active(),blocked[i]);assert(b&&lv_obj_has_state(b,LV_STATE_DISABLED));lv_obj_send_event(b,LV_EVENT_CLICKED,NULL);}for(unsigned face=0;face<6;face++){assert(lv_obj_has_state(dicekeys[face],LV_STATE_DISABLED));lv_obj_send_event(dicekeys[face],LV_EVENT_CLICKED,NULL);}
 el_compute(&owned,&r);assert(!r.rc&&!r.weak);gui_result(&r);assert_words(&r);render("dice-full-results.ppm");click("Clear");click("Test results");assert(!find(lv_screen_active(),r.fingerprint));click("Back to dice input");roll_face(1);reject=true;click("Calculate");reject=false;click("Calculate");el_compute(&owned,&r);r.rc=-4;gui_result(&r);assert(find(lv_screen_active(),"--------"));render("dice-failure.ppm");
 assert(find(lv_screen_active(),"WEAK_INPUT_LAB_ONLY / Calculation failed\nBack to input and retry."));
 click("Back to dice input");reject=true;click("Calculate");reject=false;
 assert(find(lv_screen_active(),"WEAK_INPUT_LAB_ONLY / Worker unavailable"));render("dice-retry.ppm");
 click("Calculate");el_compute(&owned,&r);strcpy(r.mnemonic,"bad");gui_result(&r);
 assert(find(lv_screen_active(),"WEAK_INPUT_LAB_ONLY / Invalid result\nBack to input and retry."));render("dice-invalid-result.ppm");
 click("Back to dice input");
 for(unsigned m=2;m<=3;m++){
  click(m==2?"D6 raw":"D6 6->0");
  for(unsigned w=12;w<=24;w+=3){char s[4];snprintf(s,sizeof s,"%u",w);click(s);click("Calculate");assert(owned.length==1&&owned.words==w&&owned.mode==m);el_compute(&owned,&r);assert(r.rc==1&&r.weak);gui_result(&r);assert_words(&r);render(NULL);click("Back to dice input");}
 }
 click("D6 raw");for(unsigned i=1;i<1024;i++)roll_face(6);click("12");click("Calculate");assert(owned.length==1024&&owned.words==12);el_compute(&owned,&r);gui_result(&r);assert_words(&r);render(NULL);
 click("Back to dice input");click("24");click("Calculate");assert(owned.length==1024&&owned.words==24);el_compute(&owned,&r);gui_result(&r);assert_words(&r);render(NULL);
 puts("PASS dice GUI: isolated modes, weak warning input/results, full1024 owned snapshot, busy guards, selector preservation, invalidation, failure clear, geometry");
}
