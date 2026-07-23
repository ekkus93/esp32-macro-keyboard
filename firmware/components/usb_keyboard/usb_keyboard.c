#include "usb_keyboard.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tusb.h"
#include "usb_descriptors.h"

static portMUX_TYPE state_lock = portMUX_INITIALIZER_UNLOCKED;
static usb_keyboard_state_t state = USB_KEYBOARD_UNINITIALIZED;

static void set_state(usb_keyboard_state_t next)
{
    portENTER_CRITICAL(&state_lock);
    state = next;
    portEXIT_CRITICAL(&state_lock);
}

usb_keyboard_state_t usb_keyboard_get_state(void)
{
    portENTER_CRITICAL(&state_lock);
    const usb_keyboard_state_t current = state;
    portEXIT_CRITICAL(&state_lock);
    return current;
}

app_error_code_t usb_keyboard_init(void)
{
    set_state(USB_KEYBOARD_ENUMERATING);
    const tinyusb_config_t configuration = {
        .device_descriptor = usb_descriptors_device(),
        .string_descriptor = usb_descriptors_strings(),
        .string_descriptor_count = (int)usb_descriptors_string_count(),
        .external_phy = false,
        .configuration_descriptor = usb_descriptors_configuration(),
    };
    if (tinyusb_driver_install(&configuration) != ESP_OK) {
        set_state(USB_KEYBOARD_ERROR);
        return APP_ERROR_INTERNAL;
    }
    set_state(USB_KEYBOARD_DISCONNECTED);
    return APP_ERROR_NONE;
}

static app_error_code_t wait_ready(void)
{
    const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(100U);
    while (!tud_hid_ready()) {
        if (usb_keyboard_get_state() != USB_KEYBOARD_READY ||
            (int32_t)(xTaskGetTickCount() - deadline) >= 0) {
            return APP_ERROR_USB_NOT_READY;
        }
        vTaskDelay(pdMS_TO_TICKS(1U));
    }
    return APP_ERROR_NONE;
}

app_error_code_t usb_keyboard_press(uint8_t modifiers, uint8_t usage)
{
    if (usb_keyboard_get_state() != USB_KEYBOARD_READY || usage == 0U) {
        return APP_ERROR_USB_NOT_READY;
    }
    const app_error_code_t ready = wait_ready();
    if (ready != APP_ERROR_NONE) {
        return ready;
    }
    uint8_t keycodes[6] = {usage, 0U, 0U, 0U, 0U, 0U};
    return tud_hid_keyboard_report(0U, modifiers, keycodes) ? APP_ERROR_NONE : APP_ERROR_IO;
}

app_error_code_t usb_keyboard_release_all(void)
{
    if (!tud_mounted() || tud_suspended()) {
        return APP_ERROR_USB_NOT_READY;
    }
    const app_error_code_t ready = wait_ready();
    if (ready != APP_ERROR_NONE) {
        return ready;
    }
    return tud_hid_keyboard_report(0U, 0U, NULL) ? APP_ERROR_NONE : APP_ERROR_IO;
}

void tud_mount_cb(void)
{
    set_state(USB_KEYBOARD_READY);
}

void tud_umount_cb(void)
{
    set_state(USB_KEYBOARD_DISCONNECTED);
}

void tud_suspend_cb(bool remote_wakeup_en)
{
    (void)remote_wakeup_en;
    set_state(USB_KEYBOARD_SUSPENDED);
}

void tud_resume_cb(void)
{
    set_state(tud_mounted() ? USB_KEYBOARD_READY : USB_KEYBOARD_DISCONNECTED);
}
