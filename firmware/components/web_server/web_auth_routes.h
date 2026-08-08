#ifndef WEB_AUTH_ROUTES_H
#define WEB_AUTH_ROUTES_H

#include <stddef.h>
#include <stdint.h>

#include "app_error.h"

/* Business logic for the login/session response shapes in SPEC_V2 13.5. No
 * esp_http_server dependency, so this module is host-testable; the httpd
 * adapters (web_server_login.c, web_api_administration.c) call it directly. */

/* Builds the exact
 * {"authenticated":true,"idleExpiresInSeconds":N,"absoluteExpiresInSeconds":N}
 * object shared by a successful login response and GET /api/v1/auth/session.
 * *out_json is heap-allocated (cJSON_free()/web_api_handler_json_free()). */
app_error_code_t web_auth_session_response_json(uint32_t idle_seconds_remaining,
                                                uint32_t absolute_seconds_remaining,
                                                char **out_json);

typedef enum {
    WEB_AUTH_LOGIN_PARSE_OK = 0,
    WEB_AUTH_LOGIN_PARSE_INVALID_BODY,
} web_auth_login_parse_result_t;

/* Parses `body` (a buffer of capacity `body_length` holding a NUL-terminated
 * JSON document somewhere within it, matching web_server_setup_submit.c's
 * convention) as the exact single-field V2 login schema
 * {"adminPassword": string}: rejects unknown fields,
 * duplicate fields, missing fields, wrong-type values, trailing content
 * after the JSON value, and malformed JSON. On success, copies the password
 * into `out_password` (NUL-terminated, capacity `out_password_capacity`) and
 * sets `*out_password_length`; a value that does not fit is treated as an
 * invalid body. `body` is wiped (every byte set to 0) before this function
 * returns on every path, mirroring the discipline web_setup_submit_handle()
 * applies to its own request body. */
web_auth_login_parse_result_t web_auth_login_parse(char *body, size_t body_length,
                                                   char *out_password, size_t out_password_capacity,
                                                   size_t *out_password_length);

#endif
