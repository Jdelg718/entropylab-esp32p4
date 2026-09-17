#ifndef ENTROPYLAB_EXTRA_DICE_NATIVE_H
#define ENTROPYLAB_EXTRA_DICE_NATIVE_H

#include "gui08_native.h"
#include "extra_dice.h"
#include "lvgl.h"

typedef struct {
    gui08_editor editor;
    lv_obj_t *panel;
    lv_obj_t *method_buttons[2];
    lv_obj_t *word_buttons[5];
    lv_obj_t *keys[16];
    lv_obj_t *transcript;
    lv_obj_t *status;
    lv_obj_t *final_status;
    lv_obj_t *load_public;
    lv_obj_t *use_final;
    lv_obj_t *previous_final;
    lv_obj_t *next_final;
    lv_obj_t *undo;
    lv_obj_t *clear;
    lv_obj_t *derive;
    fingerprint_view fingerprint;
    lv_style_t panel_style;
    lv_style_t button_style;
    lv_style_t selected_style;
    lv_style_t disabled_style;
    unsigned method;
    unsigned final_choice;
    bool final_selected;
    char final_roll[3];
    size_t final_length;
    char feedback[128];
    gui08_native_guard_fn guard;
    gui08_native_passphrase_fn passphrase;
    gui08_native_cancel_fn cancel;
} extra_dice_native;

void extra_dice_native_create(extra_dice_native *native, lv_obj_t *parent,
                              gui08_native_guard_fn guard,
                              gui08_native_passphrase_fn passphrase,
                              gui08_native_cancel_fn cancel);
void extra_dice_native_show(extra_dice_native *native);
void extra_dice_native_hide(extra_dice_native *native);
bool extra_dice_native_visible(const extra_dice_native *native);
bool extra_dice_native_ready(const extra_dice_native *native);
bool extra_dice_native_source_matches(const extra_dice_native *native,
                                      const gui08_request *request);
bool extra_dice_native_pending(extra_dice_native *native,
                               const gui08_request *request);
bool extra_dice_native_accept(extra_dice_native *native,
                              const gui08_request *request);
bool extra_dice_native_cancelled(extra_dice_native *native, uint64_t request_id);
bool extra_dice_native_lifehash(extra_dice_native *native,
                                const gui08_request *request,
                                const char fingerprint[9], const uint8_t *rgb,
                                size_t rgb_size);
void extra_dice_native_lifehash_blank(extra_dice_native *native);

#endif
