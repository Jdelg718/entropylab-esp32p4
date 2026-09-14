#include "gui.h"
#include <stdio.h>
#include <string.h>

/* Source tokens: EntropyLab 6e1f39c src/css/styles.css. Fixed portrait layout.
 * Disabled keypad is a visual preview ONLY; no input buffer or input API exists.
 * All calls originate in the LVGL task/lock; the worker owns computation. */
LV_FONT_DECLARE(el_sans_16);
LV_FONT_DECLARE(el_sans_14);
LV_FONT_DECLARE(el_mono_16);
LV_FONT_DECLARE(el_mono_20);
LV_FONT_DECLARE(el_serif_24);
static lv_style_t base, card, well, readout, control, selected, disabled, tab;
static lv_obj_t *input, *output, *tabs[2], *run, *status, *note, *words[12], *fp, *addr;
static void (*request_fixture)(void);
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
static void show(bool results) {
    if(results){lv_obj_add_flag(input,LV_OBJ_FLAG_HIDDEN);lv_obj_remove_flag(output,LV_OBJ_FLAG_HIDDEN);}
    else {lv_obj_remove_flag(input,LV_OBJ_FLAG_HIDDEN);lv_obj_add_flag(output,LV_OBJ_FLAG_HIDDEN);}
    lv_obj_remove_state(tabs[!results],LV_STATE_CHECKED);lv_obj_add_state(tabs[results],LV_STATE_CHECKED);
}
static void navigate(lv_event_t *e){show(lv_event_get_user_data(e)!=NULL);}
static void calculate(lv_event_t *e){(void)e;if(request_fixture)request_fixture();}
void gui_busy(void) {
    lv_obj_add_state(run,LV_STATE_DISABLED);
    lv_label_set_text(status,"Computing the public zero fixture...\nKeypad preview only - editing disabled.");
    lv_label_set_text(note,"Computing public fixture on this device.\nEmpty BIP39 passphrase; no input accepted.");
    for(int i=0;i<12;i++)lv_label_set_text(words[i],"--");
    lv_label_set_text(fp,"--------");lv_label_set_text(addr,"Calculating...");
}
void gui_result(bool pass,const char *mnemonic,const char *fingerprint,const char *address) {
    lv_obj_remove_state(run,LV_STATE_DISABLED);
    if(!pass){
        lv_label_set_text(status,"Fixture check FAILED - do not use.\nKeypad preview only - editing disabled.");
        lv_label_set_text(note,"Calculation/check FAILED - do not use.");
        for(int i=0;i<12;i++)lv_label_set_text(words[i],"--");
        lv_label_set_text(fp,"--------");lv_label_set_text(addr,"Unavailable");show(true);return;
    }
    /* The worker validates the whole fixed phrase first; copy each word into a
     * bounded local array. lv_label_set_text copies bytes, retaining no pointers. */
    const char *p=mnemonic;
    for(int i=0;i<12;i++){
        char word[16];size_t n=strcspn(p," ");if(n>=sizeof word)n=sizeof word-1;
        memcpy(word,p,n);word[n]='\0';lv_label_set_text(words[i],word);
        p+=n;if(*p==' ')p++;
    }
    lv_label_set_text(fp,fingerprint);lv_label_set_text(addr,address);
    lv_label_set_text(status,"Public fixture matches known test vector.\nKeypad preview only - editing disabled.");
    lv_label_set_text(note,"Computed public zero fixture; vector matched.\nEmpty BIP39 passphrase. NEVER fund.");
    show(true);
}
void gui_create(void (*cb)(void)) {
    request_fixture=cb;
    style_surface(&base,0x000000,0,false);style_surface(&card,0x141414,20,true);
    style_surface(&well,0x000000,12,true);style_surface(&readout,0x202020,0,false);
    style_surface(&control,0x202020,8,true);style_surface(&selected,0xff9900,8,true);
    lv_style_set_text_color(&selected,lv_color_hex(0x000000));lv_style_set_border_color(&selected,lv_color_hex(0xff9900));
    style_surface(&disabled,0x202020,8,true);lv_style_set_text_color(&disabled,lv_color_hex(0x737373));
    style_surface(&tab,0x000000,8,true);lv_style_set_text_color(&tab,lv_color_hex(0xa3a3a3));
    lv_obj_t *s=lv_screen_active();lv_obj_remove_style_all(s);lv_obj_add_style(s,&base,0);lv_obj_remove_flag(s,LV_OBJ_FLAG_SCROLLABLE);
    text(s,16,12,220,"EntropyLab",&el_serif_24,0xeeeeee);
    text(s,322,19,142,"P4 / Native fixture",&el_sans_14,0xa3a3a3);
    lv_obj_t *line=box(s,16,47,448,1,&readout);(void)line;
    lv_obj_t *warning=box(s,16,56,448,60,&card);
    /* Offline OKLab mix(danger 14%, surface) from the approved browser design. */
    lv_obj_set_style_bg_color(warning,lv_color_hex(0x2c1e1c),0);
    lv_obj_set_style_border_color(warning,lv_color_hex(0xd4574a),0);lv_obj_set_style_radius(warning,10,0);
    text(warning,12,8,420,"PUBLIC FIXTURE - TEST ONLY",&el_sans_14,0xff4438);
    text(warning,12,31,420,"No real secrets or funds. Never fund this address.",&el_sans_14,0xeeeeee);
    lv_obj_t *folder=box(s,16,124,80,44,&well);text(folder,18,12,60,"Keys",&el_sans_16,0xeeeeee);
    text(s,270,141,194,"Hex > BIP39 > BIP84",&el_sans_14,0xa3a3a3);
    lv_obj_t *panel=box(s,16,167,448,571,&well);lv_obj_set_style_radius(panel,0,0);
    text(panel,12,10,420,"YOUR ENTROPY ENTERS THE LAB",&el_sans_14,0xd8892b);
    tabs[0]=button(panel,12,35,207,"Hex input",navigate,NULL);
    tabs[1]=button(panel,227,35,207,"Fixture results",navigate,(void*)1);
    lv_obj_add_style(tabs[0],&tab,0);lv_obj_add_style(tabs[1],&tab,0);
    input=box(panel,12,89,422,468,&card);output=box(panel,12,89,422,468,&card);
    text(input,16,12,200,"Hex entropy",&el_sans_16,0xeeeeee);
    text(input,264,13,142,"32 / 32 digits",&el_mono_16,0xa3a3a3);
    lv_obj_t *field=box(input,16,40,388,50,&well);
    text(field,12,16,366,"00000000000000000000000000000000",&el_mono_16,0xeeeeee);
    status=text(input,16,100,388,"Public zero fixture / 128 bits / 12 words.\nKeypad preview only - editing disabled.",&el_sans_14,0xa3a3a3);
    lv_obj_t *keypad=box(input,16,144,388,206,&well);
    for(int i=0;i<16;i++){
        char k[2]={"0123456789ABCDEF"[i],0};
        lv_obj_t *b=button(keypad,6+(i%4)*95,6+(i/4)*50,89,k,NULL,NULL);
        lv_obj_set_style_text_font(b,&el_mono_20,0);lv_obj_set_style_radius(b,7,0);
        lv_obj_add_state(b,LV_STATE_DISABLED);
    }
    lv_obj_t *clear=button(input,16,358,190,"Clear (disabled)",NULL,NULL);lv_obj_add_state(clear,LV_STATE_DISABLED);
    lv_obj_t *del=button(input,214,358,190,"Delete (disabled)",NULL,NULL);lv_obj_add_state(del,LV_STATE_DISABLED);
    text(input,16,424,182,"Fixed public input only",&el_sans_14,0xa3a3a3);
    run=button(input,214,410,190,"Run public fixture",calculate,NULL);lv_obj_add_style(run,&selected,0);
    note=text(output,16,12,388,"No result yet. Run the public fixture.\nKeypad preview does not accept input.",&el_sans_14,0xa3a3a3);
    text(output,16,56,205,"BIP39 mnemonic",&el_sans_16,0xeeeeee);
    text(output,244,58,160,"12 words / English",&el_sans_14,0xa3a3a3);
    for(int i=0;i<12;i++){
        lv_obj_t *cell=box(output,16+(i/6)*198,83+(i%6)*28,190,24,&readout);
        char n[4];snprintf(n,sizeof n,"%02d",i+1);
        text(cell,8,4,26,n,&el_sans_14,0xa3a3a3);
        words[i]=text(cell,34,3,150,"--",&el_mono_16,0xeeeeee);
    }
    lv_obj_t *f=box(output,16,255,388,44,&readout);
    text(f,12,14,241,"Master fingerprint / fixture",&el_sans_14,0xeeeeee);
    fp=text(f,274,12,110,"--------",&el_mono_20,0xeeeeee);
    lv_obj_t *a=box(output,16,307,388,106,&readout);
    text(a,12,10,364,"BIP84 / Native SegWit / fixture",&el_sans_16,0xeeeeee);
    text(a,12,34,364,"m/84'/0'/0'/0/0 / Mainnet",&el_mono_16,0xa3a3a3);
    addr=text(a,12,56,364,"Not calculated",&el_mono_16,0xeeeeee);
    button(output,16,421,388,"Back to hex input",navigate,NULL);
    text(s,46,751,408,"Public test only / No storage or network calls",&el_sans_14,0xa3a3a3);
    show(false);
}
