/* Host coverage for web_api_handle_administration() (V2-051 / V2-057): the
 * dispatcher's own composing logic for GET /api/v1/auth/session,
 * POST /api/v1/device/restart, and the provisioned-mode setup 404/409
 * fallback -- none of which is compiled into any other host test target.
 *
 * web_api_administration.c binds to auth.c, device_controls.c, and
 * device_settings.c's *public* entry points (auth_session_remaining,
 * auth_password_verify, auth_password_create, auth_session_logout_all,
 * device_controls_restart/reset_settings/factory_reset,
 * device_settings_read/replace) -- none of which are host-linkable (they pull
 * in mbedtls, FreeRTOS, NVS, and GPIO). This file substitutes narrow,
 * behavior-controllable stand-ins with the exact production signature for
 * each of those, the same technique test_web_api_dispatch.c already uses for
 * web_api_handle_administration() itself. web_diagnostics_handle() (real
 * definition in web_server_diagnostics.c, itself not host-testable -- see
 * V2-056) gets the same treatment: a stub that is never exercised by any test
 * below, present only so the linker resolves it. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "auth.h"
#include "cJSON.h"
#include "device_controls.h"
#include "device_settings.h"
#include "device_settings_v2.h"
#include "test_assert.h"
#include "test_secret_sentinel.h"
#include "web_api_handlers.h"
#include "web_api_response.h"
#include "web_cookie.h"
#include "web_diagnostics.h"
#include "web_server.h"

#define FAKE_SESSION_TOKEN "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef"

/* ---------------------------------------------------------------------- *
 * Fakes for auth.c's public entry points.
 * ---------------------------------------------------------------------- */

typedef struct {
    app_error_code_t session_remaining_result;
    uint32_t idle_seconds;
    uint32_t absolute_seconds;
    char observed_token[AUTH_TOKEN_HEX_BYTES];
    size_t session_remaining_calls;
    app_error_code_t password_verify_result;
    bool password_matches;
    app_error_code_t password_create_result;
    app_error_code_t logout_all_result;
} fake_auth_t;

static fake_auth_t fake_auth;

app_error_code_t auth_session_remaining(const char *session_token,
                                        uint32_t *out_idle_seconds_remaining,
                                        uint32_t *out_absolute_seconds_remaining) {
    ++fake_auth.session_remaining_calls;
    TEST_CHECK(session_token != NULL);
    (void)snprintf(fake_auth.observed_token, sizeof(fake_auth.observed_token), "%s", session_token);
    if (fake_auth.session_remaining_result != APP_ERROR_NONE) {
        return fake_auth.session_remaining_result;
    }
    *out_idle_seconds_remaining = fake_auth.idle_seconds;
    *out_absolute_seconds_remaining = fake_auth.absolute_seconds;
    return APP_ERROR_NONE;
}

app_error_code_t auth_password_verify(const char *password, size_t password_length,
                                      const auth_password_record_t *record, bool *out_matches) {
    (void)password;
    (void)password_length;
    (void)record;
    if (fake_auth.password_verify_result != APP_ERROR_NONE) {
        return fake_auth.password_verify_result;
    }
    *out_matches = fake_auth.password_matches;
    return APP_ERROR_NONE;
}

app_error_code_t auth_password_create(const char *password, size_t password_length,
                                      auth_password_record_t *out_record) {
    (void)password;
    (void)password_length;
    if (fake_auth.password_create_result != APP_ERROR_NONE) {
        return fake_auth.password_create_result;
    }
    memset(out_record, 0, sizeof(*out_record));
    out_record->iterations = AUTH_PBKDF2_ITERATIONS;
    return APP_ERROR_NONE;
}

app_error_code_t auth_session_logout_all(void) {
    return fake_auth.logout_all_result;
}

/* ---------------------------------------------------------------------- *
 * Fakes for device_controls.c's public entry points.
 * ---------------------------------------------------------------------- */

typedef struct {
    app_error_code_t restart_result;
    app_error_code_t reset_settings_result;
    device_controls_factory_reset_outcome_t factory_reset_outcome;
    size_t restart_calls;
    size_t reset_settings_calls;
    size_t factory_reset_calls;
} fake_device_controls_t;

