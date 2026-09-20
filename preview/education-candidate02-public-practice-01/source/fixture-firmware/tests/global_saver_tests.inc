/* Route regression: actual shared GUI and LVGL input pipeline, no policy mock. */
static void global_saver_fresh_interval(void){
 saver_time(100);saver_time(59900);assert(!saver_active);
 saver_time(100);assert(saver_active);
}
static void global_saver_reset(unsigned route){
 lv_obj_clean(lv_screen_active());gui_create(request);nav_commit(route);
}
static void global_saver_tests(lv_indev_t *dev){
 for(unsigned route=0;route<10;route++){
  global_saver_reset(route);
  saver_time(100);saver_time(59900);assert(!saver_active);
  saver_time(100);
  fprintf(stderr,"GLOBAL_SAVER blank route %u (%s) must enter\n",route,nav_names[route]);
  assert(saver_active&&lv_obj_has_flag(app_content,LV_OBJ_FLAG_HIDDEN));
  touch_at(dev,40,78,true);assert(!saver_active&&wake_latch&&!nav_open);
  touch_at(dev,40,78,false);assert(wake_latch&&!nav_open);
  lv_obj_send_event(nav_family,LV_EVENT_CLICKED,NULL);assert(!nav_open);
  touch_at(dev,40,78,true);touch_at(dev,40,78,false);assert(!wake_latch&&nav_open);
  click("Close");assert(!nav_open);
  /* Global exclusions, including a worker owned by a HIDDEN panel. Each
   * exclusion must discard accrued idle credit, not simply pause it. */
  for(unsigned guard=0;guard<7;guard++){
   global_saver_reset(route);saver_time(100);saver_time(59000);
   switch(guard){
    case 0:busy=true;break;
    case 1:gui08_ui.editor.pending_id=123;break;
    case 2:seed_ui.editor.pending_id=123;break;
    case 3:extra_dice_ui.editor.pending_id=123;break;
    case 4:mn_confirm=true;break;
    case 5:pi_ui.live=true;break;
    case 6:legacy_pending=123;break;
   }
   saver_time(60100);assert(!saver_active);
   busy=false;gui08_ui.editor.pending_id=0;seed_ui.editor.pending_id=0;
   extra_dice_ui.editor.pending_id=0;mn_confirm=false;pi_ui.live=false;legacy_pending=0;
   global_saver_fresh_interval();
  }
  /* Nonempty new editors are conservatively not stable idle views. Words
   * preserves the pre-existing committed/nonvalidated policy, tested below. */
  if(route!=4){
   global_saver_reset(route);
   if(route<4)lengths[mode]=1;
   else if(route==5)seed_ui.draft_len=1;
   else if(route<8)gui08_ui.editor.length=1;
   else extra_dice_ui.final_length=1;
   saver_time(100);saver_time(61000);assert(!saver_active);
  }
  global_saver_reset(route);
  if(route<4){valid_result=true;lv_obj_remove_flag(output,LV_OBJ_FLAG_HIDDEN);}
  else if(route==4)mn_valid=true;
  else if(route==5)seed_ui.editor.result_visible=true;
  else if(route<8)gui08_ui.editor.result_visible=true;
  else extra_dice_ui.editor.result_visible=true;
  saver_time(100);saver_time(61000);assert(!saver_active);
 }
 /* Every native owner uses gui08_guard before mutating state. Exercise real
  * coordinate hit testing over its input keys, plus synthetic clicks while
  * active and while the wake latch is waiting for a new distinct contact. */
 for(unsigned route=5;route<10;route++){
  global_saver_reset(route);
  lv_obj_t *key=route==5?seed_ui.digits[1]:route<8?gui08_ui.symbol_buttons[0]:extra_dice_ui.keys[1];
  lv_obj_update_layout(lv_screen_active());
  lv_area_t area;lv_obj_get_coords(key,&area);int x=(area.x1+area.x2)/2,y=(area.y1+area.y2)/2;
  fprintf(stderr,"GLOBAL_SAVER key route %u xy=%d,%d\n",route,x,y);
  assert(area.x1>=0&&area.x2<480&&area.y1>=0&&area.y2<800);
  assert(area.x2-area.x1+1>=44&&area.y2-area.y1+1>=44);
  global_saver_fresh_interval();
  lv_obj_send_event(key,LV_EVENT_CLICKED,NULL);
  assert(!seed_ui.draft_len&&!gui08_ui.editor.length&&!extra_dice_ui.editor.length);
  touch_at(dev,0,0,false);
  touch_at(dev,x,y,true);touch_at(dev,x,y,false);assert(wake_latch&&!saver_active);
  lv_obj_send_event(key,LV_EVENT_CLICKED,NULL);
  assert(!seed_ui.draft_len&&!gui08_ui.editor.length&&!extra_dice_ui.editor.length);
  touch_at(dev,x,y,true);touch_at(dev,x,y,false);assert(!wake_latch);
  assert(route==5?seed_ui.draft_len>0:route<8?gui08_ui.editor.length>0:extra_dice_ui.editor.length>0);
 }
 /* Known stable submodes: all word counts and canonical Bases radices. */
 static const unsigned counts[]={12,15,18,21,24};
 for(unsigned route=0;route<10;route++)for(unsigned w=0;w<5;w++){
  global_saver_reset(route);
  if(route<4)selector=counts[w];else if(route==4)mn_target=counts[w];
  else if(route==5)seed_ui.editor.words=counts[w];
  else if(route<8)gui08_ui.editor.words=counts[w];else extra_dice_ui.editor.words=counts[w];
  global_saver_fresh_interval();
 }
 static const unsigned bases[]={4,8,32,64};
 for(unsigned b=0;b<4;b++){
  global_saver_reset(7);gui08_ui.editor.base=bases[b];global_saver_fresh_interval();
 }
 /* Rollover uses actual LVGL uint32 tick domain. */
 global_saver_reset(0);lv_tick_inc(UINT32_MAX-lv_tick_get()-30000u);
 saver_tick(NULL);lv_tick_inc(59999);saver_tick(NULL);assert(!saver_active);
 lv_tick_inc(1);saver_tick(NULL);assert(saver_active);
 global_saver_reset(0);
 puts("PASS global saver ten routes, native key wake, exclusions, submodes and tick rollover");
}
