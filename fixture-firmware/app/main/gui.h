#pragma once
#include <stdbool.h>
#include "hex_core.h"
/* Owned transcript: ASCII hex in mode 0, ASCII bits in mode 1. */
typedef struct { char hex[257]; size_t length; uint32_t mode, words; } hex_request_t;
typedef struct { char mnemonic[216],fingerprint[9],address[43]; int32_t rc; } hex_result_t;
/* GUI functions only under LVGL lock. Request callback must copy synchronously.
 * Edits and submission disabled until completion. Worker uses owned queue bytes. */
void gui_create(bool (*submit)(const hex_request_t *));
void gui_result(const hex_result_t *result);
