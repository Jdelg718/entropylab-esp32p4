/* Real LVGL -> application owner -> reviewed dispatcher -> worker test. */
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

extern "C" {
#include "gui08_native.h"
#include "lvgl.h"
}
#include "all_features_application.h"
#include "lifehash_fingerprint.h"

static uint16_t pixels[480 * 32];
static uint16_t framebuffer[480 * 800];
static gui08_native ui;
static bool queue_accept = true;
static uint64_t queued_token;
static unsigned lock_depth;
static gui08_passphrase_marker selected_marker = GUI08_PASSPHRASE_EMPTY;
static gui08_request converted;
static pc_result_v1 converted_result;
static unsigned conversions;
static int conversion_status;
static bool cancel_during_convert;

static void flush(lv_display_t *display, const lv_area_t *area, uint8_t *data) {
    const int width = area->x2 - area->x1 + 1;
    const int height = area->y2 - area->y1 + 1;
    const uint16_t *source = reinterpret_cast<const uint16_t *>(data);
    for (int y = 0; y < height; ++y)
        std::memcpy(&framebuffer[(area->y1 + y) * 480 + area->x1],
                    &source[y * width], (size_t)width * sizeof(uint16_t));
    lv_display_flush_ready(display);
}
static void capture(lv_display_t *display, const char *path) {
    lv_refr_now(display);
    FILE *file = std::fopen(path, "wb"); assert(file);
    std::fprintf(file, "P6\n480 800\n255\n");
    for (uint16_t pixel : framebuffer) {
        const uint8_t rgb[3] = {
            (uint8_t)(((pixel >> 11) & 31) * 255 / 31),
            (uint8_t)(((pixel >> 5) & 63) * 255 / 63),
            (uint8_t)((pixel & 31) * 255 / 31)};
        assert(std::fwrite(rgb, 1, sizeof rgb, file) == sizeof rgb);
    }
    assert(std::fclose(file) == 0);
}
static void lock_app() { assert(lock_depth++ == 0); }
static void unlock_app() { assert(lock_depth-- == 1); }
static bool publish(uint64_t token) {
    assert(lock_depth == 0);
    if (!queue_accept || queued_token) return false;
    queued_token = token;
    return true;
}
static gui08_passphrase_marker marker(const gui08_request*, uint8_t out[256], size_t *n) { std::memset(out,0,256); if(selected_marker==GUI08_PASSPHRASE_ACTIVE){out[0]='x';*n=1;}else *n=0; return selected_marker; }
static int derive(const gui08_request*, const pc_result_v1*, all_features_derivation_result *out) { std::memset(out,0,sizeof *out); std::strcpy(out->entropy,"00"); std::strcpy(out->fingerprint,"00000000"); std::strcpy(out->address,"bc1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"); return 0; }
static int render_lifehash(const all_features_lifehash_request *request,
                           all_features_lifehash_result *result) {
    assert(lock_depth == 0);
    result->route_epoch = request->route_epoch;
    result->request_id = request->request_id;
    result->revision = request->revision;
    return (int)lifehash_fingerprint_render(request->raw4, 4,
        LIFEHASH_FINGERPRINT_VERSION_2, result->rgb, sizeof result->rgb);
}
static bool image(const gui08_request *request, const char fingerprint[9],
                  const uint8_t *rgb, size_t size) {
    return gui08_native_lifehash(&ui, request, fingerprint, rgb, size);
}
static void blank() { gui08_native_lifehash_blank(&ui); }
static bool pending(const gui08_request *request) { return gui08_native_pending(&ui, request); }
static bool result(const gui08_request *request, const pc_result_v1 *value, const all_features_derivation_result*) {
    converted_result = *value;
    return gui08_native_accept(&ui, request);
}
static bool cancelled(uint64_t request_id) { return gui08_native_cancelled(&ui, request_id); }
static int convert(const gui08_request *request, pc_result_v1 *value) {
    converted = *request;
    ++conversions;
    std::memset(value, 0, sizeof *value);
    value->version = PC_V1_VERSION;
    value->mode = PC_V1_DIRECT;
    value->words = request->words;
    value->mnemonic_len = 1;
    value->mnemonic[0] = 'x';
    if (cancel_during_convert) {
        assert(all_features_application_cancel(request->request_id));
        assert(gui08_native_cancelled(&ui, request->request_id));
    }
    return conversion_status;
}
static bool guard(lv_event_t *) { return false; }

