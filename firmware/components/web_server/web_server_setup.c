#include "web_server_internal.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "api_contracts_v2.h"
#include "app_error.h"
#include "app_limits_v2.h"
#include "auth.h"
#include "cJSON.h"
#include "device_settings.h"
#include "device_settings_v2.h"
#include "esp_http_server.h"
#include "esp_system.h"
#include "setup_contract_v2.h"
#include "web_server.h"
#include "web_server_setup_submit.h"

/* AUTH_SALT_BYTES/AUTH_HASH_BYTES (the auth component's PBKDF2 record shape)
 * and APP_V2_PASSWORD_SALT_BYTES/APP_V2_PASSWORD_VERIFIER_BYTES (the V2
 * device-settings wire shape) must stay byte-for-byte identical for the
 * direct memcpy in setup_password_create() below to be correct. */
_Static_assert(AUTH_SALT_BYTES == APP_V2_PASSWORD_SALT_BYTES,
               "auth salt size must match the V2 password-verifier salt size");
_Static_assert(AUTH_HASH_BYTES == APP_V2_PASSWORD_VERIFIER_BYTES,
               "auth hash size must match the V2 password-verifier size");

#define SETUP_BODY_MAX_BYTES APP_V2_JSON_BODY_MAX_BYTES

static esp_err_t send_setup_state(httpd_req_t *request) {
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return send_error(request, "500 Internal Server Error", APP_ERROR_INTERNAL,
                          "setup response allocation failed");
    }
    if (cJSON_AddBoolToObject(root, "provisioned", false) == NULL ||
        cJSON_AddStringToObject(root, "deviceName", server_configuration.setup_device_name) ==
            NULL) {
        cJSON_Delete(root);
        return send_error(request, "500 Internal Server Error", APP_ERROR_INTERNAL,
                          "setup response allocation failed");
    }
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (json == NULL) {
        return send_error(request, "500 Internal Server Error", APP_ERROR_INTERNAL,
                          "setup response serialization failed");
    }
    const esp_err_t result = send_json(request, json, "200 OK");
    cJSON_free(json);
    return result;
}

esp_err_t setup_state_handler(httpd_req_t *request) {
    if (request == NULL) {
        return ESP_FAIL;
    }
    if (server_configuration.mode != WEB_SERVER_MODE_SETUP) {
        return send_error(request, "404 Not Found", APP_ERROR_NOT_FOUND, "route not found");
    }
    return send_setup_state(request);
}

static void setup_secure_zero(void *context, void *memory, size_t size) {
    (void)context;
    volatile uint8_t *bytes = memory;
    for (size_t index = 0U; index < size; ++index) {
        bytes[index] = 0U;
    }
}

static app_error_code_t setup_settings_read(void *context, app_v2_device_settings_t *out_settings) {
    (void)context;
    return device_settings_read(out_settings);
}

static app_error_code_t
setup_settings_replace(void *context, const app_v2_device_settings_t *settings, bool *out_changed) {
    (void)context;
    return device_settings_replace(settings, out_changed);
}

/* Bridges the setup contract's password-material shape to the V2
 * password-verifier path in the auth component (SPEC 18.4 / TODO V2-041).
 * AUTH_PBKDF2_ITERATIONS is the hardware-measured value frozen by V2-041 --
 * see auth.h. */
