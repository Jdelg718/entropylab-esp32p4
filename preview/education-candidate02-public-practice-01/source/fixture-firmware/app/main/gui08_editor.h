#ifndef ENTROPYLAB_GUI08_EDITOR_H
#define ENTROPYLAB_GUI08_EDITOR_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    GUI08_CARDS = 1,
    GUI08_BASES = 2,
    /* Seed Helpers remain a distinct application route, not an el_mode_t. */
    GUI08_SEED = 3,
    /* BitBox and D++ are distinct from the legacy hashed/raw D6 modes. */
    GUI08_DICE = 4
} gui08_method;
typedef enum {
    GUI08_DICE_BITBOX = 1,
    GUI08_DICE_DPLUS = 2
} gui08_dice_method;
typedef enum {
    GUI08_PASSPHRASE_UNSPECIFIED = 0,
    GUI08_PASSPHRASE_EMPTY = 1,
    GUI08_PASSPHRASE_ACTIVE = 2
} gui08_passphrase_marker;
typedef enum {
    GUI08_ACCEPTED = 0,
    GUI08_REJECTED_SYMBOL,
    GUI08_REJECTED_RANGE,
    GUI08_REJECTED_EXTRA
} gui08_commit_status;

typedef struct {
    gui08_method method;
    unsigned words;
    unsigned base;
    char transcript[1025];
    size_t length;
    uint64_t revision;
    uint64_t request_serial;
    uint64_t pending_id;
    uint64_t pending_revision;
    gui08_passphrase_marker pending_passphrase_marker;
    bool result_visible;
} gui08_editor;

typedef struct {
    uint64_t request_id;
    uint64_t revision;
    gui08_method method;
    unsigned words;
    unsigned base;
    /* SH_* for GUI08_SEED, zero for Cards/Bases. */
    unsigned seed_operation;
    /* Extra-dice selector and explicit checksum-candidate choice. D++ keeps
     * its word-count-specific D8/D16 final rolls as copied request bytes. */
    unsigned dice_method;
    unsigned dice_final_choice;
    char dice_final[3];
    size_t dice_final_length;
    gui08_passphrase_marker passphrase_marker;
    uint8_t passphrase[256];
    size_t passphrase_len;
    size_t length;
    char transcript[1025];
} gui08_request;

void gui08_editor_init(gui08_editor *editor, gui08_method method, unsigned words, unsigned base);
void gui08_change_context(gui08_editor *editor, gui08_method method, unsigned words, unsigned base);
void gui08_clear(gui08_editor *editor);
bool gui08_undo(gui08_editor *editor);
gui08_commit_status gui08_commit(gui08_editor *editor, char symbol);
size_t gui08_target(const gui08_editor *editor);
bool gui08_ready(const gui08_editor *editor);
unsigned gui08_cards_radix(const gui08_editor *editor);
unsigned gui08_base64_remainder_bits(const gui08_editor *editor);
bool gui08_base64_bit_input(const gui08_editor *editor);
bool gui08_symbol_allowed(const gui08_editor *editor, char symbol);
const char *gui08_alphabet(const gui08_editor *editor);
char gui08_symbol_at(const gui08_editor *editor, unsigned index);
const char *gui08_cards_instruction(const gui08_editor *editor);
gui08_request gui08_begin(gui08_editor *editor);
bool gui08_mark_pending(gui08_editor *editor, const gui08_request *request);
bool gui08_cancel(gui08_editor *editor, uint64_t request_id);
bool gui08_accept_result(gui08_editor *editor, uint64_t request_id, uint64_t revision,
                         gui08_method method, unsigned words, unsigned base,
                         gui08_passphrase_marker passphrase_marker);
#endif
