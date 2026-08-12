#include "app_core.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_core_ops.h"
#include "app_core_sequence.h"
#include "app_error.h"
#include "app_lifecycle_health.h"
#include "auth.h"
#include "auth_health.h"
#include "device_controls.h"
#include "device_settings.h"
#include "device_settings_v2.h"
#include "esp_log.h"
#include "esp_random.h"
#include "executor_health.h"
#include "factory_reset_state.h"
#include "http_health.h"
#include "macro_executor.h"
#include "nvs_flash.h"
#include "provisioning_bootstrap.h"
#include "setup_contract_v2.h"
#include "storage.h"
#include "storage_health.h"
#include "usb_health.h"
#include "usb_keyboard.h"
#include "web_server.h"
#include "wifi_ap.h"

static const char *const TAG = "app_core";

static void secure_zero_bytes(void *memory, size_t length) {
    volatile uint8_t *bytes = memory;
    for (size_t index = 0U; index < length; ++index) {
        bytes[index] = 0U;
    }
}

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

static app_error_code_t adapter_settings_init(void *context) {
    (void)context;
    factory_reset_state_t reset_state = FACTORY_RESET_STATE_NONE;
    const app_error_code_t reset_result = factory_reset_state_read(&reset_state);
    if (reset_result != APP_ERROR_NONE) {
        return reset_result;
    }
    if (reset_state == FACTORY_RESET_STATE_PENDING) {
        ESP_LOGE(TAG, "factory reset recovery is pending; normal/setup services are blocked");
        return APP_ERROR_RESET_RECOVERY_REQUIRED;
    }
    return device_settings_init();
}

static app_error_code_t adapter_settings_read(void *context,
                                              app_v2_device_settings_t *out_settings) {
    (void)context;
    return device_settings_read(out_settings);
}

static app_error_code_t adapter_bootstrap_derive(void *context,
                                                 provisioning_bootstrap_t *out_bootstrap) {
    (void)context;
    return provisioning_bootstrap_derive(out_bootstrap);
}

static bool adapter_setup_random_u32(void *context, uint32_t *out_value) {
    (void)context;
    if (out_value == NULL) {
        return false;
    }
    *out_value = esp_random();
    return true;
}