static app_error_code_t setup_password_create(void *context, const char *password,
                                              size_t password_length,
                                              app_v2_setup_password_material_t *out_material) {
    (void)context;
    memset(out_material, 0, sizeof(*out_material));
    auth_password_record_t record = {0};
    const app_error_code_t result = auth_password_create(password, password_length, &record);
    if (result != APP_ERROR_NONE) {
        setup_secure_zero(NULL, &record, sizeof(record));
        return result;
    }
    if (record.iterations != AUTH_PBKDF2_ITERATIONS) {
        /* Defensive: the auth component's default iteration count drifted
         * from what V2 setup expects. Fail closed rather than store
         * material the login floor check would reject later. */
        setup_secure_zero(NULL, &record, sizeof(record));
        return APP_ERROR_INTERNAL;
    }
    out_material->credential_version = APP_V2_CREDENTIAL_VERSION;
    out_material->password_algorithm_version = APP_V2_PASSWORD_ALGORITHM_VERSION;
    out_material->password_iterations = record.iterations;
    memcpy(out_material->password_salt, record.salt, sizeof(out_material->password_salt));
    memcpy(out_material->password_verifier, record.hash, sizeof(out_material->password_verifier));
    setup_secure_zero(NULL, &record, sizeof(record));
    return APP_ERROR_NONE;
}

static const char *status_for_detail(app_error_code_t detail) {
    switch (detail) {
    case APP_ERROR_STORAGE_FULL:
        return "507 Insufficient Storage";
    case APP_ERROR_STORAGE_UNAVAILABLE:
    case APP_ERROR_STORAGE_CORRUPT:
    case APP_ERROR_TIMEOUT:
        return "503 Service Unavailable";
    default:
        return "500 Internal Server Error";
    }
}

static esp_err_t send_setup_submit_error(httpd_req_t *request, web_setup_submit_outcome_t outcome) {
    switch (outcome.result) {
    case WEB_SETUP_SUBMIT_INVALID_BODY:
        return send_error(request, "400 Bad Request", APP_ERROR_INVALID_ARGUMENT,
                          "invalid setup request");
    case WEB_SETUP_SUBMIT_MALFORMED_CODE:
        return send_error(request, "400 Bad Request", APP_ERROR_INVALID_ARGUMENT,
                          "setup code must be exactly eight digits");
    case WEB_SETUP_SUBMIT_CODE_MISMATCH:
        return send_error(request, "401 Unauthorized", APP_ERROR_AUTH_FAILED,
                          "incorrect setup code");
    case WEB_SETUP_SUBMIT_CODE_CONSUMED:
        return send_error(request, "409 Conflict", APP_ERROR_CONFLICT,
                          "setup code has already been used");
    case WEB_SETUP_SUBMIT_ALREADY_PROVISIONED:
        return send_error(request, "409 Conflict", APP_ERROR_CONFLICT,
                          "device is already provisioned");
    case WEB_SETUP_SUBMIT_INVALID_DEVICE_NAME:
        return send_error(request, "400 Bad Request", APP_ERROR_INVALID_ARGUMENT,
                          "invalid device name");
    case WEB_SETUP_SUBMIT_INVALID_AP_SSID:
        return send_error(request, "400 Bad Request", APP_ERROR_INVALID_ARGUMENT,
                          "invalid access-point SSID");
    case WEB_SETUP_SUBMIT_INVALID_AP_PASSPHRASE:
        return send_error(request, "400 Bad Request", APP_ERROR_INVALID_ARGUMENT,
                          "invalid access-point passphrase");
    case WEB_SETUP_SUBMIT_INVALID_ADMIN_PASSWORD:
        return send_error(request, "400 Bad Request", APP_ERROR_INVALID_ARGUMENT,
                          "invalid administrator password");
    case WEB_SETUP_SUBMIT_SETTINGS_UNAVAILABLE:
        return send_error(request, status_for_detail(outcome.detail), outcome.detail,
                          "settings unavailable");
    case WEB_SETUP_SUBMIT_PASSWORD_DERIVATION_FAILED:
        return send_error(request, "500 Internal Server Error", APP_ERROR_INTERNAL,
                          "password derivation failed");
    case WEB_SETUP_SUBMIT_COMMIT_FAILED:
        return send_error(request, status_for_detail(outcome.detail), outcome.detail,
                          "settings commit failed");
    case WEB_SETUP_SUBMIT_OK:
    case WEB_SETUP_SUBMIT_INTERNAL:
    default:
        return send_error(request, "500 Internal Server Error", APP_ERROR_INTERNAL,
                          "setup submission failed");
    }
}

