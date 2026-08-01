#include "wifi_ap.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "wifi_ap_ops.h"
#include "wifi_ap_state.h"

static const char *const TAG = "wifi_ap";

static portMUX_TYPE status_lock = portMUX_INITIALIZER_UNLOCKED;
static wifi_ap_status_t status = {
    .state = WIFI_AP_STOPPED,
    .client_count = 0U,
    .last_error = APP_ERROR_NONE,
    .cleanup_error = APP_ERROR_NONE,
};
static esp_netif_t *ap_netif;
static wifi_ap_engine_t engine;
static bool engine_initialized;

static wifi_ap_status_t adapter_status_get(void *context) {
    (void)context;
    portENTER_CRITICAL(&status_lock);
    const wifi_ap_status_t snapshot = status;
    portEXIT_CRITICAL(&status_lock);
    return snapshot;
}

static void adapter_status_set(void *context, const wifi_ap_status_t *next) {
    (void)context;
    if (next == NULL) {
        return;
    }
    portENTER_CRITICAL(&status_lock);
    status = *next;
    portEXIT_CRITICAL(&status_lock);
}

static app_error_code_t adapter_netif_init(void *context) {
    (void)context;
    return esp_netif_init() == ESP_OK ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static wifi_ap_event_loop_result_t adapter_event_loop_create(void *context) {
    (void)context;
    const esp_err_t result = esp_event_loop_create_default();
    if (result == ESP_OK) {
        return WIFI_AP_EVENT_LOOP_CREATED;
    }
    return result == ESP_ERR_INVALID_STATE ? WIFI_AP_EVENT_LOOP_ALREADY_EXISTS
                                           : WIFI_AP_EVENT_LOOP_FAILED;
}

static app_error_code_t adapter_netif_create(void *context) {
    (void)context;
    ap_netif = esp_netif_create_default_wifi_ap();
    return ap_netif != NULL ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static app_error_code_t adapter_wifi_init(void *context) {
    (void)context;
    const wifi_init_config_t configuration = WIFI_INIT_CONFIG_DEFAULT();
    return esp_wifi_init(&configuration) == ESP_OK ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static void wifi_event(void *argument, esp_event_base_t base, int32_t event_id, void *event_data) {
    (void)argument;
    (void)base;
    (void)event_data;
    wifi_ap_event_t event = WIFI_AP_EVENT_UNKNOWN;
    if (event_id == WIFI_EVENT_AP_START) {
        event = WIFI_AP_EVENT_STARTED;
    } else if (event_id == WIFI_EVENT_AP_STOP) {
        event = WIFI_AP_EVENT_STOPPED;
    } else if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        event = WIFI_AP_EVENT_CLIENT_CONNECTED;
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        event = WIFI_AP_EVENT_CLIENT_DISCONNECTED;
    }
    wifi_ap_engine_handle_event(&engine, event);
}

static app_error_code_t adapter_handler_register(void *context) {
    (void)context;
    return esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event, NULL) == ESP_OK
               ? APP_ERROR_NONE
               : APP_ERROR_INTERNAL;
}

static app_error_code_t adapter_set_mode_ap(void *context) {
    (void)context;
    return esp_wifi_set_mode(WIFI_MODE_AP) == ESP_OK ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static app_error_code_t adapter_set_config(void *context,
                                           const wifi_ap_runtime_config_t *runtime_configuration) {
    (void)context;
    if (runtime_configuration == NULL ||
        runtime_configuration->ssid_length > WIFI_AP_SSID_MAX_BYTES ||
        !runtime_configuration->wpa2_wpa3_psk || !runtime_configuration->pmf_required) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    wifi_config_t configuration = {0};
    memcpy(configuration.ap.ssid, runtime_configuration->ssid, runtime_configuration->ssid_length);
    configuration.ap.ssid_len = (uint8_t)runtime_configuration->ssid_length;
    memcpy(configuration.ap.password, runtime_configuration->passphrase,
           strlen(runtime_configuration->passphrase));
    configuration.ap.channel = runtime_configuration->channel;
    configuration.ap.max_connection = runtime_configuration->maximum_clients;
    configuration.ap.authmode = WIFI_AUTH_WPA2_WPA3_PSK;
    configuration.ap.pmf_cfg.required = true;
    return esp_wifi_set_config(WIFI_IF_AP, &configuration) == ESP_OK ? APP_ERROR_NONE
                                                                     : APP_ERROR_INTERNAL;
}

static app_error_code_t adapter_wifi_start(void *context) {
    (void)context;
    return esp_wifi_start() == ESP_OK ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static app_error_code_t adapter_wifi_stop(void *context) {
    (void)context;
    const esp_err_t result = esp_wifi_stop();
    return result == ESP_OK || result == ESP_ERR_WIFI_NOT_STARTED ? APP_ERROR_NONE
                                                                  : APP_ERROR_INTERNAL;
}

static app_error_code_t adapter_handler_unregister(void *context) {
    (void)context;
    return esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event) == ESP_OK
               ? APP_ERROR_NONE
               : APP_ERROR_INTERNAL;
}

static app_error_code_t adapter_wifi_deinit(void *context) {
    (void)context;
    return esp_wifi_deinit() == ESP_OK ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static app_error_code_t adapter_netif_destroy(void *context) {
    (void)context;
    if (ap_netif != NULL) {
        esp_netif_destroy(ap_netif);
        ap_netif = NULL;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t ensure_engine(void) {
    if (engine_initialized) {
        return APP_ERROR_NONE;
    }
    const wifi_ap_ops_t operations = {
        .context = NULL,
        .status_get = adapter_status_get,
        .status_set = adapter_status_set,
        .netif_init = adapter_netif_init,
        .event_loop_create = adapter_event_loop_create,
        .netif_create = adapter_netif_create,
        .wifi_init = adapter_wifi_init,
        .handler_register = adapter_handler_register,
        .set_mode_ap = adapter_set_mode_ap,
        .set_config = adapter_set_config,
        .wifi_start = adapter_wifi_start,
        .wifi_stop = adapter_wifi_stop,
        .handler_unregister = adapter_handler_unregister,
        .wifi_deinit = adapter_wifi_deinit,
        .netif_destroy = adapter_netif_destroy,
    };
    const app_error_code_t result = wifi_ap_engine_init(&engine, &operations);
    if (result == APP_ERROR_NONE) {
        engine_initialized = true;
    }
    return result;
}

wifi_ap_status_t wifi_ap_get_status(void) {
    return adapter_status_get(NULL);
}

app_error_code_t wifi_ap_start(const char *ssid, const char *passphrase) {
    const app_error_code_t init_result = ensure_engine();
    return init_result == APP_ERROR_NONE ? wifi_ap_engine_start(&engine, ssid, passphrase)
                                         : init_result;
}

app_error_code_t wifi_ap_stop(void) {
    const app_error_code_t init_result = ensure_engine();
    return init_result == APP_ERROR_NONE ? wifi_ap_engine_stop(&engine) : init_result;
}

bool wifi_ap_owns_resources(void) {
    return engine_initialized && wifi_ap_engine_owns_resources(&engine);
}

/* --- Station mode (debug console feature; see wifi_ap.h) --- */

static esp_netif_t *sta_netif;
static bool sta_netif_created;
static SemaphoreHandle_t sta_outcome_semaphore;
static volatile bool sta_got_ip;
static volatile uint32_t sta_ip_addr;
static volatile int32_t sta_disconnect_reason;

static void station_event(void *argument, esp_event_base_t base, int32_t event_id,
                          void *event_data) {
    (void)argument;
    if (base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        const wifi_event_sta_disconnected_t *disconnected = event_data;
        sta_disconnect_reason = disconnected != NULL ? disconnected->reason : -1;
        ESP_LOGW(TAG, "station disconnected, reason %" PRId32, sta_disconnect_reason);
        if (sta_outcome_semaphore != NULL) {
            xSemaphoreGive(sta_outcome_semaphore);
        }
    } else if (base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *got_ip = event_data;
        sta_got_ip = true;
        sta_ip_addr = got_ip != NULL ? got_ip->ip_info.ip.addr : 0U;
        if (sta_outcome_semaphore != NULL) {
            xSemaphoreGive(sta_outcome_semaphore);
        }
    }
}

static app_error_code_t ensure_station_netif(void) {
    if (sta_netif_created) {
        return APP_ERROR_NONE;
    }
    if (esp_netif_init() != ESP_OK) {
        return APP_ERROR_INTERNAL;
    }
    const esp_err_t event_loop_result = esp_event_loop_create_default();
    if (event_loop_result != ESP_OK && event_loop_result != ESP_ERR_INVALID_STATE) {
        return APP_ERROR_INTERNAL;
    }
    sta_netif = esp_netif_create_default_wifi_sta();
    if (sta_netif == NULL) {
        return APP_ERROR_INTERNAL;
    }
    if (esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, station_event, NULL) !=
            ESP_OK ||
        esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, station_event, NULL) != ESP_OK) {
        return APP_ERROR_INTERNAL;
    }
    sta_netif_created = true;
    return APP_ERROR_NONE;
}

app_error_code_t wifi_ap_connect_station(const char *ssid, const char *password,
                                         uint32_t timeout_ms, char *out_ip, size_t out_ip_size) {
    if (ssid == NULL || password == NULL || out_ip == NULL ||
        out_ip_size < WIFI_STA_IP_STRING_BYTES || strlen(ssid) == 0U ||
        strlen(ssid) >= sizeof(((wifi_sta_config_t *)NULL)->ssid) ||
        strlen(password) >= sizeof(((wifi_sta_config_t *)NULL)->password)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    const app_error_code_t netif_result = ensure_station_netif();
    if (netif_result != APP_ERROR_NONE) {
        return netif_result;
    }
    if (sta_outcome_semaphore == NULL) {
        sta_outcome_semaphore = xSemaphoreCreateBinary();
        if (sta_outcome_semaphore == NULL) {
            return APP_ERROR_INTERNAL;
        }
    }
    (void)xSemaphoreTake(sta_outcome_semaphore, 0);
    sta_got_ip = false;
    sta_disconnect_reason = 0;

    if (esp_wifi_set_mode(WIFI_MODE_APSTA) != ESP_OK) {
        return APP_ERROR_INTERNAL;
    }
    wifi_config_t configuration = {0};
    memcpy(configuration.sta.ssid, ssid, strlen(ssid));
    memcpy(configuration.sta.password, password, strlen(password));
    if (esp_wifi_set_config(WIFI_IF_STA, &configuration) != ESP_OK) {
        return APP_ERROR_INTERNAL;
    }
    if (esp_wifi_connect() != ESP_OK) {
        return APP_ERROR_INTERNAL;
    }

    const TickType_t ticks_to_wait = pdMS_TO_TICKS(timeout_ms);
    if (xSemaphoreTake(sta_outcome_semaphore, ticks_to_wait) != pdTRUE) {
        return APP_ERROR_TIMEOUT;
    }
    if (!sta_got_ip) {
        return APP_ERROR_INTERNAL;
    }
    const uint32_t address = sta_ip_addr;
    const int written =
        snprintf(out_ip, out_ip_size, "%lu.%lu.%lu.%lu", (unsigned long)(address & 0xFFU),
                 (unsigned long)((address >> 8) & 0xFFU), (unsigned long)((address >> 16) & 0xFFU),
                 (unsigned long)((address >> 24) & 0xFFU));
    return written > 0 && (size_t)written < out_ip_size ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}
