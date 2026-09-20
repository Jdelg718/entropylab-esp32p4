#ifndef ENTROPYLAB_SEED_NATIVE_H
#define ENTROPYLAB_SEED_NATIVE_H

#include "gui08_editor.h"
#include "gui08_native.h"
#include "seed_helpers_target.h"
#include "lvgl.h"

typedef struct {
    gui08_editor editor;
    lv_obj_t *panel;
    lv_obj_t *word_buttons[5];
    lv_obj_t *range_status;
    lv_obj_t *range;
    lv_obj_t *digits[10];
    lv_obj_t *delete_key;
    lv_obj_t *next_word;
    lv_obj_t *candidate_buttons[6];
    lv_obj_t *previous_page;
    lv_obj_t *next_page;
    lv_obj_t *clear;
    lv_obj_t *derive;
    lv_obj_t *progress;
    lv_obj_t *preview;
    lv_obj_t *status;
    lv_obj_t *transcript;
    fingerprint_view fingerprint;
    uint16_t indices[24];
    uint16_t candidates[128];
    unsigned committed;
    unsigned candidate_count;
    unsigned candidate_page;
    char draft[5];
    unsigned draft_len;
    char feedback[128];
    gui08_native_guard_fn guard;
    gui08_native_passphrase_fn passphrase;
    gui08_native_cancel_fn cancel;
} seed_native;

void seed_native_create(seed_native *native, lv_obj_t *parent,
                        gui08_native_guard_fn guard,
                        gui08_native_passphrase_fn passphrase,
                        gui08_native_cancel_fn cancel);
void seed_native_show(seed_native *native);
void seed_native_hide(seed_native *native);
bool seed_native_visible(const seed_native *native);
bool seed_native_ready(const seed_native *native);
bool seed_native_source_matches(const seed_native *native,
                                const gui08_request *request);
bool seed_native_pending(seed_native *native, const gui08_request *request);
bool seed_native_accept(seed_native *native, const gui08_request *request);
bool seed_native_cancelled(seed_native *native, uint64_t request_id);
bool seed_native_lifehash(seed_native *native, const gui08_request *request,
                          const char fingerprint[9], const uint8_t *rgb,
                          size_t rgb_size);
void seed_native_lifehash_blank(seed_native *native);

#endif
