#include "macro_keymap_us_v2.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "macro_keymap_us.h"

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

/* CTRL/ALT/SHIFT/GUI are standalone directives too (SPEC_V2 7.7): outside a
 * [...] group they tap and release that modifier alone; inside one they
 * contribute their bit with no usage byte. Checked before the named-key table
 * since they are not in it. */
bool macro_keymap_us_v2_named_directive(const char *name, macro_hid_key_t *out_key) {
    if (name == NULL || out_key == NULL) {
        return false;
    }
    uint8_t modifier = 0U;
    if (macro_keymap_us_modifier(name, &modifier)) {
        *out_key = (macro_hid_key_t){.modifiers = modifier, .usage = 0U};
        return true;
    }
    const size_t length = strlen(name);
    return length > 1U && is_uppercase_decimal_token(name, length) &&
           macro_keymap_us_named(name, out_key);
}
