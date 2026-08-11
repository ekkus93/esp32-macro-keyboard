/* HTTP-adapter-level tests for POST/GET/DELETE /api/v1/send
 * (firmware/components/web_server/web_server_send.c; SPEC_V2 13.10, TODO_V2
 * V2-057). Unlike test_web_send.c (which only exercises the pure
 * web_send_create_handle()/web_send_get_handle()/web_send_cancel_handle()
 * core against a fake_send_backend_t), this drives the real
 * send_create_handler()/send_get_handler()/send_cancel_handler() httpd_req_t-
 * taking functions against a fake esp_http_server.h (fakes/esp_http_server_stub,
 * fakes/fake_httpd.c) -- the same technique test_web_server_blob.c uses -- so
 * it also covers the auth gate (authorize_mutation), the Content-Type/body-
 * size checks that only exist in the httpd adapter, and the real
 * macro_parser_v2 compile path.
 *
 * /api/v1/send is a fixed-URI route registered directly ahead of the generic
 * "/api/v1/" wildcard (see web_server_common.c's route table) for GET, POST,
 * and DELETE alike -- it never reaches web_api_parse_path()/
 * web_api_route_allows_method(), so "malformed-path" (there is no path
 * parameter) and "method-error" (e.g. PUT /api/v1/send) are covered instead
 * at the pure-function level in test_web_api_core.c
 * (WEB_API_ROUTE_SEND cases in test_method_and_body_policy()). */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "app_limits_v2.h"
#include "app_uuid.h"
#include "auth.h"
#include "cJSON.h"
#include "fake_httpd.h"
#include "macro_executor.h"
#include "test_assert.h"
#include "test_examples_fixture.h"
#include "web_server_internal.h"

#define SEND_TEST_SESSION_TOKEN                                                                    \
    "0123456789abcdef"                                                                             \
    "0123456789abcdef"                                                                             \
    "0123456789abcdef"                                                                             \
    "0123456789abcdef"

/* ---------------------------------------------------------------------- *
 * Test doubles for auth.c's and macro_executor.c's public entry points --
 * neither links on a host build (NVS/mbedtls and FreeRTOS respectively). The
 * same narrow-substitution technique test_web_server_blob_fixture.inc and
 * test_web_api_administration.c already use.
 * ---------------------------------------------------------------------- */

static app_error_code_t g_auth_result;

app_error_code_t auth_session_validate(const char *session_token) {
    (void)session_token;
    return g_auth_result;
}

static app_error_code_t g_submit_result;
static bool g_submit_called;
static macro_execution_request_t g_last_submitted_request;

app_error_code_t macro_executor_submit(macro_execution_request_t *request) {
    g_submit_called = true;
    g_last_submitted_request = *request;
    if (g_submit_result == APP_ERROR_NONE) {
        /* web_send.h: ownership of request->plan transfers to the executor on
         * success -- mirror that here instead of leaking it. */
        macro_plan_v2_free(&request->plan);
        request->plan = (macro_plan_t){0};
    }
    return g_submit_result;
}

static macro_execution_status_t g_execution_status;

macro_execution_status_t macro_executor_get_status(void) {
    return g_execution_status;
}

static app_error_code_t g_cancel_result;
static bool g_cancel_called;

app_error_code_t macro_executor_cancel(void) {
    g_cancel_called = true;
    return g_cancel_result;
}

app_error_code_t macro_executor_confirm(void) {
    TEST_CHECK(false); /* Not reached by any route under test. */
    return APP_ERROR_INTERNAL;
}

/* ---------------------------------------------------------------------- */

static void reset_fakes(void) {
    g_auth_result = APP_ERROR_NONE;
    g_submit_result = APP_ERROR_NONE;
    g_submit_called = false;
    g_last_submitted_request = (macro_execution_request_t){0};
    g_execution_status = (macro_execution_status_t){.state = EXECUTION_IDLE, .available = true};
    g_cancel_result = APP_ERROR_NONE;
    g_cancel_called = false;
}

