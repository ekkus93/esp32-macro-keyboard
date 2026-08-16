#ifndef PROVISIONING_H
#define PROVISIONING_H

/* Record shapes only.
 *
 * The NVS-backed provisioning store this header used to declare
 * (provisioning.c / provisioning_core.c) was deleted 2026-08-16: the v2 device
 * settings record supersedes it and the V2-140 firmware audit found its entire
 * public API unreachable from shipped firmware. What remains here are the two
 * structs still named by the retained LEGACY / NOT SHIPPED setup reference code
 * (web_setup_core.*), which is on disk but not compiled. */

#include <stdbool.h>
#include <stdint.h>

#include "app_uuid.h"
#include "auth.h"
#include "wifi_ap.h"

#define PROVISIONING_CREDENTIAL_VERSION_INITIAL 1U

typedef struct {
    uint32_t schema_version;
    uint32_t revision;
    uint32_t credential_version;
    bool provisioned;
    char ap_ssid[WIFI_AP_SSID_MAX_BYTES + 1U];
    char ap_passphrase[WIFI_AP_PASSPHRASE_MAX_BYTES + 1U];
    auth_password_record_t password_record;
    bool require_physical_confirmation;
    bool always_select_package;
    /* Station-mode credentials for an existing Wi-Fi network, persisted in the
     * same NVS record as everything else the device remembers. When present the
     * device joins this network at boot and falls back to AP-only if it cannot
     * (SPEC 15.1). An empty password is a valid open network. */
    bool has_station;
    char station_ssid[WIFI_AP_SSID_MAX_BYTES + 1U];
    char station_password[WIFI_AP_PASSPHRASE_MAX_BYTES + 1U];
} provisioning_config_t;

typedef struct {
    uint32_t schema_version;
    uint32_t revision;
    bool require_physical_confirmation;
    bool always_select_package;
} provisioning_settings_t;

#endif
