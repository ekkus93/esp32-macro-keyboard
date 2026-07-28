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
    bool always_select_set;
    bool has_active_set;
    app_uuid_t active_set_id;
} provisioning_config_t;

typedef struct {
    uint32_t schema_version;
    uint32_t revision;
    bool require_physical_confirmation;
    bool always_select_set;
    bool has_active_set;
    app_uuid_t active_set_id;
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
app_error_code_t provisioning_clear_active_set_if_matches(const app_uuid_t *set_id,
                                                          bool *out_cleared);
app_error_code_t provisioning_clear_credentials(void);
app_error_code_t provisioning_factory_reset(void);
app_error_code_t provisioning_deinit(void);
bool provisioning_owns_resources(void);

#endif
