#ifndef FAKE_USB_BACKEND_H
#define FAKE_USB_BACKEND_H

#include <stddef.h>
#include <stdint.h>

#include "fake_call_log.h"

typedef struct {
    int state;
    int press_result;
    int release_result;
    uint8_t last_modifiers;
    uint8_t last_usage;
    size_t press_count;
    size_t release_count;
    fake_call_log_t calls;
} fake_usb_backend_t;

void fake_usb_backend_reset(fake_usb_backend_t *usb);
int fake_usb_backend_state(fake_usb_backend_t *usb);
int fake_usb_backend_press(fake_usb_backend_t *usb, uint8_t modifiers, uint8_t usage);
int fake_usb_backend_release_all(fake_usb_backend_t *usb);

#endif
