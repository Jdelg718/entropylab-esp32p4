#include "gui08_native.h"
#include <stdio.h>
#include "native_theme.h"
#include <string.h>

static const unsigned words[] = {12, 15, 18, 21, 24};
static const unsigned bases[] = {4, 8, 32, 64};

static gui08_native *owner(lv_event_t *event) { return lv_event_get_user_data(event); }
static bool blocked(gui08_native *native, lv_event_t *event) {
    return native->guard && native->guard(event);
}
static bool editing_blocked(gui08_native *native, lv_event_t *event) {
    return blocked(native, event) || native->editor.pending_id != 0;
}
static lv_obj_t *label(lv_obj_t *parent, int x, int y, int width, const char *value) {
    lv_obj_t *object = lv_label_create(parent);
    lv_obj_set_pos(object, x, y); lv_obj_set_width(object, width);
    lv_label_set_long_mode(object, LV_LABEL_LONG_WRAP); lv_label_set_text(object, value);
    return object;
}
static lv_obj_t *button(lv_obj_t *parent, int x, int y, int width, const char *value,
                        lv_event_cb_t callback, gui08_native *native) {
    lv_obj_t *object = lv_button_create(parent); el_native_surface(object,true);
    lv_obj_set_style_pad_all(object, 0, 0);
    lv_obj_set_pos(object, x, y); lv_obj_set_size(object, width, 44);
    lv_obj_t *caption = lv_label_create(object); lv_label_set_text(caption, value); lv_obj_center(caption);
    lv_obj_add_event_cb(object, callback, LV_EVENT_CLICKED, native);
    return object;
}
static void checked(lv_obj_t *object, bool value) {
    if (value) lv_obj_add_state(object, LV_STATE_CHECKED); else lv_obj_remove_state(object, LV_STATE_CHECKED);
}
static void enabled(lv_obj_t *object, bool value) {
    if (value) lv_obj_remove_state(object, LV_STATE_DISABLED); else lv_obj_add_state(object, LV_STATE_DISABLED);
}
static void visible(lv_obj_t *object, bool value) {
    if (value) lv_obj_remove_flag(object, LV_OBJ_FLAG_HIDDEN); else lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
}
static void clear_feedback(gui08_native *native) { native->feedback[0] = 0; }

static char symbol_for_slot(const gui08_native *native, unsigned slot) {
    const gui08_editor *editor = &native->editor;
    if (gui08_base64_bit_input(editor)) return slot < 2 ? "01"[slot] : 0;
    if (editor->method == GUI08_BASES && editor->base == 64) {
        static const unsigned starts[] = {0, 26, 52};
        static const unsigned counts[] = {26, 26, 12};
        return native->page < 3 && slot < counts[native->page]
                   ? gui08_symbol_at(editor, starts[native->page] + slot) : 0;
    }
    const unsigned offset = editor->method == GUI08_BASES ? native->page * 26u : 0;
    return gui08_symbol_at(editor, offset + slot);
}

static void refresh_pages(gui08_native *native) {
    const bool bit_input = gui08_base64_bit_input(&native->editor);
    const bool base64 = native->editor.method == GUI08_BASES && native->editor.base == 64 && !bit_input;
    const bool base32 = native->editor.method == GUI08_BASES && native->editor.base == 32;
    for (unsigned i = 0; i < 3; ++i) visible(native->page_buttons[i], base64 || base32);
    if (base64) {
        static const char *const names[] = {"ABC", "abc", "0-9+/"};
        for (unsigned i = 0; i < 3; ++i) {
            lv_label_set_text(lv_obj_get_child(native->page_buttons[i], 0), names[i]);
            enabled(native->page_buttons[i], true); checked(native->page_buttons[i], native->page == i);
        }
    } else if (base32) {
        lv_label_set_text(lv_obj_get_child(native->page_buttons[0], 0), "Previous");
        lv_label_set_text(lv_obj_get_child(native->page_buttons[1], 0), native->page ? "Page 2 of 2" : "Page 1 of 2");
        lv_label_set_text(lv_obj_get_child(native->page_buttons[2], 0), "Next");
        enabled(native->page_buttons[0], native->page > 0);
        enabled(native->page_buttons[1], false);
        enabled(native->page_buttons[2], native->page == 0);
        for (unsigned i = 0; i < 3; ++i) checked(native->page_buttons[i], false);
    }
}

