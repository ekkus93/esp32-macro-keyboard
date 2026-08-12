#ifndef WEB_DEVICE_ACTIONS_H
#define WEB_DEVICE_ACTIONS_H

#include <stdbool.h>
#include <stddef.h>

#include "app_error.h"
#include "device_controls.h"
#include "device_settings_v2.h"

/* Business logic for the SPEC_V2 13.12 device actions
 * (POST /api/v1/device/{restart,reset-settings,factory-reset}). No
 * esp_http_server dependency, so this module is host-testable; the httpd
 * adapter (web_server_device_actions.c) supplies `ops`, bridging to
 * device_settings, auth, and device_controls (which already implements the
 * settings-then-sessions-then-blobs-then-reboot sequencing per
 * device_controls_reset.h -- this module only validates the request and
 * decides whether to call it). */
typedef struct {
    void *context;
    app_error_code_t (*settings_read)(void *context, app_v2_device_settings_t *out_settings);
    /* Verifies `password` against the credential material already present in
     * `settings`. Used only by factory-reset. */
    app_error_code_t (*password_verify)(void *context, const char *password, size_t password_length,
                                        const app_v2_device_settings_t *settings,
                                        bool *out_matches);
    app_error_code_t (*restart)(void *context);
    device_controls_reset_settings_outcome_t (*reset_settings)(void *context);
    device_controls_factory_reset_outcome_t (*factory_reset)(void *context);
} web_device_actions_ops_t;

typedef enum {
    WEB_DEVICE_RESTART_OK = 0,
    WEB_DEVICE_RESTART_BACKEND_UNAVAILABLE,
    WEB_DEVICE_RESTART_INTERNAL,
} web_device_restart_result_t;

typedef struct {
    web_device_restart_result_t result;
    /* Underlying backend error for BACKEND_UNAVAILABLE; APP_ERROR_NONE
     * otherwise. */
    app_error_code_t detail;
} web_device_restart_outcome_t;

/* POST /api/v1/device/restart. No request body (SPEC_V2 13.12). */
web_device_restart_outcome_t web_device_restart_handle(const web_device_actions_ops_t *ops);

typedef enum {
    WEB_DEVICE_RESET_SETTINGS_OK = 0,
    WEB_DEVICE_RESET_SETTINGS_INVALID_BODY,
    WEB_DEVICE_RESET_SETTINGS_INVALID_CONFIRMATION,
    WEB_DEVICE_RESET_SETTINGS_BACKEND_UNAVAILABLE,
    WEB_DEVICE_RESET_SETTINGS_REBOOT_RECOVERY_REQUIRED,
    WEB_DEVICE_RESET_SETTINGS_COMMITTED_RESTART_INCOMPLETE,
    WEB_DEVICE_RESET_SETTINGS_INTERNAL,
} web_device_reset_settings_result_t;

typedef struct {
    web_device_reset_settings_result_t result;
    /* Primary settings/session error. */
    app_error_code_t detail;
    /* Restart-ownership error, kept separate from the primary failure. */
    app_error_code_t restart_detail;
} web_device_reset_settings_outcome_t;

/* POST /api/v1/device/reset-settings. Requires the exact confirmation
 * phrase "RESET SETTINGS" (SPEC_V2 13.12); the destructive operation does
 * not begin until it has been validated. `body` is wiped (every byte set to
 * 0) before this function returns on every path. */
web_device_reset_settings_outcome_t
web_device_reset_settings_handle(char *body, size_t body_capacity,
                                 const web_device_actions_ops_t *ops);

typedef enum {
    WEB_DEVICE_FACTORY_RESET_OK = 0,
    WEB_DEVICE_FACTORY_RESET_INVALID_BODY,
    WEB_DEVICE_FACTORY_RESET_INVALID_CONFIRMATION,
    WEB_DEVICE_FACTORY_RESET_INCORRECT_PASSWORD,
    WEB_DEVICE_FACTORY_RESET_BACKEND_UNAVAILABLE,
    WEB_DEVICE_FACTORY_RESET_RECOVERY_REQUIRED,
    WEB_DEVICE_FACTORY_RESET_INTERNAL,
} web_device_factory_reset_result_t;

typedef struct {
    web_device_factory_reset_result_t result;
    app_error_code_t detail;
} web_device_factory_reset_outcome_t;

/* POST /api/v1/device/factory-reset. Requires the exact confirmation phrase
 * "FACTORY RESET" and the current administrator password; the destructive
 * operation does not begin until the complete request, password, and
 * confirmation phrase have all been validated (SPEC_V2 13.12). `body` is
 * wiped before this function returns on every path. */
web_device_factory_reset_outcome_t
web_device_factory_reset_handle(char *body, size_t body_capacity,
                                const web_device_actions_ops_t *ops);

/* Builds the exact SPEC_V2 13.12 accepted-response JSON for restart:
 * {"accepted":true,"connectionWillClose":true,"reprovisioningRequired":false}.
 * *out_json is heap-allocated (web_api_handler_json_free()/cJSON_free()). */
app_error_code_t web_device_restart_accepted_json(char **out_json);

/* Builds the exact SPEC_V2 13.12 accepted-response JSON shared by
 * reset-settings and factory-reset (adds repositoryBlobsPreserved to the
 * restart shape). *out_json is heap-allocated. */
app_error_code_t web_device_reset_accepted_json(bool reprovisioning_required,
                                                bool repository_blobs_preserved, char **out_json);

#endif
