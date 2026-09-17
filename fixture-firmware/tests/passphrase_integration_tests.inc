/* Public real-runtime tracer; host serialization is not FreeRTOS evidence. */
static uint64_t pi_test_token;
static bool pi_test_full;
static unsigned pi_test_depth;
static void pi_test_lock(void) {assert(!pi_test_depth);pi_test_depth++;}
static void pi_test_unlock(void) {assert(pi_test_depth==1);pi_test_depth--;}
static bool pi_test_publish(uint64_t token){assert(pi_test_depth==0);if(pi_test_full)return false;pi_test_token=token;return true;}
static unsigned gui08_passphrase_submits,gui08_passphrase_cancels;
static unsigned pi_lifehash_render_calls;
static bool pi_lifehash_render_fail;
static int pi_lifehash_render_probe(const uint8_t raw4[4],uint8_t rgb[32*32*3]){
 assert(pi_test_depth==0);assert(raw4&&rgb);pi_lifehash_render_calls++;
 if(pi_lifehash_render_fail)return -1;
 return (int)lifehash_fingerprint_render(raw4,4,LIFEHASH_FINGERPRINT_VERSION_2,
                                         rgb,LIFEHASH_FINGERPRINT_RGB_SIZE);
}
static gui08_request gui08_passphrase_owned;
static bool gui08_passphrase_submit_probe(const gui08_request *request){
 gui08_passphrase_submits++;gui08_passphrase_owned=*request;
 return gui_cards_bases_pending(request);
}
static bool gui08_passphrase_cancel_probe(uint64_t request_id){
 (void)request_id;gui08_passphrase_cancels++;return true;
}
static void gui08_click_symbol_index(unsigned index){
 lv_obj_t *key=gui08_ui.symbol_buttons[index];assert(key&&!lv_obj_has_flag(key,LV_OBJ_FLAG_HIDDEN));
 assert(!lv_obj_has_state(key,LV_STATE_DISABLED));lv_obj_send_event(key,LV_EVENT_CLICKED,NULL);
}
static void gui08_source_capture(const char *name){
 lv_obj_update_layout(lv_screen_active());lv_obj_invalidate(lv_screen_active());lv_refr_now(NULL);
 FILE *f=fopen(name,"wb");assert(f);fprintf(f,"P6\n480 800\n255\n");
 assert(fwrite(pixels,1,sizeof pixels,f)==sizeof pixels);fclose(f);
}
static void pi_assert_orange_scrollbar(const char *state){
 lv_area_t horizontal,vertical;
 lv_obj_get_scrollbar_area(pi_ui.viewport,&horizontal,&vertical);
 lv_obj_update_layout(pi_ui.root);lv_obj_invalidate(pi_ui.root);lv_refr_now(NULL);
 unsigned orange=0,total=0;
 if(vertical.x2>=vertical.x1&&vertical.y2>=vertical.y1){
  for(int y=vertical.y1;y<=vertical.y2;y++)for(int x=vertical.x1;x<=vertical.x2;x++){
   const unsigned char *p=pixels+3*(y*480+x);total++;
   if(p[0]>=240&&p[1]>=140&&p[1]<=170&&p[2]<=20)orange++;
  }
 }
 fprintf(stderr,"PASSPHRASE_SCROLLBAR state=%s area=%d,%d-%d,%d orange=%u/%u\n",
         state,vertical.x1,vertical.y1,vertical.x2,vertical.y2,orange,total);
 assert(total>0&&orange*2>=total);
}
static void cards_bases_visible_passphrase_test(void){
 lv_obj_clean(lv_screen_active());gui_create(request);
 gui_cards_bases_configure(gui08_passphrase_submit_probe,gui08_passphrase_cancel_probe);
 gui08_passphrase_submits=gui08_passphrase_cancels=0;memset(&gui08_passphrase_owned,0,sizeof gui08_passphrase_owned);
 click("Cards");
 for(unsigned i=0;i<47;i++)gui08_click_symbol_index(0);
 assert(gui08_ready(&gui08_ui.editor));
 click("Continue to Passphrase");
 assert(gui08_passphrase_submits==0); /* navigation alone must never derive */
 assert(pi_ui.live&&!lv_obj_has_flag(pi_ui.root,LV_OBJ_FLAG_HIDDEN));
 assert(passphrase_ui_raw_len(&pi_ui)==0);
 gui08_source_capture("cards-passphrase-empty.ppm");
 lv_obj_send_event(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_BACK),LV_EVENT_CLICKED,NULL);
 assert(gui08_passphrase_submits==0&&!gui08_ui.editor.pending_id);
 click("Continue to Passphrase");
 assert(passphrase_ui_set_raw(&pi_ui,(const uint8_t*)"TREZOR",6)==PASSPHRASE_UI_OK);
 lv_obj_send_event(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_CONTINUE),LV_EVENT_CLICKED,NULL);
 assert(gui08_passphrase_submits==1&&gui08_ui.editor.pending_id);
 assert(gui08_passphrase_owned.passphrase_marker==GUI08_PASSPHRASE_ACTIVE);
 assert(gui08_passphrase_owned.passphrase_len==6&&!memcmp(gui08_passphrase_owned.passphrase,"TREZOR",6));
 assert(gui08_passphrase_owned.method==GUI08_CARDS&&gui08_passphrase_owned.revision==gui08_ui.editor.revision);
 click("Cancel");assert(gui08_passphrase_cancels==1&&!gui08_ui.editor.pending_id);

 /* Same-context Back retains a draft, but a source revision change invalidates it. */
 click("Continue to Passphrase");
 assert(passphrase_ui_set_raw(&pi_ui,(const uint8_t*)"OLD",3)==PASSPHRASE_UI_OK);
 lv_obj_send_event(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_BACK),LV_EVENT_CLICKED,NULL);
 click("Undo");gui08_click_symbol_index(0);
 click("Continue to Passphrase");
 assert(passphrase_ui_raw_len(&pi_ui)==0);
 lv_obj_send_event(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_CONTINUE),LV_EVENT_CLICKED,NULL);
 assert(gui08_passphrase_submits==2&&gui08_passphrase_owned.passphrase_marker==GUI08_PASSPHRASE_EMPTY);
 assert(gui08_passphrase_owned.passphrase_len==0&&gui08_passphrase_owned.revision==gui08_ui.editor.revision);
 click("Cancel");assert(gui08_passphrase_cancels==2);

 /* Cards -> Bases is a distinct source context and cannot consume a retained draft. */
 click("Bases");
 for(unsigned i=0;i<gui08_target(&gui08_ui.editor);i++)gui08_click_symbol_index(0);
 click("Continue to Passphrase");
 assert(gui08_passphrase_submits==2&&passphrase_ui_raw_len(&pi_ui)==0);
 assert(pi_ui.context.mode!=GUI08_CARDS&&pi_ui.context.source_revision==gui08_ui.editor.revision);
 gui08_source_capture("bases-passphrase-context-reset.ppm");
 lv_obj_send_event(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_BACK),LV_EVENT_CLICKED,NULL);
 puts("PASS Cards/Bases real event sequence: navigation/back zero, explicit Active/Empty exactly once, copied bytes, revision/context invalidation");
 gui_cards_bases_configure(NULL,NULL);
 gui_passphrase_dispose();
}
static void pi_test_reachable(lv_obj_t *o){
 lv_obj_scroll_to_view_recursive(o,LV_ANIM_OFF);lv_obj_update_layout(pi_ui.root);
 lv_area_t a,b;lv_obj_get_coords(o,&a);
 for(lv_obj_t *p=lv_obj_get_parent(o);p;p=lv_obj_get_parent(p)){
  lv_obj_get_coords(p,&b);
  assert(a.x1>=b.x1&&a.x2<=b.x2&&a.y1>=b.y1&&a.y2<=b.y2);
 }
 render(NULL);
}
static void passphrase_integration_tests(void){
 cards_bases_visible_passphrase_test();
 gui_passphrase_configure(pi_test_lock,pi_test_unlock,pi_test_publish);
 gui_passphrase_lifehash_configure(pi_lifehash_render_probe);
 pi_lifehash_render_calls=0;pi_lifehash_render_fail=false;
 lv_obj_clean(lv_screen_active());gui_create(request);
 for(unsigned i=0;i<32;i++)click("0");
 click("Calculate");
 assert(passphrase_ui_root(&pi_ui));
 passphrase_ui_set_raw(&pi_ui,(const uint8_t*)"TREZOR",6);
 lv_obj_send_event(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_CONTINUE),LV_EVENT_CLICKED,NULL);
 assert(pi_dispatch.state==EL_DISPATCH_QUEUED);
 assert(pi_dispatch.request.passphrase_len==6);
 assert(!memcmp(pi_dispatch.request.passphrase,"TREZOR",6));
 gui_passphrase_work(pi_test_token);gui_passphrase_poll();
 assert(pi_dispatch.state==EL_DISPATCH_FREE);
 assert(passphrase_ui_result_visible(&pi_ui));
 assert(lv_obj_get_child_count(pi_ui.result)>3); /* mnemonic/entropy must survive worker reduction */
 assert(!strcmp(lv_label_get_text(pi_mnemonic_label),"abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon abandon about"));
 assert(!strcmp(lv_label_get_text(pi_ui.result_fp),"b4e3f5ed"));
 assert(!strcmp(lv_label_get_text(pi_ui.result_addr),"bc1qv5rmq0kt9yz3pm36wvzct7p3x6mtgehjul0feu"));
 assert(pi_lifehash_render_calls==1);
 assert(passphrase_ui_lifehash_visible(&pi_ui));
 assert(find(lv_screen_active(),"Fingerprint b4e3f5ed"));
 assert(find(lv_screen_active(),"LifeHash version2 - display only, never input."));
 lv_obj_update_layout(pi_ui.root);
 fprintf(stderr,"PASSPHRASE_VIEWPORT y=%d h=%d bottom=%d\n",(int)lv_obj_get_y(pi_ui.viewport),(int)lv_obj_get_height(pi_ui.viewport),(int)(lv_obj_get_y(pi_ui.viewport)+lv_obj_get_height(pi_ui.viewport)));
 assert(lv_obj_get_y(pi_ui.viewport)+lv_obj_get_height(pi_ui.viewport)<=752);
 assert(pi_safety_dock&&lv_obj_is_visible(pi_safety_dock)&&lv_obj_get_height(pi_safety_dock)>=44);
 assert(strstr(lv_label_get_text(lv_obj_get_child(pi_safety_dock,0)),"Public TEST input only / NEVER fund."));
 assert(lv_obj_get_scrollbar_mode(pi_ui.viewport)==LV_SCROLLBAR_MODE_ON);
 pi_assert_orange_scrollbar("integrated-top");
 lv_obj_send_event(pi_safety_dock,LV_EVENT_CLICKED,NULL);assert(mn_safety_open&&lv_obj_is_visible(mn_safety_close));
 lv_obj_send_event(mn_safety_close,LV_EVENT_CLICKED,NULL);assert(!mn_safety_open&&lv_obj_is_visible(pi_safety_dock));
 lv_obj_scroll_to_y(pi_ui.viewport,lv_obj_get_scroll_y(pi_ui.viewport)+lv_obj_get_scroll_bottom(pi_ui.viewport),LV_ANIM_OFF);lv_obj_update_layout(pi_ui.root);
 assert(lv_obj_get_scroll_bottom(pi_ui.viewport)==0);render("passphrase-integrated-result-bottom.ppm");
 pi_assert_orange_scrollbar("integrated-bottom");
 /* Fault injection is opt-in and must terminate in the SAME geometry oracle
  * used by render(). Refresh first so a rejected case has real pixel evidence. */
 if(getenv("PASSPHRASE_GEOMETRY_NEGATIVE")){
  lv_obj_set_y(pi_entropy_label,445);
  lv_obj_scroll_to_y(pi_ui.viewport,!strcmp(getenv("PASSPHRASE_GEOMETRY_NEGATIVE"),"end")?400:0,LV_ANIM_OFF);
  lv_obj_update_layout(pi_ui.root);lv_obj_invalidate(pi_ui.root);lv_refr_now(NULL);
  FILE *f=fopen("overflow-entropy.ppm","wb");assert(f);
  fprintf(f,"P6\n480 800\n255\n");assert(fwrite(pixels,1,sizeof pixels,f)==sizeof pixels);fclose(f);
  geometry(pi_ui.root);
  puts("BUG: geometry accepted deliberately overflowing entropy");
  exit(0);
 }
 passphrase_ui_back(&pi_ui);passphrase_ui_clear(&pi_ui);
 pi_test_full=true;
 lv_obj_send_event(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_CONTINUE),LV_EVENT_CLICKED,NULL);
 assert(pi_dispatch.state==EL_DISPATCH_FREE&&!passphrase_ui_pending(&pi_ui));
 pi_test_full=false;
 lv_obj_send_event(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_CONTINUE),LV_EVENT_CLICKED,NULL);
 assert(pi_dispatch.state==EL_DISPATCH_QUEUED);
 gui_passphrase_work(pi_test_token);gui_passphrase_poll();
 assert(passphrase_ui_result_visible(&pi_ui));
 assert(!strcmp(lv_label_get_text(pi_ui.result_fp),"73c5da0a"));
 passphrase_ui_back(&pi_ui);
 lv_obj_send_event(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_CONTINUE),LV_EVENT_CLICKED,NULL);
 uint64_t old=pi_test_token,epoch=pi_dispatch.route;
 lv_obj_delete(pi_ui.root);assert(!pi_dispatch.route&&!pi_ui.live);
 pi_enter();assert(pi_dispatch.route>epoch);
 gui_passphrase_work(old);gui_passphrase_poll();assert(!passphrase_ui_result_visible(&pi_ui));
 gui_passphrase_dispose();gui_passphrase_dispose();
 gui_passphrase_configure(NULL,NULL,NULL);
 lv_obj_clean(lv_screen_active());gui_create(request);
 gui_passphrase_configure(pi_test_lock,pi_test_unlock,pi_test_publish);
 for(unsigned m=0;m<MODE_COUNT;m++)for(unsigned w=12;w<=24;w+=3)for(unsigned p=0;p<2;p++){
  gui_passphrase_dispose();mode=(el_mode_t)m;selector=w;mn_target=w;
  memset(transcripts,0,sizeof transcripts);memset(lengths,0,sizeof lengths);
  unsigned n=m==MODE_HEX?w*8/3:m==MODE_COINS?w*32/3:100;
  memset(transcripts[m],m==MODE_HEX||m==MODE_COINS?'0':'1',n);lengths[m]=n;
  if(m==MODE_MNEMONIC){char zero[65]={0};memset(zero,'0',w*8/3);assert(el_input_to_mnemonic((uint8_t*)zero,w*8/3,MODE_HEX,w,(uint8_t*)mnemonic_transcript,sizeof mnemonic_transcript)==0);}
  pi_enter();passphrase_ui_set_raw(&pi_ui,(const uint8_t*)(p?"TREZOR":""),p?6:0);
  lv_obj_send_event(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_CONTINUE),LV_EVENT_CLICKED,NULL);
  assert(pi_dispatch.state==EL_DISPATCH_QUEUED);
  el_passphrase_request_t copied=pi_dispatch.request;hex_result_t expected={0};el_compute_passphrase(&copied,&expected);assert(expected.rc==0);
  unsigned renders_before=pi_lifehash_render_calls;
  gui_passphrase_work(pi_test_token);gui_passphrase_poll();assert(passphrase_ui_result_visible(&pi_ui));
  assert(pi_lifehash_render_calls==renders_before+1&&passphrase_ui_lifehash_visible(&pi_ui));
  assert(find(lv_screen_active(),"LifeHash version2 - display only, never input."));
  assert(!strcmp(lv_label_get_text(pi_ui.result_fp),expected.fingerprint));assert(!strcmp(lv_label_get_text(pi_ui.result_addr),expected.address));
  static const char *legacy_captures[]={"legacy46-hex-result.ppm","legacy46-coins-result.ppm","legacy46-d6raw-result.ppm","legacy46-d6coleman-result.ppm","legacy46-words-result.ppm"};
  if(m==MODE_HEX&&w==12&&p)lv_obj_scroll_to_y(pi_ui.viewport,0,LV_ANIM_OFF);
  render(w==12&&!p?legacy_captures[m]:m==MODE_HEX&&w==12&&p?"passphrase-integrated.ppm":NULL);
  if(m==MODE_HEX&&p){
   assert(!strcmp(lv_label_get_text(pi_mnemonic_label),expected.mnemonic));
   assert(!strcmp(lv_label_get_text(pi_entropy_label),expected.entropy));
   for(unsigned page=0;page<4;page++){
    pi_ui.keyboard_open=true;pi_ui.page=page;pp_page(&pi_ui);lv_obj_remove_flag(pi_ui.keyboard,LV_OBJ_FLAG_HIDDEN);
    for(unsigned y=0;y<=400;y+=20){lv_obj_scroll_to_y(pi_ui.viewport,y,LV_ANIM_OFF);render(NULL);}
    for(unsigned k=0;k<35;k++)if(!lv_obj_has_flag(pi_ui.keys[k],LV_OBJ_FLAG_HIDDEN))pi_test_reachable(pi_ui.keys[k]);
    for(unsigned k=0;k<4;k++)pi_test_reachable(pi_ui.controls[k]);
    pi_test_reachable(pi_mnemonic_label);pi_test_reachable(pi_entropy_label);
   }
   printf("PASS raw geometry, recursive reachability, real render: words=%u four keypages / 21 scroll offsets\n",w);
  }
 }
 /* Renderer failure preserves the textual derivation result and removes the icon. */
 passphrase_ui_back(&pi_ui);pi_lifehash_render_fail=true;
 lv_obj_send_event(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_CONTINUE),LV_EVENT_CLICKED,NULL);
 unsigned failure_renders=pi_lifehash_render_calls;gui_passphrase_work(pi_test_token);gui_passphrase_poll();
 assert(pi_lifehash_render_calls==failure_renders+1&&passphrase_ui_result_visible(&pi_ui));
 assert(!passphrase_ui_lifehash_visible(&pi_ui));assert(find(lv_screen_active(),"LifeHash failed - no image was published."));render("legacy46-lifehash-failed.ppm");
 pi_lifehash_render_fail=false;
 /* A completed old render cannot publish after the source generation changes. */
 passphrase_ui_back(&pi_ui);
 lv_obj_send_event(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_CONTINUE),LV_EVENT_CLICKED,NULL);
 unsigned stale_renders=pi_lifehash_render_calls;gui_passphrase_work(pi_test_token);assert(pi_lifehash_render_calls==stale_renders+1);
 pi_source_changed();gui_passphrase_poll();assert(!passphrase_ui_result_visible(&pi_ui));assert(!passphrase_ui_lifehash_visible(&pi_ui));render("legacy46-stale-cleared.ppm");
 gui_passphrase_dispose();lv_obj_clean(lv_screen_active());gui_create(request);
 pi_enter();passphrase_ui_set_raw(&pi_ui,(const uint8_t*)"TREZOR",6);pi_back_event(NULL);
 uint64_t noop_rev=pi_revision;
 lv_obj_t *delete_disabled=find_button(lv_screen_active(),"Delete");
 assert(delete_disabled&&lv_obj_has_state(delete_disabled,LV_STATE_DISABLED));
 lv_obj_send_event(delete_disabled,LV_EVENT_CLICKED,NULL);
 assert(pi_revision==noop_rev&&passphrase_ui_raw_len(&pi_ui)==6);
 lv_obj_t *clear_disabled=find_button(lv_screen_active(),"Clear");
 assert(clear_disabled&&lv_obj_has_state(clear_disabled,LV_STATE_DISABLED));
 lv_obj_send_event(clear_disabled,LV_EVENT_CLICKED,NULL);
 assert(pi_revision==noop_rev&&passphrase_ui_raw_len(&pi_ui)==6);
 click("Load public zero");noop_rev=pi_revision;
 pi_enter();passphrase_ui_set_raw(&pi_ui,(const uint8_t*)"TREZOR",6);pi_back_event(NULL);
 click("Load public zero");assert(pi_revision==noop_rev&&passphrase_ui_raw_len(&pi_ui)==6);
 mode=MODE_MNEMONIC;mn_count=0;mnemonic_transcript[0]=0;noop_rev=pi_revision;mn_apply(3);assert(pi_revision==noop_rev);
 gui_passphrase_dispose();lv_obj_clean(lv_screen_active());gui_create(request);
 mode=MODE_COINS;pi_enter();passphrase_ui_set_raw(&pi_ui,(const uint8_t*)"TREZOR",6);
 uint64_t rev=pi_revision;lv_obj_send_event(choices[0],LV_EVENT_CLICKED,NULL);
 assert(pi_revision==rev&&passphrase_ui_raw_len(&pi_ui)==6);
 lv_obj_send_event(choices[1],LV_EVENT_CLICKED,NULL);assert(pi_revision>rev&&passphrase_ui_raw_len(&pi_ui)==0);
 for(unsigned lifecycle=0;lifecycle<3;lifecycle++){
  gui_passphrase_dispose();lv_obj_clean(lv_screen_active());gui_create(request);for(unsigned i=0;i<32;i++)click("0");pi_enter();
  passphrase_ui_set_raw(&pi_ui,(const uint8_t*)"TREZOR",6);
  lv_obj_send_event(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_CONTINUE),LV_EVENT_CLICKED,NULL);
  uint64_t token=pi_test_token,route=pi_dispatch.route;const el_passphrase_request_t *q=el_dispatch_claim(&pi_dispatch,token);assert(q);
  el_passphrase_request_t before=*q;el_dispatch_meta_t meta=pi_dispatch.meta;
  if(lifecycle==0)pi_cancel_event(NULL);else if(lifecycle==1)lv_obj_delete(app_content);else gui_create(request);
  assert(pi_dispatch.state==EL_DISPATCH_RUNNING&&pi_dispatch.cancelled);assert(!memcmp(q,&before,sizeof before));
  hex_result_t computed={0};el_compute_passphrase(q,&computed);assert(computed.rc==0);
  el_dispatch_result_t done={.meta=meta,.rc=computed.rc};memcpy(done.fingerprint,computed.fingerprint,sizeof done.fingerprint);memcpy(done.address,computed.address,sizeof done.address);
  assert(!el_dispatch_finish(&pi_dispatch,token,&done));assert(pi_dispatch.state==EL_DISPATCH_FREE);
  if(lifecycle==1)gui_create(request);
  pi_enter();if(lifecycle)assert(pi_dispatch.route>route);gui_passphrase_work(token);gui_passphrase_poll();assert(!passphrase_ui_result_visible(&pi_ui));
 }
 for(unsigned dm=MODE_D6_RAW;dm<=MODE_D6_COLEMAN;dm++){
  gui_passphrase_dispose();mode=(el_mode_t)dm;selector=12;strcpy(transcripts[dm],"1");lengths[dm]=1;pi_enter();
  lv_obj_send_event(passphrase_ui_control(&pi_ui,PASSPHRASE_UI_CONTROL_CONTINUE),LV_EVENT_CLICKED,NULL);
  gui_passphrase_work(pi_test_token);gui_passphrase_poll();assert(passphrase_ui_result_visible(&pi_ui));
  assert(strstr(passphrase_ui_status(&pi_ui),"WEAK_INPUT_LAB_ONLY"));
   lv_obj_scroll_to_y(pi_ui.viewport,0,LV_ANIM_OFF);render("passphrase-weak.ppm");
   if(dm==MODE_D6_RAW)pi_assert_orange_scrollbar("weak-top");
   lv_obj_scroll_to_y(pi_ui.viewport,lv_obj_get_scroll_y(pi_ui.viewport)+lv_obj_get_scroll_bottom(pi_ui.viewport),LV_ANIM_OFF);lv_obj_update_layout(pi_ui.root);
   assert(lv_obj_get_scroll_bottom(pi_ui.viewport)==0);render(dm==MODE_D6_RAW?"passphrase-weak-result-bottom.ppm":NULL);
   if(dm==MODE_D6_RAW)pi_assert_orange_scrollbar("weak-bottom");
 }
 gui_passphrase_dispose();gui_passphrase_configure(NULL,NULL,NULL);lv_obj_clean(lv_screen_active());gui_create(request);
 puts("PASS integration 50 real-runtime mode/word/passphrase cases, actual LifeHash worker publication/failure/stale clearing, geometry/render, synchronous snapshot / dispose");
}
