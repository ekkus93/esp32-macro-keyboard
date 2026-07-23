#include "app_core.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "auth.h"
#include "device_controls.h"
#include "esp_log.h"
#include "esp_random.h"
#include "macro_executor.h"
#include "nvs_flash.h"
#include "sdkconfig.h"
#include "storage.h"
#include "usb_keyboard.h"
#include "web_server.h"
#include "wifi_ap.h"

#define DEVELOPMENT_PASSWORD_BYTES 20U

static const char *const TAG = "app_core";

static app_error_code_t initialize_nvs(void)
{
    const esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGE(TAG, "NVS requires explicit recovery; automatic erase is prohibited");
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return result == ESP_OK ? APP_ERROR_NONE : APP_ERROR_STORAGE_UNAVAILABLE;
}

static app_error_code_t stage(const char *name, app_error_code_t result)
{
    if (result == APP_ERROR_NONE) {
        ESP_LOGI(TAG, "stage complete: %s", name);
    } else {
        ESP_LOGE(TAG, "stage failed: %s (%s)", name, app_error_code_string(result));
    }
    return result;
}

static app_error_code_t random_password(char *output, size_t output_size)
{
    static const char alphabet[] =
        "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789";
    if (output == NULL || output_size < DEVELOPMENT_PASSWORD_BYTES + 1U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    uint8_t random_bytes[DEVELOPMENT_PASSWORD_BYTES];
    esp_fill_random(random_bytes, sizeof(random_bytes));
    for (size_t index = 0U; index < sizeof(random_bytes); ++index) {
        output[index] = alphabet[random_bytes[index] % (sizeof(alphabet) - 1U)];
    }
    output[DEVELOPMENT_PASSWORD_BYTES] = '\0';
    return APP_ERROR_NONE;
}

static app_error_code_t cleanup_after_failure(bool web_started,
                                              bool wifi_started,
                                              bool storage_mounted,
                                              app_error_code_t original)
{
    app_error_code_t cleanup_error = APP_ERROR_NONE;
    if (web_started) {
        const app_error_code_t result = web_server_stop();
        if (result != APP_ERROR_NONE && cleanup_error == APP_ERROR_NONE) {
            cleanup_error = result;
        }
    }
    if (wifi_started) {
        const app_error_code_t result = wifi_ap_stop();
        if (result != APP_ERROR_NONE && cleanup_error == APP_ERROR_NONE) {
            cleanup_error = result;
        }
    }
    if (storage_mounted) {
        const app_error_code_t result = storage_unmount_all();
        if (result != APP_ERROR_NONE && cleanup_error == APP_ERROR_NONE) {
            cleanup_error = result;
        }
    }
    if (cleanup_error != APP_ERROR_NONE) {
        ESP_LOGE(TAG,
                 "cleanup failed after %s: %s",
                 app_error_code_string(original),
                 app_error_code_string(cleanup_error));
    }
    device_controls_set_indicator(DEVICE_INDICATOR_FATAL);
    return original;
}

app_error_code_t app_core_start(void)
{
    bool storage_mounted = false;
    bool wifi_started = false;
    bool web_started = false;
    bool storage_degraded = false;

    device_controls_set_indicator(DEVICE_INDICATOR_BOOTING);
    app_error_code_t result = stage("nvs", initialize_nvs());
    if (result != APP_ERROR_NONE) {
        return cleanup_after_failure(web_started, wifi_started, storage_mounted, result);
    }

    result = stage("storage_mount", storage_mount_all());
    if (result != APP_ERROR_NONE) {
        return cleanup_after_failure(web_started, wifi_started, storage_mounted, result);
    }
    storage_mounted = true;

    result = storage_transaction_recover_all();
    if (result == APP_ERROR_STORAGE_CORRUPT) {
        storage_degraded = true;
        ESP_LOGW(TAG, "storage recovery requires operator review; evidence was preserved");
    } else if (stage("storage_recovery", result) != APP_ERROR_NONE) {
        return cleanup_after_failure(web_started, wifi_started, storage_mounted, result);
    }

    result = stage("authentication", auth_init());
    if (result != APP_ERROR_NONE) {
        return cleanup_after_failure(web_started, wifi_started, storage_mounted, result);
    }
    result = stage("usb", usb_keyboard_init());
    if (result != APP_ERROR_NONE) {
        return cleanup_after_failure(web_started, wifi_started, storage_mounted, result);
    }
    result = stage("executor", macro_executor_init());
    if (result != APP_ERROR_NONE) {
        return cleanup_after_failure(web_started, wifi_started, storage_mounted, result);
    }
    result = stage("controls", device_controls_init());
    if (result != APP_ERROR_NONE) {
        return cleanup_after_failure(web_started, wifi_started, storage_mounted, result);
    }

#if CONFIG_APP_DEVELOPMENT_PROVISIONING_LOG
    char ap_passphrase[DEVELOPMENT_PASSWORD_BYTES + 1U];
    char web_password[DEVELOPMENT_PASSWORD_BYTES + 1U];
    result = random_password(ap_passphrase, sizeof(ap_passphrase));
    if (result == APP_ERROR_NONE) {
        result = random_password(web_password, sizeof(web_password));
    }
    web_server_config_t web_configuration = {.login_enabled = false};
    if (result == APP_ERROR_NONE) {
        result = auth_password_create(web_password,
                                      DEVELOPMENT_PASSWORD_BYTES,
                                      &web_configuration.password_record);
    }
    if (result != APP_ERROR_NONE) {
        return cleanup_after_failure(web_started, wifi_started, storage_mounted, result);
    }
    web_configuration.login_enabled = true;

    ESP_LOGW(TAG, "development-only AP SSID: ESP32-Macro-Setup");
    ESP_LOGW(TAG, "development-only AP passphrase: %s", ap_passphrase);
    ESP_LOGW(TAG, "development-only web password: %s", web_password);

    result = stage("wifi", wifi_ap_start("ESP32-Macro-Setup", ap_passphrase));
    if (result != APP_ERROR_NONE) {
        return cleanup_after_failure(web_started, wifi_started, storage_mounted, result);
    }
    wifi_started = true;

    result = stage("http", web_server_start(&web_configuration));
    if (result != APP_ERROR_NONE) {
        return cleanup_after_failure(web_started, wifi_started, storage_mounted, result);
    }
    web_started = true;
#else
    ESP_LOGE(TAG,
             "persistent encrypted provisioning is not implemented; refusing to start a network");
    result = APP_ERROR_AUTH_REQUIRED;
    return cleanup_after_failure(web_started, wifi_started, storage_mounted, result);
#endif

    device_controls_set_indicator(storage_degraded ? DEVICE_INDICATOR_DEGRADED
                                                    : DEVICE_INDICATOR_READY);
    return APP_ERROR_NONE;
}
