#pragma once
#include <stdbool.h>
#include "hex_core.h"
/* Owned ASCII transcript: 0 hex, 1 raw coin bits, 2 raw D6, 3 Coleman D6.
 * Per-mode UI bounds 64/256/1024/1024. No implicit reinterpretation. */
typedef struct { char hex[1025]; size_t length; uint32_t mode, words; } hex_request_t;
typedef struct { char mnemonic[216],fingerprint[9],address[43]; int32_t rc; bool weak; uint32_t mode, words; } hex_result_t;
/* GUI functions only under LVGL lock. Request callback must copy synchronously.
 * Edits and submission disabled until completion. Worker uses owned queue bytes. */
void gui_create(bool (*submit)(const hex_request_t *));
void gui_result(const hex_result_t *result);
