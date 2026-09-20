#include "extra_dice_native.h"
#include "native_theme.h"

#include <stdio.h>
#include <string.h>

static const unsigned word_counts[] = {12, 15, 18, 21, 24};
static extra_dice_native *owner(lv_event_t *event) { return lv_event_get_user_data(event); }
static bool blocked(extra_dice_native *native, lv_event_t *event) {
    return (native->guard && native->guard(event)) || native->editor.pending_id != 0;
}
static lv_obj_t *label(lv_obj_t *parent,int x,int y,int width,const char *value){
    lv_obj_t *object=lv_label_create(parent);lv_obj_set_pos(object,x,y);lv_obj_set_width(object,width);
    lv_label_set_long_mode(object,LV_LABEL_LONG_WRAP);lv_label_set_text(object,value);return object;
}
static lv_obj_t *button(lv_obj_t *parent,int x,int y,int width,const char *value,
                        lv_event_cb_t callback,extra_dice_native *native){
    lv_obj_t *object=lv_button_create(parent);lv_obj_remove_style_all(object);
    lv_obj_add_style(object,&native->button_style,0);
    lv_obj_add_style(object,&native->selected_style,LV_STATE_CHECKED);
    lv_obj_add_style(object,&native->selected_style,LV_STATE_PRESSED);
    lv_obj_add_style(object,&native->disabled_style,LV_STATE_DISABLED);
    lv_obj_set_style_pad_all(object,0,0);
    lv_obj_set_pos(object,x,y);lv_obj_set_size(object,width,44);lv_obj_t *caption=lv_label_create(object);
    lv_label_set_text(caption,value);lv_obj_center(caption);lv_obj_add_event_cb(object,callback,LV_EVENT_CLICKED,native);return object;
}
static void enabled(lv_obj_t *object,bool value){if(value)lv_obj_remove_state(object,LV_STATE_DISABLED);else lv_obj_add_state(object,LV_STATE_DISABLED);}
static void checked(lv_obj_t *object,bool value){if(value)lv_obj_add_state(object,LV_STATE_CHECKED);else lv_obj_remove_state(object,LV_STATE_CHECKED);}
static void visible(lv_obj_t *object,bool value){if(value)lv_obj_remove_flag(object,LV_OBJ_FLAG_HIDDEN);else lv_obj_add_flag(object,LV_OBJ_FLAG_HIDDEN);}
static void invalidate(extra_dice_native *native){native->editor.pending_id=0;native->editor.pending_revision=0;native->editor.pending_passphrase_marker=GUI08_PASSPHRASE_UNSPECIFIED;native->editor.result_visible=false;if(++native->editor.revision==0)++native->editor.revision;fingerprint_view_blank(&native->fingerprint);}
static size_t parsed_count(const extra_dice_native *native,int *status){
    uint16_t indices[23];size_t count=0;
    *status=native->method==GUI08_DICE_BITBOX?
        el_bitbox_indices(native->editor.transcript,native->editor.length,indices,23,&count):
        el_dplus_indices(native->editor.transcript,native->editor.length,indices,23,&count);
    return count;
}
static unsigned candidate_count(const extra_dice_native *native){return 1u<<(11u-native->editor.words/3u);}
static bool prefix_complete(const extra_dice_native *native){int status=0;size_t count=parsed_count(native,&status);return status==EL_DICE_OK&&count==native->editor.words-1u;}
bool extra_dice_native_ready(const extra_dice_native *native){
    if(!native||!prefix_complete(native))return false;
    if(native->method==GUI08_DICE_BITBOX)return native->final_selected&&native->final_choice<candidate_count(native);
    uint8_t choice=0;return el_dplus_final_index(native->final_roll,native->final_length,native->editor.words,&choice)==EL_DICE_OK;
}
static unsigned dplus_prefix_phase(const extra_dice_native *native){
    unsigned rolls=0;
    for(size_t i=0;i<native->editor.length;++i)
        if(strchr("0123456789ABCDEFabcdef",native->editor.transcript[i]))++rolls;
    return rolls%3u;
}
static unsigned dplus_final_radix(const extra_dice_native *native){
    static const unsigned radices[5][2]={{8,16},{8,8},{16,8},{16,0},{8,0}};
    unsigned row=native->editor.words==12?0:native->editor.words==15?1:
                 native->editor.words==18?2:native->editor.words==21?3:4;
    return native->final_length<2?radices[row][native->final_length]:0;
}
static bool key_allowed(const extra_dice_native *native,unsigned key,bool complete){
    if(native->method==GUI08_DICE_BITBOX)return !complete&&key>=1&&key<=6;
    if(!complete)return dplus_prefix_phase(native)==0?(key>=1&&key<=8):key<16;
    const unsigned radix=dplus_final_radix(native);
    return radix==16?key<16:radix==8?key>=1&&key<=8:false;
}
static void refresh(extra_dice_native *native){
    const bool pending=native->editor.pending_id!=0,complete=prefix_complete(native);
    const bool result=native->editor.result_visible;
    for(unsigned i=0;i<2;++i){checked(native->method_buttons[i],native->method==i+1);enabled(native->method_buttons[i],!pending);}
    for(unsigned i=0;i<5;++i){checked(native->word_buttons[i],native->editor.words==word_counts[i]);enabled(native->word_buttons[i],!pending);visible(native->word_buttons[i],!result);}
    for(unsigned i=0;i<16;++i){enabled(native->keys[i],!pending&&key_allowed(native,i,complete));visible(native->keys[i],!result);}
    enabled(native->load_public,!pending);enabled(native->undo,!pending&&(native->editor.length||native->final_length));enabled(native->clear,!pending&&(native->editor.length||native->final_length||native->final_selected));
    enabled(native->use_final,!pending&&native->method==GUI08_DICE_BITBOX&&complete);
    enabled(native->previous_final,!pending&&native->method==GUI08_DICE_BITBOX&&native->final_selected&&native->final_choice>0);
    enabled(native->next_final,!pending&&native->method==GUI08_DICE_BITBOX&&native->final_selected&&native->final_choice+1<candidate_count(native));
    enabled(native->derive,pending?native->cancel!=NULL:extra_dice_native_ready(native));
    visible(native->load_public,!result);visible(native->undo,!result);
    visible(native->use_final,!result);visible(native->previous_final,!result);
    visible(native->next_final,!result);visible(native->derive,!result||pending);
    lv_label_set_text(lv_obj_get_child(native->derive,0),pending?"Cancel":"Continue to Passphrase");
    char text[256];int parse_status=0;size_t count=parsed_count(native,&parse_status);
    if(result){
        snprintf(text,sizeof text,"RESULT READY / %s / %u words",native->method==GUI08_DICE_BITBOX?"BitBox diceware":"D++ D8/D16",native->editor.words);
    }else{
        snprintf(text,sizeof text,"%s / %u words / %u/%u prefix words",native->method==GUI08_DICE_BITBOX?"BitBox diceware":"D++ D8 + D16 + D16",native->editor.words,(unsigned)count,native->editor.words-1u);
    }
    lv_label_set_text(native->transcript,text);
    if(result)snprintf(text,sizeof text,"Derived result published for this exact input.");
    else if(native->method==GUI08_DICE_BITBOX)snprintf(text,sizeof text,native->final_selected?"Checksum candidate %u of %u selected":"Prefix complete -> explicitly choose checksum candidate",native->final_choice+1,candidate_count(native));
    else snprintf(text,sizeof text,"D++ final rolls: %s / %s",native->final_length?native->final_roll:"none",native->editor.words==12?"D8,D16":native->editor.words==15?"D8,D8":native->editor.words==18?"D16,coin D8":native->editor.words==21?"D16":"D8");
    lv_label_set_text(native->final_status,text);
    if(result)snprintf(text,sizeof text,"Public TEST result / NEVER fund. Clear returns to input.");
    else if(pending)snprintf(text,sizeof text,"Calculating... Passphrase %s. Input controls locked.",native->editor.pending_passphrase_marker==GUI08_PASSPHRASE_ACTIVE?"Active":"Empty");
    else if(native->feedback[0])snprintf(text,sizeof text,"%s",native->feedback);
    else if(extra_dice_native_ready(native))snprintf(text,sizeof text,"%s ready / one shared derivation next",native->method==GUI08_DICE_BITBOX?"Checksum choice":"Final rolls");
    else if(parse_status!=EL_DICE_OK&&native->editor.length)snprintf(text,sizeof text,"Incomplete or invalid roll sequence; edit before continuing.");
    else snprintf(text,sizeof text,"Public TEST input only / NEVER fund. Mapping is not entropy proof.");
    lv_label_set_text(native->status,text);
    lv_obj_set_pos(native->fingerprint.label,8,result?222:566);
    lv_obj_set_pos(native->fingerprint.explanation,8,result?242:586);
    lv_obj_set_pos(native->fingerprint.canvas,262,result?222:566);
}
static void reset(extra_dice_native *native){memset(native->editor.transcript,0,sizeof native->editor.transcript);native->editor.length=0;memset(native->final_roll,0,sizeof native->final_roll);native->final_length=0;native->final_choice=0;native->final_selected=false;native->feedback[0]=0;invalidate(native);}
static void method_event(lv_event_t *event){extra_dice_native *native=owner(event);if(blocked(native,event))return;for(unsigned i=0;i<2;++i)if(lv_event_get_target(event)==native->method_buttons[i]&&native->method!=i+1){native->method=i+1;reset(native);}refresh(native);}
static void words_event(lv_event_t *event){extra_dice_native *native=owner(event);if(blocked(native,event))return;for(unsigned i=0;i<5;++i)if(lv_event_get_target(event)==native->word_buttons[i]&&native->editor.words!=word_counts[i]){native->editor.words=word_counts[i];reset(native);}refresh(native);}
static void key_event(lv_event_t *event){extra_dice_native *native=owner(event);if(blocked(native,event))return;unsigned key=16;for(unsigned i=0;i<16;++i)if(lv_event_get_target(event)==native->keys[i])key=i;if(key==16)return;const char value="0123456789ABCDEF"[key];if(native->method==GUI08_DICE_DPLUS&&prefix_complete(native)){if(native->final_length<2){native->final_roll[native->final_length++]=value;native->final_roll[native->final_length]=0;}}else if(native->editor.length<1024){native->editor.transcript[native->editor.length++]=value;native->editor.transcript[native->editor.length]=0;}native->feedback[0]=0;invalidate(native);refresh(native);}
static void load_event(lv_event_t *event){extra_dice_native *native=owner(event);if(blocked(native,event))return;reset(native);const char *group=native->method==GUI08_DICE_BITBOX?"111111":"100";for(unsigned i=0;i<native->editor.words-1u;++i){if(i)native->editor.transcript[native->editor.length++]=' ';size_t n=strlen(group);memcpy(native->editor.transcript+native->editor.length,group,n);native->editor.length+=n;}native->editor.transcript[native->editor.length]=0;snprintf(native->feedback,sizeof native->feedback,"Loaded deterministic public zero prefix.");invalidate(native);refresh(native);}
static void final_event(lv_event_t *event){extra_dice_native *native=owner(event);if(blocked(native,event)||native->method!=GUI08_DICE_BITBOX||!prefix_complete(native))return;if(lv_event_get_target(event)==native->use_final){native->final_selected=true;native->final_choice=0;}else if(lv_event_get_target(event)==native->previous_final&&native->final_choice)native->final_choice--;else if(lv_event_get_target(event)==native->next_final&&native->final_choice+1<candidate_count(native))native->final_choice++;native->feedback[0]=0;invalidate(native);refresh(native);}
static void undo_event(lv_event_t *event){extra_dice_native *native=owner(event);if(blocked(native,event))return;if(native->final_length)native->final_roll[--native->final_length]=0;else if(native->editor.length)native->editor.transcript[--native->editor.length]=0;native->final_selected=false;invalidate(native);refresh(native);}
static void clear_event(lv_event_t *event){extra_dice_native *native=owner(event);if(blocked(native,event))return;reset(native);refresh(native);}
static gui08_request begin(extra_dice_native *native){gui08_request request;memset(&request,0,sizeof request);if(!extra_dice_native_ready(native))return request;request.request_id=++native->editor.request_serial;if(!request.request_id)request.request_id=++native->editor.request_serial;request.revision=native->editor.revision;request.method=GUI08_DICE;request.words=native->editor.words;request.dice_method=native->method;request.dice_final_choice=native->final_choice;request.dice_final_length=native->final_length;memcpy(request.dice_final,native->final_roll,native->final_length+1);request.length=native->editor.length;memcpy(request.transcript,native->editor.transcript,native->editor.length+1);return request;}
static void derive_event(lv_event_t *event){extra_dice_native *native=owner(event);if(native->guard&&native->guard(event))return;if(native->editor.pending_id){uint64_t id=native->editor.pending_id;if(native->cancel&&native->cancel(id)&&extra_dice_native_cancelled(native,id))snprintf(native->feedback,sizeof native->feedback,"Cancelled. Input remains ready; no result published.");refresh(native);return;}gui08_request request=begin(native);if(!request.request_id||!native->passphrase||!native->passphrase(&request))snprintf(native->feedback,sizeof native->feedback,"Unable to continue to Passphrase.");else native->feedback[0]=0;refresh(native);}
void extra_dice_native_create(extra_dice_native *native, lv_obj_t *parent,
                              gui08_native_guard_fn guard,
                              gui08_native_passphrase_fn passphrase,
                              gui08_native_cancel_fn cancel) {
  memset(native, 0, sizeof *native);
  native->guard = guard;
  native->passphrase = passphrase;
  native->cancel = cancel;
  native->method = GUI08_DICE_BITBOX;
  gui08_editor_init(&native->editor, GUI08_DICE, 12, 0);
  lv_style_init(&native->panel_style);
  lv_style_set_bg_color(&native->panel_style, lv_color_hex(0x141414));
  lv_style_set_bg_opa(&native->panel_style, LV_OPA_COVER);
  lv_style_set_border_width(&native->panel_style, 1);
  lv_style_set_border_color(&native->panel_style, lv_color_hex(0x333333));
  lv_style_set_radius(&native->panel_style, 20);
  lv_style_set_pad_all(&native->panel_style, 0);
  lv_style_set_text_color(&native->panel_style, lv_color_hex(0xeeeeee));
  lv_style_init(&native->button_style);
  lv_style_set_bg_color(&native->button_style, lv_color_hex(0x202020));
  lv_style_set_bg_opa(&native->button_style, LV_OPA_COVER);
  lv_style_set_border_width(&native->button_style, 1);
  lv_style_set_border_color(&native->button_style, lv_color_hex(0x333333));
  lv_style_set_radius(&native->button_style, 8);
  lv_style_set_text_color(&native->button_style, lv_color_hex(0xeeeeee));
  lv_style_init(&native->selected_style);
  lv_style_set_bg_color(&native->selected_style, lv_color_hex(0xff9900));
  lv_style_set_bg_opa(&native->selected_style, LV_OPA_COVER);
  lv_style_set_border_color(&native->selected_style, lv_color_hex(0xff9900));
  lv_style_set_text_color(&native->selected_style, lv_color_hex(0x000000));
  lv_style_init(&native->disabled_style);
  lv_style_set_bg_color(&native->disabled_style, lv_color_hex(0x202020));
  lv_style_set_bg_opa(&native->disabled_style, LV_OPA_COVER);
  lv_style_set_text_color(&native->disabled_style, lv_color_hex(0x737373));
  native->panel = lv_obj_create(parent);
  lv_obj_remove_style_all(native->panel);
  lv_obj_add_style(native->panel, &native->panel_style, 0);
  lv_obj_set_style_pad_bottom(native->panel, 8, 0);
  lv_obj_set_pos(native->panel, 16, 104);
  lv_obj_set_size(native->panel, 448, 660);
  lv_obj_remove_flag(native->panel, LV_OBJ_FLAG_SCROLLABLE);
  /* Method identity is shown by the unified Dice disclosure. */
  native->method_buttons[0] =
      button(native->panel, 82, 0, 132, "BitBox", method_event, native);
  native->method_buttons[1] =
      button(native->panel, 222, 0, 182, "D++ D8/D16", method_event, native);
  for (unsigned i = 0; i < 5; ++i) {
    char value[4];
    snprintf(value, sizeof value, "%u", word_counts[i]);
    native->word_buttons[i] = button(native->panel, 8 + (int)i * 82, 52, 76,
                                     value, words_event, native);
  }
  native->transcript = label(native->panel, 8, 104, 396, "");
  native->status = label(native->panel, 8, 132, 396, "");
  lv_obj_set_style_text_color(native->status, lv_color_hex(0xff9900), 0);
  lv_obj_set_height(native->status, 48);
  for (unsigned i = 0; i < 16; ++i) {
    char value[2] = {"0123456789ABCDEF"[i], 0};
    native->keys[i] =
        button(native->panel, 8 + (int)(i % 8) * 49, 188 + (int)(i / 8) * 48,
               44, value, key_event, native);
  }
  native->load_public = button(native->panel, 8, 292, 194, "Load public zero",
                               load_event, native);
  native->undo =
      button(native->panel, 210, 292, 94, "Undo", undo_event, native);
  native->clear =
      button(native->panel, 312, 292, 92, "Clear", clear_event, native);
  native->final_status = label(native->panel, 8, 346, 396, "");
  lv_obj_set_height(native->final_status, 48);
  native->use_final =
      button(native->panel, 8, 398, 194, "Use final #1", final_event, native);
  native->previous_final =
      button(native->panel, 210, 398, 94, "Final -", final_event, native);
  native->next_final =
      button(native->panel, 312, 398, 92, "Final +", final_event, native);
  native->derive = button(native->panel, 8, 452, 396, "Continue to Passphrase",
                          derive_event, native);
  label(native->panel, 8, 504, 396,
        "BitBox: five D4 rolls + D6 coin per prefix word. D++: D8 + D16 + D16; "
        "final rolls depend on word count.");
  fingerprint_view_create(&native->fingerprint, native->panel, 8, 604);
  lv_obj_set_style_text_color(native->fingerprint.explanation,
                              lv_color_hex(0xa3a3a3), 0);
  el_native_body_dock(native->panel,native->clear,native->derive);
  refresh(native);
  extra_dice_native_hide(native);
}
void extra_dice_native_show(extra_dice_native *native){lv_obj_remove_flag(native->panel,LV_OBJ_FLAG_HIDDEN);refresh(native);}void extra_dice_native_hide(extra_dice_native *native){lv_obj_add_flag(native->panel,LV_OBJ_FLAG_HIDDEN);}bool extra_dice_native_visible(const extra_dice_native *native){return native&&native->panel&&!lv_obj_has_flag(native->panel,LV_OBJ_FLAG_HIDDEN);}
bool extra_dice_native_source_matches(const extra_dice_native *native,const gui08_request *request){return native&&request&&request->request_id&&request->method==GUI08_DICE&&request->revision==native->editor.revision&&request->words==native->editor.words&&request->dice_method==native->method&&request->dice_final_choice==native->final_choice&&request->dice_final_length==native->final_length&&!memcmp(request->dice_final,native->final_roll,native->final_length+1)&&request->length==native->editor.length&&!memcmp(request->transcript,native->editor.transcript,native->editor.length+1);}
bool extra_dice_native_pending(extra_dice_native *native,const gui08_request *request){if(!extra_dice_native_source_matches(native,request)||native->editor.pending_id||request->passphrase_marker==GUI08_PASSPHRASE_UNSPECIFIED)return false;native->editor.pending_id=request->request_id;native->editor.pending_revision=request->revision;native->editor.pending_passphrase_marker=request->passphrase_marker;native->editor.result_visible=false;fingerprint_view_rendering(&native->fingerprint);refresh(native);return true;}
bool extra_dice_native_accept(extra_dice_native *native,const gui08_request *request){if(!extra_dice_native_source_matches(native,request))return false;bool accepted=gui08_accept_result(&native->editor,request->request_id,request->revision,request->method,request->words,request->base,request->passphrase_marker);refresh(native);return accepted;}
bool extra_dice_native_cancelled(extra_dice_native *native,uint64_t request_id){bool accepted=gui08_cancel(&native->editor,request_id);if(accepted){snprintf(native->feedback,sizeof native->feedback,"Conversion unavailable or failed; no result published.");fingerprint_view_failed(&native->fingerprint);}refresh(native);return accepted;}
bool extra_dice_native_lifehash(extra_dice_native *native,const gui08_request *request,const char fingerprint[9],const uint8_t *rgb,size_t rgb_size){if(!native||!request||!native->editor.result_visible||!extra_dice_native_source_matches(native,request))return false;return fingerprint_view_publish(&native->fingerprint,fingerprint,rgb,rgb_size);}void extra_dice_native_lifehash_blank(extra_dice_native *native){if(native)fingerprint_view_blank(&native->fingerprint);}
