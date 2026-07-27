#ifndef WEB_SETUP_CORE_H
#define WEB_SETUP_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "auth.h"
#include "provisioning.h"
#include "provisioning_bootstrap.h"
#include "wifi_ap.h"

#define WEB_SETUP_CONFIRMATION_TIMEOUT_MS 30000U

typedef struct {
    char device_id[PROVISIONING_DEVICE_ID_HEX_BYTES + 1U];
    char ap_ssid[WIFI_AP_SSID_MAX_BYTES + 1U];
    char setup_code[PROVISIONING_SETUP_SECRET_BUFFER_BYTES];
    bool physical_confirmation_required;
    bool manufacturing_bypass;
} web_setup_configuration_t;

typedef struct {
    char setup_code[PROVISIONING_SETUP_SECRET_BUFFER_BYTES];
    char ap_ssid[WIFI_AP_SSID_MAX_BYTES + 1U];
    char ap_passphrase[WIFI_AP_PASSPHRASE_MAX_BYTES + 1U];
    char administrator_password[AUTH_PASSWORD_MAX_BYTES + 1U];
    bool require_physical_confirmation;
    bool always_select_set;
} web_setup_submission_t;

typedef struct {
    char device_id[PROVISIONING_DEVICE_ID_HEX_BYTES + 1U];
    char ap_ssid[WIFI_AP_SSID_MAX_BYTES + 1U];
    bool completed;
    bool physical_confirmation_required;
} web_setup_state_t;

typedef struct {
    void *context;
    app_error_code_t (*provisioning_load)(void *context, provisioning_config_t *out_configuration);
    app_error_code_t (*password_create)(void *context, const char *password, size_t password_length,
                                        auth_password_record_t *out_record);
    app_error_code_t (*provisioning_commit)(void *context, const provisioning_config_t *replacement,
                                            uint32_t expected_revision,
                                            provisioning_config_t *out_committed);
    app_error_code_t (*wait_for_confirmation)(void *context, uint32_t timeout_ms);
    app_error_code_t (*schedule_restart)(void *context);
    void (*secure_zero)(void *context, void *memory, size_t size);
} web_setup_ops_t;

typedef struct {
    web_setup_ops_t operations;
    web_setup_configuration_t configuration;
    bool initialized;
    bool completed;
} web_setup_core_t;

app_error_code_t web_setup_core_init(web_setup_core_t *core, const web_setup_ops_t *operations,
                                     const web_setup_configuration_t *configuration);
web_setup_state_t web_setup_core_get_state(const web_setup_core_t *core);
app_error_code_t web_setup_core_submit(web_setup_core_t *core, web_setup_submission_t *submission,
                                       provisioning_config_t *out_committed);
app_error_code_t web_setup_core_restart(web_setup_core_t *core);
app_error_code_t web_setup_core_deinit(web_setup_core_t *core);

#endif
