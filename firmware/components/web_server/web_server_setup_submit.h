#ifndef WEB_SERVER_SETUP_SUBMIT_H
#define WEB_SERVER_SETUP_SUBMIT_H

#include <stdbool.h>
#include <stddef.h>

#include "api_contracts_v2.h"
#include "app_error.h"
#include "device_settings_v2.h"
#include "setup_contract_v2.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Business-logic outcome of a POST /api/v1/setup submission. This module
 * contains no esp_http_server dependency so it is host-testable; the httpd
 * adapter (web_server_setup.c) maps each result to an HTTP status and
 * message. */
typedef enum {
    WEB_SETUP_SUBMIT_OK = 0,
    WEB_SETUP_SUBMIT_INVALID_BODY,
    WEB_SETUP_SUBMIT_MALFORMED_CODE,
    WEB_SETUP_SUBMIT_CODE_MISMATCH,
    WEB_SETUP_SUBMIT_CODE_CONSUMED,
    WEB_SETUP_SUBMIT_ALREADY_PROVISIONED,
    WEB_SETUP_SUBMIT_INVALID_DEVICE_NAME,
    WEB_SETUP_SUBMIT_INVALID_AP_SSID,
    WEB_SETUP_SUBMIT_INVALID_AP_PASSPHRASE,
    WEB_SETUP_SUBMIT_INVALID_ADMIN_PASSWORD,
    WEB_SETUP_SUBMIT_SETTINGS_UNAVAILABLE,
    WEB_SETUP_SUBMIT_PASSWORD_DERIVATION_FAILED,
    WEB_SETUP_SUBMIT_COMMIT_FAILED,
    WEB_SETUP_SUBMIT_INTERNAL,
} web_setup_submit_result_t;

typedef struct {
    web_setup_submit_result_t result;
    /* Underlying backend error for the SETTINGS_UNAVAILABLE/COMMIT_FAILED/
     * PASSWORD_DERIVATION_FAILED results; APP_ERROR_NONE otherwise. */
    app_error_code_t detail;
} web_setup_submit_outcome_t;

/* Backend operations. In production these bridge to device_settings_read(),
 * the V2 password-verifier path in the auth component, and
 * device_settings_replace(); host tests supply fakes. */
typedef struct {
    void *context;
    app_error_code_t (*settings_read)(void *context, app_v2_device_settings_t *out_settings);
    app_error_code_t (*password_create)(void *context, const char *password, size_t password_length,
                                        app_v2_setup_password_material_t *out_material);
    app_error_code_t (*settings_replace)(void *context, const app_v2_device_settings_t *settings,
                                         bool *out_changed);
    void (*secure_zero)(void *context, void *memory, size_t size);
} web_setup_submit_ops_t;

/* Parses and validates `body` (a NUL-terminated buffer of capacity
 * `body_capacity`) as the exact V2 setup-submission JSON schema, derives
 * password material, prepares a candidate settings record with
 * app_v2_setup_prepare_candidate(), commits it, and only then consumes
 * `session`. `body` and every secret-bearing intermediate value are wiped via
 * operations->secure_zero before this function returns, on every exit path.
 * On WEB_SETUP_SUBMIT_OK, `*out_response` holds the exact non-secret
 * accepted/restart/reconnect response. `session` is never mutated unless the
 * settings commit succeeds. */
web_setup_submit_outcome_t web_setup_submit_handle(char *body, size_t body_capacity,
                                                   const web_setup_submit_ops_t *operations,
                                                   app_v2_setup_session_t *session,
                                                   app_v2_setup_accepted_t *out_response);

#ifdef __cplusplus
}
#endif

#endif
