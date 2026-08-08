#ifndef WIFI_AP_STATION_H
#define WIFI_AP_STATION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "wifi_ap.h"

/* Host-testable bounded-retry engine behind wifi_ap_connect_station(). The
 * hardware adapter (wifi_ap.c) supplies connect_attempt as a single blocking
 * ESP-IDF connection attempt; this module owns only the retry bound and the
 * wifi_station_status_t state machine, so both are exercised by host tests
 * through a fake connect_attempt without touching esp_wifi_*. */
typedef struct {
    void *context;
    wifi_station_status_t (*status_get)(void *context);
    void (*status_set)(void *context, const wifi_station_status_t *status);
    app_error_code_t (*connect_attempt)(void *context, const char *ssid, const char *password,
                                        uint32_t timeout_ms, char *out_ip, size_t out_ip_size);
} wifi_ap_station_ops_t;

bool wifi_ap_station_ops_is_valid(const wifi_ap_station_ops_t *operations);

/* Runs at most WIFI_STATION_MAX_ATTEMPTS calls to operations->connect_attempt,
 * publishing WIFI_STATION_CONNECTING before each attempt. Stops at the first
 * successful attempt and publishes WIFI_STATION_CONNECTED, or, once attempts
 * are exhausted, publishes WIFI_STATION_FAILED carrying the last attempt's
 * error (SPEC_V2.md §12.1: bounded, never an unbounded retry loop). Returns
 * APP_ERROR_NONE on success or the last attempt's error on exhaustion; the
 * access point is never touched by this module either way. */
app_error_code_t wifi_ap_station_connect(const wifi_ap_station_ops_t *operations, const char *ssid,
                                         const char *password, uint32_t timeout_ms, char *out_ip,
                                         size_t out_ip_size);

#endif