static fake_device_controls_t fake_device_controls;

app_error_code_t device_controls_restart(void) {
    ++fake_device_controls.restart_calls;
    return fake_device_controls.restart_result;
}

app_error_code_t device_controls_reset_settings(void) {
    ++fake_device_controls.reset_settings_calls;
    return fake_device_controls.reset_settings_result;
}

device_controls_factory_reset_outcome_t device_controls_factory_reset(void) {
    ++fake_device_controls.factory_reset_calls;
    return fake_device_controls.factory_reset_outcome;
}

/* ---------------------------------------------------------------------- *
 * Fakes for device_settings.c's public entry points.
 * ---------------------------------------------------------------------- */

typedef struct {
    app_v2_device_settings_t record;
    app_error_code_t read_result;
    app_error_code_t replace_result;
    bool replace_changed;
} fake_device_settings_t;

static fake_device_settings_t fake_device_settings;

app_error_code_t device_settings_read(app_v2_device_settings_t *out_settings) {
    if (fake_device_settings.read_result != APP_ERROR_NONE) {
        return fake_device_settings.read_result;
    }
    *out_settings = fake_device_settings.record;
    return APP_ERROR_NONE;
}

app_error_code_t device_settings_replace(const app_v2_device_settings_t *settings,
                                         bool *out_changed) {
    if (fake_device_settings.replace_result != APP_ERROR_NONE) {
        return fake_device_settings.replace_result;
    }
    fake_device_settings.record = *settings;
    *out_changed = fake_device_settings.replace_changed;
    return APP_ERROR_NONE;
}

/* ---------------------------------------------------------------------- *
 * Stub for web_server_diagnostics.c's web_diagnostics_handle(). Never
 * exercised below (WEB_API_ROUTE_DIAGNOSTICS_FULL coverage belongs to
 * V2-056, a different track); present only so the linker resolves the
 * symbol web_api_handle_administration()'s switch statement references.
 * ---------------------------------------------------------------------- */

app_error_code_t web_diagnostics_handle(web_api_response_t *response) {
    return web_api_response_success(response, 200U, "{\"stub\":true}");
}

/* handle_change_password() (not exercised by any test below -- out of this
 * track's scope, see the file header) reads/writes this after a successful
 * password change; real production storage lives in web_server_common.c,
 * which is not host-linkable. */
web_server_config_t server_configuration;

/* ---------------------------------------------------------------------- */

static void reset_fakes(void) {
    fake_auth = (fake_auth_t){0};
    fake_device_controls = (fake_device_controls_t){
        .factory_reset_outcome =
            {
                .durably_accepted = true,
                .recovery_required = false,
                .primary_error = APP_ERROR_NONE,
            },
    };
    fake_device_settings = (fake_device_settings_t){0};
}

static cJSON *parse_response_body(const web_api_response_t *response) {
    TEST_CHECK(response->body != NULL);
    cJSON *root = cJSON_ParseWithLength(response->body, response->body_length);
    TEST_CHECK(root != NULL);
    return root;
}

/* -------------------------------------------------------------------------
 * GET /api/v1/auth/session (handle_session)
 * ---------------------------------------------------------------------- */

static void test_handle_session_success(void) {
    reset_fakes();
    fake_auth.session_remaining_result = APP_ERROR_NONE;
    fake_auth.idle_seconds = 86400U;
    fake_auth.absolute_seconds = 604800U;

    web_api_call_t call = {
        .method = WEB_API_METHOD_GET,
        .path = {.route = WEB_API_ROUTE_AUTH_SESSION},
    };
    (void)snprintf(call.session_token, sizeof(call.session_token), "%s", FAKE_SESSION_TOKEN);

    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_handle_administration(&call, &response));
    TEST_CHECK_EQ_U64(200U, response.status);
    TEST_CHECK_EQ_U64(1U, fake_auth.session_remaining_calls);
    TEST_CHECK_EQ_STRING(FAKE_SESSION_TOKEN, fake_auth.observed_token);

    cJSON *root = parse_response_body(&response);
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "authenticated")));
    TEST_CHECK_EQ_U64(
        86400U,
        (uint64_t)cJSON_GetObjectItemCaseSensitive(root, "idleExpiresInSeconds")->valuedouble);
    TEST_CHECK_EQ_U64(
        604800U,
        (uint64_t)cJSON_GetObjectItemCaseSensitive(root, "absoluteExpiresInSeconds")->valuedouble);
    TEST_CHECK(cJSON_GetObjectItemCaseSensitive(root, "error") == NULL);
    cJSON_Delete(root);
    web_api_response_free(&response);
}

