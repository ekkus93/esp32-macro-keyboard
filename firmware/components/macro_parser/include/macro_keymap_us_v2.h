#ifndef MACRO_KEYMAP_US_V2_H
#define MACRO_KEYMAP_US_V2_H

#include <stdbool.h>

#include "macro_keymap_us.h"

bool macro_keymap_us_v2_named_directive(const char *name, macro_hid_key_t *out_key);
bool macro_keymap_us_v2_chord_key(const char *name, macro_hid_key_t *out_key);

#endif
