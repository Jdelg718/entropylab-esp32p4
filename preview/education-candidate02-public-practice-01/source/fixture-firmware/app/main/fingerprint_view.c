#include "fingerprint_view.h"

#include <stdio.h>
#include <string.h>

static bool valid_fingerprint(const char value[9]) {
    if (!value || value[8] != '\0') return false;
    for (size_t i = 0; i < 8; ++i) {
        const char c = value[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F'))) return false;
    }
    return true;
}

static void hide_icon(fingerprint_view *view) {
    memset(view->pixels, 0, sizeof view->pixels);
    lv_obj_invalidate(view->canvas);
    lv_obj_add_flag(view->canvas, LV_OBJ_FLAG_HIDDEN);
    view->visible = false;
}

void fingerprint_view_create(fingerprint_view *view, lv_obj_t *parent,
                             int x, int y) {
    memset(view, 0, sizeof *view);
    /* Caller owns layout; clamping caused notice/readout overlap. */
    view->label = lv_label_create(parent);
    lv_obj_set_pos(view->label, x, y);
    lv_obj_set_width(view->label, 242);
    lv_label_set_text(view->label, "Fingerprint --------");
    view->explanation = lv_label_create(parent);
    lv_obj_set_pos(view->explanation, x, y + 20);
    lv_obj_set_width(view->explanation, 242);
    lv_label_set_long_mode(view->explanation, LV_LABEL_LONG_WRAP);
    view->canvas = lv_canvas_create(parent);
    lv_canvas_set_buffer(view->canvas, view->pixels, 32, 32,
                         LV_COLOR_FORMAT_ARGB8888);
    lv_obj_set_pos(view->canvas, x + 254, y);
    fingerprint_view_blank(view);
}

bool fingerprint_view_publish(fingerprint_view *view, const char fingerprint[9],
                              const uint8_t *rgb, size_t rgb_size) {
    if (!view || !view->canvas || !view->label || !view->explanation ||
        !valid_fingerprint(fingerprint) || !rgb ||
        rgb_size != FINGERPRINT_VIEW_RGB_SIZE) {
        if (view && view->canvas && view->label && view->explanation)
            fingerprint_view_failed(view);
        return false;
    }
    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            const size_t offset = (size_t)(y * 32 + x) * 3;
            lv_canvas_set_px(view->canvas, x, y,
                lv_color_make(rgb[offset], rgb[offset + 1], rgb[offset + 2]),
                LV_OPA_COVER);
        }
    }
    char text[32];
    snprintf(text, sizeof text, "Fingerprint %.8s", fingerprint);
    lv_label_set_text(view->label, text);
    lv_label_set_text(view->explanation,
                      "LifeHash version2 - display only, never input.");
    lv_obj_remove_flag(view->canvas, LV_OBJ_FLAG_HIDDEN);
    view->visible = true;
    return true;
}

void fingerprint_view_blank(fingerprint_view *view) {
    if (!view || !view->canvas || !view->label || !view->explanation) return;
    hide_icon(view);
    lv_label_set_text(view->label, "Fingerprint --------");
    lv_label_set_text(view->explanation,
                      "LifeHash unavailable - no published result.");
}

void fingerprint_view_rendering(fingerprint_view *view) {
    if (!view || !view->canvas || !view->label || !view->explanation) return;
    hide_icon(view);
    lv_label_set_text(view->label, "Fingerprint --------");
    lv_label_set_text(view->explanation,
                      "LifeHash rendering - display only.");
}

void fingerprint_view_failed(fingerprint_view *view) {
    if (!view || !view->canvas || !view->label || !view->explanation) return;
    hide_icon(view);
    lv_label_set_text(view->label, "Fingerprint unavailable");
    lv_label_set_text(view->explanation,
                      "LifeHash failed - no image was published.");
}
