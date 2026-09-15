#pragma once
#include <stdbool.h>
#include "hex_core.h"
typedef enum { MODE_HEX=0, MODE_COINS=1, MODE_D6_RAW=2, MODE_D6_COLEMAN=3, MODE_MNEMONIC=4, MODE_COUNT=5 } el_mode_t;
enum { GUI_MODE_COUNT=5 };
static inline bool el_mode_is_dice(uint32_t mode){return mode==MODE_D6_RAW || mode==MODE_D6_COLEMAN;}
static inline bool el_mode_is_legacy(uint32_t mode){return mode==MODE_HEX || mode==MODE_COINS || el_mode_is_dice(mode);}
/* Full queue-owned bytes. Mnemonic length <=215, canonical text plus NUL;
 * other modes keep their existing 64/256/1024 bounds. No borrowed pointers. */
typedef struct { char hex[1025]; size_t length; uint32_t mode, words; uint64_t request_id, revision; } hex_request_t;
typedef struct { char mnemonic[216],entropy[65],fingerprint[9],address[43]; int32_t rc; bool weak; uint32_t mode, words; uint64_t request_id, revision; } hex_result_t;
/* GUI functions only under LVGL lock. Request callback copies synchronously. */
void gui_create(bool (*submit)(const hex_request_t *));
void gui_result(const hex_result_t *result);
