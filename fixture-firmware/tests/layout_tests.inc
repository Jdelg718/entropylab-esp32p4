/* Vertical tracer: actual LVGL whole-row clipping regression. */
static void layout_tests(void){
 click("Words");click("24");
 for(unsigned i=0;i<24;i++)type_word("abstract");
 lv_obj_scroll_to_y(mn_list,0,LV_ANIM_OFF);render("layout-clipping.ppm");
 lv_area_t v;lv_obj_get_content_coords(mn_list,&v);
 unsigned whole=0;
 for(unsigned i=0;i<24;i++){lv_area_t a;lv_obj_get_coords(mn_slots[i],&a);if(a.y1>=v.y1&&a.y2<=v.y2)whole++;}
 fprintf(stderr,"whole rows regression: viewport=%d whole=%u expected=144/9\n",lv_obj_get_height(mn_list),whole);
 assert(whole==9);assert(lv_obj_get_height(mn_list)==144);
 lv_area_t a;lv_obj_get_coords(mn_panel,&a);
 assert(a.x1==16&&a.y1==104&&lv_obj_get_height(mn_panel)==648);
 lv_obj_get_coords(mn_list,&a);assert(a.x1==28&&a.y1==182);
 assert(lv_obj_get_style_border_width(mn_list,0)==0&&lv_obj_get_style_radius(mn_list,0)==0);
 assert(lv_obj_get_height(mn_draft_label)==44&&lv_obj_get_height(mn_pages)==44&&lv_obj_get_height(mn_hint)==20&&lv_obj_get_height(mn_status)==16);
 lv_obj_get_coords(header_title,&a);assert(a.x1==16&&a.y1==8&&lv_obj_get_width(header_title)==260&&lv_obj_get_height(header_title)==36);
 lv_obj_get_coords(mn_validate,&a);assert(a.x1==316&&a.y1==706&&a.y2==749);
 lv_obj_get_coords(modes[0],&a);assert(a.y1==56);
 assert(!find(lv_screen_active(),"PUBLIC TEST INPUT - NO FUNDS"));
 assert(lv_obj_get_height(mn_review)==648&&lv_obj_get_height(mn_output)==648);
 assert(lv_obj_get_style_pad_bottom(mn_list,0)==0);
 /* Settled offsets include scrollbar release; exactly 384 content pixels. */
 for(int y=0;y<=260;y+=13){
  lv_obj_scroll_to_y(mn_list,y,LV_ANIM_OFF);lv_obj_send_event(mn_list,LV_EVENT_SCROLL_END,NULL);
  int sy=lv_obj_get_scroll_y(mn_list);assert(sy%48==0&&sy>=0&&sy<=240);
 }
 lv_obj_scroll_to_y(mn_list,1000,LV_ANIM_OFF);lv_obj_send_event(mn_list,LV_EVENT_SCROLL_END,NULL);
 assert(lv_obj_get_scroll_y(mn_list)==240);render("layout-24-bottom.ppm");
 click("01 abstract");type_word("abandon");assert(lv_obj_get_scroll_y(mn_list)==0);
 click("24 abstract");type_word("abandon");assert(lv_obj_get_scroll_y(mn_list)==240);
 click("12");click("Confirm");assert(lv_obj_get_scroll_y(mn_list)==48);
 clear_words();assert(lv_obj_get_scroll_y(mn_list)==0);
 for(unsigned n=1;n<=12;n++){type_word("abstract");int want=n<=9?0:48;assert(lv_obj_get_scroll_y(mn_list)==want);}
 clear_words();
 render("layout-empty.ppm");
 for(unsigned n=12;n<=24;n+=3){
  char target[4];snprintf(target,sizeof target,"%u",n);click(target);
  for(unsigned j=0;j<n;j++){type_word("abstract");int want=j<9?0:((int)j/3-2)*48;assert(lv_obj_get_scroll_y(mn_list)==want);}
  int max=(int)(n/3)*48-144;unsigned reached=0;
  for(int y=0;y<=max;y+=48){
   lv_obj_scroll_to_y(mn_list,y,LV_ANIM_OFF);lv_obj_send_event(mn_list,LV_EVENT_SCROLL_END,NULL);render(NULL);
   lv_area_t clip;lv_obj_get_coords(mn_list,&clip);unsigned visible=0;
   for(unsigned j=0;j<n;j++){lv_area_t cell;lv_obj_get_coords(mn_slots[j],&cell);if(cell.y2<clip.y1||cell.y1>clip.y2)continue;assert(cell.y1>=clip.y1&&cell.y2<=clip.y2);visible++;reached|=1u<<j;}
   assert(visible==9);char image[64];snprintf(image,sizeof image,"layout-%u-scroll-%d.ppm",n,y);render(image);
  }
  assert(reached==((1u<<n)-1));
  assert(lv_obj_get_scroll_y(mn_list)+lv_obj_get_scroll_bottom(mn_list)==max);
  clear_words();assert(lv_obj_get_scroll_y(mn_list)==0);
 }
 click("12");
 lv_obj_t *safety=find_button(lv_screen_active(),"Public test only / Safety");assert(safety);
 lv_obj_update_layout(lv_screen_active());lv_obj_get_coords(safety,&a);assert(a.x1==16&&a.y1==756&&a.y2==799);
 void *oldkey=saved_action(mn_keys[0]);click("Public test only / Safety");
 assert(find(lv_screen_active(),"Checksum validity does not establish entropy quality."));
 render("layout-safety.ppm");replay_action(oldkey);lv_obj_send_event(modes[0],LV_EVENT_CLICKED,NULL);
 lv_obj_send_event(mn_keys[0],LV_EVENT_CLICKED,NULL);lv_obj_send_event(mn_clear,LV_EVENT_CLICKED,NULL);lv_obj_send_event(mn_targets[1],LV_EVENT_CLICKED,NULL);assert(!mn_confirm&&mn_target==12);
 assert(mode==MODE_MNEMONIC&&!mn_draft[0]);click("Close");replay_action(oldkey);assert(!mn_draft[0]);
 click("Hex");assert(find_button(lv_screen_active(),"Public test only / Safety"));
 assert(!find(lv_screen_active(),"PUBLIC TEST INPUT - NO FUNDS"));
 lv_obj_update_layout(lv_screen_active());lv_obj_get_coords(modes[0],&a);assert(a.y1==56);
 puts("PASS layout whole-row viewport");
}
