#pragma once
#include <stdbool.h>
#include "lvgl.h"
#include "hex_core.h"
typedef struct { char hex[65]; size_t length; } hex_request_t;
typedef struct { char mnemonic[216],fingerprint[9],address[43]; int32_t rc; } hex_result_t;
/* GUI functions only under LVGL lock. Request callback must copy synchronously.
 * Edits and submission disabled until completion. Worker uses owned queue bytes. */
void gui_create(bool (*submit)(const hex_request_t *));
void gui_result(const hex_result_t *result);