static void test_handle_session_backend_failure(void) {
    reset_fakes();
    fake_auth.session_remaining_result = APP_ERROR_AUTH_REQUIRED;

    web_api_call_t call = {
        .method = WEB_API_METHOD_GET,
        .path = {.route = WEB_API_ROUTE_AUTH_SESSION},
    };
    (void)snprintf(call.session_token, sizeof(call.session_token), "%s", FAKE_SESSION_TOKEN);

    web_api_response_t response = {0};
    /* web_api_handle_administration()'s own return value reports whether the
     * response envelope was built, not the domain outcome it describes --
     * mirroring web_api_dispatch()'s identical convention -- so this is
     * APP_ERROR_NONE even for a mapped-to-401 error response; response.status
     * and the "error" JSON body below carry the actual failure. */
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_handle_administration(&call, &response));
    TEST_CHECK_EQ_U64(401U, response.status);

    cJSON *root = parse_response_body(&response);
    const cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
    TEST_CHECK(error != NULL);
    TEST_CHECK_EQ_STRING("auth_required",
                         cJSON_GetObjectItemCaseSensitive(error, "code")->valuestring);
    TEST_CHECK_EQ_STRING("session unavailable",
                         cJSON_GetObjectItemCaseSensitive(error, "message")->valuestring);
    cJSON_Delete(root);
    web_api_response_free(&response);
}

/* A password never flows through handle_session() at all (it only reads an
 * already-validated session's remaining TTLs), so the strongest available
 * proof that session composition cannot leak one is exercising the actual
 * production JSON builder and cookie builder together and scanning both
 * outputs with the same script CI's secret-sentinel gate runs. */
static void test_session_and_cookie_outputs_carry_no_password_sentinel(void) {
    reset_fakes();
    static const char secret_sentinel[] = "phase_v2_051_session_secret_M7pQ2xL9vC4hT6nY";
    fake_auth.session_remaining_result = APP_ERROR_NONE;
    fake_auth.idle_seconds = 3600U;
    fake_auth.absolute_seconds = 7200U;

    web_api_call_t call = {
        .method = WEB_API_METHOD_GET,
        .path = {.route = WEB_API_ROUTE_AUTH_SESSION},
    };
    (void)snprintf(call.session_token, sizeof(call.session_token), "%s", FAKE_SESSION_TOKEN);

    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_handle_administration(&call, &response));

    char cookie_header[160U];
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        web_cookie_build_session_header(FAKE_SESSION_TOKEN, cookie_header, sizeof(cookie_header)));

    const char *outputs[] = {response.body, cookie_header};
    test_assert_no_secret_sentinel(secret_sentinel, outputs, 2U);

    web_api_response_free(&response);
}

/* -------------------------------------------------------------------------
 * POST /api/v1/device/restart (handle_device_restart)
 * ---------------------------------------------------------------------- */

static void test_handle_device_restart_success(void) {
    reset_fakes();
    fake_device_controls.restart_result = APP_ERROR_NONE;

    const web_api_call_t call = {
        .method = WEB_API_METHOD_POST,
        .path = {.route = WEB_API_ROUTE_DEVICE_RESTART},
    };
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_handle_administration(&call, &response));
    TEST_CHECK_EQ_U64(202U, response.status);
    TEST_CHECK_EQ_U64(1U, fake_device_controls.restart_calls);

    cJSON *root = parse_response_body(&response);
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "accepted")));
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "connectionWillClose")));
    TEST_CHECK(cJSON_IsFalse(cJSON_GetObjectItemCaseSensitive(root, "reprovisioningRequired")));
    cJSON_Delete(root);
    web_api_response_free(&response);
}

