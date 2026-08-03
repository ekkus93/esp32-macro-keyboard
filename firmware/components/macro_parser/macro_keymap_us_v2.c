#include "macro_keymap_us_v2.h"

#include <stddef.h>
#include <string.h>

static bool is_uppercase_decimal_token(const char *name, size_t length) {
    if (name == NULL || length == 0U) {
        return false;
    }
    for (size_t index = 0U; index < length; ++index) {
        const char character = name[index];
        const bool uppercase = character >= 'A' && character <= 'Z';
        const bool decimal = character >= '0' && character <= '9';
        if (!uppercase && !decimal) {
            return false;
        }
    }
    return true;
}

bool macro_keymap_us_v2_named_directive(const char *name, macro_hid_key_t *out_key) {
    if (name == NULL || out_key == NULL) {
        return false;
    }
    const size_t length = strlen(name);
    return length > 1U && is_uppercase_decimal_token(name, length) &&
           macro_keymap_us_named(name, out_key);
}

bool macro_keymap_us_v2_chord_key(const char *name, macro_hid_key_t *out_key) {
    if (name == NULL || out_key == NULL) {
        return false;
    }
    const size_t length = strlen(name);
    if (!is_uppercase_decimal_token(name, length)) {
        return false;
    }
    if (length == 1U) {
        return macro_keymap_us_named(name, out_key);
    }
    return macro_keymap_us_v2_named_directive(name, out_key);
}
