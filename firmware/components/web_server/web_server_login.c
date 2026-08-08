#include "web_server_internal.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "app_limits_v2.h"
#include "auth.h"
#include "lwip/sockets.h"
#include "web_api_core.h"
#include "web_auth_routes.h"

#define LOGIN_COOKIE_BYTES 160U
#define LOGIN_CONTENT_TYPE_MAX_BYTES 64U

static app_error_code_t login_source_ipv4(httpd_req_t *request, uint32_t *out_source_ipv4) {
    if (request == NULL || out_source_ipv4 == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const int socket_fd = httpd_req_to_sockfd(request);
    if (socket_fd < 0) {
        return APP_ERROR_INTERNAL;
    }
    struct sockaddr_storage peer = {0};
    socklen_t peer_length = sizeof(peer);
    if (getpeername(socket_fd, (struct sockaddr *)&peer, &peer_length) != 0 ||
        peer.ss_family != AF_INET) {
        return APP_ERROR_INTERNAL;
    }
    const struct sockaddr_in *ipv4 = (const struct sockaddr_in *)&peer;
    *out_source_ipv4 = ipv4->sin_addr.s_addr;
    return APP_ERROR_NONE;
}

static esp_err_t enforce_login_rate_limit(httpd_req_t *request, uint32_t source_ipv4,
                                          bool *out_proceed) {
    *out_proceed = false;
    uint32_t retry_after = 0U;
    const app_error_code_t allowed = auth_login_attempt_allowed(source_ipv4, &retry_after);
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

static esp_err_t login_failure(httpd_req_t *request, uint32_t source_ipv4) {
    const app_error_code_t failure_result = auth_login_record_failure(source_ipv4);
    if (failure_result == APP_ERROR_RATE_LIMITED) {
        return send_error(request, "429 Too Many Requests", failure_result,
                          "login throttle capacity unavailable");
    }
    if (failure_result != APP_ERROR_NONE) {
        return send_error(request, "500 Internal Server Error", failure_result,
                          "could not record login failure");
    }
    return send_error(request, "401 Unauthorized", APP_ERROR_AUTH_FAILED, "invalid credentials");
}

/* `session` (rather than a second bare `const char *`) so this and the
 * caller-supplied message string cannot be swapped by mistake at a call
 * site. */
static esp_err_t send_login_error_and_forget_session(httpd_req_t *request,
                                                     const auth_session_view_t *session,
                                                     const char *message) {
    const app_error_code_t logout_result = auth_session_logout(session->session_token);
    return send_error(request, "500 Internal Server Error",
                      logout_result == APP_ERROR_NONE ? APP_ERROR_INTERNAL : logout_result,
                      message);
}

static esp_err_t send_login_accepted(httpd_req_t *request, const auth_session_view_t *session,
                                     uint32_t idle_seconds, uint32_t absolute_seconds) {
    char cookie[LOGIN_COOKIE_BYTES];
    const int cookie_length = snprintf(cookie, sizeof(cookie),
                                       SESSION_COOKIE_NAME "=%s; HttpOnly; SameSite=Strict; Path=/",
                                       session->session_token);
    if (cookie_length < 0 || (size_t)cookie_length >= sizeof(cookie) ||
        httpd_resp_set_hdr(request, "Set-Cookie", cookie) != ESP_OK) {
        return send_login_error_and_forget_session(request, session,
                                                   "could not create login response");
    }
    char *json = NULL;
    if (web_auth_session_response_json(idle_seconds, absolute_seconds, &json) != APP_ERROR_NONE) {
        return send_login_error_and_forget_session(request, session, "response encoding failed");
    }
    const esp_err_t send_result = send_json(request, json, "200 OK");
    free(json);
    return send_result;
}

esp_err_t login_handler(httpd_req_t *request) {
    if (!server_configuration.login_enabled) {
        return send_error(request, "503 Service Unavailable", APP_ERROR_AUTH_REQUIRED,
                          "login is not provisioned");
    }
    uint32_t source_ipv4 = 0U;
    const app_error_code_t source_result = login_source_ipv4(request, &source_ipv4);
    if (source_result != APP_ERROR_NONE) {
        return send_error(request, "500 Internal Server Error", source_result,
                          "login peer address unavailable");
    }
    bool proceed = false;
    const esp_err_t throttle = enforce_login_rate_limit(request, source_ipv4, &proceed);
    if (!proceed) {
        return throttle;
    }

    char content_type[LOGIN_CONTENT_TYPE_MAX_BYTES] = {0};
    if (web_server_get_header(request, "Content-Type", content_type, sizeof(content_type)) !=
            APP_ERROR_NONE ||
        !web_api_content_type_is_json(content_type)) {
        return send_error(request, "415 Unsupported Media Type", APP_ERROR_INVALID_ARGUMENT,
                          "application/json content type required");
    }

    const size_t content_length = request->content_len;
    if (content_length == 0U) {
        return send_error(request, "400 Bad Request", APP_ERROR_INVALID_ARGUMENT,
                          "login body is required");
    }
    if (content_length > (size_t)APP_V2_JSON_BODY_MAX_BYTES) {
        return send_error(request, "413 Payload Too Large", APP_ERROR_INVALID_ARGUMENT,
                          "login body exceeds route limit");
    }

    /* Heap-allocated, matching web_server_setup.c's identical rationale:
     * scripts/check-stack-usage.sh fails any unallowlisted first-party frame
     * over 4096 bytes, and an 8 KiB JSON body limit becomes exactly that as
     * a stack array. */
    const size_t body_capacity = (size_t)APP_V2_JSON_BODY_MAX_BYTES + 1U;
    char *body = calloc(body_capacity, 1U);
    if (body == NULL) {
        return send_error(request, "500 Internal Server Error", APP_ERROR_INTERNAL,
                          "login request allocation failed");
    }
    const app_error_code_t body_result =
        read_bounded_body(request, body, body_capacity, (size_t)APP_V2_JSON_BODY_MAX_BYTES);
    if (body_result != APP_ERROR_NONE) {
        memset(body, 0, body_capacity);
        free(body);
        return send_error(request, "400 Bad Request", body_result, "invalid request body");
    }

    char password[AUTH_PASSWORD_MAX_BYTES + 1U] = {0};
    size_t password_length = 0U;
    const web_auth_login_parse_result_t parse_result =
        web_auth_login_parse(body, body_capacity, password, sizeof(password), &password_length);
    free(body);
    if (parse_result != WEB_AUTH_LOGIN_PARSE_OK) {
        memset(password, 0, sizeof(password));
        return send_error(request, "400 Bad Request", APP_ERROR_INVALID_ARGUMENT,
                          "invalid login request");
    }

    bool password_matches = false;
    const app_error_code_t verify_result = auth_password_verify(
        password, password_length, &server_configuration.password_record, &password_matches);
    memset(password, 0, sizeof(password));
    if (verify_result != APP_ERROR_NONE) {
        return send_error(request, "500 Internal Server Error", verify_result,
                          "authentication subsystem unavailable");
    }
    if (!password_matches) {
        return login_failure(request, source_ipv4);
    }
    const app_error_code_t success_result = auth_login_record_success(source_ipv4);
    if (success_result != APP_ERROR_NONE) {
        return send_error(request, "500 Internal Server Error", success_result,
                          "could not reset login throttle");
    }
    auth_session_view_t session = {0};
    const app_error_code_t session_result = auth_session_create(&session);
    if (session_result != APP_ERROR_NONE) {
        return send_error(request, "500 Internal Server Error", session_result,
                          "could not create session");
    }
    uint32_t idle_seconds = 0U;
    uint32_t absolute_seconds = 0U;
    const app_error_code_t remaining_result =
        auth_session_remaining(session.session_token, &idle_seconds, &absolute_seconds);
    if (remaining_result != APP_ERROR_NONE) {
        return send_login_error_and_forget_session(request, &session,
                                                   "could not read session lifetime");
    }
    return send_login_accepted(request, &session, idle_seconds, absolute_seconds);
}