static void test_handle_device_restart_backend_unavailable(void) {
    reset_fakes();
    fake_device_controls.restart_result = APP_ERROR_STORAGE_UNAVAILABLE;

    const web_api_call_t call = {
        .method = WEB_API_METHOD_POST,
        .path = {.route = WEB_API_ROUTE_DEVICE_RESTART},
    };
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_handle_administration(&call, &response));
    TEST_CHECK_EQ_U64(503U, response.status);

    cJSON *root = parse_response_body(&response);
    const cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
    TEST_CHECK_EQ_STRING("storage_unavailable",
                         cJSON_GetObjectItemCaseSensitive(error, "code")->valuestring);
    TEST_CHECK_EQ_STRING("restart could not be scheduled",
                         cJSON_GetObjectItemCaseSensitive(error, "message")->valuestring);
    cJSON_Delete(root);
    web_api_response_free(&response);
}

/* -------------------------------------------------------------------------
 * POST /api/v1/device/factory-reset — H3-032 accepted/recovery semantics.
 * ---------------------------------------------------------------------- */

static void prepare_factory_reset_call(web_api_call_t *call, char *body, size_t body_size) {
    TEST_CHECK(call != NULL);
    TEST_CHECK(body != NULL);
    const int written =
        snprintf(body, body_size,
                 "{\"confirmation\":\"FACTORY RESET\",\"adminPassword\":\"example-password\"}");
    TEST_CHECK(written > 0 && (size_t)written < body_size);
    *call = (web_api_call_t){
        .method = WEB_API_METHOD_POST,
        .path = {.route = WEB_API_ROUTE_DEVICE_FACTORY_RESET},
        .body = body,
        .body_length = (size_t)written,
    };
}

static void test_factory_reset_precommit_failure_is_not_202(void) {
    reset_fakes();
    fake_auth.password_matches = true;
    fake_device_controls.factory_reset_outcome = (device_controls_factory_reset_outcome_t){
        .durably_accepted = false,
        .recovery_required = false,
        .primary_error = APP_ERROR_STORAGE_UNAVAILABLE,
    };
    char body[256];
    web_api_call_t call;
    prepare_factory_reset_call(&call, body, sizeof(body));
    web_api_response_t response = {0};

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_handle_administration(&call, &response));
    TEST_CHECK_EQ_U64(503U, response.status);
    TEST_CHECK_EQ_U64(1U, fake_device_controls.factory_reset_calls);
    cJSON *root = parse_response_body(&response);
    const cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
    TEST_CHECK_EQ_STRING("storage_unavailable",
                         cJSON_GetObjectItemCaseSensitive(error, "code")->valuestring);
    cJSON_Delete(root);
    web_api_response_free(&response);
}

static void test_factory_reset_postcommit_failure_is_202_recovery(void) {
    reset_fakes();
    fake_auth.password_matches = true;
    fake_device_controls.factory_reset_outcome = (device_controls_factory_reset_outcome_t){
        .durably_accepted = true,
        .recovery_required = true,
        .primary_error = APP_ERROR_IO,
    };
    char body[256];
    web_api_call_t call;
    prepare_factory_reset_call(&call, body, sizeof(body));
    web_api_response_t response = {0};

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_handle_administration(&call, &response));
    TEST_CHECK_EQ_U64(202U, response.status);
    TEST_CHECK_EQ_U64(1U, fake_device_controls.factory_reset_calls);
    cJSON *root = parse_response_body(&response);
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "accepted")));
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "connectionWillClose")));
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "reprovisioningRequired")));
    TEST_CHECK(cJSON_IsFalse(cJSON_GetObjectItemCaseSensitive(root, "repositoryBlobsPreserved")));
    cJSON_Delete(root);
    web_api_response_free(&response);
}

/* -------------------------------------------------------------------------
 * The provisioned-mode WEB_API_ROUTE_SETUP fallback (setup_route_response):
 * GET -> 404, POST -> 409 (SPEC_V2 13.4). This is the live routing path a
 * real provisioned-device request answers through -- distinct from
 * test_web_server_setup_submit.c's test_already_provisioned_rejected(),
 * which exercises setup_submit_handler()'s own defense-in-depth check on the
 * (unprovisioned-only-registered) httpd route instead.
 * ---------------------------------------------------------------------- */

