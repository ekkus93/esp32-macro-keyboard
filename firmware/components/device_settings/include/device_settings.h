#ifndef DEVICE_SETTINGS_H
#define DEVICE_SETTINGS_H

#include <stdbool.h>

#include "app_error.h"
#include "device_settings_v2.h"

#ifdef __cplusplus
extern "C" {
#endif

app_error_code_t device_settings_init(void);
app_error_code_t device_settings_deinit(void);
app_error_code_t device_settings_read(app_v2_device_settings_t *out_settings);
app_error_code_t device_settings_replace(const app_v2_device_settings_t *settings,
                                         bool *out_changed);
app_error_code_t device_settings_set_station(const char *ssid, const char *passphrase,
                                             bool *out_changed);
/* SPEC_V2.md §12.4: the physical console's set-ap-passphrase command.
 * Replaces the device's own access-point passphrase directly, without any
 * other settings field, rejecting a passphrase outside the same 8-63 byte
 * bound `PUT /api/v1/settings`'s accessPoint.passphrase field enforces. */
app_error_code_t device_settings_set_access_point_passphrase(const char *passphrase,
                                                             bool *out_changed);
app_error_code_t device_settings_reset_noncredential(app_v2_device_settings_t *out_settings,
                                                     bool *out_changed);
/* SPEC_V2.md §11.4 "Factory reset": replaces the stored record with the
 * unprovisioned default, erasing device name, credentials, AP/station Wi-Fi
 * configuration, and the provisioned flag. Does not touch repository blobs;
 * callers implementing the full factory-reset device action also erase blobs
 * separately (see device_controls). */
app_error_code_t device_settings_factory_reset(app_v2_device_settings_t *out_settings,
                                               bool *out_changed);

#ifdef __cplusplus
}
#endif

#endif
