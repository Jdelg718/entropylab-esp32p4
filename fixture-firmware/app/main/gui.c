#include "gui.h"
#include "lvgl.h"
#include "dice_core.h"
#include <stdio.h>
#include <string.h>

/* Source tokens: EntropyLab 6e1f39c src/css/styles.css. Fixed portrait layout.
 * Public-test hex entry, bounded request copies, no secret-erasure guarantees.
 * All calls originate in the LVGL task/lock; the worker owns computation. */
LV_FONT_DECLARE(el_sans_16);
LV_FONT_DECLARE(el_sans_14);
LV_FONT_DECLARE(el_mono_16);
LV_FONT_DECLARE(el_mono_20);
LV_FONT_DECLARE(el_serif_24);
static lv_style_t base, card, well, readout, control, selected, disabled, tab;
static lv_obj_t *input, *output, *tabs[2], *run, *status, *note, *words[24], *fp, *addr, *count, *hexlabel, *edits[20];
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
static uint64_t legacy_id,legacy_pending,legacy_revision;
static void refresh(void);
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
#include "mnemonic_gui.inc"
#include "saver.inc"
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
static void refresh(void) {
    mn_enable(mn_safety,!busy&&!mn_confirm&&!mn_safety_open);
    char c[64];snprintf(c,sizeof c,"%u / 64 chars  |  %u bits",(unsigned)length,(unsigned)length*4);
    if(mode==MODE_COINS)snprintf(c,sizeof c,"%u / %u encoded bits",(unsigned)length,selector*32/3);
    if(el_mode_is_dice(mode))snprintf(c,sizeof c,"%u / %d nominal rolls (max 1024)",(unsigned)length,(int)el_dice_required_rolls(selector));
    lv_label_set_text(count,c);
    /* Coin preview is explicitly the latest 64 bits; full owned transcript retained. */
    gui_label_reset(hexlabel,length?(mode!=MODE_HEX&&length>64?hex+length-64:hex):(el_mode_is_dice(mode)?"Tap 1-6: public laboratory rolls":mode!=MODE_HEX?"Heads=0 / Tails=1. Raw bits, MSB first.":"Tap 0-F to enter public test hex"));
    lv_label_set_text(lv_obj_get_child(edits[16],0),mode!=MODE_HEX?"Undo":"Delete");
    lv_label_set_text(lv_obj_get_child(tabs[0],0),el_mode_is_dice(mode)?"Dice input":mode!=MODE_HEX?"Coin input":"Hex input");
    lv_label_set_text(lv_obj_get_child(back,0),el_mode_is_dice(mode)?"Back to dice input":mode!=MODE_HEX?"Back to coin input":"Back to hex input");
    lv_obj_add_flag(dicepad,LV_OBJ_FLAG_HIDDEN);
    if(el_mode_is_dice(mode)){lv_obj_add_flag(keypad,LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(coinpad,LV_OBJ_FLAG_HIDDEN);lv_obj_remove_flag(dicepad,LV_OBJ_FLAG_HIDDEN);}
    else if(mode==MODE_COINS){lv_obj_add_flag(keypad,LV_OBJ_FLAG_HIDDEN);lv_obj_remove_flag(coinpad,LV_OBJ_FLAG_HIDDEN);}
    else {lv_obj_remove_flag(keypad,LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(coinpad,LV_OBJ_FLAG_HIDDEN);}
    for(int i=0;i<GUI_MODE_COUNT;i++){
        if(busy){lv_obj_add_state(modes[i],LV_STATE_DISABLED);if(i<2)lv_obj_add_state(flips[i],LV_STATE_DISABLED);}
        else {lv_obj_remove_state(modes[i],LV_STATE_DISABLED);if(i<2)lv_obj_remove_state(flips[i],LV_STATE_DISABLED);}
        if(mode==(unsigned)i)lv_obj_add_state(modes[i],LV_STATE_CHECKED);else lv_obj_remove_state(modes[i],LV_STATE_CHECKED);
    }
    for(int i=0;i<5;i++){
        if(busy)lv_obj_add_state(choices[i],LV_STATE_DISABLED);else lv_obj_remove_state(choices[i],LV_STATE_DISABLED);
        if(selector==(unsigned)(12+3*i))lv_obj_add_state(choices[i],LV_STATE_CHECKED);else lv_obj_remove_state(choices[i],LV_STATE_CHECKED);
    }
    for(int i=0;i<6;i++){if(busy)lv_obj_add_state(dicekeys[i],LV_STATE_DISABLED);else lv_obj_remove_state(dicekeys[i],LV_STATE_DISABLED);}
    for(int i=0;i<5;i++){if(busy)lv_obj_add_state(dicechoices[i],LV_STATE_DISABLED);else lv_obj_remove_state(dicechoices[i],LV_STATE_DISABLED);if(selector==(unsigned)(12+3*i))lv_obj_add_state(dicechoices[i],LV_STATE_CHECKED);else lv_obj_remove_state(dicechoices[i],LV_STATE_CHECKED);}
    if(el_mode_is_dice(mode))lv_obj_add_flag(edits[18],LV_OBJ_FLAG_HIDDEN);else lv_obj_remove_flag(edits[18],LV_OBJ_FLAG_HIDDEN);
    for(int i=0;i<20;i++) {if(busy)lv_obj_add_state(edits[i],LV_STATE_DISABLED);else lv_obj_remove_state(edits[i],LV_STATE_DISABLED);}
    if(busy||!allowed())lv_obj_add_state(run,LV_STATE_DISABLED);else lv_obj_remove_state(run,LV_STATE_DISABLED);
    lv_label_set_text(dicemethod,mode==MODE_D6_COLEMAN?"Coleman: 6->0 before SHA256":"D6 raw COLDCARD-style SHA256");
    lv_label_set_text(status,busy?(el_mode_is_dice(mode)&&length<(size_t)el_dice_required_rolls(selector)?"WEAK_INPUT_LAB_ONLY / Calculating...":"Calculating outside the display lock..."):el_mode_is_dice(mode)?(length==0?"Empty input: enter at least one roll.":length<(size_t)el_dice_required_rolls(selector)?"WEAK_INPUT_LAB_ONLY":"Count only - NOT randomness quality."):allowed()?"Valid length. English / empty passphrase.":(mode!=MODE_HEX?"Exact selected bit count required. No truncation.":"Requires 32, 40, 48, 56 or 64 hex characters."));
}
static void switch_mode(lv_event_t *e){if(saver_event_blocked(e))return;if(busy||mn_confirm||mn_safety_open)return;uint32_t next=saver_payload(e);if(next>=MODE_COUNT)return;mode=(el_mode_t)next;mn_reset_draft();mn_confirm=0;mn_invalidate();mn_refresh();if(mode==MODE_MNEMONIC){lv_obj_add_flag(legacy_panel,LV_OBJ_FLAG_HIDDEN);lv_obj_remove_flag(mn_panel,LV_OBJ_FLAG_HIDDEN);mn_refresh();return;}lv_obj_add_flag(mn_panel,LV_OBJ_FLAG_HIDDEN);lv_obj_remove_flag(legacy_panel,LV_OBJ_FLAG_HIDDEN);invalidate();refresh();show(false);}
static void select_words(lv_event_t *e){if(saver_event_blocked(e))return;if(busy||!el_mode_is_legacy(mode))return;selector=saver_payload(e);invalidate();refresh();show(false);}
static void flip(lv_event_t *e){if(saver_event_blocked(e))return;if(busy||mode!=MODE_COINS||length>=256)return;hex[length++]=saver_payload(e)?'1':'0';hex[length]=0;invalidate();refresh();show(false);}
static void roll(lv_event_t *e){if(saver_event_blocked(e))return;if(busy||!el_mode_is_dice(mode)||length>=1024)return;hex[length++]='0'+saver_payload(e);hex[length]=0;invalidate();refresh();show(false);}
static void navigate(lv_event_t *e){if(saver_event_blocked(e))return;if(!el_mode_is_legacy(mode))return;show(saver_payload(e)!=0);}
static void edit(lv_event_t *e) {if(saver_event_blocked(e))return;
    if(busy||!el_mode_is_legacy(mode))return;
    uintptr_t k=saver_payload(e);
    if(k<16){if(mode!=MODE_HEX||length==64)return;hex[length++]="0123456789ABCDEF"[k];hex[length]=0;}
    else if(k==16){if(length)hex[--length]=0;}
    else {if(k==18&&el_mode_is_dice(mode))return;gui_owned_zero(hex,sizeof hex);length=0;if(k==18){length=mode!=MODE_HEX?selector*32/3:64;memset(hex,'0',length);}}
    invalidate();refresh();show(false);
}
static void calculate(lv_event_t *e){if(saver_event_blocked(e))return;
    (void)e;if(busy||!allowed())return;
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
 for(unsigned i=0;i<6;i++)saver_bind_legacy(dicekeys[i],roll,i+1);
 for(unsigned i=0;i<20;i++)saver_bind_legacy(edits[i],edit,i==19?17:i);
 saver_bind_legacy(run,calculate,0);saver_bind_legacy(back,navigate,0);
}
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
    if(saver_timer){lv_timer_delete(saver_timer);saver_timer=NULL;}
    request_hex=cb;memset(transcripts,0,sizeof transcripts);memset(lengths,0,sizeof lengths);mode=MODE_HEX;selector=12;busy=false;valid_result=false;
    style_surface(&base,0x000000,0,false);style_surface(&card,0x141414,20,true);
    style_surface(&well,0x000000,12,true);style_surface(&readout,0x202020,0,false);
    style_surface(&control,0x202020,8,true);style_surface(&selected,0xff9900,8,true);
    lv_style_set_text_color(&selected,lv_color_hex(0x000000));lv_style_set_border_color(&selected,lv_color_hex(0xff9900));
    style_surface(&disabled,0x202020,8,true);lv_style_set_text_color(&disabled,lv_color_hex(0x737373));
    style_surface(&tab,0x000000,8,true);lv_style_set_text_color(&tab,lv_color_hex(0xa3a3a3));
    lv_obj_t *s=lv_screen_active();lv_obj_remove_style_all(s);lv_obj_add_style(s,&base,0);lv_obj_remove_flag(s,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_t *root=s;app_content=box(root,0,0,480,800,&base);s=app_content;
    header_title=text(s,16,8,260,"EntropyLab",&el_serif_24,0xeeeeee);
    lv_obj_set_height(header_title,36);
    header_meta=text(s,322,12,142,"P4 / Offline lab",&el_sans_14,0xa3a3a3);
    lv_obj_t *line=box(s,16,47,448,1,&readout);(void)line;
    lv_obj_set_height(header_meta,28);
    const char *names[GUI_MODE_COUNT]={"Hex","Coins","D6 raw","D6 6->0","Words"};
    for(int i=0;i<GUI_MODE_COUNT;i++)modes[i]=button(s,16+i*90,56,88,names[i],switch_mode,(void*)(uintptr_t)i);

    lv_obj_t *panel=box(s,16,104,448,648,&well);lv_obj_set_style_radius(panel,0,0);
    text(panel,12,10,420,"YOUR ENTROPY ENTERS THE LAB",&el_sans_14,0xd8892b);
    tabs[0]=button(panel,12,35,207,"Hex input",navigate,NULL);
    tabs[1]=button(panel,227,35,207,"Test results",navigate,(void*)1);
    lv_obj_add_style(tabs[0],&tab,0);lv_obj_add_style(tabs[1],&tab,0);
    input=box(panel,12,89,422,545,&card);output=box(panel,12,89,422,545,&card);
    count=text(input,16,10,388,"",&el_mono_16,0xa3a3a3);
    lv_obj_t *field=box(input,16,36,388,126,&well);
    hexlabel=text(field,10,8,366,"",&el_mono_16,0xeeeeee);
    status=text(input,16,168,388,"",&el_sans_14,0xff9900);
    keypad=box(input,16,194,388,206,&well);
    coinpad=box(input,16,194,388,206,&well);
    flips[0]=button(coinpad,6,6,184,"Heads 0",flip,NULL);
    flips[1]=button(coinpad,198,6,184,"Tails 1",flip,(void*)1);
    text(coinpad,6,60,376,"Target BIP39 words",&el_sans_14,0xa3a3a3);
    for(int i=0;i<5;i++){char n[4];snprintf(n,sizeof n,"%d",12+3*i);choices[i]=button(coinpad,6+i*76,82,70,n,select_words,(void*)(uintptr_t)(12+3*i));}
    text(coinpad,8,140,370,"Preview: latest 64 bits / input retained.\nEncoded count is not a randomness score.\nRaw bits: no hash, padding or truncation.",&el_sans_14,0xa3a3a3);
    dicepad=box(input,16,194,388,206,&well);
    for(int i=0;i<6;i++){char k[2]={'1'+i,0};dicekeys[i]=button(dicepad,6+i*63,6,58,k,roll,(void*)(uintptr_t)(i+1));}
    for(int i=0;i<5;i++){char n[4];snprintf(n,sizeof n,"%d",12+3*i);dicechoices[i]=button(dicepad,6+i*76,58,70,n,select_words,(void*)(uintptr_t)(12+3*i));}
    dicemethod=text(dicepad,8,108,370,"",&el_sans_14,0xff9900);
    text(dicepad,8,130,370,"Nominal count: fair INDEPENDENT D6.\nHashing cannot invent randomness.\nPreview last 64; ALL rolls hashed.\n12 / 15 / 18 / 21 / 24 BIP39 words",&el_sans_14,0xa3a3a3);
    for(int i=0;i<16;i++){
        char k[2]={"0123456789ABCDEF"[i],0};
        edits[i]=button(keypad,6+(i%4)*95,6+(i/4)*50,89,k,edit,(void*)(uintptr_t)i);
        lv_obj_set_style_text_font(edits[i],&el_mono_20,0);
    }
    edits[17]=button(input,16,408,190,"Clear",edit,(void*)17);
    edits[16]=button(input,214,408,190,"Delete",edit,(void*)16);
    edits[18]=button(input,16,464,190,"Load public zero",edit,(void*)18);
    run=button(input,214,464,190,"Calculate",calculate,NULL);lv_obj_add_style(run,&selected,0);
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
    legacy_panel=panel;mn_create(s);
    invalidate();refresh();

    show(false);saver_create(root);
}