static void authenticate(fake_httpd_request_t *fake) {
    fake_httpd_add_request_header(fake, "Cookie", "MKSESSION=" SEND_TEST_SESSION_TOKEN);
}

static cJSON *parse_response(const fake_httpd_request_t *fake) {
    cJSON *root = cJSON_ParseWithLength(fake->response_body, fake->response_body_length);
    TEST_CHECK(root != NULL);
    return root;
}

/* "id" is a fresh random UUID on every real send -- see web_send.c's
 * app_uuid_generate() call -- so it can never equal
 * contracts/v2/api/examples.json's fixed example id; every other field,
 * including "estimatedDurationMs", gets full wholesale drift protection.
 *
 * examples.json's/SPEC_V2.md 13.10's estimatedDurationMs for this exact
 * request ("make -j8{ENTER}", keyPressMs 8, interKeyMs 15) previously read
 * 214, which did not match the real, deterministic formula both the
 * firmware parser (macro_parser_v2.c's v2_append_action(): each non-delay
 * action costs key_press_ms + inter_key_ms) and the webapp parser
 * (macroCompiler.ts, the same shared-corpus formula) compute for 9
 * non-delay actions at those settings: 9 * (8 + 15) = 207. Confirmed as a
 * stale hand-computed illustrative number (not a code defect), reported to
 * and fixed by Phil per this project's frozen-spec discipline -- see
 * docs/implementation-v2/V2_057_PHASE5_HARDENING_2026-08-09.md for the
 * original discrepancy writeup. examples.json and SPEC_V2.md now both read
 * 207, matching this test's separate exact assertion below. */
static void assert_matches_example_ignoring_id(const cJSON *actual, const char *example_key) {
    cJSON *comparable_actual = cJSON_Duplicate(actual, true);
    TEST_CHECK(comparable_actual != NULL);
    TEST_CHECK(cJSON_ReplaceItemInObjectCaseSensitive(comparable_actual, "id",
                                                      cJSON_CreateString("normalized-id")));
    cJSON *comparable_example = cJSON_Duplicate(test_examples_fixture_get(example_key), true);
    TEST_CHECK(comparable_example != NULL);
    TEST_CHECK(cJSON_ReplaceItemInObjectCaseSensitive(comparable_example, "id",
                                                      cJSON_CreateString("normalized-id")));
    TEST_CHECK(cJSON_Compare(comparable_actual, comparable_example, true) != 0);
    cJSON_Delete(comparable_actual);
    cJSON_Delete(comparable_example);
}

static void bind_json_body(httpd_req_t *request, fake_httpd_request_t *fake, const char *body) {
    fake_httpd_add_request_header(fake, "Content-Type", "application/json");
    const size_t length = strlen(body);
    fake_httpd_set_body(fake, body, length, 0U);
    fake_httpd_bind(request, fake, "/api/v1/send", length);
}

/* -------------------------------------------------------------------------
 * POST /api/v1/send (send_create_handler)
 * ---------------------------------------------------------------------- */

static void test_send_create_valid(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "{\"source\":\"first\",\"keyPressMs\":8,\"interKeyMs\":15}");

    TEST_CHECK_EQ_INT(ESP_OK, send_create_handler(&request));
    TEST_CHECK_EQ_STRING("202 Accepted", fake.response_status);
    TEST_CHECK_EQ_STRING("application/json", fake.response_type);
    TEST_CHECK(g_submit_called);
    TEST_CHECK_EQ_U64(8U, g_last_submitted_request.key_press_ms);
    TEST_CHECK_EQ_U64(15U, g_last_submitted_request.inter_key_ms);

    cJSON *root = parse_response(&fake);
    TEST_CHECK(app_uuid_is_valid_string(cJSON_GetObjectItemCaseSensitive(root, "id")->valuestring));
    TEST_CHECK_EQ_STRING("running", cJSON_GetObjectItemCaseSensitive(root, "state")->valuestring);
    TEST_CHECK(cJSON_GetObjectItemCaseSensitive(root, "actionCount")->valuedouble > 0.0);
    cJSON_Delete(root);
}

