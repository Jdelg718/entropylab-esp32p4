#pragma once
#include <stdbool.h>
#include "hex_core.h"
#include "gui08_editor.h"
typedef enum { MODE_HEX=0, MODE_COINS=1, MODE_D6_RAW=2, MODE_D6_COLEMAN=3, MODE_MNEMONIC=4, MODE_COUNT=5 } el_mode_t;
enum { GUI_MODE_COUNT=5 };
static inline bool el_mode_is_dice(uint32_t mode){return mode==MODE_D6_RAW || mode==MODE_D6_COLEMAN;}
static inline bool el_mode_is_legacy(uint32_t mode){return mode==MODE_HEX || mode==MODE_COINS || el_mode_is_dice(mode);}
/* Full queue-owned bytes. Mnemonic length <=215, canonical text plus NUL;
 * other modes keep their existing 64/256/1024 bounds. No borrowed pointers. */
typedef struct { char hex[1025]; size_t length; uint32_t mode, words; uint64_t request_id, revision; } hex_request_t;
typedef struct { char mnemonic[216],entropy[65],fingerprint[9],address[43]; int32_t rc; bool weak; uint32_t mode, words; uint64_t request_id, revision; } hex_result_t;
/* Separate envelope preserves every legacy aggregate initializer/queue ABI.
 * Owned source and raw bytes; no pointers. Dispatcher metadata is a later slice. */
typedef struct { hex_request_t source; uint8_t passphrase[256]; size_t passphrase_len; } el_passphrase_request_t;
/* GUI functions only under LVGL lock. Request callback copies synchronously. */
void gui_create(bool (*submit)(const hex_request_t *));
void gui_result(const hex_result_t *result);
void gui_passphrase_configure(void (*lock)(void),void (*unlock)(void),bool (*publish)(uint64_t));
typedef int (*gui_passphrase_lifehash_render_fn)(const uint8_t raw4[4],
                                                 uint8_t rgb[32 * 32 * 3]);
void gui_passphrase_lifehash_configure(gui_passphrase_lifehash_render_fn render);
void gui_passphrase_work(uint64_t token);
void gui_passphrase_poll(void);
void gui_passphrase_dispose(void);
/* Display-thread snapshot for the Cards/Bases owner; bytes are copied now. */
gui08_passphrase_marker gui_passphrase_snapshot(uint8_t passphrase[256],
                                                size_t *passphrase_len);
void gui_cards_bases_configure(bool (*submit)(const gui08_request *),
                               bool (*cancel)(uint64_t request_id));
bool gui_cards_bases_pending(const gui08_request *context);
bool gui_cards_bases_result(const gui08_request *context);
bool gui_cards_bases_cancelled(uint64_t request_id);
bool gui_cards_bases_lifehash(const gui08_request *context,
                              const char fingerprint[9],
                              const uint8_t *rgb, size_t rgb_size);
void gui_cards_bases_lifehash_blank(void);
