#include "app_core.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_core_ops.h"
#include "app_core_sequence.h"
#include "app_error.h"
#include "auth.h"
#include "device_controls.h"
#include "esp_log.h"
#include "esp_random.h"
#include "macro_executor.h"
#include "nvs_flash.h"
#include "storage.h"
#include "storage_repository.h"
#include "usb_keyboard.h"
#include "web_server.h"
#include "wifi_ap.h"

static const char *const TAG = "app_core";

static app_core_nvs_result_t adapter_nvs_init(void *context) {
    (void)context;
    const esp_err_t result = nvs_flash_init();
    if (result == ESP_OK) {
        return APP_CORE_NVS_OK;
    }
    if (result == ESP_ERR_NVS_NO_FREE_PAGES) {
        return APP_CORE_NVS_NO_FREE_PAGES;
    }
    if (result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        return APP_CORE_NVS_NEW_VERSION_FOUND;
    }
    return APP_CORE_NVS_OTHER_FAILURE;
}

static app_error_code_t adapter_storage_mount(void *context) {
    (void)context;
    return storage_mount_all();
}

static app_error_code_t adapter_storage_recover(void *context) {
    (void)context;
    /* FIX1 §7.4: reconcile leftover atomic-write artifacts before transaction
     * manifest recovery, so a manifest's own interrupted write (and any staging
     * artifact) is resolved before transaction recovery enumerates it. */
    const app_error_code_t atomic = storage_atomic_recover_all();
    if (atomic != APP_ERROR_NONE) {
        return atomic;
    }
    return storage_transaction_recover_all();
}

static app_error_code_t adapter_repository_init(void *context) {
    (void)context;
    return storage_repository_init();
}

static app_error_code_t adapter_auth_init(void *context) {
    (void)context;
    return auth_init();
}

static app_error_code_t adapter_usb_init(void *context) {
    (void)context;
    return usb_keyboard_init();
}

static app_error_code_t adapter_executor_init(void *context) {
    (void)context;
    return macro_executor_init();
}

static app_error_code_t adapter_controls_init(void *context) {
    (void)context;
    return device_controls_init();
}

