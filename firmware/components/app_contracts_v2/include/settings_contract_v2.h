#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "api_contracts_v2.h"
#include "device_settings_v2.h"
#include "setup_contract_v2.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Pure, host-testable business rules behind SPEC_V2.md 13.9 (settings) and
 * the password-change half of 13.9. No cJSON / esp_http_server dependency --
 * firmware/components/web_server/web_settings.c owns JSON marshalling and
 * calls into this module, mirroring setup_contract_v2.c's relationship to
 * web_server_setup_submit.c. */
typedef enum {
    APP_V2_SETTINGS_UPDATE_OK = 0,
    APP_V2_SETTINGS_UPDATE_INVALID_ARGUMENT,
    APP_V2_SETTINGS_UPDATE_INVALID_CURRENT_SETTINGS,
    APP_V2_SETTINGS_UPDATE_EMPTY,
    APP_V2_SETTINGS_UPDATE_INVALID_DEVICE_NAME,
    APP_V2_SETTINGS_UPDATE_INVALID_SNAPSHOT_RETENTION_TARGET,
    APP_V2_SETTINGS_UPDATE_INVALID_LAST_SELECTED_PACKAGE_ID,
    APP_V2_SETTINGS_UPDATE_INVALID_ACCESS_POINT_SSID,
    APP_V2_SETTINGS_UPDATE_INVALID_ACCESS_POINT_PASSPHRASE,
    APP_V2_SETTINGS_UPDATE_INVALID_STATION_SSID,
    APP_V2_SETTINGS_UPDATE_INVALID_STATION_PASSPHRASE,
} app_v2_settings_update_result_t;

/* Builds a sanitized response view over `settings` (SPEC_V2 13.9 GET / the
 * "settings" object nested in the PUT response). String views point into
 * `settings`, so the caller must keep it alive while using *out_response.
 * Never includes password, passphrase, or any other secret material. */
app_v2_settings_update_result_t
app_v2_settings_response_from_settings(const app_v2_device_settings_t *settings,
                                       app_v2_settings_response_t *out_response);

/* Applies a strict-partial-update request (SPEC_V2 13.9 PUT) on top of
 * `current`, producing a fully-validated candidate record.
 *
 *   - omitted fields (has_* == false) preserve `current`'s value;
 *   - `station.remove_station` (the JSON `"station": null` case) clears the
 *     configured station network;
 *   - an update with every has_* flag false is rejected
 *     (APP_V2_SETTINGS_UPDATE_EMPTY) -- SPEC_V2 13.9 "an empty update object
 *     is rejected."
 *
 * *out_restart_required / *out_reconnect_required follow SPEC_V2 13.9:
 * changing access-point credentials sets both to true (wifi_ap has no live
 * AP-reconfigure path). Changing the station network (setting new
 * credentials or removing them) sets *out_restart_required alone --
 * wifi_ap_connect_station() only ever runs once, at boot
 * (firmware/components/wifi_ap, confirmed by the V2-044 audit), so a station
 * change cannot take effect without a reboot; it does not disturb the
 * browser's own access-point session, so reconnectRequired stays false for a
 * station-only change. */
app_v2_settings_update_result_t
app_v2_settings_prepare_update(const app_v2_device_settings_t *current,
                               const app_v2_settings_update_request_t *request,
                               app_v2_device_settings_t *out_candidate, bool *out_restart_required,
                               bool *out_reconnect_required);

typedef enum {
    APP_V2_PASSWORD_CHANGE_OK = 0,
    APP_V2_PASSWORD_CHANGE_INVALID_ARGUMENT,
    APP_V2_PASSWORD_CHANGE_INVALID_CURRENT_SETTINGS,
    APP_V2_PASSWORD_CHANGE_INVALID_NEW_PASSWORD,
} app_v2_password_change_result_t;

/* Validates `new_password` (SPEC_V2 11.3: 12-128 UTF-8 bytes) against a
 * provisioned `current` record. Does not verify the current password --
 * that is the caller's job, via auth_password_verify() against the existing
 * credential material in `current`, before this is called. */
app_v2_password_change_result_t
app_v2_password_change_validate(const app_v2_device_settings_t *current,
                                app_v2_string_view_t new_password);

/* Merges freshly-derived password material onto `current`, producing a
 * candidate identical to `current` except for the credential fields.
 * Assumes app_v2_password_change_validate() already returned OK; returns
 * false only on a NULL argument or (defensively) if the merged record fails
 * app_v2_device_settings_validate(). */
bool app_v2_password_change_prepare_candidate(const app_v2_device_settings_t *current,
                                              const app_v2_setup_password_material_t *material,
                                              app_v2_device_settings_t *out_candidate);

#ifdef __cplusplus
}
#endif
