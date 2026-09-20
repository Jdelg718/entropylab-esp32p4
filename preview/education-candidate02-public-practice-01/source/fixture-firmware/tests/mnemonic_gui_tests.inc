/* Existing worker/regression assertions adapted to dictionary-only transactions. */
static void type_word(const char *s){for(const char *p=s;*p;p++){char k[2]={*p,0};click(k);}click(s);}
static void clear_words(void){click("Clear words");click("Confirm");}
static bool disabled_button(const char *s){lv_obj_t *b=find_button(lv_screen_active(),s);assert(b);return lv_obj_has_state(b,LV_STATE_DISABLED);}
static void busy_buttons(lv_obj_t *o){if(lv_obj_has_flag(o,LV_OBJ_FLAG_HIDDEN))return;if(lv_obj_check_type(o,&lv_button_class)){assert(lv_obj_has_state(o,LV_STATE_DISABLED));lv_obj_send_event(o,LV_EVENT_CLICKED,NULL);}for(uint32_t i=0;i<lv_obj_get_child_count(o);i++)busy_buttons(lv_obj_get_child(o,i));}
static void mnemonic_tests(void){
 click("Back to words");clear_words();assert(disabled_button("--")&&disabled_button("Validate"));
 for(unsigned n=12;n<=24;n+=3){
  char target[4];snprintf(target,sizeof target,"%u",n);click(target);assert(lv_obj_has_state(find_button(lv_screen_active(),target),LV_STATE_CHECKED));
  hex_request_t q={.mode=MODE_HEX,.words=n,.length=n*8/3};memset(q.hex,'0',q.length);hex_result_t fixture;el_compute(&q,&fixture);assert(fixture.rc==0);
  char phrase[216];strcpy(phrase,fixture.mnemonic);for(char *t=strtok(phrase," ");t;t=strtok(NULL," "))type_word(t);
  if(n==24){render("mnemonic24-top.ppm");lv_obj_t *well=mn_list;assert(find(lv_screen_active(),"01 abandon"));assert(lv_obj_get_style_width(well,LV_PART_SCROLLBAR)>=4);assert(lv_obj_get_style_bg_opa(well,LV_PART_SCROLLBAR)==LV_OPA_COVER);assert(lv_color_eq(lv_obj_get_style_bg_color(well,LV_PART_SCROLLBAR),lv_color_hex(0xff9900)));lv_obj_scroll_to_y(well,1000,LV_ANIM_OFF);render("mnemonic24-bottom.ppm");assert(lv_obj_get_scroll_y(well)>0);}
  click("Validate");unsigned before=runs;hex_request_t save=owned;busy_buttons(lv_screen_active());assert(runs==before&&!memcmp(&save,&owned,sizeof save));
  hex_result_t r=compute(),stale=r;stale.request_id++;gui_result(&stale);assert(disabled_button("Validate"));stale=r;stale.revision++;gui_result(&stale);assert(disabled_button("Validate"));stale=r;stale.words=0;gui_result(&stale);assert(disabled_button("Validate"));
  gui_result(&r);assert(find(lv_screen_active(),fixture.fingerprint)&&find(lv_screen_active(),fixture.address));assert(!find(lv_screen_active(),r.entropy));render(n==24?"mnemonic24-result.ppm":NULL);click("View entropy");assert(find(lv_screen_active(),r.entropy));click("Back to words");assert(!find(lv_screen_active(),r.entropy));click("Backspace");assert(mn_count==n);gui_result(&r);assert(!find(lv_screen_active(),r.fingerprint));clear_words();
 }
 click("24");for(int i=0;i<24;i++){for(const char *s="abstract";*s;s++){char k[2]={*s,0};click(k);}lv_obj_send_event(find_button(lv_screen_active(),"z"),LV_EVENT_CLICKED,NULL);assert(!strcmp(mn_draft,"abstract"));click("abstract");}
 assert(find(lv_screen_active(),"MNEMONIC  24/24 words  215/215 bytes"));lv_obj_send_event(find_button(lv_screen_active(),"z"),LV_EVENT_CLICKED,NULL);assert(disabled_button("--"));click("12");assert(mn_target==24);click("Cancel");assert(mn_count==24);click("24");assert(find(lv_screen_active(),"MNEMONIC  24/24 words  215/215 bytes"));click("Validate");assert(owned.length==215&&owned.hex[215]==0);hex_result_t r;el_compute(&owned,&r);gui_result(&r);assert(r.rc<0);clear_words();click("12");
 for(int i=0;i<12;i++)type_word("abandon");
 click("Validate");el_compute(&owned,&r);assert(r.rc==-6);gui_result(&r);assert(find(lv_screen_active(),"Checksum invalid. Check your words."));
 /* Unknown editor input no longer committable. Preserve real downstream -111 test. */
 click("12 abandon");type_word("about");click("Validate");hex_request_t bad=owned;strcpy(strrchr(bad.hex,' ')+1,"zzzz");bad.length=strlen(bad.hex);el_compute(&bad,&r);assert(r.rc==-111);gui_result(&r);assert(find(lv_screen_active(),"Unknown word at position 12"));
 reject=true;click("Validate");assert(find(lv_screen_active(),"Worker unavailable. Retry validation."));reject=false;
 const int codes[]={-1,-2,-3,-4,-5,-124};const char *messages[]={"Validation unavailable: buffer contract error","Validation unavailable: buffer contract error","Derivation failed. No result.","Noncanonical English text","Use exactly 12, 15, 18, 21 or 24 words","Validation failed. Retry."};
 for(unsigned i=0;i<6;i++){click("Validate");r=compute();r.rc=codes[i];gui_result(&r);assert(find(lv_screen_active(),messages[i]));}
 for(int i=0;i<4;i++){click("Validate");r=compute();if(i==0)memset(r.entropy,'a',sizeof r.entropy);if(i==1)memset(r.fingerprint,'a',sizeof r.fingerprint);if(i==2)memset(r.address,'q',sizeof r.address);if(i==3)r.address[5]='!';gui_result(&r);assert(find(lv_screen_active(),"Malformed result. Retry."));}
 click("Validate");r=compute();gui_result(&r);click("Back to words");click("Hex");click("Words");assert(!find(lv_screen_active(),r.entropy));assert(!disabled_button("Validate"));clear_words();assert(disabled_button("Validate"));
 puts("PASS mnemonic GUI: 5 real-core counts; busy event injection; ID/revision/target stale drops; 215-byte bound, draft8, confirmed count; checksum/unknown downstream; 6 error mappings; 4 malformed outputs; retry, reopen, clear, reveal reset");
}
