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
#include "macro_executor.h"
#include "nvs_flash.h"
#include "provisioning.h"
#include "provisioning_bootstrap.h"
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

static app_error_code_t adapter_provisioning_init(void *context) {
    (void)context;
    return provisioning_init();
}

static app_error_code_t adapter_provisioning_load(void *context,
                                                  provisioning_config_t *out_configuration) {
    (void)context;
    return provisioning_load(out_configuration);
}

static app_error_code_t adapter_bootstrap_derive(void *context,
                                                 provisioning_bootstrap_t *out_bootstrap) {
    (void)context;
    return provisioning_bootstrap_derive(out_bootstrap);
}

static app_error_code_t adapter_storage_mount(void *context) {
    (void)context;
    /* Mount both web assets and user data so the first-run web application is
     * available in setup mode. The repository mutation lock is created here and
     * remains unused until normal-operation recovery/repository initialization. */
    const app_error_code_t mount = storage_mount_all();
    if (mount != APP_ERROR_NONE) {
        return mount;
    }
    return storage_repository_lock_init();
}

static app_error_code_t adapter_storage_recover(void *context) {
    (void)context;
    const app_error_code_t atomic = storage_atomic_recover_all();
    if (atomic != APP_ERROR_NONE) {
        return atomic;
    }
    const app_error_code_t transactions = storage_transaction_recover_all();
    if (transactions != APP_ERROR_NONE) {
        return transactions;
    }
    return storage_quarantine_recover_all();
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
    const app_error_code_t lock = storage_repository_lock_deinit();
    const app_error_code_t unmount = storage_unmount_all();
    return unmount != APP_ERROR_NONE ? unmount : lock;
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

static app_error_code_t adapter_provisioning_deinit(void *context) {
    (void)context;
    return provisioning_deinit();
}

static app_error_code_t adapter_nvs_deinit(void *context) {
    (void)context;
    return nvs_flash_deinit() == ESP_OK ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static bool adapter_http_owns_resources(void *context) {
    (void)context;
    return web_server_owns_resources();
}

static bool adapter_wifi_owns_resources(void *context) {
    (void)context;
    return wifi_ap_owns_resources();
}

static bool adapter_storage_owns_mount(void *context) {
    (void)context;
    const storage_mount_state_t state = storage_mount_state();
    return state.web_mounted || state.data_mounted;
}

static bool adapter_provisioning_owns_resources(void *context) {
    (void)context;
    return provisioning_owns_resources();
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
    case APP_CORE_LOG_MANUFACTURING_CREDENTIALS:
#if CONFIG_APP_MANUFACTURING_PROVISIONING_LOG
        ESP_LOGE(TAG, "MANUFACTURING MODE ENABLED: plaintext one-time credentials follow; "
                      "never deploy this build");
        ESP_LOGW(TAG, "manufacturing-only AP SSID: %s", event->ssid);
        ESP_LOGW(TAG, "manufacturing-only AP passphrase: %s", event->ap_passphrase);
        ESP_LOGW(TAG, "manufacturing-only setup code: %s", event->setup_code);
#else
        ESP_LOGE(TAG, "manufacturing credential event rejected by production build");
#endif
        break;
    case APP_CORE_LOG_PROVISIONING_REQUIRED:
        ESP_LOGW(TAG, "device is unprovisioned; starting protected setup-only service");
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
        .provisioning_init = adapter_provisioning_init,
        .provisioning_load = adapter_provisioning_load,
        .bootstrap_derive = adapter_bootstrap_derive,
        .storage_mount = adapter_storage_mount,
        .storage_recover = adapter_storage_recover,
        .repository_init = adapter_repository_init,
        .auth_init = adapter_auth_init,
        .usb_init = adapter_usb_init,
        .executor_init = adapter_executor_init,
        .controls_init = adapter_controls_init,
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
        .provisioning_deinit = adapter_provisioning_deinit,
        .nvs_deinit = adapter_nvs_deinit,
        .http_owns_resources = adapter_http_owns_resources,
        .wifi_owns_resources = adapter_wifi_owns_resources,
        .storage_owns_mount = adapter_storage_owns_mount,
        .provisioning_owns_resources = adapter_provisioning_owns_resources,
        .set_indicator = adapter_set_indicator,
        .secure_zero = adapter_secure_zero,
        .log_event = adapter_log_event,
    };
    const app_core_policy_t policy = {
#if CONFIG_APP_MANUFACTURING_PROVISIONING_LOG
        .manufacturing_provisioning_enabled = true,
#else
        .manufacturing_provisioning_enabled = false,
#endif
    };
    return app_core_sequence_start(&operations, &policy);
}