static app_error_code_t adapter_setup_code_generate(void *context,
                                                    char out_code[APP_V2_SETUP_CODE_BUFFER_BYTES]) {
    (void)context;
    if (out_code == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const app_v2_setup_random_ops_t operations = {
        .context = NULL,
        .random_u32 = adapter_setup_random_u32,
    };
    app_v2_setup_session_t session = {0};
    const app_v2_setup_result_t result = app_v2_setup_session_generate(&operations, &session);
    if (result == APP_V2_SETUP_OK) {
        memcpy(out_code, session.code, sizeof(session.code));
    }
    secure_zero_bytes(&session, sizeof(session));
    return result == APP_V2_SETUP_OK ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static app_error_code_t adapter_storage_mount(void *context) {
    (void)context;
    const app_error_code_t result = storage_mount_all();
    storage_health_record_primary(result);
    return result;
}

static app_error_code_t adapter_auth_init(void *context) {
    (void)context;
    const app_error_code_t result = auth_init();
    auth_health_record_primary(result);
    return result;
}

static app_error_code_t adapter_usb_init(void *context) {
    (void)context;
    const app_error_code_t result = usb_keyboard_init();
    usb_health_record_primary(result);
    return result;
}

static app_error_code_t adapter_executor_init(void *context) {
    (void)context;
    const app_error_code_t result = macro_executor_init();
    executor_health_record_primary(result);
    return result;
}

static app_error_code_t adapter_controls_init(void *context) {
    (void)context;
    return device_controls_init();
}

#define WIFI_STATION_BOOT_TIMEOUT_MS 15000U

static app_error_code_t adapter_wifi_start(void *context,
                                           const app_core_wifi_configuration_t *configuration) {
    (void)context;
    if (configuration == NULL || configuration->ap_ssid == NULL ||
        configuration->ap_passphrase == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const app_error_code_t started =
        wifi_ap_start(configuration->ap_ssid, configuration->ap_passphrase);
    if (started != APP_ERROR_NONE) {
        return started;
    }
    if (!configuration->station_configured) {
        return APP_ERROR_NONE;
    }
    if (configuration->station_ssid == NULL || configuration->station_passphrase == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    char ip_address[WIFI_STA_IP_STRING_BYTES] = {0};
    const app_error_code_t joined =
        wifi_ap_connect_station(configuration->station_ssid, configuration->station_passphrase,
                                WIFI_STATION_BOOT_TIMEOUT_MS, ip_address, sizeof(ip_address));
    if (joined == APP_ERROR_NONE) {
        ESP_LOGI(TAG, "joined configured Wi-Fi network, IP address: %s", ip_address);
    } else {
        ESP_LOGW(TAG, "configured Wi-Fi network unavailable (%s); access point remains active",
                 app_error_code_string(joined));
    }
    return APP_ERROR_NONE;
}

static app_error_code_t adapter_http_start(void *context,
                                           const web_server_config_t *configuration) {
    (void)context;
    const app_error_code_t result = web_server_start(configuration);
    http_health_record_primary(result);
    return result;
}

static app_error_code_t adapter_http_stop(void *context) {
    (void)context;
    const app_error_code_t result = web_server_stop();
    http_health_record_cleanup(result, result != APP_ERROR_NONE);
    return result;
}

static app_error_code_t adapter_wifi_stop(void *context) {
    (void)context;
    return wifi_ap_stop();
}

static app_error_code_t adapter_storage_unmount(void *context) {
    (void)context;
    const app_error_code_t result = storage_unmount_all();
    storage_health_record_cleanup(result, result != APP_ERROR_NONE);
    return result;
}

static app_error_code_t adapter_auth_deinit(void *context) {
    (void)context;
    const app_error_code_t result = auth_deinit();
    auth_health_record_cleanup(result, result != APP_ERROR_NONE);
    return result;
}

static app_error_code_t adapter_usb_deinit(void *context) {
    (void)context;
    const app_error_code_t result = usb_keyboard_deinit();
    usb_health_record_cleanup(result, result != APP_ERROR_NONE);
    return result;
}

static app_error_code_t adapter_executor_deinit(void *context) {
    (void)context;
    const app_error_code_t result = macro_executor_deinit();
    executor_health_record_cleanup(result, result != APP_ERROR_NONE);
    return result;
}

static app_error_code_t adapter_controls_deinit(void *context) {
    (void)context;
    return device_controls_deinit();
}

static app_error_code_t adapter_settings_deinit(void *context) {
    (void)context;
    return device_settings_deinit();
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

static app_error_code_t adapter_set_indicator(void *context, device_indicator_state_t indicator) {
    (void)context;
    device_controls_set_indicator(indicator);
    return APP_ERROR_NONE;
}

static void adapter_secure_zero(void *context, void *memory, size_t length) {
    (void)context;
    secure_zero_bytes(memory, length);
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
            app_lifecycle_health_record_stage_failure(event->primary_error);
        }
        break;
    case APP_CORE_LOG_SETUP_CODE:
        ESP_LOGI(TAG, "setup credentials are available from the manufacturing label");
        break;
    case APP_CORE_LOG_PROVISIONING_REQUIRED:
        ESP_LOGW(TAG, "device is unprovisioned; starting protected V2 setup-only service");
        break;
    case APP_CORE_LOG_CLEANUP_FAILED:
        ESP_LOGE(TAG, "cleanup failed after %s: %s (cleanup %s)",
                 app_error_code_string(event->primary_error),
                 app_error_code_string(event->cleanup_error),
                 event->cleanup_incomplete ? "incomplete" : "complete");
        app_lifecycle_health_record_stage_failure(event->primary_error);
        app_lifecycle_health_record_cleanup_failed(event->cleanup_error, event->cleanup_incomplete);
        break;
    default:
        ESP_LOGE(TAG, "unknown startup log event");
        break;
    }
}

app_error_code_t app_core_start(void) {
    app_lifecycle_health_reset();
    storage_health_reset();
    auth_health_reset();
    usb_health_reset();
    executor_health_reset();
    http_health_reset();
    const app_core_ops_t operations = {
        .context = NULL,
        .nvs_init = adapter_nvs_init,
        .settings_init = adapter_settings_init,
        .settings_read = adapter_settings_read,
        .bootstrap_derive = adapter_bootstrap_derive,
        .setup_code_generate = adapter_setup_code_generate,
        .storage_mount = adapter_storage_mount,
        .auth_init = adapter_auth_init,
        .usb_init = adapter_usb_init,
        .executor_init = adapter_executor_init,
        .controls_init = adapter_controls_init,
        .wifi_start = adapter_wifi_start,
        .http_start = adapter_http_start,
        .http_stop = adapter_http_stop,
        .wifi_stop = adapter_wifi_stop,
        .storage_unmount = adapter_storage_unmount,
        .auth_deinit = adapter_auth_deinit,
        .usb_deinit = adapter_usb_deinit,
        .executor_deinit = adapter_executor_deinit,
        .controls_deinit = adapter_controls_deinit,
        .settings_deinit = adapter_settings_deinit,
        .nvs_deinit = adapter_nvs_deinit,
        .http_owns_resources = adapter_http_owns_resources,
        .wifi_owns_resources = adapter_wifi_owns_resources,
        .storage_owns_mount = adapter_storage_owns_mount,
        .set_indicator = adapter_set_indicator,
        .secure_zero = adapter_secure_zero,
        .log_event = adapter_log_event,
    };
    return app_core_sequence_start(&operations);
}

app_lifecycle_health_t app_core_get_health(void) {
    return app_lifecycle_health_snapshot();
}
