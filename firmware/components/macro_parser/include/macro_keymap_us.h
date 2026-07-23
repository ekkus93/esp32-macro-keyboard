#ifndef MACRO_KEYMAP_US_H
#define MACRO_KEYMAP_US_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t modifiers;
    uint8_t usage;
} macro_hid_key_t;

bool macro_keymap_us_printable(char character, macro_hid_key_t *out_key);
bool macro_keymap_us_named(const char *name, macro_hid_key_t *out_key);
bool macro_keymap_us_modifier(const char *name, uint8_t *out_modifier);

#endif
