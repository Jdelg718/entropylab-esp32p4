#pragma once
#include <stdbool.h>
#include "lvgl.h"
void gui_create(void (*run_fixture)(void));
void gui_busy(void);
void gui_result(bool pass, const char *mnemonic, const char *fingerprint, const char *address);
