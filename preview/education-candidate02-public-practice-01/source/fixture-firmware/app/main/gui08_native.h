#ifndef ENTROPYLAB_GUI08_NATIVE_H
#define ENTROPYLAB_GUI08_NATIVE_H
#include "gui08_editor.h"
#include "fingerprint_view.h"
#include "lvgl.h"

typedef bool (*gui08_native_guard_fn)(lv_event_t *event);
/* Navigation callback: opens the visible Passphrase owner. It must not derive. */
typedef bool (*gui08_native_passphrase_fn)(const gui08_request *source);
typedef bool (*gui08_native_cancel_fn)(uint64_t request_id);

typedef struct {
    gui08_editor editor;
    lv_obj_t *panel;
    lv_obj_t *method_buttons[2];
    lv_obj_t *word_buttons[5];
    lv_obj_t *base_buttons[4];
    lv_obj_t *symbol_buttons[26];
    lv_obj_t *status;
    lv_obj_t *transcript;
    lv_obj_t *result;
    fingerprint_view fingerprint;
    lv_obj_t *derive;
    lv_obj_t *undo;
    lv_obj_t *clear;
    lv_obj_t *prev_page;
    lv_obj_t *next_page;
    lv_obj_t *page_buttons[3];
    unsigned page;
    char feedback[96];
    gui08_native_guard_fn guard;
    gui08_native_passphrase_fn passphrase;
    gui08_native_cancel_fn cancel;
} gui08_native;

void gui08_native_create(gui08_native *native, lv_obj_t *parent,
                         gui08_native_guard_fn guard, gui08_native_passphrase_fn passphrase,
                         gui08_native_cancel_fn cancel);
void gui08_native_show(gui08_native *native, gui08_method method);
void gui08_native_hide(gui08_native *native);
bool gui08_native_visible(const gui08_native *native);
bool gui08_native_pending(gui08_native *native, const gui08_request *context);
bool gui08_native_accept(gui08_native *native, const gui08_request *result_context);
bool gui08_native_cancelled(gui08_native *native, uint64_t request_id);
bool gui08_native_lifehash(gui08_native *native, const gui08_request *context,
                           const char fingerprint[9], const uint8_t *rgb,
                           size_t rgb_size);
void gui08_native_lifehash_blank(gui08_native *native);
#endif
