#include "gui.h"
#include "lvgl.h"
#include "dice_core.h"
#include "gui08_native.h"
#include "seed_native.h"
#include "extra_dice_native.h"
#include <stdio.h>
#include <string.h>

#define SAFETY_DOCK_TEXT "Public TEST input only / NEVER fund.\nSafety / About >"

/* Source tokens: EntropyLab 6e1f39c src/css/styles.css. Fixed portrait layout.
 * Public-test hex entry, bounded request copies, no secret-erasure guarantees.
 * All calls originate in the LVGL task/lock; the worker owns computation. */
LV_FONT_DECLARE(el_sans_16);
LV_FONT_DECLARE(el_sans_14);
LV_FONT_DECLARE(el_mono_16);
LV_FONT_DECLARE(el_mono_20);
LV_FONT_DECLARE(el_serif_24);
static lv_style_t base, card, well, readout, control, selected, disabled, tab, d6_track_style;
static lv_obj_t *input, *output, *tabs[2], *run, *status, *note, *words[24], *fp, *addr, *count, *hexlabel, *edits[20];
#define D6_POOL_CELLS 32
static lv_obj_t *d6_viewport,*d6_scroll_track,*d6_spacer,*d6_cells[D6_POOL_CELLS],*d6_indices[D6_POOL_CELLS],*d6_faces[D6_POOL_CELLS],*d6_replace[6];
static size_t d6_cell_index[D6_POOL_CELLS];
static bool d6_rebinding;
static size_t d6_selection[GUI_MODE_COUNT];
static int32_t d6_scroll_y[GUI_MODE_COUNT];
static el_mode_t d6_rendered_mode=MODE_COUNT;
static size_t d6_rendered_length,d6_rendered_selection;
static bool d6_rendered_busy;
static bool (*request_hex)(const hex_request_t *);
static hex_request_t snapshot(const char *input_text,size_t n,uint32_t input_mode,uint32_t words_count){
    hex_request_t request={0};
    memcpy(request.hex,input_text,n);request.length=n;request.mode=input_mode;request.words=words_count;
    return request;
}
static char transcripts[GUI_MODE_COUNT][1025];
static size_t lengths[GUI_MODE_COUNT];
static el_mode_t mode=MODE_HEX;
static unsigned selector=12;
#define hex transcripts[mode]
#define length lengths[mode]
static lv_obj_t *back, *keypad, *coinpad, *dicepad, *dicekeys[6], *dicechoices[5], *dicemethod, *modes[GUI_MODE_COUNT], *choices[5], *flips[2];
static bool busy, valid_result;
static lv_obj_t *header_title,*header_meta;
static lv_obj_t *gui08_routes[4];
static gui08_native gui08_ui;
static seed_native seed_ui;
static extra_dice_native extra_dice_ui;
static bool (*request_gui08)(const gui08_request *);
static gui08_native_cancel_fn cancel_gui08;
static uint64_t legacy_id,legacy_pending,legacy_revision;
static void refresh(void);
static bool nav_open;
static bool nav_busy(void);
static bool nav_context_guard(lv_event_t *e);
static void nav_bind(void);
static void nav_refresh(void);
static const char *active_about_name(void);
static const char *active_about_copy(void);
static void style_surface(lv_style_t *s, uint32_t color, int radius, bool border) {
    lv_style_init(s);
    lv_style_set_bg_color(s,lv_color_hex(color));
    lv_style_set_bg_opa(s,LV_OPA_COVER);
    lv_style_set_radius(s,radius);
    lv_style_set_border_width(s,border?1:0);
    lv_style_set_border_color(s,lv_color_hex(0x333333));
    lv_style_set_pad_all(s,0);
    lv_style_set_shadow_width(s,0);
    lv_style_set_text_color(s,lv_color_hex(0xeeeeee));
    lv_style_set_text_font(s,&el_sans_16);
}
static lv_obj_t *box(lv_obj_t *p,int x,int y,int w,int h,lv_style_t *s) {
    lv_obj_t *o=lv_obj_create(p);lv_obj_remove_style_all(o);lv_obj_add_style(o,s,0);
    lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);
    lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE);return o;
}
static lv_obj_t *text(lv_obj_t *p,int x,int y,int w,const char *t,const lv_font_t *font,uint32_t color) {
    lv_obj_t *o=lv_label_create(p);lv_obj_set_pos(o,x,y);lv_obj_set_width(o,w);
    lv_obj_set_style_text_font(o,font,0);lv_obj_set_style_text_color(o,lv_color_hex(color),0);
    lv_label_set_long_mode(o,LV_LABEL_LONG_WRAP);lv_label_set_text(o,t);return o;
}
static lv_obj_t *button(lv_obj_t *p,int x,int y,int w,const char *s,lv_event_cb_t cb,void *data) {
    lv_obj_t *o=lv_button_create(p);lv_obj_remove_style_all(o);
    lv_obj_add_style(o,&control,0);lv_obj_add_style(o,&selected,LV_STATE_PRESSED);
    lv_obj_add_style(o,&selected,LV_STATE_CHECKED);lv_obj_add_style(o,&disabled,LV_STATE_DISABLED);
    lv_obj_set_style_outline_color(o,lv_color_hex(0xff9900),LV_STATE_FOCUS_KEY);
    lv_obj_set_style_outline_width(o,2,LV_STATE_FOCUS_KEY);
    lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,44);lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *l=lv_label_create(o);lv_label_set_text(l,s);lv_obj_center(l);
    if(cb)lv_obj_add_event_cb(o,cb,LV_EVENT_CLICKED,data);
    return o;
}
/* Volatile byte stores are observable C side effects: known owned storage only.
 * This does not cover prior allocator, compiler, LVGL or crypto copies. */
static void gui_owned_zero(void *storage,size_t n){
    volatile unsigned char *p=storage;
    while(n--)*p++=0;
}
/* These labels use lv_label_set_text (owned, writable LVGL text), never static.
 * Clear their current text before the library frees/replaces that allocation. */