static void refresh(gui08_native *native) {
    if (!native->editor.result_visible)
        fingerprint_view_blank(&native->fingerprint);
    const bool pending = native->editor.pending_id != 0;
    checked(native->method_buttons[0], native->editor.method == GUI08_CARDS);
    checked(native->method_buttons[1], native->editor.method == GUI08_BASES);
    for (unsigned i = 0; i < 2; ++i) enabled(native->method_buttons[i], !pending);
    for (unsigned i = 0; i < 5; ++i) {
        checked(native->word_buttons[i], native->editor.words == words[i]);
        enabled(native->word_buttons[i], !pending);
    }
    for (unsigned i = 0; i < 4; ++i) {
        checked(native->base_buttons[i], native->editor.method == GUI08_BASES && native->editor.base == bases[i]);
        visible(native->base_buttons[i], native->editor.method == GUI08_BASES);
        enabled(native->base_buttons[i], !pending);
    }
    for (unsigned i = 0; i < 26; ++i) {
        lv_obj_t *key = native->symbol_buttons[i];
        const char symbol = symbol_for_slot(native, i);
        if (symbol) {
            char text[2] = {symbol, 0};
            lv_label_set_text(lv_obj_get_child(key, 0), text); visible(key, true);
            enabled(key, !pending && gui08_symbol_allowed(&native->editor, symbol));
        } else visible(key, false);
    }
    refresh_pages(native);
    if (pending) for (unsigned i = 0; i < 3; ++i) enabled(native->page_buttons[i], false);
    enabled(native->undo, !pending);
    enabled(native->clear, !pending);
    lv_label_set_text(native->transcript, native->editor.length ? native->editor.transcript : "No symbols committed.");
    char status[192];
    if (pending) {
        snprintf(status, sizeof status, "Calculating... Passphrase %s.\nInput controls are locked to this exact request.",
                 native->editor.pending_passphrase_marker == GUI08_PASSPHRASE_ACTIVE ? "Active" : "Empty");
    } else if (native->feedback[0]) {
        snprintf(status, sizeof status, "%s", native->feedback);
    } else if (native->editor.method == GUI08_CARDS) {
        unsigned word = (unsigned)(native->editor.length / 4u) + 1u;
        if (word > native->editor.words) word = native->editor.words;
        snprintf(status, sizeof status, "Cards %u words: Word %u of %u; %u/%u ranks.\n%s",
                 native->editor.words, word, native->editor.words, (unsigned)native->editor.length,
                 (unsigned)gui08_target(&native->editor), gui08_cards_instruction(&native->editor));
    } else if (gui08_base64_bit_input(&native->editor) && !gui08_ready(&native->editor)) {
        const unsigned remainder = gui08_base64_remainder_bits(&native->editor);
        const unsigned entered = (unsigned)(native->editor.length - (gui08_target(&native->editor) - remainder));
        snprintf(status, sizeof status, "Base 64 / %u words: Remainder bit %u of %u.\nChoose only 0 or 1.",
                 native->editor.words, entered + 1u, remainder);
    } else {
        snprintf(status, sizeof status, "Base %u / %u words: %u/%u symbols.\nExact case; no trim/fold/padding/truncation.",
                 native->editor.base, native->editor.words, (unsigned)native->editor.length,
                 (unsigned)gui08_target(&native->editor));
    }
    lv_label_set_text(native->status, status);
    lv_label_set_text(lv_obj_get_child(native->derive, 0), pending ? "Cancel" : "Continue to Passphrase");
    enabled(native->derive, pending ? native->cancel != NULL : gui08_ready(&native->editor));
    lv_label_set_text(native->result, native->editor.result_visible ? "Test result accepted for this exact input." : "No result.");
}
static void method_event(lv_event_t *event) {
    gui08_native *native = owner(event); if (editing_blocked(native, event)) return;
    gui08_method method = lv_event_get_target(event) == native->method_buttons[0] ? GUI08_CARDS : GUI08_BASES;
    gui08_change_context(&native->editor, method, native->editor.words, method == GUI08_BASES ? 4 : 0);
    native->page = 0; clear_feedback(native); refresh(native);
}
static void word_event(lv_event_t *event) {
    gui08_native *native = owner(event); if (editing_blocked(native, event)) return;
    for (unsigned i = 0; i < 5; ++i) if (lv_event_get_target(event) == native->word_buttons[i])
        gui08_change_context(&native->editor, native->editor.method, words[i], native->editor.base);
    native->page = 0; clear_feedback(native); refresh(native);
}
static void base_event(lv_event_t *event) {
    gui08_native *native = owner(event); if (editing_blocked(native, event)) return;
    for (unsigned i = 0; i < 4; ++i) if (lv_event_get_target(event) == native->base_buttons[i])
        gui08_change_context(&native->editor, GUI08_BASES, native->editor.words, bases[i]);
    native->page = 0; clear_feedback(native); refresh(native);
}
static void symbol_event(lv_event_t *event) {
    gui08_native *native = owner(event); if (editing_blocked(native, event)) return;
    for (unsigned i = 0; i < 26; ++i) if (lv_event_get_target(event) == native->symbol_buttons[i]) {
        const char symbol = symbol_for_slot(native, i);
        const gui08_commit_status status = symbol ? gui08_commit(&native->editor, symbol) : GUI08_REJECTED_SYMBOL;
        if (status == GUI08_ACCEPTED) clear_feedback(native);
        else if (status == GUI08_REJECTED_EXTRA) snprintf(native->feedback, sizeof native->feedback, "Input is already complete; no extra symbol accepted.");
        else snprintf(native->feedback, sizeof native->feedback, "That symbol is not valid at this remainder position.");
    }
    refresh(native);
}
static void undo_event(lv_event_t *event) { gui08_native *native = owner(event); if (!editing_blocked(native, event)) { (void)gui08_undo(&native->editor); clear_feedback(native); refresh(native); } }
static void clear_event(lv_event_t *event) { gui08_native *native = owner(event); if (!editing_blocked(native, event)) { gui08_clear(&native->editor); native->page = 0; clear_feedback(native); refresh(native); } }
static void page_event(lv_event_t *event) {
    gui08_native *native = owner(event); if (editing_blocked(native, event)) return;
    if (native->editor.base == 64) {
        for (unsigned i = 0; i < 3; ++i) if (lv_event_get_target(event) == native->page_buttons[i]) native->page = i;
    } else if (native->editor.base == 32) {
        if (lv_event_get_target(event) == native->page_buttons[0] && native->page) --native->page;
        if (lv_event_get_target(event) == native->page_buttons[2] && native->page == 0) ++native->page;
    }
    clear_feedback(native); refresh(native);
}
static void derive_event(lv_event_t *event) {
    gui08_native *native = owner(event); if (blocked(native, event)) return;
    if (native->editor.pending_id) {
        const uint64_t request_id = native->editor.pending_id;
        if (native->cancel && native->cancel(request_id) && gui08_cancel(&native->editor, request_id))
            snprintf(native->feedback, sizeof native->feedback, "Cancelled. Input remains ready; no result was published.");
        refresh(native);
        return;
    }
    gui08_request request = gui08_begin(&native->editor);
    if (!request.request_id || !native->passphrase || !native->passphrase(&request))
        snprintf(native->feedback, sizeof native->feedback, "Unable to continue to Passphrase.");
    else clear_feedback(native);
    refresh(native);
}
void gui08_native_create(gui08_native *native, lv_obj_t *parent,
                         gui08_native_guard_fn guard, gui08_native_passphrase_fn passphrase,
                         gui08_native_cancel_fn cancel) {
    memset(native, 0, sizeof *native); native->guard = guard; native->passphrase = passphrase; native->cancel = cancel;
    gui08_editor_init(&native->editor, GUI08_CARDS, 12, 0);
    native->panel = lv_obj_create(parent); el_native_surface(native->panel,false); lv_obj_set_pos(native->panel, 16, 104); lv_obj_set_size(native->panel, 448, 660);
    lv_obj_remove_flag(native->panel, LV_OBJ_FLAG_SCROLLABLE);
    native->method_buttons[0] = button(native->panel, 8, 8, 100, "Cards", method_event, native);
    native->method_buttons[1] = button(native->panel, 116, 8, 100, "Bases", method_event, native);
    label(native->panel, 226, 18, 178, "Direct rank / canonical bases");
    for (unsigned i = 0; i < 5; ++i) { char text[4]; snprintf(text, sizeof text, "%u", words[i]); native->word_buttons[i] = button(native->panel, 8 + (int)i * 82, 56, 76, text, word_event, native); }
    for (unsigned i = 0; i < 4; ++i) { char text[4]; snprintf(text, sizeof text, "%u", bases[i]); native->base_buttons[i] = button(native->panel, 8 + (int)i * 102, 104, 94, text, base_event, native); }
    native->status = label(native->panel, 8, 148, 396, ""); lv_obj_set_height(native->status, 54);
    native->transcript = label(native->panel, 8, 206, 396, ""); lv_obj_set_height(native->transcript, 54);
    for (unsigned i = 0; i < 26; ++i) native->symbol_buttons[i] = button(native->panel, 8 + (int)(i % 9) * 45, 264 + (int)(i / 9) * 48, 44, "", symbol_event, native);
    native->page_buttons[0] = button(native->panel, 8, 408, 128, "", page_event, native);
    native->page_buttons[1] = button(native->panel, 144, 408, 128, "", page_event, native);
    native->page_buttons[2] = button(native->panel, 280, 408, 128, "", page_event, native);
    native->prev_page = native->page_buttons[0]; native->next_page = native->page_buttons[2];
    native->undo = button(native->panel, 8, 456, 92, "Undo", undo_event, native);
    native->clear = button(native->panel, 108, 456, 92, "Clear", clear_event, native);
    native->derive = button(native->panel, 208, 456, 200, "Continue to Passphrase", derive_event, native);
    native->result = label(native->panel, 8, 504, 396, "No result."); lv_obj_set_height(native->result, 52);
    fingerprint_view_create(&native->fingerprint, native->panel, 8, 588);
    el_native_body_dock(native->panel,native->clear,native->derive);
    refresh(native); gui08_native_hide(native);
}
void gui08_native_show(gui08_native *native, gui08_method method) {
    gui08_change_context(&native->editor, method, native->editor.words, method == GUI08_BASES ? (native->editor.base ? native->editor.base : 4) : 0);
    native->page = 0; clear_feedback(native); lv_obj_remove_flag(native->panel, LV_OBJ_FLAG_HIDDEN); refresh(native);
}
void gui08_native_hide(gui08_native *native) { lv_obj_add_flag(native->panel, LV_OBJ_FLAG_HIDDEN); }
bool gui08_native_visible(const gui08_native *native) { return !lv_obj_has_flag(native->panel, LV_OBJ_FLAG_HIDDEN); }
bool gui08_native_pending(gui08_native *native, const gui08_request *context) {
    const bool accepted = gui08_mark_pending(&native->editor, context);
    if (accepted) clear_feedback(native);
    refresh(native);
    return accepted;
}
bool gui08_native_accept(gui08_native *native, const gui08_request *context) {
    if (!context) return false;
    bool accepted = gui08_accept_result(&native->editor, context->request_id, context->revision,
                                        context->method, context->words, context->base,
                                        context->passphrase_marker);
    refresh(native); return accepted;
}
bool gui08_native_cancelled(gui08_native *native, uint64_t request_id) {
    bool accepted = gui08_cancel(&native->editor, request_id);
    if (accepted) snprintf(native->feedback, sizeof native->feedback,
                           "Conversion unavailable or failed; no result was published.");
    refresh(native); return accepted;
}
bool gui08_native_lifehash(gui08_native *native, const gui08_request *context,
                           const char fingerprint[9], const uint8_t *rgb,
                           size_t rgb_size) {
    if (!native || !context || !native->editor.result_visible ||
        context->request_id != native->editor.request_serial ||
        context->revision != native->editor.revision ||
        context->method != native->editor.method) return false;
    return fingerprint_view_publish(&native->fingerprint, fingerprint, rgb, rgb_size);
}
void gui08_native_lifehash_blank(gui08_native *native) {
    if (native) fingerprint_view_blank(&native->fingerprint);
}
