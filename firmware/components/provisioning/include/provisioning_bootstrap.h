#ifndef PROVISIONING_BOOTSTRAP_H
#define PROVISIONING_BOOTSTRAP_H

#include "app_error.h"
#include "wifi_ap.h"

#define PROVISIONING_DEVICE_ID_HEX_BYTES 12U
#define PROVISIONING_SETUP_SECRET_HEX_BYTES 24U
#define PROVISIONING_SETUP_SECRET_BUFFER_BYTES (PROVISIONING_SETUP_SECRET_HEX_BYTES + 1U)

typedef struct {
    char device_id[PROVISIONING_DEVICE_ID_HEX_BYTES + 1U];
    char ap_ssid[WIFI_AP_SSID_MAX_BYTES + 1U];
    char ap_passphrase[PROVISIONING_SETUP_SECRET_BUFFER_BYTES];
    char setup_code[PROVISIONING_SETUP_SECRET_BUFFER_BYTES];
} provisioning_bootstrap_t;

app_error_code_t provisioning_bootstrap_derive(provisioning_bootstrap_t *out_bootstrap);
void provisioning_bootstrap_clear(provisioning_bootstrap_t *bootstrap);

#endif
