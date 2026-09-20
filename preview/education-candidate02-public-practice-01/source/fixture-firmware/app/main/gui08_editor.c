#include "gui08_editor.h"
#include "number_bases.h"
#include <string.h>

static const char *const BASE4 = "0123";
static const char *const BASE8 = "01234567";
static const char *const BASE32 = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
static const char *const BASE64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static unsigned entropy_bits(unsigned words) {
    switch (words) {
        case 12: return 128;
        case 15: return 160;
        case 18: return 192;
        case 21: return 224;
        case 24: return 256;
        default: return 0;
    }
}

static void invalidate(gui08_editor *editor) {
    editor->pending_id = 0;
    editor->pending_revision = 0;
    editor->pending_passphrase_marker = GUI08_PASSPHRASE_UNSPECIFIED;
    editor->result_visible = false;
    ++editor->revision;
    if (editor->revision == 0) ++editor->revision;
}

void gui08_editor_init(gui08_editor *editor, gui08_method method, unsigned words, unsigned base) {
    memset(editor, 0, sizeof *editor);
    editor->revision = 1;
    editor->method = method;
    editor->words = words;
    editor->base = base;
}

void gui08_change_context(gui08_editor *editor, gui08_method method, unsigned words, unsigned base) {
    if (editor->method == method && editor->words == words && editor->base == base) return;
    memset(editor->transcript, 0, sizeof editor->transcript);
    editor->length = 0;
    editor->method = method;
    editor->words = words;
    editor->base = base;
    invalidate(editor);
}

void gui08_clear(gui08_editor *editor) {
    volatile char *p = editor->transcript;
    for (size_t i = 0; i < sizeof editor->transcript; ++i) p[i] = 0;
    editor->length = 0;
    invalidate(editor);
}

bool gui08_undo(gui08_editor *editor) {
    if (!editor->length) return false;
    editor->transcript[--editor->length] = 0;
    invalidate(editor);
    return true;
}

const char *gui08_alphabet(const gui08_editor *editor) {
    if (editor->method != GUI08_BASES) return "A2345678";
    switch (editor->base) {
        case 4: return BASE4;
        case 8: return BASE8;
        case 32: return BASE32;
        case 64: return BASE64;
        default: return "";
    }
}

size_t gui08_target(const gui08_editor *editor) {
    if (editor->method == GUI08_BASES) return nb_encoded_length(editor->base, editor->words);
    if (editor->method != GUI08_CARDS || !entropy_bits(editor->words)) return 0;
    return (editor->words - 1u) * 4u + (editor->words == 12 ? 3u : editor->words == 24 ? 1u : 2u);
}

unsigned gui08_cards_radix(const gui08_editor *editor) {
    if (editor->method != GUI08_CARDS || editor->length >= gui08_target(editor)) return 0;
    const size_t partial = (editor->words - 1u) * 4u;
    if (editor->length < partial) return editor->length % 4u == 3u ? 4u : 8u;
    const size_t final = editor->length - partial;
    switch (editor->words) {
        case 12: { static const unsigned r[] = {8, 8, 2}; return r[final]; }
        case 15: return 8;
        case 18: { static const unsigned r[] = {8, 4}; return r[final]; }
        case 21: { static const unsigned r[] = {8, 2}; return r[final]; }
        case 24: return 8;
        default: return 0;
    }
}

char gui08_symbol_at(const gui08_editor *editor, unsigned index) {
    const char *alphabet = gui08_alphabet(editor);
    const size_t n = strlen(alphabet);
    return index < n ? alphabet[index] : 0;
}

static int digit_index(const char *alphabet, char symbol) {
    const char *p = strchr(alphabet, symbol);
    return p ? (int)(p - alphabet) : -1;
}

