#ifndef USB_KEYBOARD_STATE_H
#define USB_KEYBOARD_STATE_H

#include <stdbool.h>
#include <stdint.h>

#include "app_error.h"
#include "usb_keyboard_ops.h"

#define USB_KEYBOARD_READY_TIMEOUT_MS 100U

bool usb_keyboard_ops_is_valid(const usb_keyboard_ops_t *operations);
app_error_code_t usb_keyboard_state_init(const usb_keyboard_ops_t *operations);
app_error_code_t usb_keyboard_state_press(const usb_keyboard_ops_t *operations,
                                          uint8_t modifiers,
                                          uint8_t usage);
app_error_code_t usb_keyboard_state_release_all(const usb_keyboard_ops_t *operations);
void usb_keyboard_state_mount(const usb_keyboard_ops_t *operations);
void usb_keyboard_state_unmount(const usb_keyboard_ops_t *operations);
void usb_keyboard_state_suspend(const usb_keyboard_ops_t *operations);
void usb_keyboard_state_resume(const usb_keyboard_ops_t *operations);

#endif
