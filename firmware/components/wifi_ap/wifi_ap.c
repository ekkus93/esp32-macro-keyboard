#include "wifi_ap.h"

#include <stdbool.h>
#include <string.h>

#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"

#define WIFI_AP_MAX_CLIENTS 4U

static portMUX_TYPE status_lock = portMUX_INITIALIZER_UNLOCKED;
static wifi_ap_status_t status = {.state = WIFI_AP_STOPPED, .client_count = 0U};
static esp_netif_t *ap_netif;
static bool wifi_initialized;
static bool handler_registered;

static void set_status(wifi_ap_state_t state, size_t clients)
{
    portENTER_CRITICAL(&status_lock);
    status.state = state;
    status.client_count = clients;
    portEXIT_CRITICAL(&status_lock);
}

wifi_ap_status_t wifi_ap_get_status(void)
{
    portENTER_CRITICAL(&status_lock);
    const wifi_ap_status_t snapshot = status;
    portEXIT_CRITICAL(&status_lock);
    return snapshot;
}

static void wifi_event(void *argument, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void)argument;
    (void)base;
    (void)event_data;
    const wifi_ap_status_t current = wifi_ap_get_status();
    if (event_id == WIFI_EVENT_AP_START) {
        set_status(WIFI_AP_READY, current.client_count);
    } else if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        const size_t next = current.client_count < WIFI_AP_MAX_CLIENTS
                                ? current.client_count + 1U
                                : WIFI_AP_MAX_CLIENTS;
        set_status(current.state, next);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        set_status(current.state, current.client_count > 0U ? current.client_count - 1U : 0U);
    } else if (event_id == WIFI_EVENT_AP_STOP) {
        set_status(WIFI_AP_STOPPED, 0U);
    }
}

static app_error_code_t rollback_start(void)
{
    app_error_code_t result = APP_ERROR_NONE;
    if (handler_registered) {
        if (esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event) != ESP_OK) {
            result = APP_ERROR_INTERNAL;
        } else {
            handler_registered = false;
        }
    }
    if (wifi_initialized) {
        const esp_err_t deinit_result = esp_wifi_deinit();
        if (deinit_result != ESP_OK && result == APP_ERROR_NONE) {
            result = APP_ERROR_INTERNAL;
        }
        if (deinit_result == ESP_OK) {
            wifi_initialized = false;
        }
    }
    if (ap_netif != NULL) {
        esp_netif_destroy(ap_netif);
        ap_netif = NULL;
    }
    set_status(WIFI_AP_ERROR, 0U);
    return result;
}

app_error_code_t wifi_ap_start(const char *ssid, const char *passphrase)
{
    if (wifi_ap_get_status().state != WIFI_AP_STOPPED || ssid == NULL || passphrase == NULL ||
        strlen(ssid) == 0U || strlen(ssid) > 32U || strlen(passphrase) < 12U ||
        strlen(passphrase) > 63U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    set_status(WIFI_AP_STARTING, 0U);
    if (esp_netif_init() != ESP_OK) {
        set_status(WIFI_AP_ERROR, 0U);
        return APP_ERROR_INTERNAL;
    }
    const esp_err_t event_result = esp_event_loop_create_default();
    if (event_result != ESP_OK && event_result != ESP_ERR_INVALID_STATE) {
        set_status(WIFI_AP_ERROR, 0U);
        return APP_ERROR_INTERNAL;
    }
    ap_netif = esp_netif_create_default_wifi_ap();
    if (ap_netif == NULL) {
        set_status(WIFI_AP_ERROR, 0U);
        return APP_ERROR_INTERNAL;
    }
    const wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&init) != ESP_OK) {
        const app_error_code_t cleanup = rollback_start();
        return cleanup == APP_ERROR_NONE ? APP_ERROR_INTERNAL : cleanup;
    }
    wifi_initialized = true;
    if (esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event, NULL) != ESP_OK) {
        const app_error_code_t cleanup = rollback_start();
        return cleanup == APP_ERROR_NONE ? APP_ERROR_INTERNAL : cleanup;
    }
    handler_registered = true;

    wifi_config_t configuration = {0};
    const size_t ssid_length = strlen(ssid);
    const size_t passphrase_length = strlen(passphrase);
    memcpy(configuration.ap.ssid, ssid, ssid_length);
    configuration.ap.ssid_len = (uint8_t)ssid_length;
    memcpy(configuration.ap.password, passphrase, passphrase_length);
    configuration.ap.channel = 1U;
    configuration.ap.max_connection = WIFI_AP_MAX_CLIENTS;
    configuration.ap.authmode = WIFI_AUTH_WPA2_WPA3_PSK;
    configuration.ap.pmf_cfg.required = true;

    if (esp_wifi_set_mode(WIFI_MODE_AP) != ESP_OK ||
        esp_wifi_set_config(WIFI_IF_AP, &configuration) != ESP_OK ||
        esp_wifi_start() != ESP_OK) {
        const app_error_code_t cleanup = rollback_start();
        return cleanup == APP_ERROR_NONE ? APP_ERROR_INTERNAL : cleanup;
    }
    return APP_ERROR_NONE;
}

app_error_code_t wifi_ap_stop(void)
{
    const wifi_ap_state_t current = wifi_ap_get_status().state;
    if (current == WIFI_AP_STOPPED) {
        return APP_ERROR_NONE;
    }
    app_error_code_t result = APP_ERROR_NONE;
    if (esp_wifi_stop() != ESP_OK) {
        result = APP_ERROR_INTERNAL;
    }
    const app_error_code_t cleanup = rollback_start();
    set_status(WIFI_AP_STOPPED, 0U);
    return result != APP_ERROR_NONE ? result : cleanup;
}
