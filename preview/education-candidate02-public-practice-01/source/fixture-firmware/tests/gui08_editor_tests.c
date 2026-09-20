#include "gui08_editor.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void cards_contract(void) {
    const unsigned words[] = {12, 15, 18, 21, 24};
    const size_t totals[] = {47, 58, 70, 82, 93};
    const unsigned final_radix[] = {2, 8, 4, 2, 8};
    for (size_t i = 0; i < 5; ++i) {
        gui08_editor e;
        gui08_editor_init(&e, GUI08_CARDS, words[i], 0);
        assert(gui08_target(&e) == totals[i]);
        assert(gui08_cards_radix(&e) == 8);
        while (e.length + 1 < totals[i]) assert(gui08_commit(&e, 'A') == GUI08_ACCEPTED);
        assert(gui08_cards_radix(&e) == final_radix[i]);
        assert(gui08_commit(&e, '8') == (final_radix[i] == 8 ? GUI08_ACCEPTED : GUI08_REJECTED_RANGE));
        if (!gui08_ready(&e)) assert(gui08_commit(&e, 'A') == GUI08_ACCEPTED);
        assert(gui08_ready(&e));
        assert(gui08_commit(&e, 'A') == GUI08_REJECTED_EXTRA);
        assert(gui08_undo(&e));
        assert(!gui08_ready(&e));
    }
}

static void bases_contract(void) {
    const unsigned bases[] = {4, 8, 32, 64};
    const char *alphabet[] = {"0123", "01234567", "qpzry9x8gf2tvdw0s3jn54khce6mua7l", "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};
    for (size_t i = 0; i < 4; ++i) {
        gui08_editor e;
        gui08_editor_init(&e, GUI08_BASES, 12, bases[i]);
        assert(strcmp(gui08_alphabet(&e), alphabet[i]) == 0);
        assert(gui08_commit(&e, ' ') == GUI08_REJECTED_SYMBOL);
        while (e.length < gui08_target(&e)) {
            char symbol = bases[i] == 64 && e.length >= 128 / 6 ? '0' : gui08_symbol_at(&e, 0);
            assert(gui08_commit(&e, symbol) == GUI08_ACCEPTED);
        }
        assert(gui08_ready(&e));
    }
    gui08_editor b8, b32, b64;
    gui08_editor_init(&b8, GUI08_BASES, 12, 8);
    while (b8.length + 1 < gui08_target(&b8)) assert(gui08_commit(&b8, '0') == GUI08_ACCEPTED);
    assert(gui08_commit(&b8, '4') == GUI08_REJECTED_RANGE);
    assert(gui08_commit(&b8, '3') == GUI08_ACCEPTED);
    gui08_editor_init(&b32, GUI08_BASES, 12, 32);
    while (b32.length + 1 < gui08_target(&b32)) assert(gui08_commit(&b32, 'q') == GUI08_ACCEPTED);
    assert(gui08_commit(&b32, 'g') == GUI08_REJECTED_RANGE);
    assert(gui08_commit(&b32, '8') == GUI08_ACCEPTED);
    gui08_editor_init(&b64, GUI08_BASES, 12, 64);
    while (b64.length < 21) assert(gui08_commit(&b64, 'A') == GUI08_ACCEPTED);
    assert(gui08_commit(&b64, 'B') == GUI08_REJECTED_SYMBOL);
    assert(gui08_commit(&b64, '1') == GUI08_ACCEPTED);
    assert(gui08_commit(&b64, '0') == GUI08_ACCEPTED);
    assert(gui08_ready(&b64));
}

static void invalidation_and_stale_callback(void) {
    gui08_editor e;
    gui08_editor_init(&e, GUI08_CARDS, 12, 0);
    assert(gui08_commit(&e, 'A') == GUI08_ACCEPTED);
    uint64_t old_revision = e.revision;
    gui08_request r = gui08_begin(&e);
    assert(!gui08_accept_result(&e, r.request_id, r.revision, GUI08_CARDS, 12, 0, GUI08_PASSPHRASE_ACTIVE));
    gui08_change_context(&e, GUI08_BASES, 15, 32);
    assert(e.length == 0 && e.revision > old_revision);
    assert(!gui08_accept_result(&e, r.request_id, r.revision, GUI08_CARDS, 12, 0, GUI08_PASSPHRASE_ACTIVE));
    while (e.length < gui08_target(&e)) assert(gui08_commit(&e, 'q') == GUI08_ACCEPTED);
    r = gui08_begin(&e);
    r.passphrase_marker = GUI08_PASSPHRASE_EMPTY;
    assert(gui08_mark_pending(&e, &r));
    assert(gui08_accept_result(&e, r.request_id, r.revision, GUI08_BASES, 15, 32, GUI08_PASSPHRASE_EMPTY));
    gui08_clear(&e);
    assert(!gui08_accept_result(&e, r.request_id, r.revision, GUI08_BASES, 15, 32, GUI08_PASSPHRASE_EMPTY));
}

int main(void) {
    cards_contract();
    bases_contract();
    invalidation_and_stale_callback();
    puts("PASS gui08 editor: Cards/Bases legal+illegal, Undo, Clear, stale callback");
    return 0;
}
