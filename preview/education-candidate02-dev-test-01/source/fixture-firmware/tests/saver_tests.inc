/* Public fixture: fake LVGL clock, real object tree and rendering. */
static void saver_time(uint32_t ms){lv_tick_inc(ms);lv_timer_handler();}
static void saver_tests(void){
 click("Words");type_word("abstract");
 saver_time(100);saver_time(59900);
 assert(find_button(lv_screen_active(),"Clear words"));
 saver_time(100);
 fprintf(stderr,"saver entry: public editor must be semantically hidden at 60s\n");
 assert(!find_button(lv_screen_active(),"Clear words"));
 assert(find(lv_screen_active(),"Touch to return"));
 lv_obj_t *logo=NULL;
 for(unsigned i=0;i<lv_obj_get_child_count(saver_screen);i++){lv_obj_t *o=lv_obj_get_child(saver_screen,i);if(lv_obj_check_type(o,&lv_image_class))logo=o;}
 fprintf(stderr,"approved saver must contain static authentic 180x270 image\n");assert(logo);
 lv_obj_update_layout(lv_screen_active());lv_area_t artwork;lv_obj_get_coords(logo,&artwork);
 assert(artwork.x1==150&&artwork.y1==222&&lv_obj_get_width(logo)==180&&lv_obj_get_height(logo)==270);
 assert(find(saver_screen,"EntropyLab (unofficial)")&&find(saver_screen,"DISPLAY IDLE"));
 lv_obj_t *brand=find_label_object(saver_screen,"EntropyLab (unofficial)");assert(brand);
 lv_area_t brand_area;lv_obj_get_coords(brand,&brand_area);
 assert(brand_area.x1==40&&brand_area.x2==439&&brand_area.y1==560&&brand_area.y2<704);
 assert(lv_obj_get_height(brand)==lv_font_get_line_height(&el_serif_24));
 lv_obj_t *ring=NULL,*dot=NULL;
 for(unsigned i=0;i<lv_obj_get_child_count(saver_screen);i++){lv_obj_t *o=lv_obj_get_child(saver_screen,i);if(lv_obj_get_width(o)==352&&lv_obj_get_height(o)==352&&!lv_obj_check_type(o,&lv_arc_class))ring=o;if(lv_obj_get_width(o)==6&&lv_obj_get_height(o)==6)dot=o;}
 fprintf(stderr,"approved orbit needs static 352px ring and 6px moving marker\n");assert(ring&&dot);
 assert(lv_obj_get_x(ring)==64&&lv_obj_get_y(ring)==184);
 int dx=lv_obj_get_x(dot),dy=lv_obj_get_y(dot);assert(dx==237&&dy==181);
 unsigned children=lv_obj_get_child_count(saver_screen);
 saver_time(99);assert(lv_obj_get_x(dot)==dx&&lv_obj_get_y(dot)==dy);saver_time(1);
 assert(lv_obj_get_x(dot)!=dx);saver_time(2900);assert(lv_obj_get_x(dot)==413&&lv_obj_get_y(dot)==357);
 saver_time(9000);assert(lv_obj_get_x(dot)==dx&&lv_obj_get_y(dot)==dy);
 assert(lv_obj_get_child_count(saver_screen)==children);render("saver-option-a.ppm");
 for(unsigned latch=0;latch<2;latch++){
  saver_active=!latch;wake_latch=latch;
  lv_obj_send_event(mn_clear,LV_EVENT_CLICKED,NULL);
  fprintf(stderr,"independent callback guards active=%d latch=%d\n",saver_active,wake_latch);
  assert(!mn_confirm);
  lv_obj_send_event(mn_validate,LV_EVENT_CLICKED,NULL);lv_obj_send_event(mn_slots[0],LV_EVENT_CLICKED,NULL);lv_obj_send_event(modes[0],LV_EVENT_CLICKED,NULL);
  assert(mode==MODE_MNEMONIC&&mn_edit<0&&!busy);
  for(unsigned m=0;m<MODE_MNEMONIC;m++){
   mode=(el_mode_t)m;unsigned sel=selector;
   lv_obj_send_event(edits[0],LV_EVENT_CLICKED,NULL);lv_obj_send_event(flips[0],LV_EVENT_CLICKED,NULL);lv_obj_send_event(dicekeys[0],LV_EVENT_CLICKED,NULL);lv_obj_send_event(choices[1],LV_EVENT_CLICKED,NULL);lv_obj_send_event(run,LV_EVENT_CLICKED,NULL);lv_obj_send_event(tabs[1],LV_EVENT_CLICKED,NULL);
   assert(!lengths[m]&&selector==sel&&!busy&&lv_obj_has_flag(output,LV_OBJ_FLAG_HIDDEN));
  }
  mode=MODE_MNEMONIC;
 }
 saver_active=true;wake_latch=false;
 assert(mn_count==1&&!strcmp(mnemonic_transcript,"abstract"));
 render("saver-lifecycle.ppm");
 lv_obj_clean(lv_screen_active());gui_create(request);
 click("Words");
 busy=true;saver_time(100);saver_time(60000);
 fprintf(stderr,"saver exclusion: busy must never enter\n");
 assert(!saver_active);busy=false;mn_refresh();saver_time(100);saver_time(59900);assert(!saver_active);saver_time(100);assert(saver_active);
 lv_obj_clean(lv_screen_active());gui_create(request);
 for(unsigned exclusion=0;exclusion<8;exclusion++){
  lv_obj_clean(lv_screen_active());gui_create(request);click("Words");saver_time(100);saver_time(59000);
  switch(exclusion){case 0:mn_confirm=3;break;case 1:mn_safety_open=true;break;case 2:mn_reviewing=true;break;case 3:mn_valid=true;break;case 4:mn_draft[0]='a';break;case 5:mn_edit=0;break;case 6:lv_obj_remove_flag(mn_output,LV_OBJ_FLAG_HIDDEN);break;case 7:mn_settling=true;break;}
  saver_time(61000);fprintf(stderr,"exclusion %u must block entry\n",exclusion);assert(!saver_active);
  mn_confirm=0;mn_safety_open=false;mn_reviewing=false;mn_valid=false;mn_draft[0]=0;mn_edit=-1;mn_settling=false;lv_obj_add_flag(mn_output,LV_OBJ_FLAG_HIDDEN);
  saver_time(100);saver_time(59900);assert(!saver_active);saver_time(100);assert(saver_active);
 }
 lv_obj_clean(lv_screen_active());gui_create(request);click("Words");saver_time(100);saver_time(59000);
 click(SAFETY_DOCK_TEXT);click("Close");saver_time(1000);
 assert(!saver_active);saver_time(59900);assert(!saver_active);saver_time(100);assert(saver_active);
 lv_obj_clean(lv_screen_active());gui_create(request);
 puts("PASS saver eligibility exclusions and fresh interval after brief activity");
}
