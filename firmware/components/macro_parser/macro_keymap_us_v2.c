#include "macro_keymap_us_v2.h"

#include <stddef.h>
#include <string.h>

bool macro_keymap_us_named_v2_canonical(const char *name, macro_hid_key_t *out_key) {
    if (name == NULL || out_key == NULL) {
        return false;
    }

    const size_t length = strlen(name);
    if (length == 0U) {
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

    if (length == 1U) {
        const char character = name[0];
        if (!((character >= 'A' && character <= 'Z') ||
              (character >= '0' && character <= '9'))) {
            return false;
        }
    }
    return macro_keymap_us_named(name, out_key);
}
