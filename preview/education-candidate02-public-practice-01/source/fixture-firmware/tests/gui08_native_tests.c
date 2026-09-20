/* Focused real-LVGL Cards/Bases route test. Public fixtures only. */
#include "gui08_native.h"
#include "lvgl.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint16_t pixels[480 * 32];
static unsigned submits;
static unsigned cancels;
static uint64_t cancelled_id;
static gui08_request submitted;
static bool blocked;

static void flush(lv_display_t *display, const lv_area_t *area, uint8_t *data) {
    (void)area; (void)data; lv_display_flush_ready(display);
}
static bool guard(lv_event_t *event) { (void)event; return blocked; }
static bool submit(const gui08_request *request) { ++submits; submitted = *request; return true; }
static bool cancel(uint64_t request_id) { ++cancels; cancelled_id = request_id; return true; }
static lv_obj_t *find_button(lv_obj_t *object, const char *label) {
    if (lv_obj_has_flag(object, LV_OBJ_FLAG_HIDDEN)) return NULL;
    if (lv_obj_check_type(object, &lv_label_class) && !strcmp(lv_label_get_text(object), label) &&
        lv_obj_check_type(lv_obj_get_parent(object), &lv_button_class)) return lv_obj_get_parent(object);
    for (uint32_t i = 0; i < lv_obj_get_child_count(object); ++i) {
        lv_obj_t *found = find_button(lv_obj_get_child(object, i), label);
        if (found) return found;
    }
    return NULL;
}
static lv_obj_t *find_symbol(gui08_native *native, const char *label) {
    const size_t count = sizeof native->symbol_buttons / sizeof native->symbol_buttons[0];
    for (size_t i = 0; i < count; ++i) {
        lv_obj_t *button = native->symbol_buttons[i];
        if (!button || lv_obj_has_flag(button, LV_OBJ_FLAG_HIDDEN)) continue;
        lv_obj_t *caption = lv_obj_get_child(button, 0);
        if (caption && !strcmp(lv_label_get_text(caption), label)) return button;
    }
    return NULL;
}
static void click(lv_obj_t *root, const char *label) {
    lv_obj_t *button = find_button(root, label); assert(button); assert(!lv_obj_has_state(button, LV_STATE_DISABLED));
    lv_obj_send_event(button, LV_EVENT_CLICKED, NULL);
}
static void click_symbol(gui08_native *native, const char *label) {
    lv_obj_t *button = find_symbol(native, label); assert(button); assert(!lv_obj_has_state(button, LV_STATE_DISABLED));
    lv_obj_send_event(button, LV_EVENT_CLICKED, NULL);
}
static void geometry(lv_obj_t *object) {
    if (lv_obj_has_flag(object, LV_OBJ_FLAG_HIDDEN)) return;
    lv_area_t area; lv_obj_get_coords(object, &area);
    assert(area.x1 >= 0 && area.y1 >= 0 && area.x2 < 480 && area.y2 < 800);
    if (lv_obj_check_type(object, &lv_button_class)) {
        assert(lv_obj_get_width(object) >= 44); assert(lv_obj_get_height(object) >= 44);
    }
    for (uint32_t i = 0; i < lv_obj_get_child_count(object); ++i) geometry(lv_obj_get_child(object, i));
}
static void assert_label_inset(lv_obj_t *panel, lv_obj_t *label) {
    lv_area_t outer, inner;
    lv_obj_get_coords(panel, &outer); lv_obj_get_coords(label, &inner);
    fprintf(stderr, "label inset panel=[%d,%d] label=[%d,%d]\n", outer.x1, outer.x2, inner.x1, inner.x2);
    assert(inner.x1 >= outer.x1 + 8);
    assert(inner.x2 <= outer.x2 - 8);
}
static void assert_button_label_contained(lv_obj_t *button, const char *expected) {
    lv_obj_t *caption = lv_obj_get_child(button, 0);
    lv_area_t outer, inner;
    assert(caption);
    assert(!strcmp(lv_label_get_text(caption), expected));
    lv_obj_get_coords(button, &outer);
    lv_obj_get_coords(caption, &inner);
    fprintf(stderr, "button label containment text='%s' button=[%d,%d] label=[%d,%d]\n",
            expected, outer.x1, outer.x2, inner.x1, inner.x2);
    assert(inner.x1 >= outer.x1);
    assert(inner.x2 <= outer.x2);
}
static void assert_right_inset(lv_obj_t *panel, lv_obj_t *object, int inset) {
    lv_area_t outer, inner;
    lv_obj_get_coords(panel, &outer);
    lv_obj_get_coords(object, &inner);
    fprintf(stderr, "right inset panel_x2=%d object_x2=%d required=%d\n", outer.x2, inner.x2, inset);
    assert(inner.x2 <= outer.x2 - inset);
}
static void assert_bottom_inset(lv_obj_t *panel, lv_obj_t *object, int inset) {
    lv_area_t outer, inner;
    lv_obj_get_coords(panel, &outer);
    lv_obj_get_coords(object, &inner);
    fprintf(stderr, "bottom inset panel_y2=%d object_y2=%d required=%d\n", outer.y2, inner.y2, inset);
    assert(inner.y2 <= outer.y2 - inset);
}
static void cards_contract(gui08_native *native) {
    gui08_native_show(native, GUI08_CARDS);
    assert(strstr(lv_label_get_text(native->status), "Word 1 of 12"));
    blocked = true; click_symbol(native, "A"); assert(native->editor.length == 0); blocked = false;
    for (unsigned i = 0; i < 4; ++i) click_symbol(native, "A");
    assert(strstr(lv_label_get_text(native->status), "Word 2 of 12"));
    for (unsigned i = 4; i < 47; ++i) click_symbol(native, "A");
    assert(gui08_ready(&native->editor));
    assert(strstr(lv_label_get_text(native->status), "Word 12 of 12"));
    assert(!find_button(native->panel, "Derive test result"));
    assert(!lv_obj_has_state(find_button(native->panel, "Continue to Passphrase"), LV_STATE_DISABLED));
    click(native->panel, "Continue to Passphrase");
    assert(submits == 1 && submitted.length == 47);
    assert(submitted.passphrase_marker == GUI08_PASSPHRASE_UNSPECIFIED);
    assert(native->editor.pending_id == 0);

    submitted.passphrase_marker = GUI08_PASSPHRASE_ACTIVE;
    assert(gui08_native_pending(native, &submitted));
    assert(native->editor.pending_id == submitted.request_id);
    assert(find_button(native->panel, "Cancel"));
    assert(lv_obj_has_state(find_button(native->panel, "Undo"), LV_STATE_DISABLED));
    assert(lv_obj_has_state(find_symbol(native, "A"), LV_STATE_DISABLED));
    click(native->panel, "Cancel");
    assert(cancels == 1 && cancelled_id == submitted.request_id);
    assert(native->editor.pending_id == 0);
    assert(strstr(lv_label_get_text(native->status), "Cancelled"));
    assert(!gui08_native_accept(native, &submitted));

    click(native->panel, "Continue to Passphrase");
    assert(submits == 2);
    submitted.passphrase_marker = GUI08_PASSPHRASE_EMPTY;
    assert(gui08_native_pending(native, &submitted));
    gui08_request wrong_marker = submitted;
    wrong_marker.passphrase_marker = GUI08_PASSPHRASE_ACTIVE;
    assert(!gui08_native_accept(native, &wrong_marker));
    assert(gui08_native_accept(native, &submitted));
    assert(native->editor.result_visible);
    click(native->panel, "Undo"); assert(!native->editor.result_visible);
    assert(!gui08_native_accept(native, &submitted));
    click(native->panel, "Clear"); assert(native->editor.length == 0);
}
static void remainder_and_paging_contract(gui08_native *native) {
    gui08_native_show(native, GUI08_BASES);

    click(native->panel, "8"); click(native->panel, "12");
    for (unsigned i = 0; i + 1 < gui08_target(&native->editor); ++i) click_symbol(native, "0");
    lv_obj_t *invalid8 = find_symbol(native, "4");
    assert(invalid8 && lv_obj_has_state(invalid8, LV_STATE_DISABLED));
    assert(!lv_obj_has_state(find_symbol(native, "3"), LV_STATE_DISABLED));
    lv_obj_send_event(invalid8, LV_EVENT_CLICKED, NULL);
    assert(native->editor.length + 1 == gui08_target(&native->editor));
    assert(strstr(lv_label_get_text(native->status), "not valid"));
    click_symbol(native, "3"); assert(gui08_ready(&native->editor));

    click(native->panel, "32");
    for (unsigned i = 0; i + 1 < gui08_target(&native->editor); ++i) click_symbol(native, "q");
    assert(lv_obj_has_state(find_symbol(native, "g"), LV_STATE_DISABLED));
    assert(!lv_obj_has_state(find_symbol(native, "8"), LV_STATE_DISABLED));

    click(native->panel, "64"); click(native->panel, "24");
    assert(find_button(native->panel, "ABC"));
    assert(find_button(native->panel, "abc"));
    assert(find_button(native->panel, "0-9+/"));
    assert(find_symbol(native, "A") && find_symbol(native, "Z"));
    click(native->panel, "abc"); assert(find_symbol(native, "a") && find_symbol(native, "z"));
    click(native->panel, "0-9+/"); assert(find_symbol(native, "0") && find_symbol(native, "+") && find_symbol(native, "/"));
    click(native->panel, "ABC");
    for (unsigned i = 0; i < 42; ++i) click_symbol(native, "A");
    assert(strstr(lv_label_get_text(native->status), "Remainder bit 1 of 4"));
    assert(find_symbol(native, "0") && find_symbol(native, "1"));
    assert(!find_symbol(native, "A") && !find_symbol(native, "2"));
    assert(!find_button(native->panel, "ABC") && !find_button(native->panel, "abc") && !find_button(native->panel, "0-9+/"));
    click_symbol(native, "0"); click_symbol(native, "1"); click_symbol(native, "0"); click_symbol(native, "1");
    assert(gui08_ready(&native->editor)); click(native->panel, "Continue to Passphrase");
    submitted.passphrase_marker = GUI08_PASSPHRASE_EMPTY;
    assert(gui08_native_pending(native, &submitted));
    assert(gui08_native_accept(native, &submitted)); assert(native->editor.result_visible);
    click(native->panel, "Clear"); assert(!native->editor.result_visible);
}
int main(void) {
    lv_init();
    lv_display_t *display = lv_display_create(480, 800); assert(display);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(display, flush);
    lv_display_set_buffers(display, pixels, NULL, sizeof pixels, LV_DISPLAY_RENDER_MODE_PARTIAL);
    gui08_native native;
    gui08_native_create(&native, lv_screen_active(), guard, submit, cancel);
    cards_contract(&native);
    remainder_and_paging_contract(&native);
    lv_obj_update_layout(lv_screen_active()); geometry(lv_screen_active());
    assert_button_label_contained(native.derive, "Continue to Passphrase");
    assert_right_inset(native.panel, native.derive, 8);
    assert_right_inset(native.panel, native.status, 8);
    assert_right_inset(native.panel, native.transcript, 8);
    assert_right_inset(native.panel, native.result, 8);
    assert_bottom_inset(native.panel, native.result, 8);
    assert_label_inset(native.panel, native.status); assert_label_inset(native.panel, native.transcript);
    gui08_native_hide(&native); assert(!gui08_native_visible(&native));
    puts("PASS native LVGL visual repair19: full Passphrase action containment, safe gutters, >=44px controls, pending/cancel/token/passphrase behavior");
    lv_deinit(); return 0;
}
