#ifndef WEB_SETTINGS_H
#define WEB_SETTINGS_H

#include <stdbool.h>
#include <stddef.h>

#include "api_contracts_v2.h"
#include "app_error.h"
#include "device_settings_v2.h"
#include "setup_contract_v2.h"

/* Business logic for GET/PUT /api/v1/settings and
 * POST /api/v1/settings/change-password (SPEC_V2 13.9). No esp_http_server
 * dependency, so this module is host-testable; the httpd adapter
 * (web_server_settings.c) supplies `ops`, bridging to device_settings and
 * auth. Mirrors web_send.h's outcome-struct-plus-ops shape. */
typedef struct {
    void *context;
    app_error_code_t (*settings_read)(void *context, app_v2_device_settings_t *out_settings);
    app_error_code_t (*settings_replace)(void *context, const app_v2_device_settings_t *settings,
                                         bool *out_changed);
    /* Verifies `password` against the credential material already present in
     * `settings` (the record ops->settings_read() just returned). */
    app_error_code_t (*password_verify)(void *context, const char *password, size_t password_length,
                                        const app_v2_device_settings_t *settings,
                                        bool *out_matches);
    app_error_code_t (*password_create)(void *context, const char *password, size_t password_length,
                                        app_v2_setup_password_material_t *out_material);
    app_error_code_t (*invalidate_all_sessions)(void *context);
} web_settings_ops_t;

typedef enum {
    WEB_SETTINGS_GET_OK = 0,
    WEB_SETTINGS_GET_BACKEND_UNAVAILABLE,
    WEB_SETTINGS_GET_INTERNAL,
} web_settings_get_result_t;

typedef struct {
    web_settings_get_result_t result;
    /* Underlying backend error for BACKEND_UNAVAILABLE; APP_ERROR_NONE
     * otherwise. */
    app_error_code_t detail;
} web_settings_get_outcome_t;

/* GET /api/v1/settings. On WEB_SETTINGS_GET_OK, *out_json is heap-allocated
 * (web_api_handler_json_free()/cJSON_free()). */
web_settings_get_outcome_t web_settings_get_handle(const web_settings_ops_t *ops, char **out_json);

typedef enum {
    WEB_SETTINGS_PUT_OK = 0,
    WEB_SETTINGS_PUT_INVALID_BODY,
    WEB_SETTINGS_PUT_EMPTY,
    WEB_SETTINGS_PUT_INVALID_DEVICE_NAME,
    WEB_SETTINGS_PUT_INVALID_SEND_MODE,
    WEB_SETTINGS_PUT_INVALID_SNAPSHOT_RETENTION_TARGET,
    WEB_SETTINGS_PUT_INVALID_LAST_SELECTED_PACKAGE_ID,
    WEB_SETTINGS_PUT_INVALID_ACCESS_POINT_SSID,
    WEB_SETTINGS_PUT_INVALID_ACCESS_POINT_PASSPHRASE,
    WEB_SETTINGS_PUT_INVALID_STATION_SSID,
    WEB_SETTINGS_PUT_INVALID_STATION_PASSPHRASE,
    WEB_SETTINGS_PUT_BACKEND_UNAVAILABLE,
    WEB_SETTINGS_PUT_INTERNAL,
} web_settings_put_result_t;

typedef struct {
    web_settings_put_result_t result;
    /* Underlying backend error for BACKEND_UNAVAILABLE; APP_ERROR_NONE
     * otherwise. */
    app_error_code_t detail;
} web_settings_put_outcome_t;

/* PUT /api/v1/settings. Parses `body` (NUL-terminated, real length
 * `body_capacity`) as the exact strict-partial-update schema -- rejecting
 * unknown, duplicate, and wrong-type fields, trailing content, and malformed
 * JSON -- applies it via app_v2_settings_prepare_update(), and commits the
 * result through ops->settings_replace(). `body` is wiped (every byte set to
 * 0) before this function returns on every path (it may carry a Wi-Fi
 * passphrase). On WEB_SETTINGS_PUT_OK, *out_json is heap-allocated. */
web_settings_put_outcome_t web_settings_put_handle(char *body, size_t body_capacity,
                                                   const web_settings_ops_t *ops, char **out_json);

typedef enum {
    WEB_CHANGE_PASSWORD_OK = 0,
    WEB_CHANGE_PASSWORD_INVALID_BODY,
    WEB_CHANGE_PASSWORD_INVALID_NEW_PASSWORD,
    WEB_CHANGE_PASSWORD_INCORRECT_CURRENT_PASSWORD,
    WEB_CHANGE_PASSWORD_BACKEND_UNAVAILABLE,
    WEB_CHANGE_PASSWORD_INTERNAL,
} web_change_password_result_t;

typedef struct {
    web_change_password_result_t result;
    /* Underlying backend error for BACKEND_UNAVAILABLE; APP_ERROR_NONE
     * otherwise. */
    app_error_code_t detail;
} web_change_password_outcome_t;

/* POST /api/v1/settings/change-password. Verifies `currentPassword` against
 * the stored credential, derives new material for `newPassword`, replaces
 * the settings record, and invalidates every session (SPEC_V2 13.9:
 * "invalidates all sessions including the current one"). Clearing the
 * session cookie on the wire is the httpd adapter's job -- this module has
 * no concept of a cookie. `body` is wiped before this function returns on
 * every path. */
web_change_password_outcome_t web_change_password_handle(char *body, size_t body_capacity,
                                                         const web_settings_ops_t *ops);

#endif
