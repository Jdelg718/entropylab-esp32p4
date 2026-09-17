#include "seed_native.h"

#include <stdio.h>
#include "native_theme.h"
#include <stdlib.h>
#include <string.h>

static const unsigned word_counts[] = {12, 15, 18, 21, 24};

static seed_native *owner(lv_event_t *event) { return lv_event_get_user_data(event); }
static bool blocked(seed_native *native, lv_event_t *event) {
    return (native->guard && native->guard(event)) || native->editor.pending_id != 0;
}
static lv_obj_t *label(lv_obj_t *parent, int x, int y, int width,
                       const char *value) {
    lv_obj_t *object = lv_label_create(parent);
    lv_obj_set_pos(object, x, y);
    lv_obj_set_width(object, width);
    lv_label_set_long_mode(object, LV_LABEL_LONG_WRAP);
    lv_label_set_text(object, value);
    return object;
}
static lv_obj_t *button(lv_obj_t *parent, int x, int y, int width,
                        const char *value, lv_event_cb_t callback,
                        seed_native *native) {
    lv_obj_t *object = lv_button_create(parent); el_native_surface(object,true);
    /* Seed has several compact >=44px controls. LVGL's theme button padding
     * can collapse their content box beneath a valid caption (for example,
     * Delete on the 58px key), so make caption containment deterministic. */
    lv_obj_set_style_pad_all(object, 0, 0);
    lv_obj_set_pos(object, x, y);
    lv_obj_set_size(object, width, 44);
    lv_obj_t *caption = lv_label_create(object);
    lv_label_set_text(caption, value);
    lv_obj_center(caption);
    lv_obj_add_event_cb(object, callback, LV_EVENT_CLICKED, native);
    return object;
}
static void enabled(lv_obj_t *object, bool value) {
    if (value) lv_obj_remove_state(object, LV_STATE_DISABLED);
    else lv_obj_add_state(object, LV_STATE_DISABLED);
}
static void visible(lv_obj_t *object, bool value) {
    if (value) lv_obj_remove_flag(object, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
}
static void checked(lv_obj_t *object, bool value) {
    if (value) lv_obj_add_state(object, LV_STATE_CHECKED);
    else lv_obj_remove_state(object, LV_STATE_CHECKED);
}
static void invalidate(seed_native *native) {
    native->editor.pending_id = 0;
    native->editor.pending_revision = 0;
    native->editor.pending_passphrase_marker = GUI08_PASSPHRASE_UNSPECIFIED;
    native->editor.result_visible = false;
    if (++native->editor.revision == 0) ++native->editor.revision;
}
static void clear_candidates(seed_native *native) {
    memset(native->candidates, 0, sizeof native->candidates);
    native->candidate_count = 0;
    native->candidate_page = 0;
}
static bool append_text(char *out, size_t capacity, size_t *written,
                        const char *text) {
    const size_t length = strlen(text);
    if (*written + length >= capacity) return false;
    memcpy(out + *written, text, length);
    *written += length;
    out[*written] = 0;
    return true;
}
static bool build_words(const seed_native *native, unsigned count,
                        char out[216]) {
    size_t written = 0;
    out[0] = 0;
    for (unsigned i = 0; i < count; ++i) {
        const char *word = seed_helpers_word_at(native->indices[i]);
        if (!word || (i && !append_text(out, 216, &written, " ")) ||
            !append_text(out, 216, &written, word)) return false;
    }
    return true;
}
static bool build_numbers(const seed_native *native, char out[1025]) {
    size_t written = 0;
    out[0] = 0;
    for (unsigned i = 0; i < native->committed; ++i) {
        char number[8];
        snprintf(number, sizeof number, "%u", native->indices[i] + native->editor.base);
        if ((i && !append_text(out, 1025, &written, " ")) ||
            !append_text(out, 1025, &written, number)) return false;
    }
    return true;
}
static void load_candidates(seed_native *native) {
    clear_candidates(native);
    if (native->committed + 1 != native->editor.words) return;
    char prefix[216];
    if (!build_words(native, native->committed, prefix)) return;
    sh_output_v1 output;
    memset(&output, 0, sizeof output);
    if (seed_helpers_target_v1((const uint8_t *)prefix, strlen(prefix),
                               SH_FINAL_WORDS, native->editor.words, 0,
                               &output, sizeof output) != SH_OK) {
        snprintf(native->feedback, sizeof native->feedback,
                 "Final-word validator unavailable.");
        return;
    }
    native->candidate_count = output.number_count;
    memcpy(native->candidates, output.numbers,
           output.number_count * sizeof output.numbers[0]);
}
static void sync_transcript(seed_native *native) {
    memset(native->editor.transcript, 0, sizeof native->editor.transcript);
    if (!build_numbers(native, native->editor.transcript)) return;
    native->editor.length = strlen(native->editor.transcript);
}
static void refresh(seed_native *native) {
    if (!native->editor.result_visible)
        fingerprint_view_blank(&native->fingerprint);
    const bool pending = native->editor.pending_id != 0;
    for (unsigned i = 0; i < 5; ++i) {
        checked(native->word_buttons[i], native->editor.words == word_counts[i]);
        enabled(native->word_buttons[i], !pending);
    }
    lv_label_set_text(lv_obj_get_child(native->range, 0),
                      native->editor.base ? "0-2047" : "1-2048");
    lv_label_set_text(native->range_status,
                      native->editor.base ? "Using 1-2048" : "Using 0-2047");
    enabled(native->range, !pending);
    for (unsigned i = 0; i < 10; ++i) enabled(native->digits[i], !pending);
    enabled(native->delete_key, !pending && native->draft_len != 0);
    enabled(native->next_word, !pending && native->draft_len != 0);
    enabled(native->clear, !pending && (native->committed || native->draft_len));

    char text[256];
    const unsigned shown_word = native->committed < native->editor.words
                                    ? native->committed + 1 : native->editor.words;
    snprintf(text, sizeof text, "Word %u of %u / %u committed",
             shown_word, native->editor.words, native->committed);
    lv_label_set_text(native->progress, text);
    if (native->draft_len) {
        const unsigned value = (unsigned)strtoul(native->draft, NULL, 10);
        const unsigned base = native->editor.base;
        const char *word = value >= base && value <= 2047u + base
                               ? seed_helpers_word_at(value - base) : NULL;
        snprintf(text, sizeof text, "%s -> %s", native->draft,
                 word ? word : "outside selected range");
    } else {
        snprintf(text, sizeof text, "Enter a number; mapping preview appears here.");
    }
    lv_label_set_text(native->preview, text);

    const unsigned pages = (native->candidate_count + 5u) / 6u;
    for (unsigned i = 0; i < 6; ++i) {
        const unsigned at = native->candidate_page * 6u + i;
        const bool present = at < native->candidate_count;
        visible(native->candidate_buttons[i], present);
        if (present) {
            const char *word = seed_helpers_word_at(native->candidates[at]);
            lv_label_set_text(lv_obj_get_child(native->candidate_buttons[i], 0), word ? word : "?");
            enabled(native->candidate_buttons[i], !pending);
        }
    }
    visible(native->previous_page, native->candidate_count != 0);
    visible(native->next_page, native->candidate_count != 0);
    enabled(native->previous_page, !pending && native->candidate_page != 0);
    enabled(native->next_page, !pending && native->candidate_page + 1u < pages);

    if (pending) {
        snprintf(text, sizeof text,
                 "Calculating... Passphrase %s. Input controls are locked.",
                 native->editor.pending_passphrase_marker == GUI08_PASSPHRASE_ACTIVE
                     ? "Active" : "Empty");
    } else if (native->feedback[0]) {
        snprintf(text, sizeof text, "%s", native->feedback);
    } else if (seed_native_ready(native)) {
        snprintf(text, sizeof text, "Checksum valid / ready to derive");
    } else if (native->candidate_count) {
        snprintf(text, sizeof text, "Choose the %uth word / %u valid",
                 native->editor.words, native->candidate_count);
    } else {
        snprintf(text, sizeof text,
                 "Word numbers / 1-2048 default / optional 0-2047. No randomness claim.");
    }
    lv_label_set_text(native->status, text);
    lv_label_set_text(native->transcript,
                      native->editor.length ? native->editor.transcript : "No numbers committed.");
    lv_label_set_text(lv_obj_get_child(native->derive, 0),
                      pending ? "Cancel" : "Continue to Passphrase");
    enabled(native->derive, pending ? native->cancel != NULL : seed_native_ready(native));
}
static void word_event(lv_event_t *event) {
    seed_native *native = owner(event);
    if (blocked(native, event)) return;
    for (unsigned i = 0; i < 5; ++i) {
        if (lv_event_get_target(event) == native->word_buttons[i] &&
            native->editor.words != word_counts[i]) {
            native->editor.words = word_counts[i];
            native->committed = native->draft_len = 0;
            memset(native->indices, 0, sizeof native->indices);
            memset(native->draft, 0, sizeof native->draft);
            native->editor.length = 0;
            native->editor.transcript[0] = 0;
            clear_candidates(native);
            native->feedback[0] = 0;
            invalidate(native);
        }
    }
    refresh(native);
}
static void range_event(lv_event_t *event) {
    seed_native *native = owner(event);
    if (blocked(native, event)) return;
    native->editor.base = native->editor.base ? 0u : 1u;
    sync_transcript(native);
    native->feedback[0] = 0;
    invalidate(native);
    refresh(native);
}
static void digit_event(lv_event_t *event) {
    seed_native *native = owner(event);
    if (blocked(native, event) || native->draft_len >= 4) return;
    for (unsigned i = 0; i < 10; ++i) if (lv_event_get_target(event) == native->digits[i]) {
        native->draft[native->draft_len++] = (char)('0' + i);
        native->draft[native->draft_len] = 0;
        native->feedback[0] = 0;
    }
    refresh(native);
}
static void delete_event(lv_event_t *event) {
    seed_native *native = owner(event);
    if (blocked(native, event) || !native->draft_len) return;
    native->draft[--native->draft_len] = 0;
    native->feedback[0] = 0;
    refresh(native);
}
static bool candidate_contains(const seed_native *native, uint16_t index) {
    for (unsigned i = 0; i < native->candidate_count; ++i)
        if (native->candidates[i] == index) return true;
    return false;
}
static bool validate_complete(seed_native *native) {
    sync_transcript(native);
    pc_result_v1 result;
    memset(&result, 0, sizeof result);
    return seed_target_convert(SH_NUMBERS_TO_WORDS, native->editor.words,
                               native->editor.base, 0,
                               (const uint8_t *)native->editor.transcript,
                               native->editor.length, &result) == SH_OK;
}
static void commit_index(seed_native *native, uint16_t index) {
    if (native->committed >= native->editor.words) return;
    native->indices[native->committed++] = index;
    native->draft_len = 0;
    native->draft[0] = 0;
    native->feedback[0] = 0;
    clear_candidates(native);
    if (native->committed + 1 == native->editor.words) load_candidates(native);
    sync_transcript(native);
    invalidate(native);
    if (native->committed == native->editor.words && !validate_complete(native)) {
        --native->committed;
        sync_transcript(native);
        load_candidates(native);
        snprintf(native->feedback, sizeof native->feedback,
                 "That final number is not checksum-valid.");
    }
}
static void next_event(lv_event_t *event) {
    seed_native *native = owner(event);
    if (blocked(native, event) || !native->draft_len) return;
    const unsigned value = (unsigned)strtoul(native->draft, NULL, 10);
    const unsigned base = native->editor.base;
    if (value < base || value > 2047u + base) {
        snprintf(native->feedback, sizeof native->feedback,
                 "Invalid number: use %u-%u.", base, 2047u + base);
    } else {
        const uint16_t index = (uint16_t)(value - base);
        if (native->committed + 1 == native->editor.words &&
            !candidate_contains(native, index)) {
            snprintf(native->feedback, sizeof native->feedback,
                     "That final number is not checksum-valid.");
        } else commit_index(native, index);
    }
    refresh(native);
}
static void candidate_event(lv_event_t *event) {
    seed_native *native = owner(event);
    if (blocked(native, event)) return;
    for (unsigned i = 0; i < 6; ++i) if (lv_event_get_target(event) == native->candidate_buttons[i]) {
        const unsigned at = native->candidate_page * 6u + i;
        if (at < native->candidate_count) commit_index(native, native->candidates[at]);
    }
    refresh(native);
}
static void page_event(lv_event_t *event) {
    seed_native *native = owner(event);
    if (blocked(native, event)) return;
    if (lv_event_get_target(event) == native->previous_page && native->candidate_page)
        --native->candidate_page;
    if (lv_event_get_target(event) == native->next_page &&
        (native->candidate_page + 1u) * 6u < native->candidate_count)
        ++native->candidate_page;
    refresh(native);
}
static void clear_event(lv_event_t *event) {
    seed_native *native = owner(event);
    if (blocked(native, event)) return;
    volatile unsigned char *bytes = (volatile unsigned char *)native->indices;
    for (size_t i = 0; i < sizeof native->indices; ++i) bytes[i] = 0;
    native->committed = native->draft_len = 0;
    memset(native->draft, 0, sizeof native->draft);
    native->editor.length = 0;
    memset(native->editor.transcript, 0, sizeof native->editor.transcript);
    clear_candidates(native);
    native->feedback[0] = 0;
    invalidate(native);
    refresh(native);
}
static gui08_request begin(seed_native *native) {
    gui08_request request;
    memset(&request, 0, sizeof request);
    if (!seed_native_ready(native)) return request;
    request.request_id = ++native->editor.request_serial;
    if (!request.request_id) request.request_id = ++native->editor.request_serial;
    request.revision = native->editor.revision;
    request.method = GUI08_SEED;
    request.words = native->editor.words;
    request.base = native->editor.base;
    request.seed_operation = SH_NUMBERS_TO_WORDS;
    request.length = native->editor.length;
    memcpy(request.transcript, native->editor.transcript, request.length + 1);
    return request;
}
static void derive_event(lv_event_t *event) {
    seed_native *native = owner(event);
    if (native->guard && native->guard(event)) return;
    if (native->editor.pending_id) {
        const uint64_t id = native->editor.pending_id;
        if (native->cancel && native->cancel(id) && seed_native_cancelled(native, id))
            snprintf(native->feedback, sizeof native->feedback,
                     "Cancelled. Input remains ready; no result was published.");
        refresh(native);
        return;
    }
    gui08_request request = begin(native);
    if (!request.request_id || !native->passphrase || !native->passphrase(&request))
        snprintf(native->feedback, sizeof native->feedback,
                 "Unable to continue to Passphrase.");
    else native->feedback[0] = 0;
    refresh(native);
}
void seed_native_create(seed_native *native, lv_obj_t *parent,
                        gui08_native_guard_fn guard,
                        gui08_native_passphrase_fn passphrase,
                        gui08_native_cancel_fn cancel) {
    memset(native, 0, sizeof *native);
    native->guard = guard;
    native->passphrase = passphrase;
    native->cancel = cancel;
    gui08_editor_init(&native->editor, GUI08_SEED, 12, 1);
    native->panel = lv_obj_create(parent); el_native_surface(native->panel,false);
    /* Keep the accepted panel geometry while making its usable bottom inset
     * explicit; the theme's larger default padding excluded the final status. */
    lv_obj_set_style_pad_bottom(native->panel, 8, 0);
    lv_obj_set_pos(native->panel, 16, 104);
    lv_obj_set_size(native->panel, 448, 660);
    lv_obj_remove_flag(native->panel, LV_OBJ_FLAG_SCROLLABLE);
    label(native->panel, 8, 6, 70, "Seed");
    label(native->panel, 82, 6, 146, "Word numbers");
    /* Passphrase distinction is explained by the Continue action and About. */
    for (unsigned i = 0; i < 5; ++i) {
        char count[4]; snprintf(count, sizeof count, "%u", word_counts[i]);
        native->word_buttons[i] = button(native->panel, 8 + (int)i * 82, 34, 76,
                                         count, word_event, native);
    }
    native->range_status = label(native->panel, 8, 84, 108, "");
    native->range = button(native->panel, 122, 78, 100, "0-2047", range_event, native);
    native->progress = label(native->panel, 230, 84, 174, "");
    native->preview = label(native->panel, 8, 128, 396, "");
    native->status = label(native->panel, 8, 158, 396, "");
    lv_obj_set_height(native->status, 48);
    for (unsigned i = 0; i < 9; ++i) {
        char digit[2] = {(char)('1' + i), 0};
        native->digits[i + 1] = button(native->panel, 8 + (int)(i % 3) * 64,
                                       210 + (int)(i / 3) * 48, 58, digit,
                                       digit_event, native);
    }
    native->delete_key = button(native->panel, 8, 354, 58, "Delete", delete_event, native);
    native->digits[0] = button(native->panel, 72, 354, 58, "0", digit_event, native);
    native->next_word = button(native->panel, 136, 354, 92, "Next word", next_event, native);
    label(native->panel, 238, 210, 166, "Valid last-word picker");
    for (unsigned i = 0; i < 6; ++i)
        native->candidate_buttons[i] = button(native->panel,
            238 + (int)(i % 2) * 82, 240 + (int)(i / 2) * 48, 78, "",
            candidate_event, native);
    native->previous_page = button(native->panel, 238, 390, 84, "Previous", page_event, native);
    native->next_page = button(native->panel, 320, 390, 84, "Next", page_event, native);
    native->transcript = label(native->panel, 8, 444, 396, "");
    lv_obj_set_height(native->transcript, 54);
    native->clear = button(native->panel, 8, 508, 92, "Clear", clear_event, native);
    native->derive = button(native->panel, 108, 508, 296,
                            "Continue to Passphrase", derive_event, native);
    label(native->panel, 8, 558, 396,
          "Public TEST input only / NEVER fund. Clear is best-effort UI clearing.");
    fingerprint_view_create(&native->fingerprint, native->panel, 8, 604);
    el_native_body_dock(native->panel,native->clear,native->derive);
    refresh(native);
    seed_native_hide(native);
}
void seed_native_show(seed_native *native) {
    lv_obj_remove_flag(native->panel, LV_OBJ_FLAG_HIDDEN);
    refresh(native);
}
void seed_native_hide(seed_native *native) {
    lv_obj_add_flag(native->panel, LV_OBJ_FLAG_HIDDEN);
}
bool seed_native_visible(const seed_native *native) {
    return native && native->panel && !lv_obj_has_flag(native->panel, LV_OBJ_FLAG_HIDDEN);
}
bool seed_native_ready(const seed_native *native) {
    return native && native->committed == native->editor.words &&
           native->editor.length != 0;
}
bool seed_native_source_matches(const seed_native *native,
                                const gui08_request *request) {
    return native && request && request->request_id &&
           request->method == GUI08_SEED && request->seed_operation == SH_NUMBERS_TO_WORDS &&
           request->revision == native->editor.revision &&
           request->words == native->editor.words && request->base == native->editor.base &&
           request->length == native->editor.length &&
           !memcmp(request->transcript, native->editor.transcript,
                   native->editor.length + 1);
}
bool seed_native_pending(seed_native *native, const gui08_request *request) {
    if (!seed_native_source_matches(native, request) || native->editor.pending_id ||
        request->passphrase_marker == GUI08_PASSPHRASE_UNSPECIFIED) return false;
    native->editor.pending_id = request->request_id;
    native->editor.pending_revision = request->revision;
    native->editor.pending_passphrase_marker = request->passphrase_marker;
    native->editor.result_visible = false;
    refresh(native);
    return true;
}
bool seed_native_accept(seed_native *native, const gui08_request *request) {
    if (!seed_native_source_matches(native, request)) return false;
    const bool accepted = gui08_accept_result(&native->editor, request->request_id,
        request->revision, request->method, request->words, request->base,
        request->passphrase_marker);
    refresh(native);
    return accepted;
}
bool seed_native_cancelled(seed_native *native, uint64_t request_id) {
    const bool accepted = gui08_cancel(&native->editor, request_id);
    if (accepted) snprintf(native->feedback, sizeof native->feedback,
                           "Conversion unavailable or failed; no result was published.");
    refresh(native);
    return accepted;
}
bool seed_native_lifehash(seed_native *native, const gui08_request *request,
                          const char fingerprint[9], const uint8_t *rgb,
                          size_t rgb_size) {
    if (!native || !request || !native->editor.result_visible ||
        !seed_native_source_matches(native, request)) return false;
    return fingerprint_view_publish(&native->fingerprint, fingerprint, rgb, rgb_size);
}
void seed_native_lifehash_blank(seed_native *native) {
    if (native) fingerprint_view_blank(&native->fingerprint);
}
