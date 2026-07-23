#include "macro_keymap_us.h"

#include <stddef.h>
#include <string.h>

#define MOD_LEFT_CTRL 0x01U
#define MOD_LEFT_SHIFT 0x02U
#define MOD_LEFT_ALT 0x04U
#define MOD_LEFT_GUI 0x08U
#define KEY_A 0x04U
#define KEY_1 0x1eU
#define KEY_ENTER 0x28U
#define KEY_ESCAPE 0x29U
#define KEY_BACKSPACE 0x2aU
#define KEY_TAB 0x2bU
#define KEY_SPACE 0x2cU
#define KEY_MINUS 0x2dU
#define KEY_EQUAL 0x2eU
#define KEY_LEFT_BRACE 0x2fU
#define KEY_RIGHT_BRACE 0x30U
#define KEY_BACKSLASH 0x31U
#define KEY_SEMICOLON 0x33U
#define KEY_APOSTROPHE 0x34U
#define KEY_GRAVE 0x35U
#define KEY_COMMA 0x36U
#define KEY_PERIOD 0x37U
#define KEY_SLASH 0x38U
#define KEY_CAPS_LOCK 0x39U
#define KEY_F1 0x3aU
#define KEY_PRINT_SCREEN 0x46U
#define KEY_INSERT 0x49U
#define KEY_HOME 0x4aU
#define KEY_PAGE_UP 0x4bU
#define KEY_DELETE 0x4cU
#define KEY_END 0x4dU
#define KEY_PAGE_DOWN 0x4eU
#define KEY_RIGHT 0x4fU
#define KEY_LEFT 0x50U
#define KEY_DOWN 0x51U
#define KEY_UP 0x52U

static void set_key(macro_hid_key_t *out_key, uint8_t modifiers, uint8_t usage)
{
    out_key->modifiers = modifiers;
    out_key->usage = usage;
}

bool macro_keymap_us_printable(char character, macro_hid_key_t *out_key)
{
    if (out_key == NULL) {
        return false;
    }
    if (character >= 'a' && character <= 'z') {
        set_key(out_key, 0U, (uint8_t)(KEY_A + (uint8_t)(character - 'a')));
        return true;
    }
    if (character >= 'A' && character <= 'Z') {
        set_key(out_key, MOD_LEFT_SHIFT, (uint8_t)(KEY_A + (uint8_t)(character - 'A')));
        return true;
    }
    if (character >= '1' && character <= '9') {
        set_key(out_key, 0U, (uint8_t)(KEY_1 + (uint8_t)(character - '1')));
        return true;
    }
    if (character == '0') {
        set_key(out_key, 0U, 0x27U);
        return true;
    }

    switch (character) {
    case ' ':
        set_key(out_key, 0U, KEY_SPACE);
        return true;
    case '-': set_key(out_key, 0U, KEY_MINUS); return true;
    case '_': set_key(out_key, MOD_LEFT_SHIFT, KEY_MINUS); return true;
    case '=': set_key(out_key, 0U, KEY_EQUAL); return true;
    case '+': set_key(out_key, MOD_LEFT_SHIFT, KEY_EQUAL); return true;
    case '[': set_key(out_key, 0U, KEY_LEFT_BRACE); return true;
    case '{': set_key(out_key, MOD_LEFT_SHIFT, KEY_LEFT_BRACE); return true;
    case ']': set_key(out_key, 0U, KEY_RIGHT_BRACE); return true;
    case '}': set_key(out_key, MOD_LEFT_SHIFT, KEY_RIGHT_BRACE); return true;
    case '\\': set_key(out_key, 0U, KEY_BACKSLASH); return true;
    case '|': set_key(out_key, MOD_LEFT_SHIFT, KEY_BACKSLASH); return true;
    case ';': set_key(out_key, 0U, KEY_SEMICOLON); return true;
    case ':': set_key(out_key, MOD_LEFT_SHIFT, KEY_SEMICOLON); return true;
    case '\'': set_key(out_key, 0U, KEY_APOSTROPHE); return true;
    case '"': set_key(out_key, MOD_LEFT_SHIFT, KEY_APOSTROPHE); return true;
    case '`': set_key(out_key, 0U, KEY_GRAVE); return true;
    case '~': set_key(out_key, MOD_LEFT_SHIFT, KEY_GRAVE); return true;
    case ',': set_key(out_key, 0U, KEY_COMMA); return true;
    case '<': set_key(out_key, MOD_LEFT_SHIFT, KEY_COMMA); return true;
    case '.': set_key(out_key, 0U, KEY_PERIOD); return true;
    case '>': set_key(out_key, MOD_LEFT_SHIFT, KEY_PERIOD); return true;
    case '/': set_key(out_key, 0U, KEY_SLASH); return true;
    case '?': set_key(out_key, MOD_LEFT_SHIFT, KEY_SLASH); return true;
    case '!': set_key(out_key, MOD_LEFT_SHIFT, KEY_1); return true;
    case '@': set_key(out_key, MOD_LEFT_SHIFT, 0x1fU); return true;
    case '#': set_key(out_key, MOD_LEFT_SHIFT, 0x20U); return true;
    case '$': set_key(out_key, MOD_LEFT_SHIFT, 0x21U); return true;
    case '%': set_key(out_key, MOD_LEFT_SHIFT, 0x22U); return true;
    case '^': set_key(out_key, MOD_LEFT_SHIFT, 0x23U); return true;
    case '&': set_key(out_key, MOD_LEFT_SHIFT, 0x24U); return true;
    case '*': set_key(out_key, MOD_LEFT_SHIFT, 0x25U); return true;
    case '(': set_key(out_key, MOD_LEFT_SHIFT, 0x26U); return true;
    case ')': set_key(out_key, MOD_LEFT_SHIFT, 0x27U); return true;
    default:
        return false;
    }
}

