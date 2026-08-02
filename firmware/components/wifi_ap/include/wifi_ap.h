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