static app_error_code_t adapter_random_fill(void *context, uint8_t *output, size_t length) {
    (void)context;
    if (output == NULL && length != 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    esp_fill_random(output, length);
    return APP_ERROR_NONE;
}

static app_error_code_t adapter_password_create(void *context, const char *password,
                                                size_t password_length,
                                                auth_password_record_t *out_record) {
    (void)context;
    return auth_password_create(password, password_length, out_record);
}

static app_error_code_t adapter_wifi_start(void *context, const char *ssid,
                                           const char *passphrase) {
    (void)context;
    return wifi_ap_start(ssid, passphrase);
}

static app_error_code_t adapter_http_start(void *context,
                                           const web_server_config_t *configuration) {
    (void)context;
    return web_server_start(configuration);
}

static app_error_code_t adapter_http_stop(void *context) {
    (void)context;
    return web_server_stop();
}

static app_error_code_t adapter_wifi_stop(void *context) {
    (void)context;
    return wifi_ap_stop();
}

static app_error_code_t adapter_storage_unmount(void *context) {
    (void)context;
    return storage_unmount_all();
}

static app_error_code_t adapter_repository_deinit(void *context) {
    (void)context;
    return storage_repository_deinit();
}

static app_error_code_t adapter_auth_deinit(void *context) {
    (void)context;
    return auth_deinit();
}

static app_error_code_t adapter_usb_deinit(void *context) {
    (void)context;
    return usb_keyboard_deinit();
}

static app_error_code_t adapter_executor_deinit(void *context) {
    (void)context;
    return macro_executor_deinit();
}

static app_error_code_t adapter_controls_deinit(void *context) {
    (void)context;
    return device_controls_deinit();
}

static app_error_code_t adapter_nvs_deinit(void *context) {
    (void)context;
    return nvs_flash_deinit() == ESP_OK ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static bool adapter_http_owns_resources(void *context) {
    (void)context;
    /* A retained server handle (including after a partial start whose stop failed)
     * means the HTTP server still owns resources, so cleanup must stop it again
     * even when http_start returned an error and http_started was never set. */
    return web_server_owns_resources();
}

static bool adapter_wifi_owns_resources(void *context) {
    (void)context;
    /* Wired to a wifi_ap ownership query when it lands; conservative default so
     * cleanup relies on the tracked wifi_started flag until then. */
    return false;
}

static bool adapter_storage_owns_mount(void *context) {
    (void)context;
    /* Either partition still being mounted -- including after a failed or partial
     * storage_mount_all() -- means storage is still owned and must be unmounted
     * during cleanup even though storage_mounted was never set. */
    const storage_mount_state_t state = storage_mount_state();
    return state.web_mounted || state.data_mounted;
}

static app_error_code_t adapter_set_indicator(void *context, device_indicator_state_t indicator) {
    (void)context;
    device_controls_set_indicator(indicator);
    return APP_ERROR_NONE;
}

static void adapter_secure_zero(void *context, void *memory, size_t length) {
    (void)context;
    volatile uint8_t *bytes = memory;
    for (size_t index = 0U; index < length; ++index) {
        bytes[index] = 0U;
    }
}

static void adapter_log_event(void *context, const app_core_log_event_t *event) {
    (void)context;
    if (event == NULL) {
        ESP_LOGE(TAG, "startup emitted a null log event");
        return;
    }

    switch (event->type) {
    case APP_CORE_LOG_STAGE:
        if (event->primary_error == APP_ERROR_NONE) {
            ESP_LOGI(TAG, "stage complete: %s", event->stage);
        } else {
            ESP_LOGE(TAG, "stage failed: %s (%s)", event->stage,
                     app_error_code_string(event->primary_error));
        }
        break;
    case APP_CORE_LOG_STORAGE_DEGRADED:
        ESP_LOGW(TAG, "storage recovery requires operator review; evidence was preserved");
        break;
    case APP_CORE_LOG_DEVELOPMENT_CREDENTIALS:
        ESP_LOGW(TAG, "development-only AP SSID: %s", event->ssid);
        ESP_LOGW(TAG, "development-only AP passphrase: %s", event->ap_passphrase);
        ESP_LOGW(TAG, "development-only web password: %s", event->web_password);
        break;
    case APP_CORE_LOG_PROVISIONING_REQUIRED:
        ESP_LOGE(
            TAG,
            "persistent encrypted provisioning is not implemented; refusing to start a network");
        break;
    case APP_CORE_LOG_CLEANUP_FAILED:
        ESP_LOGE(TAG, "cleanup failed after %s: %s (cleanup %s)",
                 app_error_code_string(event->primary_error),
                 app_error_code_string(event->cleanup_error),
                 event->cleanup_incomplete ? "incomplete" : "complete");
        break;
    default:
        ESP_LOGE(TAG, "unknown startup log event");
        break;
    }
}

app_error_code_t app_core_start(void) {
    const app_core_ops_t operations = {
        .context = NULL,
        .nvs_init = adapter_nvs_init,
        .storage_mount = adapter_storage_mount,
        .storage_recover = adapter_storage_recover,
        .repository_init = adapter_repository_init,
        .auth_init = adapter_auth_init,
        .usb_init = adapter_usb_init,
        .executor_init = adapter_executor_init,
        .controls_init = adapter_controls_init,
        .random_fill = adapter_random_fill,
        .password_create = adapter_password_create,
        .wifi_start = adapter_wifi_start,
        .http_start = adapter_http_start,
        .http_stop = adapter_http_stop,
        .wifi_stop = adapter_wifi_stop,
        .storage_unmount = adapter_storage_unmount,
        .repository_deinit = adapter_repository_deinit,
        .auth_deinit = adapter_auth_deinit,
        .usb_deinit = adapter_usb_deinit,
        .executor_deinit = adapter_executor_deinit,
        .controls_deinit = adapter_controls_deinit,
        .nvs_deinit = adapter_nvs_deinit,
        .http_owns_resources = adapter_http_owns_resources,
        .wifi_owns_resources = adapter_wifi_owns_resources,
        .storage_owns_mount = adapter_storage_owns_mount,
        .set_indicator = adapter_set_indicator,
        .secure_zero = adapter_secure_zero,
        .log_event = adapter_log_event,
    };
    const app_core_policy_t policy = {
#if CONFIG_APP_DEVELOPMENT_PROVISIONING_LOG
        .development_provisioning_enabled = true,
        .development_ssid = "ESP32-Macro-Setup",
#else
        .development_provisioning_enabled = false,
        .development_ssid = NULL,
#endif
    };
    return app_core_sequence_start(&operations, &policy);
}
