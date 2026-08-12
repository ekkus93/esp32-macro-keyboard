#include "factory_reset_state.h"

#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "factory_reset_state_core.h"
#include "nvs.h"

#define FACTORY_RESET_STATE_NAMESPACE "reset_journal"
#define FACTORY_RESET_STATE_KEY "factory_reset"

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

static app_error_code_t open_reset_state(nvs_open_mode_t mode, nvs_handle_t *out_handle) {
    if (out_handle == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_handle = 0U;
    return map_nvs_error(nvs_open(FACTORY_RESET_STATE_NAMESPACE, mode, out_handle));
}

static app_error_code_t adapter_read_value(void *context, uint8_t *out_value) {
    if (context == NULL || out_value == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const nvs_handle_t handle = *(const nvs_handle_t *)context;
    return map_nvs_error(nvs_get_u8(handle, FACTORY_RESET_STATE_KEY, out_value));
}

static app_error_code_t adapter_write_value(void *context, uint8_t value) {
    if (context == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const nvs_handle_t handle = *(const nvs_handle_t *)context;
    app_error_code_t result = map_nvs_error(nvs_set_u8(handle, FACTORY_RESET_STATE_KEY, value));
    if (result == APP_ERROR_NONE) {
        result = map_nvs_error(nvs_commit(handle));
    }
    return result;
}

static app_error_code_t adapter_erase_value(void *context) {
    if (context == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const nvs_handle_t handle = *(const nvs_handle_t *)context;
    app_error_code_t result = map_nvs_error(nvs_erase_key(handle, FACTORY_RESET_STATE_KEY));
    if (result == APP_ERROR_NOT_FOUND) {
        return APP_ERROR_NONE;
    }
    if (result == APP_ERROR_NONE) {
        result = map_nvs_error(nvs_commit(handle));
    }
    return result;
}

static factory_reset_state_core_ops_t reset_state_operations(nvs_handle_t *handle) {
    return (factory_reset_state_core_ops_t){
        .context = handle,
        .read_value = adapter_read_value,
        .write_value = adapter_write_value,
        .erase_value = adapter_erase_value,
    };
}

app_error_code_t factory_reset_state_read(factory_reset_state_t *out_state) {
    if (out_state == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_state = FACTORY_RESET_STATE_NONE;
    nvs_handle_t handle = 0U;
    app_error_code_t result = open_reset_state(NVS_READONLY, &handle);
    if (result == APP_ERROR_NOT_FOUND) {
        return APP_ERROR_NONE;
    }
    if (result != APP_ERROR_NONE) {
        return result;
    }
    const factory_reset_state_core_ops_t operations = reset_state_operations(&handle);
    result = factory_reset_state_core_read(&operations, out_state);
    nvs_close(handle);
    return result;
}

app_error_code_t factory_reset_state_mark_pending(void) {
    nvs_handle_t handle = 0U;
    app_error_code_t result = open_reset_state(NVS_READWRITE, &handle);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    const factory_reset_state_core_ops_t operations = reset_state_operations(&handle);
    result = factory_reset_state_core_mark_pending(&operations);
    nvs_close(handle);
    return result;
}

app_error_code_t factory_reset_state_clear(void) {
    nvs_handle_t handle = 0U;
    app_error_code_t result = open_reset_state(NVS_READWRITE, &handle);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    const factory_reset_state_core_ops_t operations = reset_state_operations(&handle);
    result = factory_reset_state_core_clear(&operations);
    nvs_close(handle);
    return result;
}
