/* Education contract regression. Synthetic public state only; no secrets. */
typedef struct {
 unsigned nav_route_value;
 el_mode_t mode_value;
 uint64_t legacy_revision_value;
 size_t legacy_lengths[GUI_MODE_COUNT];
 char legacy_transcripts[GUI_MODE_COUNT][1025];
 char words_phrase[216];
 char words_draft[9];
 uint16_t words_ids[24];
 unsigned words_count,words_target;
 int words_edit;
 gui08_editor cards_bases;
 gui08_editor seed;
 char seed_draft[5];
 size_t seed_draft_len;
 unsigned seed_committed;
 gui08_editor dice;
 unsigned dice_method;
 char dice_final[3];
 size_t dice_final_length;
 unsigned dice_final_choice;
 bool dice_final_selected;
} education_state;

static void education_snapshot(education_state *s){
 memset(s,0,sizeof *s);s->nav_route_value=nav_route;s->mode_value=mode;s->legacy_revision_value=legacy_revision;
 memcpy(s->legacy_lengths,lengths,sizeof lengths);memcpy(s->legacy_transcripts,transcripts,sizeof transcripts);
 memcpy(s->words_phrase,mnemonic_transcript,sizeof mnemonic_transcript);memcpy(s->words_draft,mn_draft,sizeof mn_draft);
 memcpy(s->words_ids,mn_words,sizeof mn_words);s->words_count=mn_count;s->words_target=mn_target;s->words_edit=mn_edit;
 s->cards_bases=gui08_ui.editor;s->seed=seed_ui.editor;memcpy(s->seed_draft,seed_ui.draft,sizeof seed_ui.draft);
 s->seed_draft_len=seed_ui.draft_len;s->seed_committed=seed_ui.committed;s->dice=extra_dice_ui.editor;
 s->dice_method=extra_dice_ui.method;memcpy(s->dice_final,extra_dice_ui.final_roll,sizeof extra_dice_ui.final_roll);
 s->dice_final_length=extra_dice_ui.final_length;s->dice_final_choice=extra_dice_ui.final_choice;s->dice_final_selected=extra_dice_ui.final_selected;
}
static void education_state_equal(const education_state *a,const education_state *b){assert(!memcmp(a,b,sizeof *a));}
static bool education_tree_contains(lv_obj_t *o,const char *needle){
 if(lv_obj_has_flag(o,LV_OBJ_FLAG_HIDDEN))return false;
 if(lv_obj_check_type(o,&lv_label_class)&&strstr(lv_label_get_text(o),needle))return true;
 for(uint32_t i=0;i<lv_obj_get_child_count(o);i++)if(education_tree_contains(lv_obj_get_child(o,i),needle))return true;
 return false;
}
static void education_rect(lv_obj_t *o,int x,int y,int w,int h){lv_area_t a;lv_obj_update_layout(lv_screen_active());lv_obj_get_coords(o,&a);assert(a.x1==x&&a.y1==y&&lv_obj_get_width(o)==w&&lv_obj_get_height(o)==h);}
static void education_font_covers(const lv_font_t *font,const char *text){
 for(size_t i=0;text[i];i++){
  unsigned char c=(unsigned char)text[i];
  assert(c<0x80); /* Native education copy deliberately uses the frozen ASCII glyph set. */
  if(c>' '){lv_font_glyph_dsc_t dsc;assert(lv_font_get_glyph_dsc(font,&dsc,c,0));}
 }
}
static void education_copy_glyphs(void){
 education_font_covers(&el_serif_24,lv_label_get_text(mn_about_label));
 education_font_covers(&el_sans_16,lv_label_get_text(mn_about_body));
 education_font_covers(&el_sans_16,lv_label_get_text(mn_about_practice_body));
 education_font_covers(&el_sans_16,lv_label_get_text(mn_about_after));
}
static void education_flow_layout(void){
 lv_area_t before,practice,practice_body,after;lv_obj_update_layout(lv_screen_active());
 lv_obj_get_coords(mn_about_body,&before);lv_obj_get_coords(mn_about_practice,&practice);
 lv_obj_get_coords(mn_about_practice_body,&practice_body);lv_obj_get_coords(mn_about_after,&after);
 fprintf(stderr,"EDUCATION_FLOW before=%d..%d practice=%d..%d body=%d..%d after=%d..%d\n",before.y1,before.y2,practice.y1,practice.y2,practice_body.y1,practice_body.y2,after.y1,after.y2);
 assert(practice.y1-before.y2-1>=18);
 assert(practice_body.y1>=practice.y1&&practice_body.y2<=practice.y2);
 assert(after.y1-practice.y2-1>=18);
}
static void education_fixed_geometry(void){
 education_rect(mn_about_scroll,28,84,424,532);
 education_rect(mn_about_back,28,632,424,44);education_rect(mn_safety_close,28,706,424,44);
 lv_area_t v,b,c;lv_obj_get_coords(mn_about_scroll,&v);lv_obj_get_coords(mn_about_back,&b);lv_obj_get_coords(mn_safety_close,&c);
 assert(v.y2==615&&b.y1==632&&c.y1==706);
}
static void education_drag(lv_indev_t *dev,int from_y,int to_y){
 touch_at(dev,240,from_y,false);touch_at(dev,240,from_y,true);
 int step=to_y<from_y?-40:40;for(int y=from_y+step;(step<0?y>to_y:y<to_y);y+=step)touch_at(dev,240,y,true);
 touch_at(dev,240,to_y,true);touch_at(dev,240,to_y,false);lv_obj_update_layout(lv_screen_active());
}
static void education_public_fixture(lv_indev_t *dev,unsigned route){
 if(route==4)type_word("abandon");
 else if(route==5){modal_touch(dev,seed_ui.digits[1],"education seed digit");modal_touch(dev,seed_ui.next_word,"education seed commit");}
 else if(route==6||route==7)modal_touch(dev,gui08_ui.symbol_buttons[0],"education direct symbol");
 else if(route==8||route==9)modal_touch(dev,extra_dice_ui.keys[1],"education dice key");
}
static void education_open(lv_indev_t *dev){modal_touch(dev,mn_safety,"education Safety");assert(mn_safety_open&&!mn_about_open);modal_touch(dev,mn_about,"education About");assert(mn_safety_open&&mn_about_open);}
static void education_dplus18_boundary_fixtures(void){
 static const struct {const char *input;int status;uint8_t value;} cases[]={
  {"01",EL_DICE_OK,0},{"04",EL_DICE_OK,0},{"05",EL_DICE_OK,1},{"08",EL_DICE_OK,1},{"09",EL_DICE_INVALID,0}
 };
 for(size_t i=0;i<sizeof cases/sizeof cases[0];i++){
  uint8_t actual=0xa5;int status=el_dplus_final_index(cases[i].input,2,18,&actual);
  assert(status==cases[i].status);
  if(status==EL_DICE_OK)assert(actual==cases[i].value);else assert(actual==0xa5);
 }
}
static void education_matrix(lv_indev_t *dev){
 education_dplus18_boundary_fixtures();
 static const char *route_names[]={"Words","Seed","Cards","Bases","Dice+","D++ D8/D16"};
 static const unsigned route_ids[]={4,5,6,7,8,9};
 static const char *titles[]={"Words","Seed - Word numbers","Cards","Number bases","BitBox Dice","D++ D8/D16"};
 static const char *markers[]={"Words accepts typed BIP39 English words","Seed accepts word-list numbers","direct rank phases A-8, A-4, and A-2","base 4, 8, 32, or 64","Five D4 outcomes and one coin outcome","18 words: D16 then D8"};
 for(unsigned i=0;i<6;i++){
  lv_obj_clean(lv_screen_active());gui_create(request);click(route_names[i]);assert(nav_route==route_ids[i]);education_public_fixture(dev,route_ids[i]);
  education_state before,after;education_snapshot(&before);education_open(dev);education_fixed_geometry();
  assert(!strcmp(lv_label_get_text(mn_about_label),titles[i]));
  education_copy_glyphs();education_flow_layout();
  assert(education_tree_contains(mn_about_content,"What it does"));assert(education_tree_contains(mn_about_content,"When to choose it"));
  assert(education_tree_contains(mn_about_content,"What you need"));assert(education_tree_contains(mn_about_content,"Steps"));
  assert(education_tree_contains(mn_about_content,"Where unpredictability comes from"));assert(education_tree_contains(mn_about_content,"What adds no entropy"));
  assert(education_tree_contains(mn_about_content,"Accepted input, discards, and limits"));assert(education_tree_contains(mn_about_content,"PUBLIC PRACTICE - NEVER FUND"));
  assert(education_tree_contains(mn_about_content,"Pitfalls"));assert(education_tree_contains(mn_about_content,"No strength promise"));
  assert(education_tree_contains(mn_about_content,markers[i]));assert(!education_tree_contains(mn_about_content,"PRIVATE_FIXTURE_7e91"));
  if(route_ids[i]==4)assert(!education_tree_contains(mn_about_content,"type numbers on the Seed keypad"));
  if(route_ids[i]==9){
   assert(education_tree_contains(mn_about_content,"keys 1-8"));
   assert(education_tree_contains(mn_about_content,"For 18 words, enter the actual D16 face, then the actual D8 face"));
   assert(education_tree_contains(mn_about_content,"D8 faces 1 through 4 become bit 0; faces 5 through 8 become bit 1"));
   assert(education_tree_contains(mn_about_content,"The final D8 has no discarded faces"));
  }
  if(route_ids[i]==6){assert(education_tree_contains(mn_about_content,"cannot test fairness"));assert(!education_tree_contains(mn_about_content,"7 riffle"));}
  if(route_ids[i]==8){
   assert(education_tree_contains(mn_about_content,"only if that choice comes from an independent unpredictable process"));
   assert(education_tree_contains(mn_about_content,"Five D4 outcomes made with D6 rejection"));
   assert(education_tree_contains(mn_about_content,"Then use an independent coin-like sixth outcome"));
   assert(!education_tree_contains(mn_about_content,"Five-sided attempts"));
  }
  char name[96];snprintf(name,sizeof name,"education-%u-top.ppm",route_ids[i]);render(name);assert(lv_obj_get_scroll_y(mn_about_scroll)==0);
  education_drag(dev,540,180);assert(lv_obj_get_scroll_y(mn_about_scroll)>0);int maximum=lv_obj_get_scroll_y(mn_about_scroll)+lv_obj_get_scroll_bottom(mn_about_scroll);
  lv_obj_scroll_to_y(mn_about_scroll,maximum/2,LV_ANIM_OFF);lv_obj_update_layout(lv_screen_active());education_fixed_geometry();
  int middle=lv_obj_get_scroll_y(mn_about_scroll);assert(middle>0);education_drag(dev,500,650);assert(mn_about_open); /* release over Back is consumed */
  snprintf(name,sizeof name,"education-%u-middle.ppm",route_ids[i]);render(name);
  for(unsigned drag=0;drag<256&&lv_obj_get_scroll_bottom(mn_about_scroll)>0;drag++)education_drag(dev,560,120);
  int bottom=lv_obj_get_scroll_bottom(mn_about_scroll);if(bottom<0){lv_obj_scroll_to_y(mn_about_scroll,lv_obj_get_scroll_y(mn_about_scroll)+bottom,LV_ANIM_OFF);lv_obj_update_layout(lv_screen_active());}
  fprintf(stderr,"EDUCATION_BOTTOM route=%u y=%d remaining=%d\n",route_ids[i],(int)lv_obj_get_scroll_y(mn_about_scroll),(int)lv_obj_get_scroll_bottom(mn_about_scroll));
  assert(lv_obj_get_scroll_bottom(mn_about_scroll)==0);education_fixed_geometry();assert(education_tree_contains(mn_about_content,"No strength promise"));
  snprintf(name,sizeof name,"education-%u-bottom.ppm",route_ids[i]);render(name);
  modal_touch(dev,mn_about_back,"education Back bottom");assert(mn_safety_open&&!mn_about_open);education_snapshot(&after);education_state_equal(&before,&after);
  modal_touch(dev,mn_about,"education reopen middle");lv_obj_scroll_to_y(mn_about_scroll,middle,LV_ANIM_OFF);modal_touch(dev,mn_safety_close,"education Close middle");assert(!mn_safety_open);education_snapshot(&after);education_state_equal(&before,&after);
  education_open(dev);modal_touch(dev,mn_safety_close,"education Close top");assert(!mn_safety_open);education_snapshot(&after);education_state_equal(&before,&after);
  education_open(dev);lv_obj_scroll_to_y(mn_about_scroll,100000,LV_ANIM_OFF);modal_touch(dev,mn_safety_close,"education Close bottom");assert(!mn_safety_open);education_snapshot(&after);education_state_equal(&before,&after);
  fprintf(stderr,"EDUCATION_ROUTE route=%u title='%s' top=pass middle=pass bottom=pass fixed=pass state=pass clickthrough=pass\n",route_ids[i],titles[i]);
 }
 puts("PASS six typed-route education documents and native pointer/state matrix");
}
