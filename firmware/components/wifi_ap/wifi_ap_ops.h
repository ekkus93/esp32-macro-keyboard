#ifndef WIFI_AP_OPS_H
#define WIFI_AP_OPS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "wifi_ap.h"

#define WIFI_AP_SSID_MAX_BYTES 32U
#define WIFI_AP_PASSPHRASE_MIN_BYTES 12U
#define WIFI_AP_PASSPHRASE_MAX_BYTES 63U
#define WIFI_AP_MAX_CLIENTS 4U
#define WIFI_AP_DEFAULT_CHANNEL 1U

typedef enum {
    WIFI_AP_EVENT_LOOP_CREATED = 0,
    WIFI_AP_EVENT_LOOP_ALREADY_EXISTS,
    WIFI_AP_EVENT_LOOP_FAILED
} wifi_ap_event_loop_result_t;

typedef struct {
    char ssid[WIFI_AP_SSID_MAX_BYTES + 1U];
    size_t ssid_length;
    char passphrase[WIFI_AP_PASSPHRASE_MAX_BYTES + 1U];
    uint8_t channel;
    uint8_t maximum_clients;
    bool wpa2_wpa3_psk;
    bool pmf_required;
} wifi_ap_runtime_config_t;

typedef struct {
    void *context;
    wifi_ap_status_t (*status_get)(void *context);
    void (*status_set)(void *context, const wifi_ap_status_t *status);
    app_error_code_t (*netif_init)(void *context);
    wifi_ap_event_loop_result_t (*event_loop_create)(void *context);
    app_error_code_t (*netif_create)(void *context);
    app_error_code_t (*wifi_init)(void *context);
    app_error_code_t (*handler_register)(void *context);
    app_error_code_t (*set_mode_ap)(void *context);
    app_error_code_t (*set_config)(void *context,
                                   const wifi_ap_runtime_config_t *configuration);
    app_error_code_t (*wifi_start)(void *context);
    app_error_code_t (*wifi_stop)(void *context);
    app_error_code_t (*handler_unregister)(void *context);
    app_error_code_t (*wifi_deinit)(void *context);
    app_error_code_t (*netif_destroy)(void *context);
} wifi_ap_ops_t;

#endif