static lv_obj_t *find_button(lv_obj_t *object, const char *label) {
    if (!object || lv_obj_has_flag(object, LV_OBJ_FLAG_HIDDEN)) return nullptr;
    if (lv_obj_check_type(object, &lv_label_class) &&
        !std::strcmp(lv_label_get_text(object), label) &&
        lv_obj_check_type(lv_obj_get_parent(object), &lv_button_class))
        return lv_obj_get_parent(object);
    for (uint32_t i = 0; i < lv_obj_get_child_count(object); ++i)
        if (lv_obj_t *found = find_button(lv_obj_get_child(object, i), label)) return found;
    return nullptr;
}
static lv_obj_t *find_symbol(const char *label) {
    for (lv_obj_t *button : ui.symbol_buttons) {
        if (!button || lv_obj_has_flag(button, LV_OBJ_FLAG_HIDDEN)) continue;
        lv_obj_t *caption = lv_obj_get_child(button, 0);
        if (caption && !std::strcmp(lv_label_get_text(caption), label)) return button;
    }
    return nullptr;
}
static void click_button(const char *label) {
    lv_obj_t *button = find_button(ui.panel, label);
    if (!button || lv_obj_has_state(button, LV_STATE_DISABLED))
        std::fprintf(stderr, "click unavailable: %s (found=%d disabled=%d)\n", label,
                     button != nullptr,
                     button ? lv_obj_has_state(button, LV_STATE_DISABLED) : -1);
    assert(button && !lv_obj_has_state(button, LV_STATE_DISABLED));
    lv_obj_send_event(button, LV_EVENT_CLICKED, nullptr);
}
static void click_symbol(const char *label) {
    lv_obj_t *button = find_symbol(label);
    assert(button && !lv_obj_has_state(button, LV_STATE_DISABLED));
    lv_obj_send_event(button, LV_EVENT_CLICKED, nullptr);
}
static void fill_cards() {
    for (unsigned i = 0; i < gui08_target(&ui.editor); ++i) click_symbol("A");
}
static void fill_base4() {
    for (unsigned i = 0; i < gui08_target(&ui.editor); ++i) click_symbol("0");
}
static void work_and_poll() {
    assert(queued_token);
    const uint64_t token = queued_token;
    queued_token = 0;
    all_features_application_work(token, convert);
    all_features_application_poll();
}

int main() {
    lv_init();
    lv_display_t *display = lv_display_create(480, 800);
    assert(display);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(display, flush);
    lv_display_set_buffers(display, pixels, nullptr, sizeof pixels,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    gui08_native_create(&ui, lv_screen_active(), guard,
                        all_features_application_submit,
                        all_features_application_cancel);
    assert(all_features_application_configure(lock_app, unlock_app, publish, marker,
                                               derive, pending, result, cancelled));
    assert(all_features_application_lifehash_configure(render_lifehash, image, blank));

    gui08_native_show(&ui, GUI08_CARDS);
    fill_cards();
    queue_accept = false;
    click_button("Continue to Passphrase");
    assert(!ui.editor.pending_id && !queued_token && conversions == 0);
    assert(std::strstr(ui.feedback, "Unable"));

    queue_accept = true;
    selected_marker = GUI08_PASSPHRASE_ACTIVE;
    click_button("Continue to Passphrase");
    assert(ui.editor.pending_id && queued_token);
    assert(ui.editor.pending_passphrase_marker == GUI08_PASSPHRASE_ACTIVE);
    conversion_status = 0;
    work_and_poll();
    assert(conversions == 1 && converted.method == GUI08_CARDS);
    assert(converted.passphrase_marker == GUI08_PASSPHRASE_ACTIVE);
    assert(converted.length == 47 && converted.transcript[0] == 'A');
    assert(converted_result.mnemonic_len == 1 && converted_result.mnemonic[0] == 'x');
    assert(ui.editor.result_visible && !ui.editor.pending_id);
    assert(ui.fingerprint.visible);
    capture(display, "lifehash-result-480x800.ppm");

    click_button("Clear");
    assert(!ui.fingerprint.visible);
    capture(display, "lifehash-blank-480x800.ppm");
    click_button("Bases");
    assert(ui.editor.method == GUI08_BASES);
    fill_base4();
    selected_marker = GUI08_PASSPHRASE_EMPTY;
    click_button("Continue to Passphrase");
    assert(ui.editor.pending_id && queued_token);
    assert(ui.editor.pending_passphrase_marker == GUI08_PASSPHRASE_EMPTY);
    click_button("Cancel");
    assert(!ui.editor.pending_id && !ui.editor.result_visible);
    work_and_poll();
    assert(conversions == 1);  // queued token was revoked before claim

    click_button("Continue to Passphrase");
    assert(ui.editor.pending_id && queued_token);
    conversion_status = -95;  // explicit unsupported conversion
    work_and_poll();
    assert(conversions == 2 && converted.method == GUI08_BASES);
    assert(!ui.editor.pending_id && !ui.editor.result_visible);

    /* Running cancellation revokes both stale success and stale error. */
    cancel_during_convert = true;
    conversion_status = 0;
    click_button("Continue to Passphrase");
    work_and_poll();
    assert(conversions == 3 && !ui.editor.pending_id && !ui.editor.result_visible);
    conversion_status = -95;
    click_button("Continue to Passphrase");
    work_and_poll();
    assert(conversions == 4 && !ui.editor.pending_id && !ui.editor.result_visible);
    cancel_during_convert = false;

    /* A newer method/revision cannot receive either old completion. */
    click_button("Cards");
    assert(ui.editor.method == GUI08_CARDS && !ui.editor.result_visible);

    all_features_application_dispose();
    lv_deinit();
    std::puts("PASS cards23 real GUI queue-full rollback, post-publish pending, copied ownership, cancel/stale suppression, method switch, success/error payload");
    return 0;
}
