#ifndef USB_KEYBOARD_OPS_H
#define USB_KEYBOARD_OPS_H

#include <stdbool.h>
#include <stdint.h>

#include "app_error.h"
#include "usb_keyboard.h"

typedef struct {
    void *context;
    usb_keyboard_state_t (*state_get)(void *context);
    void (*state_set)(void *context, usb_keyboard_state_t state);
    app_error_code_t (*driver_install)(void *context);
    uint32_t (*now_ms)(void *context);
    void (*delay_ms)(void *context, uint32_t milliseconds);
    bool (*mounted)(void *context);
    bool (*suspended)(void *context);
    bool (*hid_ready)(void *context);
    bool (*send_keyboard_report)(void *context,
                                 uint8_t report_id,
                                 uint8_t modifiers,
                                 const uint8_t keycodes[6]);
} usb_keyboard_ops_t;

#endif