/* TODO_V2 V2-057 "consume the same checked-in examples from C and TypeScript
 * tests": submits contracts/v2/api/examples.json's own "sendRequest" body
 * verbatim, so the response can be diffed against "sendAccepted" wholesale
 * (id normalized -- see assert_matches_example_ignoring_id()) rather than
 * the three-field spot check test_send_create_valid() above does. Also
 * proves the real macro_parser_v2 compile path for "make -j8{ENTER}" (8
 * US-ASCII key actions + one {ENTER} directive) produces exactly the
 * actionCount/estimatedDurationMs the checked-in example claims. */
static void test_send_create_valid_matches_example(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake,
                   "{\"source\":\"make -j8{ENTER}\",\"keyPressMs\":8,\"interKeyMs\":15}");

    TEST_CHECK_EQ_INT(ESP_OK, send_create_handler(&request));
    TEST_CHECK_EQ_STRING("202 Accepted", fake.response_status);

    cJSON *root = parse_response(&fake);
    TEST_CHECK(app_uuid_is_valid_string(cJSON_GetObjectItemCaseSensitive(root, "id")->valuestring));
    /* The real, deterministic value for this exact request -- see
     * assert_matches_example_ignoring_id()'s comment. */
    TEST_CHECK_EQ_U64(
        207U, (uint64_t)cJSON_GetObjectItemCaseSensitive(root, "estimatedDurationMs")->valuedouble);
    assert_matches_example_ignoring_id(root, "sendAccepted");
    cJSON_Delete(root);
}

static void test_send_create_unauthorized_without_cookie(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    bind_json_body(&request, &fake, "{\"source\":\"first\",\"keyPressMs\":8,\"interKeyMs\":15}");

    TEST_CHECK_EQ_INT(ESP_OK, send_create_handler(&request));
    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
    TEST_CHECK(!g_submit_called);
}

static void test_send_create_unauthorized_expired_session(void) {
    reset_fakes();
    g_auth_result = APP_ERROR_AUTH_REQUIRED;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "{\"source\":\"first\",\"keyPressMs\":8,\"interKeyMs\":15}");

    TEST_CHECK_EQ_INT(ESP_OK, send_create_handler(&request));
    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
    TEST_CHECK(!g_submit_called);
}

static void test_send_create_missing_content_type(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    static const char body[] = "{\"source\":\"first\",\"keyPressMs\":8,\"interKeyMs\":15}";
    fake_httpd_set_body(&fake, body, sizeof(body) - 1U, 0U);
    fake_httpd_bind(&request, &fake, "/api/v1/send", sizeof(body) - 1U);

    TEST_CHECK_EQ_INT(ESP_OK, send_create_handler(&request));
    TEST_CHECK_EQ_STRING("415 Unsupported Media Type", fake.response_status);
    TEST_CHECK(!g_submit_called);
}

static void test_send_create_wrong_content_type(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    fake_httpd_add_request_header(&fake, "Content-Type", "text/plain");
    static const char body[] = "{\"source\":\"first\",\"keyPressMs\":8,\"interKeyMs\":15}";
    fake_httpd_set_body(&fake, body, sizeof(body) - 1U, 0U);
    fake_httpd_bind(&request, &fake, "/api/v1/send", sizeof(body) - 1U);

    TEST_CHECK_EQ_INT(ESP_OK, send_create_handler(&request));
    TEST_CHECK_EQ_STRING("415 Unsupported Media Type", fake.response_status);
    TEST_CHECK(!g_submit_called);
}

static void test_send_create_missing_body(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    fake_httpd_add_request_header(&fake, "Content-Type", "application/json");
    fake_httpd_bind(&request, &fake, "/api/v1/send", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, send_create_handler(&request));
    TEST_CHECK_EQ_STRING("400 Bad Request", fake.response_status);
    TEST_CHECK(!g_submit_called);
}

static void test_send_create_oversized_body(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    fake_httpd_add_request_header(&fake, "Content-Type", "application/json");
    fake_httpd_bind(&request, &fake, "/api/v1/send", (size_t)APP_V2_JSON_BODY_MAX_BYTES + 1U);

    TEST_CHECK_EQ_INT(ESP_OK, send_create_handler(&request));
    TEST_CHECK_EQ_STRING("413 Payload Too Large", fake.response_status);
    TEST_CHECK(!g_submit_called);
}