static void test_setup_route_get_returns_not_found(void) {
    reset_fakes();
    const web_api_call_t call = {
        .method = WEB_API_METHOD_GET,
        .path = {.route = WEB_API_ROUTE_SETUP},
    };
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_handle_administration(&call, &response));
    TEST_CHECK_EQ_U64(404U, response.status);

    cJSON *root = parse_response_body(&response);
    const cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
    TEST_CHECK_EQ_STRING("not_found", cJSON_GetObjectItemCaseSensitive(error, "code")->valuestring);
    TEST_CHECK_EQ_STRING("route not found",
                         cJSON_GetObjectItemCaseSensitive(error, "message")->valuestring);
    cJSON_Delete(root);
    web_api_response_free(&response);
}

static void test_setup_route_post_returns_conflict(void) {
    reset_fakes();
    const web_api_call_t call = {
        .method = WEB_API_METHOD_POST,
        .path = {.route = WEB_API_ROUTE_SETUP},
    };
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_handle_administration(&call, &response));
    TEST_CHECK_EQ_U64(409U, response.status);

    cJSON *root = parse_response_body(&response);
    const cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
    TEST_CHECK_EQ_STRING("conflict", cJSON_GetObjectItemCaseSensitive(error, "code")->valuestring);
    TEST_CHECK_EQ_STRING("device is already provisioned",
                         cJSON_GetObjectItemCaseSensitive(error, "message")->valuestring);
    cJSON_Delete(root);
    web_api_response_free(&response);
}

/* -------------------------------------------------------------------------
 * GET /api/v1/diagnostics (WEB_API_ROUTE_DIAGNOSTICS_FULL dispatch wiring).
 * web_diagnostics_handle()'s own composition (collecting a snapshot from
 * storage/auth/usb/executor/controls/wifi/http health and building its JSON)
 * is out of this track's scope -- see the stub above and V2-056 -- but
 * nothing anywhere else in this suite proves web_api_handle_administration()'s
 * switch statement actually reaches WEB_API_ROUTE_DIAGNOSTICS_FULL's case
 * rather than silently falling into APP_ERROR_NOT_FOUND alongside every
 * dedicated-handler route (STATUS, LIMITS, SEND, BLOB_COLLECTION, BLOB_ITEM,
 * AUTH_LOGIN, AUTH_LOGOUT).
 * This closes that one gap cheaply against the stub.
 * ---------------------------------------------------------------------- */

static void test_diagnostics_route_dispatches_to_handler(void) {
    reset_fakes();
    const web_api_call_t call = {
        .method = WEB_API_METHOD_GET,
        .path = {.route = WEB_API_ROUTE_DIAGNOSTICS_FULL},
    };
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_handle_administration(&call, &response));
    TEST_CHECK_EQ_U64(200U, response.status);
    cJSON *root = parse_response_body(&response);
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "stub")));
    cJSON_Delete(root);
    web_api_response_free(&response);
}

/* -------------------------------------------------------------------------
 * Invalid arguments.
 * ---------------------------------------------------------------------- */

static void test_null_arguments_rejected(void) {
    web_api_response_t response = {0};
    const web_api_call_t call = {.path = {.route = WEB_API_ROUTE_AUTH_SESSION}};
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_handle_administration(NULL, &response));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, web_api_handle_administration(&call, NULL));
}

int main(void) {
    test_handle_session_success();
    test_handle_session_backend_failure();
    test_session_and_cookie_outputs_carry_no_password_sentinel();
    test_handle_device_restart_success();
    test_handle_device_restart_backend_unavailable();
    test_factory_reset_precommit_failure_is_not_202();
    test_factory_reset_postcommit_failure_is_202_recovery();
    test_setup_route_get_returns_not_found();
    test_setup_route_post_returns_conflict();
    test_diagnostics_route_dispatches_to_handler();
    test_null_arguments_rejected();
    puts("web api administration tests passed");
    return 0;
}
