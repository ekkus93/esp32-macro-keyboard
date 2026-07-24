#include "usb_keyboard.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "usb_keyboard_state.h"

static portMUX_TYPE state_lock = portMUX_INITIALIZER_UNLOCKED;
static usb_keyboard_state_t state = USB_KEYBOARD_UNINITIALIZED;

static usb_keyboard_state_t adapter_state_get(void *context)
{
    (void)context;
    portENTER_CRITICAL(&state_lock);
    const usb_keyboard_state_t current = state;
    portEXIT_CRITICAL(&state_lock);
    return current;
}

static void adapter_state_set(void *context, usb_keyboard_state_t next)
{
    (void)context;
    portENTER_CRITICAL(&state_lock);
    state = next;
    portEXIT_CRITICAL(&state_lock);
}

static app_error_code_t adapter_driver_install(void *context)
{
    (void)context;
    const size_t string_count = usb_descriptors_string_count();
    if (string_count > (size_t)INT_MAX) {
        return APP_ERROR_INTERNAL;
    }

    tinyusb_config_t configuration = TINYUSB_DEFAULT_CONFIG();
    configuration.descriptor.device = usb_descriptors_device();
    configuration.descriptor.string = usb_descriptors_strings();
    configuration.descriptor.string_count = (int)string_count;
    configuration.descriptor.full_speed_config =
        usb_descriptors_configuration();

    return tinyusb_driver_install(&configuration) == ESP_OK ? APP_ERROR_NONE
                                                              : APP_ERROR_INTERNAL;
}

static uint32_t adapter_now_ms(void *context)
{
    (void)context;
    const uint64_t milliseconds =
        (uint64_t)xTaskGetTickCount() * (uint64_t)portTICK_PERIOD_MS;
    return (uint32_t)milliseconds;
}

static void adapter_delay_ms(void *context, uint32_t milliseconds)
{
    (void)context;
    TickType_t ticks = pdMS_TO_TICKS(milliseconds);
    if (ticks == 0U) {
        ticks = 1U;
    }
    vTaskDelay(ticks);
}

static bool adapter_mounted(void *context)
{
    (void)context;
    return tud_mounted();
}

static bool adapter_suspended(void *context)
{
    (void)context;
    return tud_suspended();
}

static bool adapter_hid_ready(void *context)
{
    (void)context;
    return tud_hid_ready();
}

static bool adapter_send_keyboard_report(void *context,
                                         uint8_t report_id,
                                         uint8_t modifiers,
                                         const uint8_t keycodes[6])
{
    (void)context;
    return tud_hid_keyboard_report(report_id, modifiers, keycodes);
}

static const usb_keyboard_ops_t operations = {
    .context = NULL,
    .state_get = adapter_state_get,
    .state_set = adapter_state_set,
    .driver_install = adapter_driver_install,
    .now_ms = adapter_now_ms,
    .delay_ms = adapter_delay_ms,
    .mounted = adapter_mounted,
    .suspended = adapter_suspended,
    .hid_ready = adapter_hid_ready,
    .send_keyboard_report = adapter_send_keyboard_report,
};

usb_keyboard_state_t usb_keyboard_get_state(void)
{
    return adapter_state_get(NULL);
}

app_error_code_t usb_keyboard_init(void)
{
    return usb_keyboard_state_init(&operations);
}

app_error_code_t usb_keyboard_press(uint8_t modifiers, uint8_t usage)
{
    return usb_keyboard_state_press(&operations, modifiers, usage);
}

app_error_code_t usb_keyboard_release_all(void)
{
    return usb_keyboard_state_release_all(&operations);
}

void tud_mount_cb(void)
{
    usb_keyboard_state_mount(&operations);
}

void tud_umount_cb(void)
{
    usb_keyboard_state_unmount(&operations);
}

void tud_suspend_cb(bool remote_wakeup_en)
{
    (void)remote_wakeup_en;
    usb_keyboard_state_suspend(&operations);
}

void tud_resume_cb(void)
{
    usb_keyboard_state_resume(&operations);
}