typedef struct {
    const char *name;
    uint8_t usage;
} named_key_t;

bool macro_keymap_us_named(const char *name, macro_hid_key_t *out_key)
{
    static const named_key_t keys[] = {
        {"ENTER", KEY_ENTER}, {"TAB", KEY_TAB}, {"ESC", KEY_ESCAPE},
        {"BACKSPACE", KEY_BACKSPACE}, {"DELETE", KEY_DELETE}, {"INSERT", KEY_INSERT},
        {"HOME", KEY_HOME}, {"END", KEY_END}, {"PAGEUP", KEY_PAGE_UP},
        {"PAGEDOWN", KEY_PAGE_DOWN}, {"UP", KEY_UP}, {"DOWN", KEY_DOWN},
        {"LEFT", KEY_LEFT}, {"RIGHT", KEY_RIGHT}, {"SPACE", KEY_SPACE},
        {"F1", KEY_F1}, {"F2", KEY_F1 + 1U}, {"F3", KEY_F1 + 2U},
        {"F4", KEY_F1 + 3U}, {"F5", KEY_F1 + 4U}, {"F6", KEY_F1 + 5U},
        {"F7", KEY_F1 + 6U}, {"F8", KEY_F1 + 7U}, {"F9", KEY_F1 + 8U},
        {"F10", KEY_F1 + 9U}, {"F11", KEY_F1 + 10U}, {"F12", KEY_F1 + 11U},
    };
    (void)KEY_CAPS_LOCK;
    (void)KEY_PRINT_SCREEN;

    if (name == NULL || out_key == NULL) {
        return false;
    }
    for (size_t index = 0U; index < (sizeof(keys) / sizeof(keys[0])); ++index) {
        if (strcmp(name, keys[index].name) == 0) {
            set_key(out_key, 0U, keys[index].usage);
            return true;
        }
    }
    if (strlen(name) == 1U) {
        return macro_keymap_us_printable(name[0], out_key);
    }
    return false;
}

bool macro_keymap_us_modifier(const char *name, uint8_t *out_modifier)
{
    if (name == NULL || out_modifier == NULL) {
        return false;
    }
    if (strcmp(name, "CTRL") == 0) {
        *out_modifier = MOD_LEFT_CTRL;
    } else if (strcmp(name, "SHIFT") == 0) {
        *out_modifier = MOD_LEFT_SHIFT;
    } else if (strcmp(name, "ALT") == 0) {
        *out_modifier = MOD_LEFT_ALT;
    } else if (strcmp(name, "GUI") == 0) {
        *out_modifier = MOD_LEFT_GUI;
    } else {
        return false;
    }
    return true;
}
