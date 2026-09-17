/* Public blank-state regression through the real LVGL pointer reader/hit test.
 * No direct CLICKED dispatch: label lookup supplies coordinates only. */
static void modal_touch(lv_indev_t *dev,lv_obj_t *button,const char *name){
 lv_obj_update_layout(lv_screen_active());
 assert(button && lv_obj_is_visible(button));
 assert(!lv_obj_has_state(button,LV_STATE_DISABLED));
 lv_obj_update_layout(lv_screen_active());
 lv_area_t a;lv_obj_get_coords(button,&a);
 int x=(a.x1+a.x2)/2,y=(a.y1+a.y2)/2;
 touch_at(dev,x,y,false);touch_at(dev,x,y,true);
 lv_point_t point={x,y};lv_obj_t *hit=lv_indev_search_obj(lv_screen_active(),&point);
 fprintf(stderr,"MODAL_TOUCH %s xy=%d,%d target=%p hit=%p exact=%d safety=%d about=%d saver=%d latch=%d\n",name,x,y,(void*)button,(void*)hit,hit==button,mn_safety_open,mn_about_open,saver_active,wake_latch);
 if(hit!=button)fprintf(stderr,"MODAL_INTERCEPT diceplus_panel=%d seed_panel=%d cards_bases_panel=%d safety_modal=%d\n",hit==extra_dice_ui.panel,hit==seed_ui.panel,hit==gui08_ui.panel,hit==mn_safety_modal);
 touch_at(dev,x,y,false);
}
static void modal_methods(lv_indev_t *dev,const char *name){
 unsigned route=nav_route;uint64_t revision=legacy_revision;
 modal_touch(dev,nav_family,name);modal_touch(dev,nav_rows[2],"Dice methods via family");
 assert(nav_open&&nav_methods&&!nav_confirming);
 assert(nav_route==route&&legacy_revision==revision);
}
static unsigned blocker_cancels;
static bool blocker_cancel(uint64_t id){assert(id==987);blocker_cancels++;return true;}
static const char *blocker_route_name(unsigned r){
 return r==5?"Seed":r==6?"Cards":r==7?"Bases":r==8?"BitBox diceware":"D++ · D8 / D16";
}
static void assert_box(lv_obj_t *o,int x,int y,int w,int h){lv_area_t a;lv_obj_update_layout(lv_screen_active());lv_obj_get_coords(o,&a);if(a.x1!=x||a.y1!=y||lv_obj_get_width(o)!=w||lv_obj_get_height(o)!=h)fprintf(stderr,"SELECTOR_BOX expected=%d,%d %dx%d actual=%d,%d %dx%d\n",x,y,w,h,a.x1,a.y1,(int)lv_obj_get_width(o),(int)lv_obj_get_height(o));assert(a.x1==x&&a.y1==y&&lv_obj_get_width(o)==w&&lv_obj_get_height(o)==h);}
static bool selector_overlap(lv_obj_t *a,lv_obj_t *b){lv_area_t x,y;lv_obj_get_coords(a,&x);lv_obj_get_coords(b,&y);return !(x.x2<y.x1||y.x2<x.x1||x.y2<y.y1||y.y2<x.y1);}
static unsigned selector_pixels(int x1,int y1,int x2,int y2,unsigned char r,unsigned char g,unsigned char b){
 unsigned n=0;for(int y=y1;y<=y2;y++)for(int x=x1;x<=x2;x++){unsigned char *p=pixels+3*(y*480+x);n+=p[0]==r&&p[1]==g&&p[2]==b;}return n;
}
static void selector_row_content(lv_obj_t *row,const char *title,const char *description,bool selected){
 assert(lv_obj_get_child_count(row)==3);
 lv_obj_t *primary=lv_obj_get_child(row,0),*secondary=lv_obj_get_child(row,1),*check=lv_obj_get_child(row,2);
 assert(lv_obj_check_type(primary,&lv_label_class)&&!strcmp(lv_label_get_text(primary),title));
 assert(lv_obj_check_type(secondary,&lv_label_class)&&!strcmp(lv_label_get_text(secondary),description));
 assert(lv_obj_get_style_text_font(primary,LV_PART_MAIN)==&el_sans_16);
 assert(lv_obj_get_style_text_font(secondary,LV_PART_MAIN)==&el_sans_14);
 assert(lv_obj_has_state(row,LV_STATE_CHECKED)==selected);
 assert(lv_obj_has_flag(check,LV_OBJ_FLAG_HIDDEN)==!selected);
 assert(lv_obj_get_child_count(check)==2);
 lv_area_t rr,pr,sr,cr;lv_obj_get_coords(row,&rr);lv_obj_get_coords(primary,&pr);lv_obj_get_coords(secondary,&sr);lv_obj_get_coords(check,&cr);
 assert(pr.x1>=rr.x1&&pr.x2<=rr.x2&&pr.y1>=rr.y1&&pr.y2<=rr.y2);
 assert(sr.x1>=rr.x1&&sr.x2<=rr.x2&&sr.y1>=rr.y1&&sr.y2<=rr.y2);
 assert(cr.x1>=rr.x1&&cr.x2<=rr.x2&&cr.y1>=rr.y1&&cr.y2<=rr.y2);
 assert(lv_obj_is_visible(primary)&&lv_obj_is_visible(secondary));
}
static void selector_sheet_content(bool methods){
 static const char *family_titles[]={"Hex","Coins","Dice","Words","Seed","Cards","Bases"};
 static const char *family_descriptions[]={"0–F · original hexadecimal input","Heads / tails · raw encoded bits","D6 raw, Coleman, BitBox and D++","English mnemonic · dictionary entry","BIP39 word numbers · checksum picker","Direct card-rank mapping","Canonical base 4, 8, 32 and 64"};
 static const char *method_titles[]={"D6 raw · hashed rolls","D6 Coleman · hashed rolls","BitBox diceware","D++ · D8 / D16"};
 static const char *method_descriptions[]={"Original 1–6 digits · SHA-256","6 → 0 before SHA-256","Five D4 rolls + D6 coin per prefix word","D8 + D16 + D16; size-specific final rolls"};
 lv_font_glyph_dsc_t glyph={0};
 assert(lv_font_get_glyph_dsc(&el_sans_16,&glyph,0x00b7,0));
 assert(lv_font_get_glyph_dsc(&el_sans_14,&glyph,0x00b7,0));
 assert(lv_font_get_glyph_dsc(&el_sans_14,&glyph,0x2013,0));
 assert(lv_font_get_glyph_dsc(&el_sans_14,&glyph,0x2192,0));
 unsigned selected=0,count=methods?4:7;
 if(methods)selected=nav_dice==2?0:nav_dice==3?1:nav_dice==8?2:3;
 else {unsigned family=nav_route==3||nav_route>=8?2:nav_route;for(unsigned i=0;i<7;i++)if(nav_families[i]==family)selected=i;}
 for(unsigned i=0;i<count;i++)selector_row_content(nav_rows[i],methods?method_titles[i]:family_titles[i],methods?method_descriptions[i]:family_descriptions[i],i==selected);
 render(NULL);
 lv_area_t rr,cr,sr;lv_obj_get_coords(nav_rows[selected],&rr);lv_obj_get_coords(lv_obj_get_child(nav_rows[selected],2),&cr);lv_obj_get_coords(lv_obj_get_child(nav_rows[selected],1),&sr);
 /* RGB565 expansion of #ff9900, #000 and #333 proves the selected fill,
  * drawn checkmark and selected secondary copy reached the framebuffer. */
 assert(selector_pixels(rr.x1,rr.y1,rr.x2,rr.y2,255,153,0)>1000);
 assert(selector_pixels(cr.x1,cr.y1,cr.x2,cr.y2,0,0,0)>8);
 assert(selector_pixels(sr.x1,sr.y1,sr.x2,sr.y2,49,48,49)>2);
 fprintf(stderr,"SELECTOR_CONTENT sheet=%s titles=exact descriptions=visible-14px checkmark=visible selected=orange-black clipping=none\n",methods?"method":"family");
}
static void confirmation_action_rows(bool methods){
 const char *probe=getenv("CONFIRMATION_ACTION_PROBE");
 if(probe&&strcmp(probe,methods?"method":"family"))return;
 static const char *titles[]={"Keep input","Change and clear"};
 for(unsigned i=0;i<2;i++){
  assert(lv_obj_get_child_count(nav_rows[i])==3);
  lv_obj_t *primary=lv_obj_get_child(nav_rows[i],0),*secondary=lv_obj_get_child(nav_rows[i],1),*check=lv_obj_get_child(nav_rows[i],2);
  assert(!strcmp(lv_label_get_text(primary),titles[i]));
  assert(!strcmp(lv_label_get_text(secondary),""));
  assert(lv_obj_has_flag(secondary,LV_OBJ_FLAG_HIDDEN));
  assert(lv_obj_has_flag(check,LV_OBJ_FLAG_HIDDEN));
  assert(!lv_obj_has_state(nav_rows[i],LV_STATE_CHECKED));
 }
 fprintf(stderr,"CONFIRMATION_ACTION_ROWS source=%s secondary=absent checkmarks=absent titles=exact\n",methods?"method":"family");
}
static void selector_contract(lv_indev_t *dev,unsigned r){
 assert_box(nav_family,16,56,448,48);
 bool dice=r==2||r==3||r>=8;
 /* The finalized D6 panel leaves no separate method-button row. Dice methods
  * must remain reachable through the visible family sheet, not hidden events. */
 assert(lv_obj_has_flag(nav_method,LV_OBJ_FLAG_HIDDEN));
 if(dice){
  if(r<4){assert_box(tabs[0],29,140,207,44);assert_box(input,28,193,422,545);assert(!selector_overlap(nav_family,tabs[0]));assert(!selector_overlap(nav_family,tabs[1]));assert(!selector_overlap(nav_family,input));assert(!selector_overlap(nav_family,output));}
  else for(unsigned i=0;i<5;i++)assert(!selector_overlap(nav_family,extra_dice_ui.word_buttons[i]));
 }
 modal_touch(dev,nav_family,"contract family sheet");assert_box(nav_sheet,0,0,480,800);assert_box(lv_obj_get_parent(nav_title),16,116,448,590);selector_sheet_content(false);render("selector-family-sheet.ppm");modal_touch(dev,nav_close,"contract family close");
 if(dice){
  static const char *disclosures[]={"D6 raw · hashed rolls v","D6 Coleman · hashed rolls v","BitBox diceware v","D++ · D8 / D16 v"};
  assert(!strcmp(lv_label_get_text(lv_obj_get_child(nav_method,0)),disclosures[r==2?0:r==3?1:r==8?2:3]));
  modal_methods(dev,"contract method sheet");assert_box(nav_sheet,0,0,480,800);assert_box(lv_obj_get_parent(nav_title),16,116,448,374);selector_sheet_content(true);render("selector-method-sheet.ppm");modal_touch(dev,nav_close,"contract method close");
 }
 unsigned route=nav_route;uint64_t legacy=legacy_revision,feature=r<5?0:r==5?seed_ui.editor.revision:r<8?gui08_ui.editor.revision:extra_dice_ui.editor.revision;
 modal_touch(dev,nav_family,"outside-dismiss setup");lv_point_t p={8,80};assert(lv_indev_search_obj(lv_screen_active(),&p)==nav_sheet);touch_at(dev,8,80,true);touch_at(dev,8,80,false);assert(!nav_open&&nav_route==route&&legacy_revision==legacy);if(r>=5)assert(feature==(r==5?seed_ui.editor.revision:r<8?gui08_ui.editor.revision:extra_dice_ui.editor.revision));
 fprintf(stderr,"SELECTOR_GATES geometry=pass pointer-exclusion=pass outside-dismiss=pass gesture-consumed=pass keyboard-escape=unsupported(no keyboard indev) focus-return=unsupported(no LVGL group) route=%u\n",r);
}
static void blocker_context(lv_indev_t *dev,unsigned r){
 if(r<5||r==4)return;
 gui08_editor *ed=r==5?&seed_ui.editor:r<8?&gui08_ui.editor:&extra_dice_ui.editor;
 lv_obj_t **counts=r==5?seed_ui.word_buttons:r<8?gui08_ui.word_buttons:extra_dice_ui.word_buttons;
 if(r==5){modal_touch(dev,seed_ui.digits[1],"Seed draft");modal_touch(dev,seed_ui.next_word,"Seed commit");modal_touch(dev,seed_ui.digits[2],"Seed second draft");}
 else if(r<8)modal_touch(dev,gui08_ui.symbol_buttons[0],"Cards/Bases public input");
 else modal_touch(dev,extra_dice_ui.keys[1],"Advanced public input");
 /* Capture all owner bytes, including drafts, result flags and revisions. */
 void *owner=r==5?(void*)&seed_ui:r<8?(void*)&gui08_ui:(void*)&extra_dice_ui;
 size_t size=r==5?sizeof seed_ui:r<8?sizeof gui08_ui:sizeof extra_dice_ui;
 unsigned char *snapshot=malloc(size);memcpy(snapshot,owner,size);
 modal_touch(dev,counts[0],"same count no-op");assert(!nav_open&&!memcmp(snapshot,owner,size));
 modal_touch(dev,counts[1],"changed count requests confirmation");assert(nav_open&&nav_context_target==counts[1]);
 confirmation_action_rows(false);
 const char *confirmation=lv_label_get_text(nav_title);
 assert(strstr(confirmation,blocker_route_name(r))&&strstr(confirmation,"12 -> 15 words")&&strstr(confirmation,"will be cleared"));
 char name[64];snprintf(name,sizeof name,"blocker-confirm-%u.ppm",r);render(name);
 void *stale=saved_callback(nav_rows[1],nav_event);
 modal_touch(dev,nav_rows[0],"Keep all owner input");assert(!memcmp(snapshot,owner,size));replay_callback(nav_event,stale);assert(!memcmp(snapshot,owner,size));
 modal_touch(dev,counts[1],"changed count for Close");void *close_stale=saved_callback(nav_rows[1],nav_event);
 modal_touch(dev,nav_close,"Close context confirmation");assert(!memcmp(snapshot,owner,size));replay_callback(nav_event,close_stale);assert(!memcmp(snapshot,owner,size));
 modal_touch(dev,counts[1],"changed count again");uint64_t revision=ed->revision;
 modal_touch(dev,nav_rows[1],"Change and clear count");assert(ed->words==15&&ed->length==0&&ed->revision==revision+1);
 if(r==5)assert(!seed_ui.committed&&!seed_ui.draft_len);
 if(r>=8)assert(!extra_dice_ui.final_length&&!extra_dice_ui.final_selected);
 if(r==7){modal_touch(dev,gui08_ui.symbol_buttons[0],"Bases input for radix change");memcpy(snapshot,owner,size);modal_touch(dev,gui08_ui.base_buttons[1],"change radix");assert(nav_context_target);modal_touch(dev,nav_rows[0],"Keep radix input");assert(!memcmp(snapshot,owner,size));modal_touch(dev,gui08_ui.base_buttons[1],"change radix again");revision=ed->revision;modal_touch(dev,nav_rows[1],"Confirm radix");assert(ed->base==8&&!ed->length&&ed->revision==revision+1);}
 /* Result ownership must also require confirmation, even without transcript. */
 ed->result_visible=true;memcpy(snapshot,owner,size);modal_touch(dev,counts[2],"result context change");assert(nav_context_target);modal_touch(dev,nav_rows[0],"Keep result");assert(!memcmp(snapshot,owner,size));ed->result_visible=false;
 free(snapshot);
 /* Simulate owned in-flight work; no KDF runs or real secrets in this fixture. */
 ed->pending_id=987;ed->pending_revision=ed->revision;
 lv_obj_t *cancel;
 if(r==5){seed_ui.cancel=blocker_cancel;seed_native_show(&seed_ui);cancel=seed_ui.derive;}
 else if(r<8){gui08_ui.cancel=blocker_cancel;gui08_native_show(&gui08_ui,gui08_ui.editor.method);cancel=gui08_ui.derive;}
 else {extra_dice_ui.cancel=blocker_cancel;extra_dice_native_show(&extra_dice_ui);cancel=extra_dice_ui.derive;}
 saver_tick(NULL);assert(lv_obj_has_state(nav_family,LV_STATE_DISABLED));assert(lv_obj_has_state(nav_method,LV_STATE_DISABLED));
 unsigned route=nav_route;lv_obj_send_event(nav_family,LV_EVENT_CLICKED,NULL);assert(!nav_open&&nav_route==route&&ed->pending_id==987);
 lv_area_t a;lv_obj_get_coords(nav_family,&a);touch_at(dev,(a.x1+a.x2)/2,(a.y1+a.y2)/2,true);touch_at(dev,(a.x1+a.x2)/2,(a.y1+a.y2)/2,false);assert(!nav_open&&ed->pending_id==987);
 unsigned before=blocker_cancels;modal_touch(dev,cancel,"Pending owner Cancel reachable");assert(!ed->pending_id&&blocker_cancels==before+1);saver_tick(NULL);assert(!lv_obj_has_state(nav_family,LV_STATE_DISABLED));
 fprintf(stderr,"PASS B2 owner context and pending cancel route=%u\n",r);
}
static void modal_touch_tests(lv_indev_t *dev){
 const char *family=getenv("MODAL_TOUCH_FAMILY");if(!family)family="Dice+";
 unsigned r=!strcmp(family,"Hex")?0:!strcmp(family,"Coins")?1:!strcmp(family,"D6 raw")?2:!strcmp(family,"D6 6->0")?3:!strcmp(family,"Words")?4:!strcmp(family,"Seed")?5:!strcmp(family,"Cards")?6:!strcmp(family,"Bases")?7:!strcmp(family,"D++")?9:8;
 const char *safety_dock=lv_label_get_text(lv_obj_get_child(mn_safety,0));
 assert(strstr(safety_dock,"Public TEST input only / NEVER fund.")&&strstr(safety_dock,"Safety / About"));
 assert(lv_obj_get_height(mn_safety)>=44);
 modal_touch(dev,nav_family,"family selector");unsigned row=r==0?0:r==1?1:(r==2||r==3||r>=8)?2:r-1;modal_touch(dev,nav_rows[row],"family option");
 if(r==2||r==3||r>=8){assert(nav_open&&nav_methods&&!nav_confirming);assert(nav_route==0);modal_touch(dev,nav_rows[r==2?0:r==3?1:r==9?3:2],"dice option");}assert(nav_route==r);
 if(!getenv("ABOUT_BODY_ONLY"))selector_contract(dev,r);
 char capture[96];
 if(r==2||r==3){modal_touch(dev,tabs[1],"legacy Test results unobscured");assert(!lv_obj_has_flag(output,LV_OBJ_FLAG_HIDDEN));modal_touch(dev,tabs[0],"legacy Input unobscured");assert(!lv_obj_has_flag(input,LV_OBJ_FLAG_HIDDEN));}
 if(r>=8){lv_obj_t *body=lv_obj_get_parent(extra_dice_ui.keys[1]);for(unsigned i=0;i<3;i++){lv_obj_update_layout(lv_screen_active());int maximum=lv_obj_get_scroll_y(body)+lv_obj_get_scroll_bottom(body);lv_obj_scroll_to_y(body,i==0?0:i==1?maximum/2:maximum,LV_ANIM_OFF);lv_obj_update_layout(lv_screen_active());fprintf(stderr,"B1_SCROLL route=%u position=%u y=%d remaining=%d\n",r,i,(int)lv_obj_get_scroll_y(body),(int)lv_obj_get_scroll_bottom(body));if(i==2)assert(lv_obj_get_scroll_bottom(body)==0);lv_obj_t *target=i==0?extra_dice_ui.word_buttons[0]:i==1?extra_dice_ui.keys[1]:extra_dice_ui.load_public;lv_area_t a;lv_obj_get_coords(target,&a);lv_point_t point={(a.x1+a.x2)/2,(a.y1+a.y2)/2};assert(lv_indev_search_obj(lv_screen_active(),&point)==target);modal_touch(dev,target,"advanced scroll actionable control");snprintf(capture,sizeof capture,"blocker-scroll-%u-%u.ppm",r,i);render(capture);}lv_obj_scroll_to_y(body,0,LV_ANIM_OFF);lv_obj_send_event(extra_dice_ui.clear,LV_EVENT_CLICKED,NULL);}
 blocker_context(dev,r);
 if(r==6){
  modal_touch(dev,gui08_ui.symbol_buttons[0],"Cards input before family change");gui08_native saved=gui08_ui;
  modal_touch(dev,nav_family,"family change open");modal_touch(dev,nav_rows[6],"Cards to Bases asks confirmation");
  assert(nav_confirming&&strstr(lv_label_get_text(nav_title),"Cards -> Bases")&&strstr(lv_label_get_text(nav_title),"will be cleared"));
  confirmation_action_rows(false);
  render("blocker-confirm-family-cards-bases.ppm");modal_touch(dev,nav_rows[0],"Keep Cards family input");assert(!memcmp(&saved,&gui08_ui,sizeof saved));
  modal_touch(dev,nav_family,"family change reopen");modal_touch(dev,nav_rows[6],"Cards to Bases asks again");
  modal_touch(dev,nav_rows[1],"Change family and clear");assert(nav_route==7&&!gui08_ui.editor.length);
  modal_touch(dev,nav_family,"restore Cards for route-6 evidence");modal_touch(dev,nav_rows[5],"Cards route");
  assert(nav_route==6&&gui08_ui.editor.method==GUI08_CARDS&&!strcmp(active_about_name(),"Cards"));
  render("cards-ordinary-exact.ppm");
 }

 unsigned saved_route=nav_route;uint64_t saved_revision=legacy_revision;void *oldnav=saved_callback(nav_family,nav_event);
 modal_touch(dev,nav_family,"open without mutation");render("grouped-family-sheet.ppm");modal_touch(dev,nav_close,"close without mutation");assert(nav_route==saved_route&&legacy_revision==saved_revision);replay_callback(nav_event,oldnav);assert(!nav_open);
 snprintf(capture,sizeof capture,"grouped-%u.ppm",r);render(capture);
 modal_touch(dev,mn_safety,"Safety");assert(mn_safety_open&&!mn_about_open);
 assert(lv_obj_get_width(mn_safety_modal)==480&&lv_obj_get_height(mn_safety_modal)==800);
 assert(lv_obj_is_visible(mn_safety_close)&&lv_obj_get_height(mn_safety_close)>=44);
 lv_area_t close_area;lv_obj_get_coords(mn_safety_close,&close_area);assert(close_area.y1==706&&close_area.y2<800);
 snprintf(capture,sizeof capture,"safety-overlay-%u.ppm",r);render(capture);
 if(!strcmp(family,"Dice+")){
  size_t before=extra_dice_ui.editor.length;
  modal_touch(dev,extra_dice_ui.keys[1],"underlying Dice+ key while Safety open");
  assert(extra_dice_ui.editor.length==before);
  fprintf(stderr,"MODAL_GUARD underlying Dice+ input preserved length=%zu\n",before);
 }
 fprintf(stderr,"MODAL_LAYERS modal=%d cards=%d seed=%d diceplus=%d\n",(int)lv_obj_get_index(mn_safety_modal),(int)lv_obj_get_index(gui08_ui.panel),(int)lv_obj_get_index(seed_ui.panel),(int)lv_obj_get_index(extra_dice_ui.panel));
 modal_touch(dev,mn_about,"About this input");
 fprintf(stderr,"REQUIRE About opened through pointer path: actual=%d\n",mn_about_open);
 if(!mn_about_open){
  modal_touch(dev,mn_safety_close,"Close after blocked About");
  fprintf(stderr,"BLOCKED_CLOSE safety=%d\n",mn_safety_open);
  modal_touch(dev,modes[4],"Words after blocked About");
  fprintf(stderr,"BLOCKED_ROUTE words=%d diceplus_selected=%d guard=%d\n",mode==MODE_MNEMONIC,lv_obj_has_state(gui08_routes[3],LV_STATE_CHECKED),mn_safety_open);
 }
 assert(mn_about_open);assert(!strcmp(lv_label_get_text(mn_about_label),active_about_name()));
 const education_document *active_doc=active_education_document();
 if(active_doc)assert(!strcmp(lv_label_get_text(mn_about_body),active_doc->before_practice));
 else assert(!strcmp(lv_label_get_text(mn_about_body),active_about_copy()));
 if(r==6){assert(active_doc&&nav_route==6&&!strcmp(active_about_name(),"Cards"));assert(strstr(active_doc->before_practice,"direct rank phases A-8")&&!strstr(active_doc->before_practice,"base 4, 8, 32, or 64"));render("cards-about-exact.ppm");}
 if(r==7){assert(active_doc&&nav_route==7&&!strcmp(active_about_name(),"Number bases"));assert(strstr(active_doc->before_practice,"base 4, 8, 32, or 64")&&!strstr(active_doc->before_practice,"direct rank phases A-8"));render("bases-about-exact.ppm");}
 snprintf(capture,sizeof capture,"grouped-about-%u.ppm",r);render(capture);
 modal_touch(dev,mn_about_back,"Back to Safety");assert(mn_safety_open&&!mn_about_open);
 modal_touch(dev,mn_about,"About this input again");assert(mn_about_open);
 modal_touch(dev,mn_safety_close,"Close About");assert(!mn_safety_open);
 modal_touch(dev,mn_safety,"Safety again");assert(mn_safety_open&&!mn_about_open);
 modal_touch(dev,mn_safety_close,"Close Safety");assert(!mn_safety_open);
 if(!strcmp(family,"Dice+")){
  size_t before=extra_dice_ui.editor.length;
  modal_touch(dev,extra_dice_ui.keys[1],"Dice+ key after close");
  assert(extra_dice_ui.editor.length==before+1);
 }
 /* Meaningful public input: same route, cancel and accept preserve semantics. */
 if(r==8){
  size_t n=extra_dice_ui.editor.length;uint64_t revision=extra_dice_ui.editor.revision;assert(n>0);
  modal_methods(dev,"same method open");modal_touch(dev,nav_rows[2],"same method select");
  assert(nav_route==8&&extra_dice_ui.editor.length==n&&extra_dice_ui.editor.revision==revision);
  modal_methods(dev,"change method open");modal_touch(dev,nav_rows[3],"request destructive change");assert(nav_confirming);
  assert(strstr(lv_label_get_text(nav_title),"BitBox diceware -> D++ · D8 / D16")&&strstr(lv_label_get_text(nav_title),"will be cleared"));
  confirmation_action_rows(true);
  render("blocker-confirm-method-bitbox-dpp.ppm");
  modal_touch(dev,nav_rows[0],"Keep input");assert(nav_route==8&&extra_dice_ui.editor.length==n&&extra_dice_ui.editor.revision==revision);
  modal_methods(dev,"change method again");modal_touch(dev,nav_rows[3],"request change again");assert(nav_confirming);
  modal_touch(dev,nav_rows[1],"Change and clear");assert(nav_route==9&&extra_dice_ui.editor.length==0&&extra_dice_ui.editor.revision!=revision);
 }
 if(r==5){
  lv_obj_t *body=lv_obj_get_parent(seed_ui.fingerprint.label);lv_obj_scroll_to_y(body,240,LV_ANIM_OFF);render("seed-readout-scrolled.ppm");lv_obj_scroll_to_y(body,0,LV_ANIM_OFF);
 }
 /* An independent family route must remain usable without reset/reboot. */
 modal_touch(dev,nav_family,"family after close");modal_touch(dev,nav_rows[3],"Words after close");assert(mode==MODE_MNEMONIC);
 assert(!lv_obj_has_flag(mn_panel,LV_OBJ_FLAG_HIDDEN));
 fprintf(stderr,"PASS pointer Safety/About/Back/Close/reopen/navigation family=%s\n",family);
}
