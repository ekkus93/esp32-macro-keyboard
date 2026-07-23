#ifndef USB_KEYBOARD_H
#define USB_KEYBOARD_H

#include <stdint.h>
#include "app_error.h"

typedef enum {
    USB_KEYBOARD_UNINITIALIZED = 0,
    USB_KEYBOARD_DISCONNECTED,
    USB_KEYBOARD_ENUMERATING,
    USB_KEYBOARD_READY,
    USB_KEYBOARD_SUSPENDED,
    USB_KEYBOARD_ERROR
} usb_keyboard_state_t;

app_error_code_t usb_keyboard_init(void);
app_error_code_t usb_keyboard_press(uint8_t modifiers, uint8_t usage);
app_error_code_t usb_keyboard_release_all(void);
usb_keyboard_state_t usb_keyboard_get_state(void);

#endif