static esp_err_t send_setup_accepted(httpd_req_t *request,
                                     const app_v2_setup_accepted_t *accepted) {
    cJSON *root = cJSON_CreateObject();
    if (root == NULL ||
        cJSON_AddBoolToObject(root, "accepted", accepted->action.accepted) == NULL ||
        cJSON_AddBoolToObject(root, "restartRequired", accepted->restart_required) == NULL ||
        cJSON_AddBoolToObject(root, "connectionWillClose",
                              accepted->action.connection_will_close) == NULL ||
        cJSON_AddBoolToObject(root, "reprovisioningRequired",
                              accepted->action.reprovisioning_required) == NULL) {
        cJSON_Delete(root);
        return send_error(request, "500 Internal Server Error", APP_ERROR_INTERNAL,
                          "setup response allocation failed");
    }
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (json == NULL) {
        return send_error(request, "500 Internal Server Error", APP_ERROR_INTERNAL,
                          "setup response serialization failed");
    }
    const esp_err_t result = send_json(request, json, "202 Accepted");
    cJSON_free(json);
    return result;
}

esp_err_t setup_submit_handler(httpd_req_t *request) {
    if (request == NULL) {
        return ESP_FAIL;
    }
    if (server_configuration.mode != WEB_SERVER_MODE_SETUP) {
        return send_error(request, "409 Conflict", APP_ERROR_CONFLICT,
                          "device is already provisioned");
    }

    /* Heap-allocated (like the general API dispatch's read_call_body()) so an
     * 8 KiB JSON body limit does not become an 8 KiB frame on the httpd task
     * stack; scripts/check-stack-usage.sh fails any first-party frame over
     * 4096 bytes that is not explicitly allowlisted. */
    const size_t body_capacity = (size_t)SETUP_BODY_MAX_BYTES + 1U;
    char *body = calloc(body_capacity, 1U);
    if (body == NULL) {
        return send_error(request, "500 Internal Server Error", APP_ERROR_INTERNAL,
                          "setup request allocation failed");
    }
    const app_error_code_t body_result =
        read_bounded_body(request, body, body_capacity, SETUP_BODY_MAX_BYTES);
    if (body_result != APP_ERROR_NONE) {
        setup_secure_zero(NULL, body, body_capacity);
        free(body);
        return send_error(request, "400 Bad Request", APP_ERROR_INVALID_ARGUMENT,
                          "invalid request body");
    }

    const web_setup_submit_ops_t operations = {
        .context = NULL,
        .settings_read = setup_settings_read,
        .password_create = setup_password_create,
        .settings_replace = setup_settings_replace,
        .secure_zero = setup_secure_zero,
    };
    app_v2_setup_accepted_t accepted = {0};
    const web_setup_submit_outcome_t outcome =
        web_setup_submit_handle(body, body_capacity, &operations, &setup_session, &accepted);
    free(body);
    if (outcome.result != WEB_SETUP_SUBMIT_OK) {
        return send_setup_submit_error(request, outcome);
    }
    const esp_err_t send_result = send_setup_accepted(request, &accepted);
    /* Mirrors web_server_api.c's api_handler(): restart only after the
     * response was actually sent, and only for a route whose contract
     * promises one. accepted.restart_required is unconditionally true on a
     * successful setup outcome (setup_contract_v2.c), so this always fires
     * here -- without it, the device commits new settings but never leaves
     * setup mode, matching a real gap found on physical hardware 2026-08-10:
     * GET /api/v1/setup kept reporting provisioned:false and login stayed
     * disabled after a real setup submission, because nothing in this
     * dedicated setup_routes[] handler ever called esp_restart() the way the
     * generic API dispatch does for /api/v1/device/restart and
     * /api/v1/device/factory-reset. */
    if (send_result == ESP_OK && accepted.restart_required) {
        esp_restart();
    }
    return send_result;
}
