#ifndef WIFI_AP_STATE_H
#define WIFI_AP_STATE_H

#include <stdbool.h>

#include "wifi_ap_ops.h"

typedef enum {
    WIFI_AP_EVENT_STARTED = 0,
    WIFI_AP_EVENT_STOPPED,
    WIFI_AP_EVENT_CLIENT_CONNECTED,
    WIFI_AP_EVENT_CLIENT_DISCONNECTED,
    WIFI_AP_EVENT_UNKNOWN
} wifi_ap_event_t;

typedef struct {
    wifi_ap_ops_t operations;
    bool initialized;
    bool netif_created;
    bool wifi_initialized;
    bool handler_registered;
    bool wifi_started;
} wifi_ap_engine_t;

bool wifi_ap_ops_is_valid(const wifi_ap_ops_t *operations);
app_error_code_t wifi_ap_engine_init(wifi_ap_engine_t *engine,
                                     const wifi_ap_ops_t *operations);
app_error_code_t wifi_ap_engine_start(wifi_ap_engine_t *engine,
                                      const char *ssid,
                                      const char *passphrase);
app_error_code_t wifi_ap_engine_stop(wifi_ap_engine_t *engine);
void wifi_ap_engine_handle_event(wifi_ap_engine_t *engine, wifi_ap_event_t event);

#endif