static void test_send_create_wrong_type_field_rejected(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake,
                   "{\"source\":\"first\",\"keyPressMs\":\"8\",\"interKeyMs\":15}");

    TEST_CHECK_EQ_INT(ESP_OK, send_create_handler(&request));
    TEST_CHECK_EQ_STRING("400 Bad Request", fake.response_status);
    TEST_CHECK(!g_submit_called);
}

static void test_send_create_extra_field_rejected(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake,
                   "{\"source\":\"first\",\"keyPressMs\":8,\"interKeyMs\":15,\"extra\":true}");

    TEST_CHECK_EQ_INT(ESP_OK, send_create_handler(&request));
    TEST_CHECK_EQ_STRING("400 Bad Request", fake.response_status);
    TEST_CHECK(!g_submit_called);
}

static void test_send_create_missing_field_rejected(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "{\"source\":\"first\",\"keyPressMs\":8}");

    TEST_CHECK_EQ_INT(ESP_OK, send_create_handler(&request));
    TEST_CHECK_EQ_STRING("400 Bad Request", fake.response_status);
    TEST_CHECK(!g_submit_called);
}

static void test_send_create_parse_error_reports_location(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake,
                   "{\"source\":\"{DELAY:-1}\",\"keyPressMs\":8,\"interKeyMs\":15}");

    TEST_CHECK_EQ_INT(ESP_OK, send_create_handler(&request));
    TEST_CHECK_EQ_STRING("422 Unprocessable Entity", fake.response_status);
    TEST_CHECK(!g_submit_called);
    cJSON *root = parse_response(&fake);
    const cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
    TEST_CHECK_EQ_STRING("macro_parse_error",
                         cJSON_GetObjectItemCaseSensitive(error, "code")->valuestring);
    TEST_CHECK(cJSON_GetObjectItemCaseSensitive(error, "byteOffset") != NULL);
    cJSON_Delete(root);
}

static void test_send_create_busy(void) {
    reset_fakes();
    g_submit_result = APP_ERROR_EXECUTOR_BUSY;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "{\"source\":\"first\",\"keyPressMs\":8,\"interKeyMs\":15}");

    TEST_CHECK_EQ_INT(ESP_OK, send_create_handler(&request));
    TEST_CHECK_EQ_STRING("409 Conflict", fake.response_status);
    TEST_CHECK(g_submit_called);
}

static void test_send_create_usb_not_ready(void) {
    reset_fakes();
    g_submit_result = APP_ERROR_USB_NOT_READY;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "{\"source\":\"first\",\"keyPressMs\":8,\"interKeyMs\":15}");

    TEST_CHECK_EQ_INT(ESP_OK, send_create_handler(&request));
    TEST_CHECK_EQ_STRING("503 Service Unavailable", fake.response_status);
}

static void assert_internal_error_response(const fake_httpd_request_t *fake,
                                           const char *expected_message) {
    TEST_CHECK_EQ_STRING("500 Internal Server Error", fake->response_status);
    cJSON *root = parse_response(fake);
    const cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
    TEST_CHECK(error != NULL);
    TEST_CHECK_EQ_STRING("internal", cJSON_GetObjectItemCaseSensitive(error, "code")->valuestring);
    TEST_CHECK_EQ_STRING(expected_message,
                         cJSON_GetObjectItemCaseSensitive(error, "message")->valuestring);
    TEST_CHECK(strstr(fake->response_body, "storage_unavailable") == NULL);
    cJSON_Delete(root);
}

static void test_send_create_executor_unavailable_is_internal(void) {
    reset_fakes();
    g_submit_result = APP_ERROR_INTERNAL;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    bind_json_body(&request, &fake, "{\"source\":\"first\",\"keyPressMs\":8,\"interKeyMs\":15}");

    TEST_CHECK_EQ_INT(ESP_OK, send_create_handler(&request));
    TEST_CHECK(g_submit_called);
    assert_internal_error_response(&fake, "send could not be accepted");
}

