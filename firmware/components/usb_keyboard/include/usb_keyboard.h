#ifndef USB_KEYBOARD_H
#define USB_KEYBOARD_H

#include "app_error.h"
#include <stdint.h>

typedef enum {
    USB_KEYBOARD_UNINITIALIZED = 0,
    USB_KEYBOARD_DISCONNECTED,
    USB_KEYBOARD_ENUMERATING,
    USB_KEYBOARD_READY,
    USB_KEYBOARD_SUSPENDED,
    USB_KEYBOARD_ERROR
} usb_keyboard_state_t;

app_error_code_t usb_keyboard_init(void);
app_error_code_t usb_keyboard_deinit(void);
/* usages holds usage_count simultaneous non-modifier key usages (0 for a
 * modifier-only press), pressed together in one HID report. */
app_error_code_t usb_keyboard_press(uint8_t modifiers, const uint8_t *usages, uint8_t usage_count);
app_error_code_t usb_keyboard_release_all(void);
usb_keyboard_state_t usb_keyboard_get_state(void);

#endif
