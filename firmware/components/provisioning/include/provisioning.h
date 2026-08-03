#ifndef PROVISIONING_H
#define PROVISIONING_H

#include <stdbool.h>
#include <stdint.h>

#include "app_error.h"
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

app_error_code_t provisioning_init(void);
app_error_code_t provisioning_load(provisioning_config_t *out_config);
app_error_code_t provisioning_commit(const provisioning_config_t *replacement,
                                     uint32_t expected_revision,
                                     provisioning_config_t *out_committed);
app_error_code_t provisioning_settings_read(provisioning_settings_t *out_settings);
app_error_code_t provisioning_settings_update(const provisioning_settings_t *replacement,
                                              uint32_t expected_revision,
                                              provisioning_settings_t *out_committed);
/* Persists station-mode credentials, or clears them when ssid is NULL/empty.
 * Commits to NVS immediately so they survive a power cycle. */
app_error_code_t provisioning_set_station(const char *ssid, const char *password);

/* Reads ONLY the station credentials. Deliberately not "load the whole config
 * and pick two fields out of it": provisioning_config_t also holds the admin
 * password verifier, its salt, and the AP passphrase, and a caller that wants an
 * SSID has no business with a copy of those on its stack. Returns
 * APP_ERROR_NOT_FOUND when no network has been saved. */
app_error_code_t provisioning_get_station(char *out_ssid, size_t ssid_size, char *out_password,
                                          size_t password_size);
app_error_code_t provisioning_clear_credentials(void);
app_error_code_t provisioning_factory_reset(void);
app_error_code_t provisioning_deinit(void);
bool provisioning_owns_resources(void);

#endif
