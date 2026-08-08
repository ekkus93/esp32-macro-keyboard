#include "device_settings.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "device_settings_core.h"
#include "device_settings_v2.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "nvs.h"

#define DEVICE_SETTINGS_NAMESPACE "v2_settings"
#define DEVICE_SETTINGS_RECORD_KEY "record"

static SemaphoreHandle_t settings_mutex;
static nvs_handle_t settings_handle;
static bool settings_handle_open;
static bool settings_initialized;
static device_settings_core_t settings_core;

static app_error_code_t map_nvs_error(esp_err_t result) {
    switch (result) {
    case ESP_OK:
        return APP_ERROR_NONE;
    case ESP_ERR_NVS_NOT_FOUND:
        return APP_ERROR_NOT_FOUND;
    case ESP_ERR_NVS_NOT_ENOUGH_SPACE:
        return APP_ERROR_STORAGE_FULL;
    case ESP_ERR_NVS_INVALID_LENGTH:
        return APP_ERROR_STORAGE_CORRUPT;
    default:
        return APP_ERROR_STORAGE_UNAVAILABLE;
    }
}

static void secure_zero(void *context, void *memory, size_t length) {
    (void)context;
    volatile uint8_t *bytes = memory;
    for (size_t index = 0U; index < length; ++index) {
        bytes[index] = 0U;
    }
}

static app_error_code_t open_settings_handle(void) {
    if (settings_handle_open) {
        return APP_ERROR_NONE;
    }
    const app_error_code_t result =
        map_nvs_error(nvs_open(DEVICE_SETTINGS_NAMESPACE, NVS_READWRITE, &settings_handle));
    if (result == APP_ERROR_NONE) {
        settings_handle_open = true;
    }
    return result;
}

static app_error_code_t reopen_settings_handle(void) {
    if (settings_handle_open) {
        nvs_close(settings_handle);
        settings_handle = 0U;
        settings_handle_open = false;
    }
    return open_settings_handle();
}

static app_error_code_t read_record(void *context, uint8_t *record, size_t capacity,
                                    size_t *out_length) {
    (void)context;
    if (!settings_handle_open || record == NULL || out_length == NULL ||
        capacity < APP_V2_SETTINGS_RECORD_BYTES) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    size_t required = 0U;
    app_error_code_t result =
        map_nvs_error(nvs_get_blob(settings_handle, DEVICE_SETTINGS_RECORD_KEY, NULL, &required));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (required != APP_V2_SETTINGS_RECORD_BYTES || required > capacity) {
        return APP_ERROR_STORAGE_CORRUPT;
    }

    size_t actual = capacity;
    result =
        map_nvs_error(nvs_get_blob(settings_handle, DEVICE_SETTINGS_RECORD_KEY, record, &actual));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (actual != APP_V2_SETTINGS_RECORD_BYTES) {
        secure_zero(NULL, record, capacity);
        return APP_ERROR_STORAGE_CORRUPT;
    }
    *out_length = actual;
    return APP_ERROR_NONE;
}

static app_error_code_t replace_record_atomic(void *context, const uint8_t *record, size_t length) {
    (void)context;
    if (!settings_handle_open || record == NULL || length != APP_V2_SETTINGS_RECORD_BYTES) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    app_error_code_t result =
        map_nvs_error(nvs_set_blob(settings_handle, DEVICE_SETTINGS_RECORD_KEY, record, length));
    if (result == APP_ERROR_NONE) {
        result = map_nvs_error(nvs_commit(settings_handle));
    }
    if (result == APP_ERROR_NONE) {
        return APP_ERROR_NONE;
    }

    const app_error_code_t reopen_result = reopen_settings_handle();
    return reopen_result == APP_ERROR_NONE ? result : reopen_result;
}

static device_settings_core_ops_t settings_operations(void) {
    return (device_settings_core_ops_t){
        .context = NULL,
        .read_record = read_record,
        .replace_record_atomic = replace_record_atomic,
        .secure_zero = secure_zero,
    };
}

static void cleanup_partial_init(void) {
    device_settings_core_deinit(&settings_core);
    settings_initialized = false;
    if (settings_handle_open) {
        nvs_close(settings_handle);
        settings_handle = 0U;
        settings_handle_open = false;
    }
    if (settings_mutex != NULL) {
        vSemaphoreDelete(settings_mutex);
        settings_mutex = NULL;
    }
}

