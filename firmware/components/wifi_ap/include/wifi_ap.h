#ifndef WIFI_AP_H
#define WIFI_AP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "subsystem_health.h"

#define WIFI_AP_SSID_MAX_BYTES 32U
#define WIFI_AP_PASSPHRASE_MIN_BYTES 12U
#define WIFI_AP_PASSPHRASE_MAX_BYTES 63U
#define WIFI_AP_MAX_CLIENTS 4U
#define WIFI_AP_DEFAULT_CHANNEL 1U

typedef enum {
    WIFI_AP_STOPPED = 0,
    WIFI_AP_STARTING,
    WIFI_AP_READY,
    WIFI_AP_ERROR
} wifi_ap_state_t;

typedef struct {
    wifi_ap_state_t state;
    size_t client_count;
    app_error_code_t last_error;
    app_error_code_t cleanup_error;
} wifi_ap_status_t;

app_error_code_t wifi_ap_start(const char *ssid, const char *passphrase);
app_error_code_t wifi_ap_stop(void);
wifi_ap_status_t wifi_ap_get_status(void);

#define WIFI_STA_IP_STRING_BYTES 16U

/* SPEC_V2.md §12.1: "at most one station network is remembered"; a join
 * failure "MUST NOT ... cause an unbounded retry loop." This bounds a single
 * wifi_ap_connect_station() call to at most this many connection attempts
 * before giving up and publishing WIFI_STATION_FAILED. */
#define WIFI_STATION_MAX_ATTEMPTS 3U

typedef enum {
    /* No station network is configured, or the device has not attempted to
     * join one since boot/reconfiguration. */
    WIFI_STATION_DISABLED = 0,
    /* A bounded connection attempt is in progress. */
    WIFI_STATION_CONNECTING,
    /* The most recent attempt obtained a DHCP lease. */
    WIFI_STATION_CONNECTED,
    /* WIFI_STATION_MAX_ATTEMPTS attempts were exhausted without success; the
     * access point remains unaffected (SPEC_V2.md §12.1). No further
     * automatic attempt is made until wifi_ap_connect_station() is called
     * again. */
    WIFI_STATION_FAILED
} wifi_station_state_t;

typedef struct {
    wifi_station_state_t state;
    app_error_code_t last_error;
    uint32_t attempt_count;
} wifi_station_status_t;

/* Connects to an existing Wi-Fi network in station mode - SPEC.md's
 * previously deferred "station mode" feature, implemented at the repository
 * owner's explicit request to support serial-console-driven Wi-Fi
 * debugging (see firmware/components/serial_console). Deliberately not
 * part of the reviewed production security model: this is a debug/dev
 * command surface, not gated by session or physical confirmation the way
 * the HTTP API is.
 *
 * Runs alongside the existing AP (switches to WIFI_MODE_APSTA), so the
 * device's own SoftAP keeps serving setup clients. Blocks the calling task
 * up to timeout_ms waiting for a DHCP lease; on success writes the
 * dotted-quad IP address into out_ip (out_ip_size must be at least
 * WIFI_STA_IP_STRING_BYTES). Returns APP_ERROR_INTERNAL if the target AP
 * rejects or drops the connection (wrong password, not found, etc.) before
 * a lease is obtained, or APP_ERROR_TIMEOUT if no outcome is observed
 * within timeout_ms. */
app_error_code_t wifi_ap_connect_station(const char *ssid, const char *password,
                                         uint32_t timeout_ms, char *out_ip, size_t out_ip_size);

/* Current station status, published by wifi_ap_connect_station() as it runs
 * its bounded attempt loop. WIFI_STATION_DISABLED until the first call. Feeds
 * the SPEC_V2.md §13.13/13.6 diagnostics/status "station" field the web_server
 * component builds. */
wifi_station_status_t wifi_ap_get_station_status(void);

/* True when the AP still holds any acquired resource and must be stopped/cleaned
 * up (FIX1 §11.2); used by the lifecycle owner to decide whether teardown is
 * required. */
bool wifi_ap_owns_resources(void);

/* Derives a stable subsystem_health_state_t for Phase 19 diagnostics (FIX1
 * handoff §7.1) from the existing wifi_ap_status_t: FAILED if an error is
 * recorded or the state machine is in WIFI_AP_ERROR, UNAVAILABLE while
 * stopped, RECOVERING while starting (not yet serving clients), HEALTHY once
 * ready. */
subsystem_health_state_t wifi_ap_health_derive_state(wifi_ap_status_t status);

#endif