gui08_commit_status gui08_commit(gui08_editor *editor, char symbol) {
    const size_t target = gui08_target(editor);
    if (!target || editor->length >= target) return GUI08_REJECTED_EXTRA;
    if (editor->method == GUI08_CARDS) {
        const int value = symbol == 'A' ? 0 : symbol >= '2' && symbol <= '8' ? symbol - '1' : -1;
        if (value < 0) return GUI08_REJECTED_SYMBOL;
        if ((unsigned)value >= gui08_cards_radix(editor)) return GUI08_REJECTED_RANGE;
    } else if (editor->method == GUI08_BASES) {
        const char *alphabet = gui08_alphabet(editor);
        const int value = digit_index(alphabet, symbol);
        if (value < 0) return GUI08_REJECTED_SYMBOL;
        const unsigned bits = entropy_bits(editor->words);
        const unsigned width = editor->base == 4 ? 2 : editor->base == 8 ? 3 : editor->base == 32 ? 5 : 6;
        const unsigned full = bits / width;
        const unsigned remainder = bits % width;
        if (editor->base == 64 && remainder && editor->length >= full && symbol != '0' && symbol != '1')
            return GUI08_REJECTED_SYMBOL;
        if (editor->base != 64 && remainder && editor->length + 1 == target && (unsigned)value >= (1u << remainder))
            return GUI08_REJECTED_RANGE;
    } else return GUI08_REJECTED_SYMBOL;
    editor->transcript[editor->length++] = symbol;
    editor->transcript[editor->length] = 0;
    invalidate(editor);
    return GUI08_ACCEPTED;
}

bool gui08_ready(const gui08_editor *editor) {
    const size_t target = gui08_target(editor);
    return target && editor->length == target;
}

unsigned gui08_base64_remainder_bits(const gui08_editor *editor) {
    if (editor->method != GUI08_BASES || editor->base != 64) return 0;
    return entropy_bits(editor->words) % 6u;
}

bool gui08_base64_bit_input(const gui08_editor *editor) {
    const unsigned bits = entropy_bits(editor->words);
    const unsigned remainder = gui08_base64_remainder_bits(editor);
    return remainder && editor->length >= bits / 6u;
}

bool gui08_symbol_allowed(const gui08_editor *editor, char symbol) {
    gui08_editor probe = *editor;
    return gui08_commit(&probe, symbol) == GUI08_ACCEPTED;
}

const char *gui08_cards_instruction(const gui08_editor *editor) {
    switch (gui08_cards_radix(editor)) {
        case 8: return "Shuffle A-8 (any suit), then draw.";
        case 4: return "Shuffle A-4 (any suit), then draw.";
        case 2: return "Shuffle A-2 (any suit), then draw.";
        default: return "Checksum-valid seed ready to derive.";
    }
}

gui08_request gui08_begin(gui08_editor *editor) {
    gui08_request request;
    memset(&request, 0, sizeof request);
    if (!gui08_ready(editor)) return request;
    request.request_id = ++editor->request_serial;
    if (!request.request_id) request.request_id = ++editor->request_serial;
    request.revision = editor->revision;
    request.method = editor->method;
    request.words = editor->words;
    request.base = editor->base;
    request.passphrase_marker = GUI08_PASSPHRASE_UNSPECIFIED;
    request.length = editor->length;
    memcpy(request.transcript, editor->transcript, editor->length + 1);
    return request;
}

bool gui08_mark_pending(gui08_editor *editor, const gui08_request *request) {
    if (!request || !request->request_id || editor->pending_id ||
        request->passphrase_marker == GUI08_PASSPHRASE_UNSPECIFIED ||
        request->revision != editor->revision || request->method != editor->method ||
        request->words != editor->words || request->base != editor->base ||
        request->length != editor->length ||
        memcmp(request->transcript, editor->transcript, editor->length + 1)) return false;
    editor->pending_id = request->request_id;
    editor->pending_revision = request->revision;
    editor->pending_passphrase_marker = request->passphrase_marker;
    editor->result_visible = false;
    return true;
}

bool gui08_cancel(gui08_editor *editor, uint64_t request_id) {
    if (!request_id || editor->pending_id != request_id) return false;
    invalidate(editor);
    return true;
}

bool gui08_accept_result(gui08_editor *editor, uint64_t request_id, uint64_t revision,
                         gui08_method method, unsigned words, unsigned base,
                         gui08_passphrase_marker passphrase_marker) {
    if (!request_id || editor->pending_id != request_id || editor->pending_revision != revision ||
        editor->pending_passphrase_marker != passphrase_marker ||
        passphrase_marker == GUI08_PASSPHRASE_UNSPECIFIED || editor->revision != revision ||
        editor->method != method || editor->words != words || editor->base != base)
        return false;
    editor->pending_id = 0;
    editor->pending_revision = 0;
    editor->pending_passphrase_marker = GUI08_PASSPHRASE_UNSPECIFIED;
    editor->result_visible = true;
    return true;
}