static app_error_code_t lock_settings(void) {
    if (!settings_initialized || settings_mutex == NULL) {
        return APP_ERROR_CONFLICT;
    }
    return xSemaphoreTake(settings_mutex, portMAX_DELAY) == pdTRUE ? APP_ERROR_NONE
                                                                   : APP_ERROR_INTERNAL;
}

static app_error_code_t finish_locked(app_error_code_t result) {
    if (xSemaphoreGive(settings_mutex) != pdTRUE) {
        return APP_ERROR_INTERNAL;
    }
    return result;
}

app_error_code_t device_settings_init(void) {
    if (settings_initialized || settings_mutex != NULL || settings_handle_open) {
        return APP_ERROR_CONFLICT;
    }

    settings_mutex = xSemaphoreCreateMutex();
    if (settings_mutex == NULL) {
        return APP_ERROR_INTERNAL;
    }

    app_error_code_t result = open_settings_handle();
    if (result != APP_ERROR_NONE) {
        cleanup_partial_init();
        return result;
    }

    const device_settings_core_ops_t operations = settings_operations();
    result = device_settings_core_init(&settings_core, &operations);
    if (result != APP_ERROR_NONE) {
        cleanup_partial_init();
        return result;
    }

    app_v2_device_settings_t loaded = {0};
    result = device_settings_core_load(&settings_core, &loaded);
    secure_zero(NULL, &loaded, sizeof(loaded));
    if (result != APP_ERROR_NONE) {
        cleanup_partial_init();
        return result;
    }

    settings_initialized = true;
    return APP_ERROR_NONE;
}

app_error_code_t device_settings_deinit(void) {
    if (!settings_initialized && settings_mutex == NULL && !settings_handle_open) {
        return APP_ERROR_NONE;
    }
    if (settings_mutex == NULL) {
        cleanup_partial_init();
        return APP_ERROR_INTERNAL;
    }
    if (xSemaphoreTake(settings_mutex, portMAX_DELAY) != pdTRUE) {
        return APP_ERROR_INTERNAL;
    }

    device_settings_core_deinit(&settings_core);
    settings_initialized = false;
    if (settings_handle_open) {
        nvs_close(settings_handle);
        settings_handle = 0U;
        settings_handle_open = false;
    }
    if (xSemaphoreGive(settings_mutex) != pdTRUE) {
        vSemaphoreDelete(settings_mutex);
        settings_mutex = NULL;
        return APP_ERROR_INTERNAL;
    }
    vSemaphoreDelete(settings_mutex);
    settings_mutex = NULL;
    return APP_ERROR_NONE;
}

app_error_code_t device_settings_read(app_v2_device_settings_t *out_settings) {
    if (out_settings == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_settings, 0, sizeof(*out_settings));
    app_error_code_t result = lock_settings();
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = device_settings_core_load(&settings_core, out_settings);
    if (result != APP_ERROR_NONE) {
        memset(out_settings, 0, sizeof(*out_settings));
    }
    return finish_locked(result);
}

app_error_code_t device_settings_replace(const app_v2_device_settings_t *settings,
                                         bool *out_changed) {
    if (settings == NULL || out_changed == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_changed = false;
    app_error_code_t result = lock_settings();
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = device_settings_core_replace(&settings_core, settings, out_changed);
    if (result != APP_ERROR_NONE) {
        *out_changed = false;
    }
    return finish_locked(result);
}

app_error_code_t device_settings_reset_noncredential(app_v2_device_settings_t *out_settings,
                                                     bool *out_changed) {
    if (out_settings == NULL || out_changed == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_settings, 0, sizeof(*out_settings));
    *out_changed = false;
    app_error_code_t result = lock_settings();
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = device_settings_core_reset_noncredential(&settings_core, out_settings, out_changed);
    if (result != APP_ERROR_NONE) {
        memset(out_settings, 0, sizeof(*out_settings));
        *out_changed = false;
    }
    return finish_locked(result);
}

app_error_code_t device_settings_factory_reset(app_v2_device_settings_t *out_settings,
                                               bool *out_changed) {
    if (out_settings == NULL || out_changed == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_settings, 0, sizeof(*out_settings));
    *out_changed = false;
    app_error_code_t result = lock_settings();
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = device_settings_core_factory_reset(&settings_core, out_settings, out_changed);
    if (result != APP_ERROR_NONE) {
        memset(out_settings, 0, sizeof(*out_settings));
        *out_changed = false;
    }
    return finish_locked(result);
}
