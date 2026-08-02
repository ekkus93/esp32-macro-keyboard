#include "web_server_internal.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "auth.h"
#include "cJSON.h"

#define LOGIN_COOKIE_BYTES 160U
#define LOGIN_RESPONSE_BYTES 192U

/* Enforce the login throttle. Sets *out_proceed to true when the attempt is
 * allowed; otherwise sends the throttled/failed response and returns its result
 * (which the caller returns directly). */
static esp_err_t enforce_login_rate_limit(httpd_req_t *request, bool *out_proceed) {
    *out_proceed = false;
    uint32_t retry_after = 0U;
    const app_error_code_t allowed = auth_login_attempt_allowed(&retry_after);
    if (allowed == APP_ERROR_RATE_LIMITED) {
        char retry_value[16U];
        const int written =
            snprintf(retry_value, sizeof(retry_value), "%lu", (unsigned long)retry_after);
        if (written < 0 || (size_t)written >= sizeof(retry_value) ||
            httpd_resp_set_hdr(request, "Retry-After", retry_value) != ESP_OK) {
            return ESP_FAIL;
        }
        return send_error(request, "429 Too Many Requests", APP_ERROR_RATE_LIMITED,
                          "too many login attempts");
    }
    if (allowed != APP_ERROR_NONE) {
        return send_error(request, "500 Internal Server Error", allowed,
                          "login throttle unavailable");
    }
    *out_proceed = true;
    return ESP_OK;
}

esp_err_t login_handler(httpd_req_t *request) {
    if (!server_configuration.login_enabled) {
        return send_error(request, "503 Service Unavailable", APP_ERROR_AUTH_REQUIRED,
                          "login is not provisioned");
    }
    bool proceed = false;
    const esp_err_t throttle = enforce_login_rate_limit(request, &proceed);
    if (!proceed) {
        return throttle;
    }

    char body[LOGIN_BODY_MAX_BYTES + 1U];
    const app_error_code_t body_result =
        read_bounded_body(request, body, sizeof(body), LOGIN_BODY_MAX_BYTES);
    if (body_result != APP_ERROR_NONE) {
        return send_error(request, "400 Bad Request", body_result, "invalid request body");
    }
    cJSON *root = cJSON_ParseWithLength(body, strlen(body));
    if (root == NULL) {
        memset(body, 0, sizeof(body));
        return send_error(request, "400 Bad Request", APP_ERROR_INVALID_ARGUMENT, "invalid JSON");
    }
    const cJSON *password = cJSON_GetObjectItemCaseSensitive(root, "password");
    if (!cJSON_IsString(password) || password->valuestring == NULL) {
        cJSON_Delete(root);
        memset(body, 0, sizeof(body));
        return send_error(request, "400 Bad Request", APP_ERROR_INVALID_ARGUMENT,
                          "invalid credentials");
    }
    bool password_matches = false;
    const app_error_code_t verify_result =
        auth_password_verify(password->valuestring, strlen(password->valuestring),
                             &server_configuration.password_record, &password_matches);
    cJSON_Delete(root);
    memset(body, 0, sizeof(body));
    if (verify_result != APP_ERROR_NONE) {
        /* A PBKDF2 failure or a corrupt password record is a subsystem problem,
         * not a wrong password: do not count a login failure and do not answer 401
         * (FIX1 §10.2). */
        return send_error(request, "500 Internal Server Error", verify_result,
                          "authentication subsystem unavailable");
    }
    if (!password_matches) {
        const app_error_code_t failure_result = auth_login_record_failure();
        if (failure_result != APP_ERROR_NONE) {
            return send_error(request, "500 Internal Server Error", failure_result,
                              "could not record login failure");
        }
        return send_error(request, "401 Unauthorized", APP_ERROR_AUTH_FAILED,
                          "invalid credentials");
    }
    const app_error_code_t success_result = auth_login_record_success();
    if (success_result != APP_ERROR_NONE) {
        return send_error(request, "500 Internal Server Error", success_result,
                          "could not reset login throttle");
    }
    auth_session_view_t session = {0};
    const app_error_code_t session_result = auth_session_create(&session);
    if (session_result != APP_ERROR_NONE) {
        return send_error(request,
                          session_result == APP_ERROR_CONFLICT ? "503 Service Unavailable"
                                                               : "500 Internal Server Error",
                          session_result, "could not create session");
    }
    char cookie[LOGIN_COOKIE_BYTES];
    const int cookie_length = snprintf(cookie, sizeof(cookie),
                                       SESSION_COOKIE_NAME "=%s; HttpOnly; SameSite=Strict; Path=/",
                                       session.session_token);
    if (cookie_length < 0 || (size_t)cookie_length >= sizeof(cookie) ||
        httpd_resp_set_hdr(request, "Set-Cookie", cookie) != ESP_OK) {
        const app_error_code_t logout_result = auth_session_logout(session.session_token);
        return send_error(request, "500 Internal Server Error",
                          logout_result == APP_ERROR_NONE ? APP_ERROR_INTERNAL : logout_result,
                          "could not create login response");
    }
    char response[LOGIN_RESPONSE_BYTES];
    const int response_length =
        snprintf(response, sizeof(response), "{\"ok\":true,\"data\":{\"authenticated\":true}}");
    if (response_length < 0 || (size_t)response_length >= sizeof(response)) {
        const app_error_code_t logout_result = auth_session_logout(session.session_token);
        return send_error(request, "500 Internal Server Error",
                          logout_result == APP_ERROR_NONE ? APP_ERROR_INTERNAL : logout_result,
                          "response overflow");
    }
    return send_json(request, response, "200 OK");
}
