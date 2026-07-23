#include "usb_keyboard_state.h"

#include <stddef.h>
#include <stdint.h>

bool usb_keyboard_ops_is_valid(const usb_keyboard_ops_t *operations)
{
    return operations != NULL && operations->state_get != NULL &&
           operations->state_set != NULL && operations->driver_install != NULL &&
           operations->now_ms != NULL && operations->delay_ms != NULL &&
           operations->mounted != NULL && operations->suspended != NULL &&
           operations->hid_ready != NULL &&
           operations->send_keyboard_report != NULL;
}

static app_error_code_t wait_until_ready(const usb_keyboard_ops_t *operations)
{
    const uint32_t deadline = operations->now_ms(operations->context) +
                              USB_KEYBOARD_READY_TIMEOUT_MS;
    while (!operations->hid_ready(operations->context)) {
        if (operations->state_get(operations->context) != USB_KEYBOARD_READY) {
            return APP_ERROR_USB_NOT_READY;
        }
        const uint32_t now = operations->now_ms(operations->context);
        if ((int32_t)(now - deadline) >= 0) {
            return APP_ERROR_TIMEOUT;
        }
        operations->delay_ms(operations->context, 1U);
    }
    return APP_ERROR_NONE;
}

app_error_code_t usb_keyboard_state_init(const usb_keyboard_ops_t *operations)
{
    if (!usb_keyboard_ops_is_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (operations->state_get(operations->context) != USB_KEYBOARD_UNINITIALIZED) {
        return APP_ERROR_CONFLICT;
    }

    operations->state_set(operations->context, USB_KEYBOARD_ENUMERATING);
    const app_error_code_t result = operations->driver_install(operations->context);
    if (result != APP_ERROR_NONE) {
        operations->state_set(operations->context, USB_KEYBOARD_ERROR);
        return result;
    }
    operations->state_set(operations->context, USB_KEYBOARD_DISCONNECTED);
    return APP_ERROR_NONE;
}

app_error_code_t usb_keyboard_state_press(const usb_keyboard_ops_t *operations,
                                          uint8_t modifiers,
                                          uint8_t usage)
{
    if (!usb_keyboard_ops_is_valid(operations) || usage == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (operations->state_get(operations->context) != USB_KEYBOARD_READY ||
        !operations->mounted(operations->context) ||
        operations->suspended(operations->context)) {
        return APP_ERROR_USB_NOT_READY;
    }

    const app_error_code_t ready = wait_until_ready(operations);
    if (ready != APP_ERROR_NONE) {
        return ready;
    }
    const uint8_t keycodes[6] = {usage, 0U, 0U, 0U, 0U, 0U};
    return operations->send_keyboard_report(
               operations->context, 0U, modifiers, keycodes)
               ? APP_ERROR_NONE
               : APP_ERROR_IO;
}

app_error_code_t usb_keyboard_state_release_all(const usb_keyboard_ops_t *operations)
{
    if (!usb_keyboard_ops_is_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (operations->state_get(operations->context) != USB_KEYBOARD_READY ||
        !operations->mounted(operations->context) ||
        operations->suspended(operations->context)) {
        return APP_ERROR_USB_NOT_READY;
    }

    const app_error_code_t ready = wait_until_ready(operations);
    if (ready != APP_ERROR_NONE) {
        return ready;
    }
    const uint8_t keycodes[6] = {0U, 0U, 0U, 0U, 0U, 0U};
    return operations->send_keyboard_report(
               operations->context, 0U, 0U, keycodes)
               ? APP_ERROR_NONE
               : APP_ERROR_IO;
}

void usb_keyboard_state_mount(const usb_keyboard_ops_t *operations)
{
    if (usb_keyboard_ops_is_valid(operations)) {
        operations->state_set(operations->context, USB_KEYBOARD_READY);
    }
}

void usb_keyboard_state_unmount(const usb_keyboard_ops_t *operations)
{
    if (usb_keyboard_ops_is_valid(operations)) {
        operations->state_set(operations->context, USB_KEYBOARD_DISCONNECTED);
    }
}

void usb_keyboard_state_suspend(const usb_keyboard_ops_t *operations)
{
    if (usb_keyboard_ops_is_valid(operations)) {
        operations->state_set(operations->context, USB_KEYBOARD_SUSPENDED);
    }
}

void usb_keyboard_state_resume(const usb_keyboard_ops_t *operations)
{
    if (usb_keyboard_ops_is_valid(operations)) {
        operations->state_set(
            operations->context,
            operations->mounted(operations->context)
                ? USB_KEYBOARD_READY
                : USB_KEYBOARD_DISCONNECTED);
    }
}