/* -------------------------------------------------------------------------
 * GET /api/v1/send (send_get_handler)
 * ---------------------------------------------------------------------- */

static void test_send_get_valid(void) {
    reset_fakes();
    app_uuid_t id = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_generate(&id));
    g_execution_status = (macro_execution_status_t){
        .state = EXECUTION_RUNNING,
        .available = true,
        .execution_id = id,
        .action_index = 2U,
        .action_count = 9U,
    };
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/send", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, send_get_handler(&request));
    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);
    cJSON *root = parse_response(&fake);
    TEST_CHECK_EQ_STRING(id.value, cJSON_GetObjectItemCaseSensitive(root, "id")->valuestring);
    TEST_CHECK_EQ_STRING("running", cJSON_GetObjectItemCaseSensitive(root, "state")->valuestring);
    cJSON_Delete(root);
}

/* TODO_V2 V2-057 "consume the same checked-in examples from C and TypeScript
 * tests": drives a real POST /api/v1/send with the checked-in "sendRequest"
 * body first (the same request test_send_create_valid_matches_example()
 * uses), so last_send_estimated_duration_ms (web_server_send.c's own static
 * cache, populated only by a real send_create_handler() call) holds the
 * real 207 ms the parser computed rather than a hand-picked number, then
 * simulates that same send having finished (completed, every action run) and
 * diffs the live GET response against "sendStatus" wholesale (id
 * normalized). */
static void test_send_get_valid_matches_example(void) {
    reset_fakes();
    fake_httpd_request_t create_fake;
    httpd_req_t create_request;
    fake_httpd_reset(&create_fake);
    authenticate(&create_fake);
    bind_json_body(&create_request, &create_fake,
                   "{\"source\":\"make -j8{ENTER}\",\"keyPressMs\":8,\"interKeyMs\":15}");
    TEST_CHECK_EQ_INT(ESP_OK, send_create_handler(&create_request));
    TEST_CHECK_EQ_STRING("202 Accepted", create_fake.response_status);

    g_execution_status = (macro_execution_status_t){
        .state = EXECUTION_COMPLETED,
        .available = true,
        .execution_id = g_last_submitted_request.execution_id,
        .action_index = 9U,
        .action_count = 9U,
        .cancellation_requested = false,
        .error = APP_ERROR_NONE,
        .release_error = APP_ERROR_NONE,
    };
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/send", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, send_get_handler(&request));
    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);
    cJSON *root = parse_response(&fake);
    TEST_CHECK_EQ_STRING(g_last_submitted_request.execution_id.value,
                         cJSON_GetObjectItemCaseSensitive(root, "id")->valuestring);
    TEST_CHECK_EQ_U64(
        207U, (uint64_t)cJSON_GetObjectItemCaseSensitive(root, "estimatedDurationMs")->valuedouble);
    assert_matches_example_ignoring_id(root, "sendStatus");
    cJSON_Delete(root);
}

static void test_send_get_unauthorized_without_cookie(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/send", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, send_get_handler(&request));
    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
}

static void test_send_get_unauthorized_expired_session(void) {
    reset_fakes();
    g_auth_result = APP_ERROR_AUTH_REQUIRED;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/send", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, send_get_handler(&request));
    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
}

static void test_send_get_release_fault_visible_when_executor_unavailable(void) {
    reset_fakes();
    app_uuid_t id = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_generate(&id));
    g_execution_status = (macro_execution_status_t){
        .state = EXECUTION_FAILED,
        .available = false,
        .execution_id = id,
        .action_index = 1U,
        .action_count = 1U,
        .error = APP_ERROR_IO,
        .release_error = APP_ERROR_USB_NOT_READY,
    };
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/send", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, send_get_handler(&request));
    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);
    cJSON *root = parse_response(&fake);
    TEST_CHECK_EQ_STRING("failed", cJSON_GetObjectItemCaseSensitive(root, "state")->valuestring);
    TEST_CHECK_EQ_STRING(app_error_code_string(APP_ERROR_IO),
                         cJSON_GetObjectItemCaseSensitive(root, "error")->valuestring);
    TEST_CHECK_EQ_STRING(app_error_code_string(APP_ERROR_USB_NOT_READY),
                         cJSON_GetObjectItemCaseSensitive(root, "releaseError")->valuestring);
    cJSON_Delete(root);
}