static void gui_label_reset(lv_obj_t *label,const char *replacement){
    char *old=lv_label_get_text(label);
    if(old)gui_owned_zero(old,strlen(old));
    lv_label_set_text(label,replacement);
}
static void saver_activity(void);
static void saver_rebind_controls(void);
static bool saver_blocked(void);
static void pi_enter(void);
static void pi_source_changed(void);
static bool pi_enabled(void);
static bool feature_source_matches(const gui08_request *request);
static bool feature_ready(gui08_method method);
#include "education_content.inc"
#include "mnemonic_gui.inc"
#include "saver.inc"
#undef hex
#undef length
#include "passphrase_integration.inc"
static bool feature_source_matches(const gui08_request *request){
 if(request&&request->method==GUI08_SEED)return seed_native_source_matches(&seed_ui,request);
 if(request&&request->method==GUI08_DICE)return extra_dice_native_source_matches(&extra_dice_ui,request);
 const gui08_editor *e=&gui08_ui.editor;
 return request&&request->request_id&&request->revision==e->revision&&
        request->method==e->method&&request->words==e->words&&request->base==e->base&&
        request->length==e->length&&!memcmp(request->transcript,e->transcript,e->length+1);
}
static bool feature_ready(gui08_method method){return method==GUI08_SEED?seed_native_ready(&seed_ui):method==GUI08_DICE?extra_dice_native_ready(&extra_dice_ui):gui08_ready(&gui08_ui.editor);}
#define hex transcripts[mode]
#define length lengths[mode]
static bool pi_enabled(void){return pi_lock!=NULL;}
static bool gui08_guard(lv_event_t *e){(void)e;return saver_blocked()||mn_safety_open||nav_open||nav_context_guard(e);}
static void gui08_route(lv_event_t *e){
    if(gui08_guard(e)||busy||mn_confirm)return;
    gui08_method next=lv_event_get_target(e)==gui08_routes[0]?GUI08_CARDS:
                      lv_event_get_target(e)==gui08_routes[1]?GUI08_BASES:
                      lv_event_get_target(e)==gui08_routes[2]?GUI08_SEED:GUI08_DICE;
    lv_obj_add_flag(legacy_panel,LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(mn_panel,LV_OBJ_FLAG_HIDDEN);
    for(unsigned i=0;i<GUI_MODE_COUNT;i++)lv_obj_remove_state(modes[i],LV_STATE_CHECKED);
    for(unsigned i=0;i<4;i++){
        if(i+1u==(unsigned)next)lv_obj_add_state(gui08_routes[i],LV_STATE_CHECKED);
        else lv_obj_remove_state(gui08_routes[i],LV_STATE_CHECKED);
    }
    gui08_native_hide(&gui08_ui);seed_native_hide(&seed_ui);extra_dice_native_hide(&extra_dice_ui);
    if(next==GUI08_SEED)seed_native_show(&seed_ui);
    else if(next==GUI08_DICE)extra_dice_native_show(&extra_dice_ui);
    else gui08_native_show(&gui08_ui,next);
}
void gui_cards_bases_configure(bool (*submit)(const gui08_request *),bool (*cancel)(uint64_t)){request_gui08=submit;cancel_gui08=cancel;gui08_ui.cancel=cancel;seed_ui.cancel=cancel;extra_dice_ui.cancel=cancel;}
bool gui_cards_bases_pending(const gui08_request *context){return context&&context->method==GUI08_SEED?seed_native_pending(&seed_ui,context):context&&context->method==GUI08_DICE?extra_dice_native_pending(&extra_dice_ui,context):gui08_native_pending(&gui08_ui,context);}
bool gui_cards_bases_result(const gui08_request *context){return context&&context->method==GUI08_SEED?seed_native_accept(&seed_ui,context):context&&context->method==GUI08_DICE?extra_dice_native_accept(&extra_dice_ui,context):gui08_native_accept(&gui08_ui,context);}
bool gui_cards_bases_cancelled(uint64_t request_id){return seed_ui.editor.pending_id==request_id?seed_native_cancelled(&seed_ui,request_id):extra_dice_ui.editor.pending_id==request_id?extra_dice_native_cancelled(&extra_dice_ui,request_id):gui08_native_cancelled(&gui08_ui,request_id);}
bool gui_cards_bases_lifehash(const gui08_request *context,const char fingerprint[9],
                              const uint8_t *rgb,size_t rgb_size){
    return context&&context->method==GUI08_SEED?
        seed_native_lifehash(&seed_ui,context,fingerprint,rgb,rgb_size):
        context&&context->method==GUI08_DICE?
        extra_dice_native_lifehash(&extra_dice_ui,context,fingerprint,rgb,rgb_size):
        gui08_native_lifehash(&gui08_ui,context,fingerprint,rgb,rgb_size);
}
void gui_cards_bases_lifehash_blank(void){
    gui08_native_lifehash_blank(&gui08_ui);
    seed_native_lifehash_blank(&seed_ui);
    extra_dice_native_lifehash_blank(&extra_dice_ui);
}
static void show(bool results) {
    if(results){lv_obj_add_flag(input,LV_OBJ_FLAG_HIDDEN);lv_obj_remove_flag(output,LV_OBJ_FLAG_HIDDEN);}
    else {lv_obj_remove_flag(input,LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(output,LV_OBJ_FLAG_HIDDEN);}
    lv_obj_remove_state(tabs[!results],LV_STATE_CHECKED);lv_obj_add_state(tabs[results],LV_STATE_CHECKED);
}
static bool allowed(void) {if(!el_mode_is_legacy(mode))return false;return el_mode_is_dice(mode)?(length>0 && length<=1024):mode!=MODE_HEX?length==selector*32/3:(length>=32 && length<=64 && length%8==0);}
static void invalidate(void) {
    valid_result=false;legacy_pending=0;legacy_revision++;
    for(int i=0;i<24;i++)gui_label_reset(words[i],"--");
    gui_label_reset(fp,"--------");gui_label_reset(addr,"Not calculated");
    lv_label_set_text(note,el_mode_is_dice(mode)&&length&&length<(size_t)el_dice_required_rolls(selector)?"WEAK_INPUT_LAB_ONLY / No result\nPublic TEST input only / NEVER fund.":"No result. Enter public TEST INPUT only.\nEnglish / empty passphrase / NEVER fund.");
}
static bool d6_mode(void){return mode==MODE_D6_RAW||mode==MODE_D6_COLEMAN;}
static void d6_count_refresh(void){
    if(!d6_mode())return;
    char c[64];size_t selected_index=d6_selection[mode];
    if(selected_index&&selected_index<=length)snprintf(c,sizeof c,"%u/%u selected | %d required | max 1024",(unsigned)selected_index,(unsigned)length,(int)el_dice_required_rolls(selector));
    else snprintf(c,sizeof c,"%u rolls | %d required | max 1024",(unsigned)length,(int)el_dice_required_rolls(selector));
    lv_label_set_text(count,c);
}
static lv_obj_t *d6_cell_for_index(size_t selected_index){
    for(size_t slot=0;slot<D6_POOL_CELLS;slot++)if(d6_cell_index[slot]==selected_index)return d6_cells[slot];
    return NULL;
}
static void d6_render_cell(size_t slot){
    lv_obj_t *cell=d6_cells[slot];size_t selected_index=d6_cell_index[slot];
    if(!selected_index||selected_index>length){lv_obj_add_flag(cell,LV_OBJ_FLAG_HIDDEN);return;}
    size_t i=selected_index-1;
    lv_obj_remove_flag(cell,LV_OBJ_FLAG_HIDDEN);
    char index_text[11],face_text[2]={(char)hex[i],0};snprintf(index_text,sizeof index_text,"%u",(unsigned)selected_index);
    lv_label_set_text(d6_indices[slot],index_text);lv_label_set_text(d6_faces[slot],face_text);
    bool chosen=d6_selection[mode]==selected_index;
    if(chosen)lv_obj_add_state(cell,LV_STATE_CHECKED);else lv_obj_remove_state(cell,LV_STATE_CHECKED);
    uint32_t color=busy?0x737373:chosen?0x000000:0xa3a3a3;
    lv_obj_set_style_text_color(d6_indices[slot],lv_color_hex(color),0);
    lv_obj_set_style_text_color(d6_faces[slot],lv_color_hex(busy?0x737373:chosen?0x000000:0xeeeeee),0);
    if(busy)lv_obj_add_state(cell,LV_STATE_DISABLED);else lv_obj_remove_state(cell,LV_STATE_DISABLED);
}
static void d6_pool_rebind(bool bind_events){
    if(!d6_mode()||d6_rebinding)return;
    d6_rebinding=true;
    int32_t scroll=lv_obj_get_scroll_y(d6_viewport);if(scroll<0)scroll=0;
    size_t first_row=(size_t)scroll/48;if(first_row)first_row--;
    for(size_t slot=0;slot<D6_POOL_CELLS;slot++){
        size_t logical=first_row*8+slot+1;d6_cell_index[slot]=logical<=length?logical:0;
        if(d6_cell_index[slot]){
            size_t i=logical-1;lv_obj_set_pos(d6_cells[slot],9+(int)(i%8)*46,7+(int)(i/8)*48);
        }
        d6_render_cell(slot);
    }
    d6_rebinding=false;if(bind_events)saver_rebind_controls();
}
static void d6_review_refresh(void){
    bool active=d6_mode();
    if(!active){
        lv_obj_remove_flag(hexlabel,LV_OBJ_FLAG_HIDDEN);lv_obj_remove_flag(d6_viewport,LV_OBJ_FLAG_SCROLLABLE);
        for(size_t i=0;i<D6_POOL_CELLS;i++){d6_cell_index[i]=0;lv_obj_add_flag(d6_cells[i],LV_OBJ_FLAG_HIDDEN);}
        lv_obj_add_flag(d6_spacer,LV_OBJ_FLAG_HIDDEN);d6_rendered_mode=MODE_COUNT;d6_rendered_length=0;d6_rendered_selection=0;return;
    }
    lv_obj_add_flag(hexlabel,LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(d6_viewport,LV_OBJ_FLAG_SCROLLABLE);lv_obj_set_scroll_dir(d6_viewport,LV_DIR_VER);
    if(d6_selection[mode]>length)d6_selection[mode]=length;
    size_t rows=(length+7)/8;
    lv_obj_remove_flag(d6_spacer,LV_OBJ_FLAG_HIDDEN);lv_obj_set_y(d6_spacer,(int32_t)(rows?rows*48+11:0));
    d6_pool_rebind(false);
    d6_rendered_mode=mode;d6_rendered_length=length;d6_rendered_selection=d6_selection[mode];d6_rendered_busy=busy;
    for(int i=0;i<6;i++){
        if(busy||!d6_selection[mode]||d6_selection[mode]>length)lv_obj_add_state(d6_replace[i],LV_STATE_DISABLED);
        else lv_obj_remove_state(d6_replace[i],LV_STATE_DISABLED);
    }
    d6_count_refresh();
}
static void d6_make_visible(size_t selected_index){
    if(selected_index&&selected_index<=length){
        int32_t top=8+(int32_t)((selected_index-1)/8)*48,scroll=lv_obj_get_scroll_y(d6_viewport);
        if(top<scroll)scroll=top;else if(top+44>scroll+126)scroll=top+44-126;
        lv_obj_scroll_to_y(d6_viewport,scroll,LV_ANIM_OFF);d6_pool_rebind(true);d6_scroll_y[mode]=lv_obj_get_scroll_y(d6_viewport);
    }
}
static void d6_select_roll(lv_event_t *e){
    if(saver_event_blocked(e)||busy||mn_confirm||!d6_mode())return;
    size_t selected_index=saver_payload(e)+1;
    if(selected_index>length)return;
    d6_selection[mode]=selected_index;d6_review_refresh();d6_make_visible(selected_index);
}
static void d6_replace_roll(lv_event_t *e){
    if(saver_event_blocked(e)||busy||mn_confirm||!d6_mode())return;
    size_t selected_index=d6_selection[mode];unsigned face=(unsigned)saver_payload(e);
    if(!selected_index||selected_index>length||length>1024||face<1||face>6)return;
    char replacement=(char)('0'+face);if(hex[selected_index-1]==replacement)return;
    int32_t scroll=lv_obj_get_scroll_y(d6_viewport);
    pi_source_changed();hex[selected_index-1]=replacement;d6_rendered_mode=MODE_COUNT;invalidate();refresh();show(false);
    lv_obj_scroll_to_y(d6_viewport,scroll,LV_ANIM_OFF);d6_scroll_y[mode]=lv_obj_get_scroll_y(d6_viewport);
}
static void d6_scroll_event(lv_event_t *e){
    if(!d6_mode())return;
    lv_event_code_t code=lv_event_get_code(e);
    if((busy||mn_confirm||mn_safety_open||nav_open||saver_blocked())&&code==LV_EVENT_SCROLL_BEGIN){lv_obj_scroll_to_y(d6_viewport,d6_scroll_y[mode],LV_ANIM_OFF);return;}
    saver_activity();
    if(code==LV_EVENT_SCROLL)d6_pool_rebind(true);
    if(code==LV_EVENT_SCROLL_END)d6_scroll_y[mode]=lv_obj_get_scroll_y(d6_viewport);
}
static void refresh(void) {
    nav_refresh();
    mn_enable(mn_safety,!busy&&!mn_confirm&&!mn_safety_open);
    char c[64];snprintf(c,sizeof c,"%u / 64 chars  |  %u bits",(unsigned)length,(unsigned)length*4);
    if(mode==MODE_COINS)snprintf(c,sizeof c,"%u / %u encoded bits",(unsigned)length,selector*32/3);
    if(el_mode_is_dice(mode))snprintf(c,sizeof c,"%u / %d nominal rolls (max 1024)",(unsigned)length,(int)el_dice_required_rolls(selector));
    lv_label_set_text(count,c);
    /* Coin preview is explicitly the latest 64 bits; full owned transcript retained. */
    if(!d6_mode())gui_label_reset(hexlabel,length?(mode!=MODE_HEX&&length>64?hex+length-64:hex):(mode!=MODE_HEX?"Heads=0 / Tails=1. Raw bits, MSB first.":"Tap 0-F to enter public test hex"));
    d6_review_refresh();
    lv_label_set_text(lv_obj_get_child(edits[16],0),mode!=MODE_HEX?"Undo":"Delete");
    lv_label_set_text(lv_obj_get_child(tabs[0],0),el_mode_is_dice(mode)?"Dice input":mode!=MODE_HEX?"Coin input":"Hex input");
    lv_label_set_text(lv_obj_get_child(back,0),el_mode_is_dice(mode)?"Back to dice input":mode!=MODE_HEX?"Back to coin input":"Back to hex input");
    lv_obj_add_flag(dicepad,LV_OBJ_FLAG_HIDDEN);
    if(el_mode_is_dice(mode)){lv_obj_add_flag(keypad,LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(coinpad,LV_OBJ_FLAG_HIDDEN);lv_obj_remove_flag(dicepad,LV_OBJ_FLAG_HIDDEN);}
    else if(mode==MODE_COINS){lv_obj_add_flag(keypad,LV_OBJ_FLAG_HIDDEN);lv_obj_remove_flag(coinpad,LV_OBJ_FLAG_HIDDEN);}
    else {lv_obj_remove_flag(keypad,LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(coinpad,LV_OBJ_FLAG_HIDDEN);}
    for(int i=0;i<2;i++){
        if(busy)lv_obj_add_state(tabs[i],LV_STATE_DISABLED);
        else lv_obj_remove_state(tabs[i],LV_STATE_DISABLED);
    }
    for(int i=0;i<GUI_MODE_COUNT;i++){
        if(busy){lv_obj_add_state(modes[i],LV_STATE_DISABLED);if(i<2)lv_obj_add_state(flips[i],LV_STATE_DISABLED);}
        else {lv_obj_remove_state(modes[i],LV_STATE_DISABLED);if(i<2)lv_obj_remove_state(flips[i],LV_STATE_DISABLED);}
        if(mode==(unsigned)i)lv_obj_add_state(modes[i],LV_STATE_CHECKED);else lv_obj_remove_state(modes[i],LV_STATE_CHECKED);
    }
    /* Feature routes share the same top-level navigation and must expose the
     * legacy busy guard visually, not merely ignore their click callbacks. */
    for(int i=0;i<4;i++){
        if(busy)lv_obj_add_state(gui08_routes[i],LV_STATE_DISABLED);
        else lv_obj_remove_state(gui08_routes[i],LV_STATE_DISABLED);
    }
    for(int i=0;i<5;i++){
        if(busy)lv_obj_add_state(choices[i],LV_STATE_DISABLED);else lv_obj_remove_state(choices[i],LV_STATE_DISABLED);
        if(selector==(unsigned)(12+3*i))lv_obj_add_state(choices[i],LV_STATE_CHECKED);else lv_obj_remove_state(choices[i],LV_STATE_CHECKED);
    }
    for(int i=0;i<6;i++){if(busy||(d6_mode()&&length>=1024))lv_obj_add_state(dicekeys[i],LV_STATE_DISABLED);else lv_obj_remove_state(dicekeys[i],LV_STATE_DISABLED);}
    for(int i=0;i<5;i++){if(busy)lv_obj_add_state(dicechoices[i],LV_STATE_DISABLED);else lv_obj_remove_state(dicechoices[i],LV_STATE_DISABLED);if(selector==(unsigned)(12+3*i))lv_obj_add_state(dicechoices[i],LV_STATE_CHECKED);else lv_obj_remove_state(dicechoices[i],LV_STATE_CHECKED);}
    if(el_mode_is_dice(mode))lv_obj_add_flag(edits[18],LV_OBJ_FLAG_HIDDEN);else lv_obj_remove_flag(edits[18],LV_OBJ_FLAG_HIDDEN);
    for(int i=0;i<20;i++) {if(busy||((i==16||i==17||i==19)&&!length))lv_obj_add_state(edits[i],LV_STATE_DISABLED);else lv_obj_remove_state(edits[i],LV_STATE_DISABLED);}
    if(busy||!allowed())lv_obj_add_state(run,LV_STATE_DISABLED);else lv_obj_remove_state(run,LV_STATE_DISABLED);
    lv_label_set_text(dicemethod,mode==MODE_D6_COLEMAN?"Coleman: 6->0 before SHA256":"D6 raw COLDCARD-style SHA256");
    lv_label_set_text(status,busy?(el_mode_is_dice(mode)&&length<(size_t)el_dice_required_rolls(selector)?"WEAK_INPUT_LAB_ONLY / Calculating...":"Calculating outside the display lock..."):el_mode_is_dice(mode)?(length==0?"Empty input: enter at least one roll.":length<(size_t)el_dice_required_rolls(selector)?"WEAK_INPUT_LAB_ONLY":"Count only - NOT randomness quality."):allowed()?"Valid length. English / empty passphrase.":(mode!=MODE_HEX?"Exact selected bit count required. No truncation.":"Requires 32, 40, 48, 56 or 64 hex characters."));
}
static void switch_mode(lv_event_t *e){if(saver_event_blocked(e))return;if(busy||mn_confirm||mn_safety_open)return;uint32_t next=saver_payload(e);if(next>=MODE_COUNT)return;if(d6_mode())d6_scroll_y[mode]=lv_obj_get_scroll_y(d6_viewport);gui08_native_hide(&gui08_ui);seed_native_hide(&seed_ui);extra_dice_native_hide(&extra_dice_ui);for(unsigned i=0;i<4;i++)lv_obj_remove_state(gui08_routes[i],LV_STATE_CHECKED);if(mode!=(el_mode_t)next)pi_source_changed();mode=(el_mode_t)next;mn_reset_draft();mn_confirm=0;mn_invalidate();mn_refresh();if(mode==MODE_MNEMONIC){lv_obj_add_flag(legacy_panel,LV_OBJ_FLAG_HIDDEN);lv_obj_remove_flag(mn_panel,LV_OBJ_FLAG_HIDDEN);mn_refresh();return;}lv_obj_add_flag(mn_panel,LV_OBJ_FLAG_HIDDEN);lv_obj_remove_flag(legacy_panel,LV_OBJ_FLAG_HIDDEN);invalidate();refresh();show(false);if(d6_mode()){lv_obj_scroll_to_y(d6_viewport,d6_scroll_y[mode],LV_ANIM_OFF);d6_pool_rebind(true);d6_make_visible(d6_selection[mode]);}}
static void select_words(lv_event_t *e){if(saver_event_blocked(e))return;if(busy||!el_mode_is_legacy(mode))return;if(selector==saver_payload(e))return;pi_source_changed();selector=saver_payload(e);invalidate();refresh();show(false);}
static void flip(lv_event_t *e){if(saver_event_blocked(e))return;if(busy||mode!=MODE_COINS||length>=256)return;pi_source_changed();hex[length++]=saver_payload(e)?'1':'0';hex[length]=0;invalidate();refresh();show(false);}
static void roll(lv_event_t *e){if(saver_event_blocked(e))return;if(busy||!d6_mode()||length>=1024)return;pi_source_changed();hex[length++]='0'+saver_payload(e);hex[length]=0;d6_selection[mode]=length;invalidate();refresh();show(false);d6_make_visible(length);}
static void navigate(lv_event_t *e){if(saver_event_blocked(e))return;if(busy||!el_mode_is_legacy(mode))return;bool results=saver_payload(e)!=0;show(results);if(!results&&d6_mode())d6_make_visible(d6_selection[mode]);}
static void edit(lv_event_t *e) {if(saver_event_blocked(e))return;
    if(busy||!el_mode_is_legacy(mode))return;
    uintptr_t k=saver_payload(e);
    if((k==16||k==17)&&!length)return;
    if(k==18){size_t target=mode==MODE_HEX?64:selector*32/3;bool same=length==target;for(size_t i=0;same&&i<length;i++)if(hex[i]!='0')same=false;if(same)return;}
    if(k<16){if(mode!=MODE_HEX||length==64)return;pi_source_changed();hex[length++]="0123456789ABCDEF"[k];hex[length]=0;}
    else if(k==16){pi_source_changed();if(length)hex[--length]=0;if(d6_mode()){if(!length)d6_selection[mode]=0;else if(d6_selection[mode]>length)d6_selection[mode]=length;}}
    else {if(k==18&&el_mode_is_dice(mode))return;pi_source_changed();gui_owned_zero(hex,sizeof hex);length=0;if(d6_mode()){d6_selection[mode]=0;d6_scroll_y[mode]=0;lv_obj_scroll_to_y(d6_viewport,0,LV_ANIM_OFF);}if(k==18){length=mode!=MODE_HEX?selector*32/3:64;memset(hex,'0',length);}}
    invalidate();refresh();show(false);
}
static void calculate(lv_event_t *e){if(saver_event_blocked(e))return;
    (void)e;if(busy||!allowed())return;
    if(pi_enabled()){pi_enter();return;}
    hex_request_t r=snapshot(hex,length,mode,mode!=MODE_HEX?selector:(uint32_t)(length*3/8));
    invalidate();r.request_id=++legacy_id;r.revision=legacy_revision;legacy_pending=r.request_id;busy=true;refresh();
    if(!request_hex || !request_hex(&r)){busy=false;refresh();lv_label_set_text(status,el_mode_is_dice(mode)&&length<(size_t)el_dice_required_rolls(selector)?"WEAK_INPUT_LAB_ONLY / Worker unavailable":"Worker unavailable. Retry calculation.");}
    gui_owned_zero(&r,sizeof r);
}
static void saver_bind_legacy(lv_obj_t *obj,lv_event_cb_t cb,unsigned action){
 lv_obj_remove_event_cb(obj,cb);
 if(!saver_epoch_exhausted)lv_obj_add_event_cb(obj,cb,LV_EVENT_CLICKED,(void*)((saver_epoch<<10)|action));
}
static void saver_rebind_controls(void){
 if(saver_epoch==UINTPTR_MAX>>10)saver_epoch_exhausted=true;else saver_epoch++;
 for(unsigned i=0;i<GUI_MODE_COUNT;i++)saver_bind_legacy(modes[i],switch_mode,i);
 for(unsigned i=0;i<5;i++){saver_bind_legacy(choices[i],select_words,12+3*i);saver_bind_legacy(dicechoices[i],select_words,12+3*i);}
 for(unsigned i=0;i<2;i++){saver_bind_legacy(flips[i],flip,i);saver_bind_legacy(tabs[i],navigate,i);}
 for(unsigned i=0;i<6;i++){saver_bind_legacy(dicekeys[i],roll,i+1);saver_bind_legacy(d6_replace[i],d6_replace_roll,i+1);}
 for(unsigned i=0;i<D6_POOL_CELLS;i++)saver_bind_legacy(d6_cells[i],d6_select_roll,d6_cell_index[i]?d6_cell_index[i]-1:1023);
 for(unsigned i=0;i<20;i++)saver_bind_legacy(edits[i],edit,i==19?17:i);
 saver_bind_legacy(run,calculate,0);saver_bind_legacy(back,navigate,0);nav_bind();
}
#undef length
#undef hex
#include "navigation.inc"
/* Explicit visible-owner allowlist: mode remains a legacy value when native
 * panels are selected, so it cannot identify the foreground view. New routes
 * idle only at empty input; no concealment of pending/secret/result flows. */
static bool saver_view_eligible(void){
 if(nav_open||mn_safety_open||nav_busy()||legacy_pending||pi_ui.live)return false;
 unsigned visible=!lv_obj_has_flag(legacy_panel,LV_OBJ_FLAG_HIDDEN)+
                  !lv_obj_has_flag(mn_panel,LV_OBJ_FLAG_HIDDEN)+
                  gui08_native_visible(&gui08_ui)+seed_native_visible(&seed_ui)+
                  extra_dice_native_visible(&extra_dice_ui);
 if(visible!=1)return false; /* unknown or overlapping owner: fail closed */
 if(gui08_native_visible(&gui08_ui))
  return (gui08_ui.editor.method==GUI08_CARDS||gui08_ui.editor.method==GUI08_BASES)&&
         !gui08_ui.editor.length&&!gui08_ui.editor.result_visible;
 if(seed_native_visible(&seed_ui))
  return seed_ui.editor.method==GUI08_SEED&&!seed_ui.editor.length&&
         !seed_ui.editor.result_visible&&!seed_ui.committed&&!seed_ui.draft_len&&
         !seed_ui.candidate_count;
 if(extra_dice_native_visible(&extra_dice_ui))
  return extra_dice_ui.editor.method==GUI08_DICE&&
         (extra_dice_ui.method==GUI08_DICE_BITBOX||extra_dice_ui.method==GUI08_DICE_DPLUS)&&
         !extra_dice_ui.editor.length&&!extra_dice_ui.editor.result_visible&&
         !extra_dice_ui.final_length&&!extra_dice_ui.final_selected;
 if(!lv_obj_has_flag(mn_panel,LV_OBJ_FLAG_HIDDEN))
  return mode==MODE_MNEMONIC&&!mn_reviewing&&!mn_valid&&!mn_draft[0]&&
         mn_edit<0&&!mn_settling&&lv_obj_has_flag(mn_output,LV_OBJ_FLAG_HIDDEN);
 return el_mode_is_legacy(mode)&&!lengths[mode]&&!valid_result&&
        !d6_selection[mode]&&!d6_rebinding&&!lv_obj_is_scrolling(d6_viewport)&&
        !lv_obj_has_flag(input,LV_OBJ_FLAG_HIDDEN)&&lv_obj_has_flag(output,LV_OBJ_FLAG_HIDDEN);
}
#define hex transcripts[mode]
#define length lengths[mode]
void gui_result(const hex_result_t *r) {
    if(!r)return;
    if(mode==MODE_MNEMONIC){mn_result(r);return;}
    if(!busy || !el_mode_is_legacy(r->mode)||r->mode!=mode||r->request_id!=legacy_pending||r->revision!=legacy_revision||r->words!=(mode!=MODE_HEX?selector:length*3/8))return;
    busy=false;invalidate();refresh();
    if(r->rc<0 || r->mode!=mode || r->words!=(mode!=MODE_HEX?selector:length*3/8) || r->weak!=(el_mode_is_dice(mode)&&length<(size_t)el_dice_required_rolls(selector)) || !memchr(r->mnemonic,0,sizeof r->mnemonic) || !mn_hex(r->fingerprint,sizeof r->fingerprint,8) || !mn_address(r->address,sizeof r->address)) {
        lv_label_set_text(note,el_mode_is_dice(mode)&&length<(size_t)el_dice_required_rolls(selector)?"WEAK_INPUT_LAB_ONLY / Calculation failed\nBack to input and retry.":"Calculation failed. Back to input and retry.");show(true);return;
    }
    const char *p=r->mnemonic;unsigned nwords=0;
    while(*p){size_t n=strcspn(p," ");if(!n||n>8||nwords>=24)break;nwords++;p+=n;if(*p==' ')p++;}
    if(*p || nwords!=(mode!=MODE_HEX?selector:length*3/8)){lv_label_set_text(note,el_mode_is_dice(mode)&&length<(size_t)el_dice_required_rolls(selector)?"WEAK_INPUT_LAB_ONLY / Invalid result\nBack to input and retry.":"Invalid result length. Retry.");show(true);return;}
    p=r->mnemonic;
    for(unsigned i=0;i<nwords;i++){char w[9];size_t n=strcspn(p," ");memcpy(w,p,n);w[n]=0;lv_label_set_text(words[i],w);p+=n;if(*p==' ')p++;}
    lv_label_set_text(fp,r->fingerprint);lv_label_set_text(addr,r->address);
    valid_result=true;
    if(el_mode_is_dice(mode)){char message[128];snprintf(message,sizeof message,"%s\n%s / NEVER fund",r->mode==MODE_D6_RAW?"D6 raw COLDCARD-style":"Coleman 6->0 before SHA256",r->weak?"WEAK_INPUT_LAB_ONLY":"Count only, NOT quality proof");lv_label_set_text(note,message);}else lv_label_set_text(note,"Computed TEST output - not a vector PASS.\nEnglish / empty passphrase / NEVER fund.");show(true);
}
void gui_create(bool (*cb)(const hex_request_t *)) {
    gui_passphrase_dispose();nav_family=NULL;nav_open=false;
    if(saver_timer){lv_timer_delete(saver_timer);saver_timer=NULL;}
    request_hex=cb;memset(transcripts,0,sizeof transcripts);memset(lengths,0,sizeof lengths);memset(d6_selection,0,sizeof d6_selection);memset(d6_scroll_y,0,sizeof d6_scroll_y);d6_rendered_mode=MODE_COUNT;d6_rendered_length=0;d6_rendered_selection=0;d6_rendered_busy=false;mode=MODE_HEX;selector=12;busy=false;valid_result=false;
    style_surface(&base,0x000000,0,false);style_surface(&card,0x141414,20,true);
    style_surface(&well,0x000000,12,true);style_surface(&readout,0x202020,0,false);
    style_surface(&control,0x202020,8,true);style_surface(&selected,0xff9900,8,true);
    lv_style_set_text_color(&selected,lv_color_hex(0x000000));lv_style_set_border_color(&selected,lv_color_hex(0xff9900));
    style_surface(&disabled,0x202020,8,true);lv_style_set_text_color(&disabled,lv_color_hex(0x737373));
    style_surface(&tab,0x000000,8,true);lv_style_set_text_color(&tab,lv_color_hex(0xa3a3a3));
    style_surface(&d6_track_style,0x141414,0,false);
    lv_obj_t *s=lv_screen_active();lv_obj_remove_style_all(s);lv_obj_add_style(s,&base,0);lv_obj_remove_flag(s,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *root=s;app_content=box(root,0,0,480,800,&base);s=app_content;
    lv_obj_add_event_cb(app_content,pi_deleted,LV_EVENT_DELETE,NULL);
    lv_obj_t *header_logo=lv_image_create(s);lv_image_set_src(header_logo,&saver_logo);
    lv_obj_set_pos(header_logo,16,7);lv_obj_set_size(header_logo,20,30);lv_image_set_inner_align(header_logo,LV_IMAGE_ALIGN_CONTAIN);
    lv_obj_remove_flag(header_logo,LV_OBJ_FLAG_CLICKABLE);lv_obj_remove_flag(header_logo,LV_OBJ_FLAG_SCROLLABLE);
    header_title=text(s,44,8,232,"EntropyLab",&el_serif_24,0xeeeeee);
    lv_obj_set_height(header_title,36);
    header_meta=text(s,322,12,142,"P4 / Offline lab",&el_sans_14,0xa3a3a3);
    lv_obj_t *line=box(s,16,47,448,1,&readout);(void)line;
    lv_obj_set_height(header_meta,28);
    const char *names[GUI_MODE_COUNT]={"Hex","Coins","D6 raw","D6 6->0","Words"};
    for(int i=0;i<GUI_MODE_COUNT;i++){modes[i]=button(s,13+i*55,56,54,names[i],switch_mode,(void*)(uintptr_t)i);lv_obj_set_style_text_font(lv_obj_get_child(modes[i],0),&el_sans_14,0);}
    gui08_routes[2]=button(s,288,56,44,"Seed",gui08_route,NULL);
    gui08_routes[0]=button(s,333,56,44,"Cards",gui08_route,NULL);
    gui08_routes[1]=button(s,378,56,44,"Bases",gui08_route,NULL);
    gui08_routes[3]=button(s,423,56,44,"Dice+",gui08_route,NULL);
    for(int i=0;i<4;i++)lv_obj_set_style_text_font(lv_obj_get_child(gui08_routes[i],0),&el_sans_14,0);

    lv_obj_t *panel=box(s,16,104,448,648,&well);lv_obj_set_style_radius(panel,0,0);
    text(panel,12,10,420,"YOUR ENTROPY ENTERS THE LAB",&el_sans_14,0xd8892b);
    tabs[0]=button(panel,12,35,207,"Hex input",navigate,NULL);
    tabs[1]=button(panel,227,35,207,"Test results",navigate,(void*)1);
    lv_obj_add_style(tabs[0],&tab,0);lv_obj_add_style(tabs[1],&tab,0);
    input=box(panel,11,88,422,545,&card);output=box(panel,12,89,422,545,&card);
    count=text(input,15,9,388,"",&el_sans_14,0xa3a3a3);lv_obj_set_height(count,20);lv_label_set_long_mode(count,LV_LABEL_LONG_CLIP);
    d6_viewport=box(input,15,35,388,126,&well);
    hexlabel=text(d6_viewport,10,8,366,"",&el_mono_16,0xeeeeee);
    lv_obj_set_scroll_dir(d6_viewport,LV_DIR_VER);lv_obj_set_scrollbar_mode(d6_viewport,LV_SCROLLBAR_MODE_ON);
    lv_obj_set_style_width(d6_viewport,4,LV_PART_SCROLLBAR);lv_obj_set_style_bg_color(d6_viewport,lv_color_hex(0xff9900),LV_PART_SCROLLBAR);lv_obj_set_style_bg_opa(d6_viewport,LV_OPA_COVER,LV_PART_SCROLLBAR);lv_obj_set_style_radius(d6_viewport,2,LV_PART_SCROLLBAR);
    d6_scroll_track=box(d6_viewport,383,0,4,124,&d6_track_style);lv_obj_add_flag(d6_scroll_track,LV_OBJ_FLAG_FLOATING);lv_obj_remove_flag(d6_scroll_track,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(d6_viewport,d6_scroll_event,LV_EVENT_SCROLL_BEGIN,NULL);lv_obj_add_event_cb(d6_viewport,d6_scroll_event,LV_EVENT_SCROLL,NULL);lv_obj_add_event_cb(d6_viewport,d6_scroll_event,LV_EVENT_SCROLL_END,NULL);
    for(size_t i=0;i<D6_POOL_CELLS;i++){
        lv_obj_t *cell=lv_button_create(d6_viewport);d6_cells[i]=cell;lv_obj_remove_style_all(cell);
        lv_obj_add_style(cell,&control,0);lv_obj_add_style(cell,&selected,LV_STATE_CHECKED);lv_obj_add_style(cell,&disabled,LV_STATE_DISABLED);
        lv_obj_set_style_outline_color(cell,lv_color_hex(0xff9900),LV_STATE_FOCUS_KEY|LV_STATE_CHECKED);lv_obj_set_style_outline_width(cell,2,LV_STATE_FOCUS_KEY|LV_STATE_CHECKED);lv_obj_set_style_outline_pad(cell,-3,LV_STATE_CHECKED);
        lv_obj_set_pos(cell,9+(int)(i%8)*46,7+(int)(i/8)*48);lv_obj_set_size(cell,44,44);lv_obj_remove_flag(cell,LV_OBJ_FLAG_SCROLLABLE);d6_cell_index[i]=0;
        d6_indices[i]=text(cell,2,2,40,"",&el_sans_14,0xa3a3a3);lv_obj_set_height(d6_indices[i],14);lv_obj_set_style_text_align(d6_indices[i],LV_TEXT_ALIGN_CENTER,0);
        d6_faces[i]=text(cell,2,18,40,"",&el_mono_20,0xeeeeee);lv_obj_set_height(d6_faces[i],24);lv_obj_set_style_text_align(d6_faces[i],LV_TEXT_ALIGN_CENTER,0);
        lv_obj_add_event_cb(cell,d6_select_roll,LV_EVENT_CLICKED,(void*)(uintptr_t)i);lv_obj_add_flag(cell,LV_OBJ_FLAG_HIDDEN);
    }
    d6_spacer=lv_obj_create(d6_viewport);lv_obj_remove_style_all(d6_spacer);lv_obj_set_size(d6_spacer,1,1);lv_obj_remove_flag(d6_spacer,LV_OBJ_FLAG_CLICKABLE);lv_obj_add_flag(d6_spacer,LV_OBJ_FLAG_HIDDEN);
    status=text(input,15,167,388,"",&el_sans_14,0xff9900);lv_obj_set_height(status,20);
    keypad=box(input,15,193,388,206,&well);
    coinpad=box(input,15,193,388,206,&well);
    flips[0]=button(coinpad,6,6,184,"Heads 0",flip,NULL);
    flips[1]=button(coinpad,198,6,184,"Tails 1",flip,(void*)1);
    text(coinpad,6,60,376,"Target BIP39 words",&el_sans_14,0xa3a3a3);
    for(int i=0;i<5;i++){char n[4];snprintf(n,sizeof n,"%d",12+3*i);choices[i]=button(coinpad,6+i*76,82,70,n,select_words,(void*)(uintptr_t)(12+3*i));}
    text(coinpad,8,140,370,"Preview: latest 64 bits / input retained.\nEncoded count is not a randomness score.\nRaw bits: no hash, padding or truncation.",&el_sans_14,0xa3a3a3);
    dicepad=box(input,15,193,388,206,&well);
    lv_obj_t *add_label=text(dicepad,5,5,68,"Add\nroll",&el_sans_14,0xa3a3a3);lv_obj_set_height(add_label,44);
    lv_obj_t *replace_label=text(dicepad,5,57,68,"Replace",&el_sans_14,0xa3a3a3);lv_obj_set_height(replace_label,44);
    lv_obj_t *words_label=text(dicepad,5,109,58,"Words",&el_sans_14,0xa3a3a3);lv_obj_set_height(words_label,44);
    for(int i=0;i<6;i++){
        char k[2]={'1'+i,0};dicekeys[i]=button(dicepad,82+i*51,5,44,k,roll,(void*)(uintptr_t)(i+1));
        d6_replace[i]=button(dicepad,82+i*51,57,44,k,d6_replace_roll,(void*)(uintptr_t)(i+1));
        lv_obj_set_style_text_font(lv_obj_get_child(dicekeys[i],0),&el_mono_20,0);lv_obj_set_style_text_font(lv_obj_get_child(d6_replace[i],0),&el_mono_20,0);
    }
    for(int i=0;i<5;i++){char n[4];snprintf(n,sizeof n,"%d",12+3*i);dicechoices[i]=button(dicepad,69+i*62,109,58,n,select_words,(void*)(uintptr_t)(12+3*i));}
    dicemethod=text(dicepad,7,159,370,"",&el_sans_14,0xff9900);
    for(int i=0;i<16;i++){
        char k[2]={"0123456789ABCDEF"[i],0};
        edits[i]=button(keypad,6+(i%4)*95,6+(i/4)*50,89,k,edit,(void*)(uintptr_t)i);
        lv_obj_set_style_text_font(edits[i],&el_mono_20,0);
    }
    edits[17]=button(input,15,407,190,"Clear",edit,(void*)17);
    edits[16]=button(input,213,407,190,"Delete",edit,(void*)16);
    edits[18]=button(input,15,463,190,"Load public zero",edit,(void*)18);
    run=button(input,213,463,190,"Calculate",calculate,NULL);lv_obj_add_style(run,&selected,0);
    note=text(output,16,10,388,"",&el_sans_14,0xff9900);
    for(int i=0;i<24;i++){
        lv_obj_t *cell=box(output,16+(i/12)*198,54+(i%12)*22,190,21,&readout);
        char n[4];snprintf(n,sizeof n,"%02d",i+1);
        text(cell,6,2,26,n,&el_sans_14,0xa3a3a3);
        words[i]=text(cell,34,1,150,"--",&el_mono_16,0xeeeeee);
    }
    text(output,16,324,240,"Master fingerprint",&el_sans_14,0xa3a3a3);
    fp=text(output,278,322,120,"--------",&el_mono_20,0xeeeeee);
    text(output,16,350,388,"BIP84 m/84'/0'/0'/0/0 / Mainnet",&el_sans_14,0xa3a3a3);
    addr=text(output,16,370,388,"Not calculated",&el_mono_16,0xeeeeee);
    back=button(output,16,418,190,"Back to hex input",navigate,NULL);
    edits[19]=button(output,214,418,190,"Clear",edit,(void*)17);
    legacy_panel=panel;mn_create(s);gui08_native_create(&gui08_ui,s,gui08_guard,pi_enter_gui08,cancel_gui08);seed_native_create(&seed_ui,s,gui08_guard,pi_enter_gui08,cancel_gui08);extra_dice_native_create(&extra_dice_ui,s,gui08_guard,pi_enter_gui08,cancel_gui08);
    invalidate();refresh();

    nav_create(s);show(false);saver_create(root);
}
