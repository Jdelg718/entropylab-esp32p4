#ifndef EL_NATIVE_THEME_H
#define EL_NATIVE_THEME_H
#include "lvgl.h"
/* Shared explicit local styles: lifetime follows the LVGL object. */
static inline void el_native_surface(lv_obj_t *o,bool control){
 LV_FONT_DECLARE(el_sans_16);
 lv_obj_remove_style_all(o);lv_obj_set_style_text_font(o,&el_sans_16,0);lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);
 lv_obj_set_style_bg_color(o,lv_color_hex(control?0x202020:0x141414),0);
 lv_obj_set_style_text_color(o,lv_color_hex(0xeeeeee),0);
 lv_obj_set_style_border_width(o,1,0);lv_obj_set_style_border_color(o,lv_color_hex(0x333333),0);
 lv_obj_set_style_pad_all(o,0,0);lv_obj_set_style_shadow_width(o,0,0);lv_obj_set_style_radius(o,control?8:20,0);
 if(control){
 lv_obj_set_style_bg_color(o,lv_color_hex(0xff9900),LV_STATE_PRESSED);lv_obj_set_style_text_color(o,lv_color_hex(0),LV_STATE_PRESSED);
 lv_obj_set_style_bg_color(o,lv_color_hex(0xff9900),LV_STATE_CHECKED);lv_obj_set_style_text_color(o,lv_color_hex(0),LV_STATE_CHECKED);
 lv_obj_set_style_bg_color(o,lv_color_hex(0x202020),LV_STATE_DISABLED);lv_obj_set_style_text_color(o,lv_color_hex(0x737373),LV_STATE_DISABLED);
 lv_obj_set_style_outline_color(o,lv_color_hex(0xff9900),LV_STATE_FOCUS_KEY);lv_obj_set_style_outline_width(o,2,LV_STATE_FOCUS_KEY);
 }
}
/* Reparent presentation only: owner pointers and event identities stay intact. */
static inline void el_native_body_dock(lv_obj_t *panel,lv_obj_t *clear,lv_obj_t *derive){
 lv_obj_set_pos(panel,16,116);lv_obj_set_size(panel,448,624);lv_obj_set_style_pad_all(panel,0,0);
 lv_obj_t *body=lv_obj_create(panel);el_native_surface(body,false);
 lv_obj_set_style_border_width(body,0,0);lv_obj_set_style_radius(body,0,0);
 lv_obj_set_pos(body,16,16);lv_obj_set_size(body,416,536);lv_obj_set_scroll_dir(body,LV_DIR_VER);
 lv_obj_remove_flag(body,LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_MOMENTUM);
 lv_obj_set_style_bg_color(body,lv_color_hex(0xff9900),LV_PART_SCROLLBAR);
 lv_obj_set_style_width(body,4,LV_PART_SCROLLBAR);
 for(unsigned i=0;i<lv_obj_get_child_count(panel);){lv_obj_t *c=lv_obj_get_child(panel,i);if(c==body||c==clear||c==derive){i++;continue;}lv_obj_set_parent(c,body);}
 lv_obj_set_pos(clear,16,568);lv_obj_set_size(clear,108,44);
 lv_obj_set_pos(derive,132,568);lv_obj_set_size(derive,300,44);
}
#endif
