#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

extern "C" {
#include "extra_dice_native.h"
#include "extra_dice_target.h"
#include "lifehash_fingerprint.h"
}
#include "all_features_application.h"

static uint16_t draw_buffer[480*32];
static uint16_t framebuffer[480*800];
static extra_dice_native ui;
static uint64_t queued;
static unsigned lock_depth,kdf_calls,result_calls;
static void flush(lv_display_t *display,const lv_area_t *area,uint8_t *data){int width=area->x2-area->x1+1,height=area->y2-area->y1+1;const uint16_t *source=(const uint16_t*)data;for(int y=0;y<height;++y)std::memcpy(&framebuffer[(area->y1+y)*480+area->x1],&source[y*width],(size_t)width*sizeof(uint16_t));lv_display_flush_ready(display);}
static void capture(lv_display_t *display,const char *path){lv_refr_now(display);FILE *file=std::fopen(path,"wb");assert(file);std::fprintf(file,"P6\n480 800\n255\n");for(uint16_t pixel:framebuffer){uint8_t rgb[3]={(uint8_t)(((pixel>>11)&31)*255/31),(uint8_t)(((pixel>>5)&63)*255/63),(uint8_t)((pixel&31)*255/31)};assert(std::fwrite(rgb,1,3,file)==3);}assert(std::fclose(file)==0);}
static void lock_app(){assert(lock_depth++==0);}static void unlock_app(){assert(lock_depth--==1);}static bool publish(uint64_t token){assert(lock_depth==0);if(queued)return false;queued=token;return true;}
static gui08_passphrase_marker snapshot(const gui08_request*,uint8_t out[256],size_t *length){std::memset(out,0,256);*length=0;return GUI08_PASSPHRASE_EMPTY;}
static int derive(const gui08_request *request,const pc_result_v1*,all_features_derivation_result *out){assert(lock_depth==0);++kdf_calls;std::memset(out,0,sizeof *out);std::strcpy(out->entropy,"00");std::strcpy(out->fingerprint,request->dice_method==GUI08_DICE_BITBOX?"73c5da0a":"12345678");std::strcpy(out->address,"bc1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq");return 0;}
static int convert(const gui08_request *request,pc_result_v1 *out){return extra_dice_target_convert(request,(uint32_t)request->request_id,out);}
static bool pending(const gui08_request *request){return extra_dice_native_pending(&ui,request);}static bool result(const gui08_request *request,const pc_result_v1 *value,const all_features_derivation_result*){assert(value->mnemonic_len);++result_calls;return extra_dice_native_accept(&ui,request);}static bool cancelled(uint64_t id){return extra_dice_native_cancelled(&ui,id);}
static int render(const all_features_lifehash_request *request,all_features_lifehash_result *result){result->route_epoch=request->route_epoch;result->request_id=request->request_id;result->revision=request->revision;return (int)lifehash_fingerprint_render(request->raw4,4,LIFEHASH_FINGERPRINT_VERSION_2,result->rgb,sizeof result->rgb);}
static bool image(const gui08_request *request,const char fingerprint[9],const uint8_t *rgb,size_t size){return extra_dice_native_lifehash(&ui,request,fingerprint,rgb,size);}static void blank(){extra_dice_native_lifehash_blank(&ui);}static bool guard(lv_event_t*){return false;}
static lv_obj_t *find_button(lv_obj_t *object,const char *caption){if(!object||lv_obj_has_flag(object,LV_OBJ_FLAG_HIDDEN))return nullptr;if(lv_obj_check_type(object,&lv_label_class)&&!std::strcmp(lv_label_get_text(object),caption)&&lv_obj_check_type(lv_obj_get_parent(object),&lv_button_class))return lv_obj_get_parent(object);for(uint32_t i=0;i<lv_obj_get_child_count(object);++i)if(lv_obj_t *found=find_button(lv_obj_get_child(object,i),caption))return found;return nullptr;}
static void click(const char *caption){lv_obj_t *button=find_button(ui.panel,caption);if(!button||lv_obj_has_state(button,LV_STATE_DISABLED))std::fprintf(stderr,"unavailable: %s found=%d disabled=%d\n",caption,button!=nullptr,button?lv_obj_has_state(button,LV_STATE_DISABLED):-1);assert(button&&!lv_obj_has_state(button,LV_STATE_DISABLED));lv_obj_send_event(button,LV_EVENT_CLICKED,nullptr);}
static void assert_ascii_labels(lv_obj_t *object){
 if(lv_obj_check_type(object,&lv_label_class))for(const unsigned char *p=(const unsigned char*)lv_label_get_text(object);*p;++p)assert(*p<128);
 for(uint32_t i=0;i<lv_obj_get_child_count(object);++i)assert_ascii_labels(lv_obj_get_child(object,i));
}
static void assert_key_radix(unsigned radix){
 for(unsigned i=0;i<16;++i){bool expected=radix==16||((radix==8||radix==2)&&i>=1&&i<=8);assert(!lv_obj_has_state(ui.keys[i],LV_STATE_DISABLED)==expected);}
}
static void assert_no_keys(){for(lv_obj_t *key:ui.keys)assert(lv_obj_has_state(key,LV_STATE_DISABLED));}
static void work_poll(){assert(queued);uint64_t token=queued;queued=0;all_features_application_work(token,convert);all_features_application_poll();}
int main(){
 lv_init();
 lv_display_t *display=lv_display_create(480,800);assert(display);
 lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
 lv_display_set_flush_cb(display,flush);
 lv_display_set_buffers(display,draw_buffer,nullptr,sizeof draw_buffer,LV_DISPLAY_RENDER_MODE_PARTIAL);
 extra_dice_native_create(&ui,lv_screen_active(),guard,all_features_application_submit,all_features_application_cancel);
 extra_dice_native_show(&ui);
 assert(all_features_application_configure(lock_app,unlock_app,publish,snapshot,derive,pending,result,cancelled));
 assert(all_features_application_lifehash_configure(render,image,blank));
 assert(lv_obj_has_state(ui.method_buttons[0],LV_STATE_CHECKED));
 assert(!lv_obj_has_state(ui.method_buttons[1],LV_STATE_CHECKED));
 assert(!std::strcmp(lv_label_get_text(ui.fingerprint.explanation),"LifeHash unavailable - no published result."));
 assert_ascii_labels(ui.panel);

 click("Load public zero");click("Use final #1");assert(extra_dice_native_ready(&ui));
 capture(display,"dice41-bitbox-selection-480x800.ppm");
 click("Continue to Passphrase");work_poll();
 assert(kdf_calls==1&&result_calls==1&&ui.editor.result_visible&&ui.fingerprint.visible);
 assert(!std::strncmp(lv_label_get_text(ui.transcript),"RESULT READY",12));
 assert(lv_obj_has_flag(ui.keys[0],LV_OBJ_FLAG_HIDDEN));
 assert(!std::strcmp(lv_label_get_text(ui.fingerprint.explanation),"LifeHash version2 - display only, never input."));
 capture(display,"dice41-bitbox-result-480x800.ppm");

 click("Clear");click("D++ D8/D16");
 assert(!lv_obj_has_state(ui.method_buttons[0],LV_STATE_CHECKED));
 assert(lv_obj_has_state(ui.method_buttons[1],LV_STATE_CHECKED));
 /* Rex combined43: after the leading D8, both D16 positions expose 0-F. */
 click("1");assert_key_radix(16);click("0");assert_key_radix(16);click("F");
 click("Undo");click("Undo");click("Undo");
 click("Load public zero");assert_key_radix(8);click("1");assert_key_radix(16);click("0");
 assert(extra_dice_native_ready(&ui));
 capture(display,"dice41-dplus-selection-480x800.ppm");
 click("Continue to Passphrase");work_poll();
 assert(kdf_calls==2&&result_calls==2&&ui.editor.result_visible&&ui.fingerprint.visible);
 assert(!std::strncmp(lv_label_get_text(ui.transcript),"RESULT READY",12));
 capture(display,"dice41-dplus-result-480x800.ppm");

 /* Every word count exposes exactly its physical final-roll shape. */
 click("Clear");click("15");click("Load public zero");assert_key_radix(8);click("1");assert_key_radix(8);click("8");assert(extra_dice_native_ready(&ui));assert_no_keys();
 click("Clear");click("18");click("Load public zero");assert_key_radix(16);click("F");assert_key_radix(8);click("8");assert(extra_dice_native_ready(&ui));assert_no_keys();
 click("Clear");click("21");click("Load public zero");assert_key_radix(16);click("9");assert(extra_dice_native_ready(&ui));assert_no_keys();
 click("Clear");click("24");click("Load public zero");assert_key_radix(8);click("8");assert(extra_dice_native_ready(&ui));assert_no_keys();
 assert_ascii_labels(ui.panel);
 fingerprint_view_failed(&ui.fingerprint);
 assert(!std::strcmp(lv_label_get_text(ui.fingerprint.label),"Fingerprint unavailable"));
 assert(!std::strcmp(lv_label_get_text(ui.fingerprint.explanation),"LifeHash failed - no image was published."));
 capture(display,"dice41-lifehash-failed-480x800.ppm");

 all_features_application_dispose();lv_deinit();
 std::puts("PASS visual44 native Dice+ real D8/D16 input, exact final shapes, selected methods, distinct results, ASCII glyphs, and LifeHash states");
}
