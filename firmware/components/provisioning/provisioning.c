#include "provisioning.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "nvs.h"
#include "provisioning_core.h"

#define PROVISIONING_NAMESPACE "provisioning"
#define PROVISIONING_CONFIG_KEY "config"

static const char *const TAG = "provisioning";
static SemaphoreHandle_t provisioning_mutex;
static nvs_handle_t provisioning_handle;
static bool namespace_open;
static volatile bool shutting_down;
static provisioning_core_t core;

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

static app_error_code_t adapter_read_blob(void *context,
                                          uint8_t *output,
                                          size_t capacity,
                                          size_t *out_size) {
    (void)context;
    if (!namespace_open || output == NULL || out_size == NULL || capacity == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    size_t size = capacity;
    const esp_err_t result =
        nvs_get_blob(provisioning_handle, PROVISIONING_CONFIG_KEY, output, &size);
    if (result == ESP_OK) {
        *out_size = size;
    }
    return map_nvs_error(result);
}

static app_error_code_t adapter_write_blob(void *context,
                                           const uint8_t *data,
                                           size_t size) {
    (void)context;
    if (!namespace_open || data == NULL || size == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return map_nvs_error(
        nvs_set_blob(provisioning_handle, PROVISIONING_CONFIG_KEY, data, size));
}

static app_error_code_t adapter_erase_blob(void *context) {
    (void)context;
    if (!namespace_open) {
        return APP_ERROR_STORAGE_UNAVAILABLE;
    }
    const esp_err_t result =
        nvs_erase_key(provisioning_handle, PROVISIONING_CONFIG_KEY);
    return result == ESP_ERR_NVS_NOT_FOUND ? APP_ERROR_NONE : map_nvs_error(result);
}

static app_error_code_t adapter_commit(void *context) {
    (void)context;
    return namespace_open ? map_nvs_error(nvs_commit(provisioning_handle))
                          : APP_ERROR_STORAGE_UNAVAILABLE;
}

static void adapter_secure_zero(void *context, void *memory, size_t size) {
    (void)context;
    volatile uint8_t *bytes = memory;
    for (size_t index = 0U; index < size; ++index) {
        bytes[index] = 0U;
    }
}

static provisioning_ops_t provisioning_operations(void) {
    return (provisioning_ops_t){
        .context = NULL,
        .read_blob = adapter_read_blob,
        .write_blob = adapter_write_blob,
        .erase_blob = adapter_erase_blob,
        .commit = adapter_commit,
        .secure_zero = adapter_secure_zero,
    };
}

static app_error_code_t lock_provisioning(void) {
    if (provisioning_mutex == NULL || shutting_down) {
        return APP_ERROR_CONFLICT;
    }
    if (xSemaphoreTake(provisioning_mutex, portMAX_DELAY) != pdTRUE) {
        return APP_ERROR_INTERNAL;
    }
    if (shutting_down) {
        if (xSemaphoreGive(provisioning_mutex) != pdTRUE) {
            ESP_LOGE(TAG, "failed to release provisioning mutex during shutdown");
            return APP_ERROR_INTERNAL;
        }
        return APP_ERROR_CONFLICT;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t finish_locked(app_error_code_t result) {
    if (xSemaphoreGive(provisioning_mutex) != pdTRUE) {
        ESP_LOGE(TAG, "failed to release provisioning mutex");
        return result != APP_ERROR_NONE ? result : APP_ERROR_INTERNAL;
    }
    return result;
}

static void cleanup_partial_init(void) {
    if (core.initialized) {
        const app_error_code_t result = provisioning_core_deinit(&core);
        if (result != APP_ERROR_NONE) {
            ESP_LOGE(TAG, "failed to clear partial provisioning core");
        }
    }
    if (namespace_open) {
        nvs_close(provisioning_handle);
        namespace_open = false;
        provisioning_handle = 0U;
    }
    if (provisioning_mutex != NULL) {
        vSemaphoreDelete(provisioning_mutex);
        provisioning_mutex = NULL;
    }
    shutting_down = false;
}

app_error_code_t provisioning_init(void) {
    if (provisioning_mutex != NULL || namespace_open || core.initialized) {
        return APP_ERROR_CONFLICT;
    }
    shutting_down = false;
    provisioning_mutex = xSemaphoreCreateMutex();
    if (provisioning_mutex == NULL) {
        return APP_ERROR_INTERNAL;
    }
    app_error_code_t result = map_nvs_error(
        nvs_open(PROVISIONING_NAMESPACE, NVS_READWRITE, &provisioning_handle));
    if (result != APP_ERROR_NONE) {
        cleanup_partial_init();
        return result;
    }
    namespace_open = true;

    const provisioning_ops_t operations = provisioning_operations();
    result = provisioning_core_init(&core, &operations);
    if (result != APP_ERROR_NONE) {
        cleanup_partial_init();
        return result;
    }
    provisioning_config_t loaded;
    result = provisioning_core_load(&core, &loaded);
    adapter_secure_zero(NULL, &loaded, sizeof(loaded));
    if (result != APP_ERROR_NONE) {
        cleanup_partial_init();
    }
    return result;
}

app_error_code_t provisioning_load(provisioning_config_t *out_config) {
    if (out_config == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = lock_provisioning();
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = provisioning_core_load(&core, out_config);
    return finish_locked(result);
}

app_error_code_t provisioning_commit(const provisioning_config_t *replacement,
                                     uint32_t expected_revision,
                                     provisioning_config_t *out_committed) {
    if (replacement == NULL || out_committed == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = lock_provisioning();
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = provisioning_core_commit(
        &core, replacement, expected_revision, out_committed);
    return finish_locked(result);
}

app_error_code_t provisioning_clear_credentials(void) {
    app_error_code_t result = lock_provisioning();
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = provisioning_core_clear_credentials(&core);
    return finish_locked(result);
}

app_error_code_t provisioning_factory_reset(void) {
    app_error_code_t result = lock_provisioning();
    if (result != APP_ERROR_NONE) {
        return result;
    }
    result = provisioning_core_factory_reset(&core);
    return finish_locked(result);
}

app_error_code_t provisioning_deinit(void) {
    if (!provisioning_owns_resources()) {
        return APP_ERROR_NONE;
    }
    if (provisioning_mutex == NULL) {
        cleanup_partial_init();
        return APP_ERROR_INTERNAL;
    }
    shutting_down = true;
    if (xSemaphoreTake(provisioning_mutex, portMAX_DELAY) != pdTRUE) {
        return APP_ERROR_INTERNAL;
    }

    app_error_code_t result = provisioning_core_deinit(&core);
    if (namespace_open) {
        nvs_close(provisioning_handle);
        namespace_open = false;
        provisioning_handle = 0U;
    }
    if (xSemaphoreGive(provisioning_mutex) != pdTRUE) {
        ESP_LOGE(TAG, "failed to release provisioning mutex during deinit");
        if (result == APP_ERROR_NONE) {
            result = APP_ERROR_INTERNAL;
        }
    }
    vSemaphoreDelete(provisioning_mutex);
    provisioning_mutex = NULL;
    shutting_down = false;
    return result;
}

bool provisioning_owns_resources(void) {
    return provisioning_mutex != NULL || namespace_open || core.initialized;
}
