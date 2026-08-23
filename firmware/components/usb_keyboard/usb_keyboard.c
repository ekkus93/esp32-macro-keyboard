#include "usb_keyboard.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyusb.h"
#include "tinyusb_default_config.h"
#include "tusb.h"
#include "usb_descriptors.h"
#include "usb_keyboard_ops.h"
#include "usb_keyboard_state.h"

static portMUX_TYPE state_lock = portMUX_INITIALIZER_UNLOCKED;
static usb_keyboard_state_t state = USB_KEYBOARD_UNINITIALIZED;

static usb_keyboard_state_t adapter_state_get(void *context) {
    (void)context;
    portENTER_CRITICAL(&state_lock);
    const usb_keyboard_state_t current = state;
    portEXIT_CRITICAL(&state_lock);
    return current;
}

static void adapter_state_set(void *context, usb_keyboard_state_t next) {
    (void)context;
    portENTER_CRITICAL(&state_lock);
    state = next;
    portEXIT_CRITICAL(&state_lock);
}

/*
 * esp_tinyusb 2.x owns tud_mount_cb/tud_umount_cb and reports attach/detach
 * through this event callback; defining those symbols in the application would
 * clash at link time. Suspend/resume are still delivered through TinyUSB's weak
 * tud_suspend_cb/tud_resume_cb below, which esp_tinyusb does not override unless
 * the corresponding Kconfig callbacks are enabled.
 */
static void usb_event_handler(tinyusb_event_t *event, void *arg);

static app_error_code_t adapter_driver_install(void *context) {
    (void)context;
    const size_t string_count = usb_descriptors_string_count();
    if (string_count > (size_t)INT_MAX) {
        return APP_ERROR_INTERNAL;
    }

    tinyusb_config_t configuration = TINYUSB_DEFAULT_CONFIG();
    configuration.descriptor.device = usb_descriptors_device();
    configuration.descriptor.string = usb_descriptors_strings();
    configuration.descriptor.string_count = (int)string_count;
    configuration.descriptor.full_speed_config = usb_descriptors_configuration();
    configuration.event_cb = usb_event_handler;

    return tinyusb_driver_install(&configuration) == ESP_OK ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static app_error_code_t adapter_driver_uninstall(void *context) {
    (void)context;
    return tinyusb_driver_uninstall() == ESP_OK ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static uint32_t adapter_now_ms(void *context) {
    (void)context;
    const uint64_t milliseconds = (uint64_t)xTaskGetTickCount() * (uint64_t)portTICK_PERIOD_MS;
    return (uint32_t)milliseconds;
}

static void adapter_delay_ms(void *context, uint32_t milliseconds) {
    (void)context;
    TickType_t ticks = pdMS_TO_TICKS(milliseconds);
    if (ticks == 0U) {
        ticks = 1U;
    }
    vTaskDelay(ticks);
}

static bool adapter_mounted(void *context) {
    (void)context;
    return tud_mounted();
}

static bool adapter_suspended(void *context) {
    (void)context;
    return tud_suspended();
}

static bool adapter_hid_ready(void *context) {
    (void)context;
    return tud_hid_ready();
}

static bool adapter_send_keyboard_report(void *context, uint8_t report_id, uint8_t modifiers,
                                         const uint8_t keycodes[USB_KEYBOARD_KEYCODE_COUNT]) {
    (void)context;
    return tud_hid_keyboard_report(report_id, modifiers, keycodes);
}

static const usb_keyboard_ops_t operations = {
    .context = NULL,
    .state_get = adapter_state_get,
    .state_set = adapter_state_set,
    .driver_install = adapter_driver_install,
    .driver_uninstall = adapter_driver_uninstall,
    .now_ms = adapter_now_ms,
    .delay_ms = adapter_delay_ms,
    .mounted = adapter_mounted,
    .suspended = adapter_suspended,
    .hid_ready = adapter_hid_ready,
    .send_keyboard_report = adapter_send_keyboard_report,
};

usb_keyboard_state_t usb_keyboard_get_state(void) {
    return adapter_state_get(NULL);
}

app_error_code_t usb_keyboard_init(void) {
    return usb_keyboard_state_init(&operations);
}

app_error_code_t usb_keyboard_deinit(void) {
    return usb_keyboard_state_deinit(&operations);
}

app_error_code_t usb_keyboard_press(uint8_t modifiers, const uint8_t *usages, uint8_t usage_count) {
    usb_keyboard_key_t key = {.modifiers = modifiers, .usage_count = 0U};
    memset(key.usages, 0, sizeof(key.usages));
    const uint8_t bounded_count =
        usage_count > USB_KEYBOARD_KEYCODE_COUNT ? USB_KEYBOARD_KEYCODE_COUNT : usage_count;
    if (usages != NULL) {
        for (uint8_t index = 0U; index < bounded_count; ++index) {
            key.usages[index] = usages[index];
        }
    }
    key.usage_count = bounded_count;
    return usb_keyboard_state_press(&operations, key);
}

app_error_code_t usb_keyboard_release_all(void) {
    return usb_keyboard_state_release_all(&operations);
}

static void usb_event_handler(tinyusb_event_t *event, void *arg) {
    (void)arg;
    if (event == NULL) {
        return;
    }
    switch (event->id) {
    case TINYUSB_EVENT_ATTACHED:
        usb_keyboard_state_mount(&operations);
        break;
    case TINYUSB_EVENT_DETACHED:
        usb_keyboard_state_unmount(&operations);
        break;
    default:
        break;
    }
}

void tud_suspend_cb(bool remote_wakeup_en) {
    (void)remote_wakeup_en;
    usb_keyboard_state_suspend(&operations);
}

void tud_resume_cb(void) {
    usb_keyboard_state_resume(&operations);
}
