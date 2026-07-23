#ifndef WIFI_AP_H
#define WIFI_AP_H

#include <stddef.h>
#include "app_error.h"

typedef enum {
    WIFI_AP_STOPPED = 0,
    WIFI_AP_STARTING,
    WIFI_AP_READY,
    WIFI_AP_ERROR
} wifi_ap_state_t;

typedef struct {
    wifi_ap_state_t state;
    size_t client_count;
} wifi_ap_status_t;

app_error_code_t wifi_ap_start(const char *ssid, const char *passphrase);
app_error_code_t wifi_ap_stop(void);
wifi_ap_status_t wifi_ap_get_status(void);

#endif