static void test_send_get_generic_executor_unavailable_is_internal(void) {
    reset_fakes();
    g_execution_status = (macro_execution_status_t){
        .state = EXECUTION_RUNNING,
        .available = false,
        .release_error = APP_ERROR_NONE,
    };
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/send", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, send_get_handler(&request));
    assert_internal_error_response(&fake, "send status unavailable");
}

static void test_send_get_never_sent(void) {
    reset_fakes();
    g_execution_status = (macro_execution_status_t){.state = EXECUTION_IDLE, .available = true};
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/send", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, send_get_handler(&request));
    TEST_CHECK_EQ_STRING("404 Not Found", fake.response_status);
}

/* -------------------------------------------------------------------------
 * DELETE /api/v1/send (send_cancel_handler)
 * ---------------------------------------------------------------------- */

static void test_send_cancel_valid(void) {
    reset_fakes();
    g_execution_status = (macro_execution_status_t){.state = EXECUTION_RUNNING, .available = true};
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/send", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, send_cancel_handler(&request));
    TEST_CHECK_EQ_STRING("202 Accepted", fake.response_status);
    TEST_CHECK(g_cancel_called);
    TEST_CHECK(strstr(fake.response_body, "\"accepted\":true") != NULL);
}

static void test_send_cancel_unauthorized_without_cookie(void) {
    reset_fakes();
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/send", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, send_cancel_handler(&request));
    TEST_CHECK_EQ_STRING("401 Unauthorized", fake.response_status);
    TEST_CHECK(!g_cancel_called);
}

static void test_send_cancel_executor_unavailable_is_internal(void) {
    reset_fakes();
    g_execution_status = (macro_execution_status_t){.state = EXECUTION_RUNNING, .available = true};
    g_cancel_result = APP_ERROR_INTERNAL;
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/send", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, send_cancel_handler(&request));
    TEST_CHECK(g_cancel_called);
    assert_internal_error_response(&fake, "cancellation could not be recorded");
}

static void test_send_cancel_never_sent(void) {
    reset_fakes();
    g_execution_status = (macro_execution_status_t){.state = EXECUTION_IDLE};
    fake_httpd_request_t fake;
    httpd_req_t request;
    fake_httpd_reset(&fake);
    authenticate(&fake);
    fake_httpd_bind(&request, &fake, "/api/v1/send", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, send_cancel_handler(&request));
    TEST_CHECK_EQ_STRING("404 Not Found", fake.response_status);
}

int main(void) {
    test_send_create_valid();
    test_send_create_valid_matches_example();
    test_send_create_unauthorized_without_cookie();
    test_send_create_unauthorized_expired_session();
    test_send_create_missing_content_type();
    test_send_create_wrong_content_type();
    test_send_create_missing_body();
    test_send_create_oversized_body();
    test_send_create_wrong_type_field_rejected();
    test_send_create_extra_field_rejected();
    test_send_create_missing_field_rejected();
    test_send_create_parse_error_reports_location();
    test_send_create_busy();
    test_send_create_usb_not_ready();
    test_send_create_executor_unavailable_is_internal();

    test_send_get_valid();
    test_send_get_valid_matches_example();
    test_send_get_unauthorized_without_cookie();
    test_send_get_unauthorized_expired_session();
    test_send_get_release_fault_visible_when_executor_unavailable();
    test_send_get_generic_executor_unavailable_is_internal();
    test_send_get_never_sent();

    test_send_cancel_valid();
    test_send_cancel_unauthorized_without_cookie();
    test_send_cancel_executor_unavailable_is_internal();
    test_send_cancel_never_sent();

    puts("web server send route tests passed");
    return 0;
}
