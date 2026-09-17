#ifndef ENTROPYLAB_FINGERPRINT_VIEW_H
#define ENTROPYLAB_FINGERPRINT_VIEW_H

#include "lvgl.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum { FINGERPRINT_VIEW_RGB_SIZE = 32 * 32 * 3 };
typedef struct {
    lv_obj_t *label;
    lv_obj_t *explanation;
    lv_obj_t *canvas;
    uint32_t pixels[32 * 32];
    bool visible;
} fingerprint_view;

void fingerprint_view_create(fingerprint_view *view, lv_obj_t *parent,
                             int x, int y);
bool fingerprint_view_publish(fingerprint_view *view, const char fingerprint[9],
                              const uint8_t *rgb, size_t rgb_size);
void fingerprint_view_blank(fingerprint_view *view);
void fingerprint_view_rendering(fingerprint_view *view);
void fingerprint_view_failed(fingerprint_view *view);

#endif
