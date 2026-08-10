/* HTTP-adapter-level test for the unprovisioned-mode GET /api/v1/setup route
 * (firmware/components/web_server/web_server_setup.c's setup_state_handler();
 * SPEC_V2 13.4, TODO_V2 V2-057). This is the fixed, single-purpose httpd_uri_t
 * registration web_server_lifecycle.c wires up only while
 * server_configuration.mode == WEB_SERVER_MODE_SETUP -- distinct from
 * web_api_administration.c's setup_route_response() (already covered by
 * test_web_server_administration_route.c's test_setup_get_not_found_when_
 * provisioned()/test_setup_post_conflict_when_provisioned()), which answers
 * GET/POST /api/v1/setup through the generic /api/v1/ wildcard route AFTER
 * the device is provisioned. Drives the real setup_state_handler() against a
 * fake esp_http_server.h (fakes/esp_http_server_stub, fakes/fake_httpd.c) --
 * the same technique test_web_server_status_limits_route.c uses -- to prove
 * both halves of the one remaining V2-057 setup-state bullet: the 200
 * response while unprovisioned contains exactly the two SPEC_V2 13.4-approved
 * fields, and the route answers 404 once server_configuration.mode flips to
 * WEB_SERVER_MODE_NORMAL after provisioning.
 *
 * setup_state_handler() takes no session/body -- it is reachable without
 * authentication by design (SPEC_V2 13.4: unauthenticated GET) -- so the
 * usual unauthorized/expired-session/wrong-content-type/oversized/wrong-type/
 * extra/missing-body categories do not apply, the same bodyless-GET carve-out
 * already recorded for status/limits. Malformed-path and method-error are
 * likewise inapplicable for the same structural reason status/limits/send
 * document: this is a fixed, single-method URI registration that never
 * reaches web_api_parse_path()/web_api_route_allows_method(); those two
 * categories are unprovisioned-route-surface concerns already covered by
 * TODO_V2 V2-057's "unprovisioned route surface" bullet
 * (test_web_api_core.c/test_web_server_lifecycle.c, not this file).
 *
 * web_server_setup.c also defines setup_submit_handler() (the unprovisioned-
 * mode POST handler). This file's GET coverage predates TODO_V2 V2-057's
 * remaining setup-state bullet; POST coverage below was added 2026-08-10
 * after real hardware testing found a genuine production bug this file's
 * previous TEST_CHECK(false)-stub device_settings_read/replace and
 * auth_password_create never could have caught: a successful setup
 * submission committed new settings but never called esp_restart(), leaving
 * the device stuck in setup mode indefinitely (GET /api/v1/setup kept
 * reporting provisioned:false, login stayed disabled) -- fixed in
 * web_server_setup.c's setup_submit_handler(). test_setup_submit_success_
 * restarts_device() below is the regression test for that fix. */

#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "auth.h"
#include "cJSON.h"
#include "device_settings.h"
#include "device_settings_v2.h"
#include "esp_system.h"
#include "fake_httpd.h"
#include "setup_contract_v2.h"
#include "test_assert.h"
#include "test_examples_fixture.h"
#include "web_server.h"
#include "web_server_internal.h"

/* ---------------------------------------------------------------------- *
 * Test doubles so web_server_setup.c links, and so setup_submit_handler()'s
 * success path is genuinely exercised (not just stubbed out) below.
 * ---------------------------------------------------------------------- */

app_error_code_t auth_session_validate(const char *session_token) {
    (void)session_token;
    /* Never reached: neither setup_state_handler() nor setup_submit_handler()
     * performs session authentication (SPEC_V2 13.4: unauthenticated). */
    TEST_CHECK(false);
    return APP_ERROR_AUTH_REQUIRED;
}

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

static app_error_code_t g_password_create_result;

app_error_code_t auth_password_create(const char *password, size_t password_length,
                                      auth_password_record_t *out_record) {
    (void)password;
    (void)password_length;
    if (g_password_create_result != APP_ERROR_NONE) {
        return g_password_create_result;
    }
    /* setup_password_create() (web_server_setup.c) rejects iterations !=
     * AUTH_PBKDF2_ITERATIONS defensively, and app_v2_setup_prepare_candidate()
     * separately rejects an all-zero salt/verifier via password_material_valid()
     * -- both must be realistic, not merely non-error, for the success path
     * below to actually reach WEB_SETUP_SUBMIT_OK. */
    memset(out_record->salt, 0x5A, sizeof(out_record->salt));
    memset(out_record->hash, 0xA5, sizeof(out_record->hash));
    out_record->iterations = AUTH_PBKDF2_ITERATIONS;
    return APP_ERROR_NONE;
}

static size_t g_esp_restart_calls;

/* Deliberately returns normally (unlike the real, noreturn esp_restart()) so
 * the caller's post-restart code -- here, nothing, since setup_submit_
 * handler() returns immediately after -- still executes under test. Mirrors
 * test_web_server_administration_route.c's identical fake. */
void esp_restart(void) {
    ++g_esp_restart_calls;
}

/* ---------------------------------------------------------------------- */

static void reset_fakes(void) {
    server_configuration = (web_server_config_t){0};
    fake_device_settings = (fake_device_settings_t){0};
    app_v2_device_settings_init_unprovisioned(&fake_device_settings.record);
    g_password_create_result = APP_ERROR_NONE;
    g_esp_restart_calls = 0U;
}

static void bind_json_body(httpd_req_t *request, fake_httpd_request_t *fake, const char *uri,
                           const char *body) {
    const size_t length = strlen(body);
    fake_httpd_set_body(fake, body, length, 0U);
    fake_httpd_bind(request, fake, uri, length);
}

static cJSON *parse_response(const fake_httpd_request_t *fake) {
    cJSON *root = cJSON_ParseWithLength(fake->response_body, fake->response_body_length);
    TEST_CHECK(root != NULL);
    return root;
}

/* -------------------------------------------------------------------------
 * GET /api/v1/setup (unprovisioned mode)
 * ---------------------------------------------------------------------- */

static void test_setup_state_valid_exactly_two_fields(void) {
    reset_fakes();
    /* Seeded from contracts/v2/api/examples.json's own "setupState" fixture
     * (via server_configuration.setup_device_name below) so the real
     * response can be compared against it directly, not just against
     * hand-typed literals -- TODO_V2 V2-057's "consume the same checked-in
     * examples from C and TypeScript tests" bullet. The TypeScript side
     * already does this (webapp/tests/v2-api-contracts.test.ts's
     * isSetupStateResponse(examples.setupState) check); this is the C side's
     * first case of the same practice. */
    const cJSON *example = test_examples_fixture_get("setupState");
    const char *example_device_name =
        cJSON_GetObjectItemCaseSensitive(example, "deviceName")->valuestring;
    server_configuration.mode = WEB_SERVER_MODE_SETUP;
    (void)snprintf(server_configuration.setup_device_name,
                   sizeof(server_configuration.setup_device_name), "%s", example_device_name);
    fake_httpd_request_t fake;
    fake_httpd_reset(&fake);
    httpd_req_t request;
    fake_httpd_bind(&request, &fake, "/api/v1/setup", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, setup_state_handler(&request));
    TEST_CHECK_EQ_STRING("200 OK", fake.response_status);
    TEST_CHECK_EQ_STRING("application/json", fake.response_type);
    TEST_CHECK_EQ_STRING("no-store", fake_httpd_response_header(&fake, "Cache-Control"));

    cJSON *root = parse_response(&fake);
    /* SPEC_V2 13.4: "The response has no optional fields" -- a full,
     * order-independent deep comparison against the checked-in example
     * proves both that no field is missing and that none was added (a
     * mismatched member count, an extra field, or a wrong value all fail
     * cJSON_Compare, not just a hand-picked subset of assertions). */
    TEST_CHECK(cJSON_Compare(root, example, true) != 0);
    TEST_CHECK(cJSON_IsFalse(cJSON_GetObjectItemCaseSensitive(root, "provisioned")));
    TEST_CHECK_EQ_STRING(example_device_name,
                         cJSON_GetObjectItemCaseSensitive(root, "deviceName")->valuestring);
    cJSON_Delete(root);
}

static void test_setup_state_reflects_configured_device_name(void) {
    reset_fakes();
    server_configuration.mode = WEB_SERVER_MODE_SETUP;
    (void)snprintf(server_configuration.setup_device_name,
                   sizeof(server_configuration.setup_device_name), "%s", "Desk Macro Keyboard");
    fake_httpd_request_t fake;
    fake_httpd_reset(&fake);
    httpd_req_t request;
    fake_httpd_bind(&request, &fake, "/api/v1/setup", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, setup_state_handler(&request));
    cJSON *root = parse_response(&fake);
    TEST_CHECK_EQ_STRING("Desk Macro Keyboard",
                         cJSON_GetObjectItemCaseSensitive(root, "deviceName")->valuestring);
    cJSON_Delete(root);
}

static void test_setup_state_not_found_after_provisioning(void) {
    reset_fakes();
    server_configuration.mode = WEB_SERVER_MODE_NORMAL;
    fake_httpd_request_t fake;
    fake_httpd_reset(&fake);
    httpd_req_t request;
    fake_httpd_bind(&request, &fake, "/api/v1/setup", 0U);

    TEST_CHECK_EQ_INT(ESP_OK, setup_state_handler(&request));
    TEST_CHECK_EQ_STRING("404 Not Found", fake.response_status);
    cJSON *root = parse_response(&fake);
    const cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
    TEST_CHECK_EQ_STRING("not_found", cJSON_GetObjectItemCaseSensitive(error, "code")->valuestring);
    cJSON_Delete(root);
}

/* -------------------------------------------------------------------------
 * POST /api/v1/setup (unprovisioned mode) -- setup_submit_handler()
 * ---------------------------------------------------------------------- */

static void seed_valid_setup_session(void) {
    const app_v2_string_view_t code = {.data = "12345678", .length = 8U};
    TEST_CHECK_EQ_INT(APP_V2_SETUP_OK, (int)app_v2_setup_session_init(&setup_session, code));
}

static const char *const VALID_SETUP_BODY =
    "{\"setupCode\":\"12345678\",\"deviceName\":\"Desk Macro Keyboard\","
    "\"apSsid\":\"MacroKeyboard\",\"apPassphrase\":\"example-passphrase\","
    "\"adminPassword\":\"example-admin-password\",\"requireSerialConfirmation\":false}";

/* Regression test for a real hardware bug found 2026-08-10: a successful
 * setup submission committed new settings (confirmed separately on real
 * hardware: resubmitting the same code afterward correctly got 409 "already
 * provisioned") but never restarted the device, so it never left setup mode.
 * Root cause: setup_submit_handler() is its own dedicated httpd_uri_t
 * registration (web_server_lifecycle.c's setup_routes[]), bypassing the
 * generic api_handler() dispatch that is the only place esp_restart() was
 * ever called for /api/v1/device/restart and /api/v1/device/factory-reset. */
static void test_setup_submit_success_restarts_device(void) {
    reset_fakes();
    server_configuration.mode = WEB_SERVER_MODE_SETUP;
    seed_valid_setup_session();
    fake_httpd_request_t fake;
    fake_httpd_reset(&fake);
    httpd_req_t request;
    bind_json_body(&request, &fake, "/api/v1/setup", VALID_SETUP_BODY);

    TEST_CHECK_EQ_INT(ESP_OK, setup_submit_handler(&request));
    TEST_CHECK_EQ_STRING("202 Accepted", fake.response_status);
    cJSON *root = parse_response(&fake);
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "accepted")));
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "restartRequired")));
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "connectionWillClose")));
    cJSON_Delete(root);
    TEST_CHECK_EQ_U64(1U, (uint64_t)g_esp_restart_calls);
}

/* Symmetry check: a rejected submission must never restart the device
 * (esp_restart() gated on WEB_SETUP_SUBMIT_OK, not just "a response was
 * sent" -- see setup_submit_handler()). */
static void test_setup_submit_failure_does_not_restart_device(void) {
    reset_fakes();
    server_configuration.mode = WEB_SERVER_MODE_SETUP;
    seed_valid_setup_session();
    fake_httpd_request_t fake;
    fake_httpd_reset(&fake);
    httpd_req_t request;
    static const char *const wrong_code_body =
        "{\"setupCode\":\"99999999\",\"deviceName\":\"Desk Macro Keyboard\","
        "\"apSsid\":\"MacroKeyboard\",\"apPassphrase\":\"example-passphrase\","
        "\"adminPassword\":\"example-admin-password\",\"requireSerialConfirmation\":false}";
    bind_json_body(&request, &fake, "/api/v1/setup", wrong_code_body);

    TEST_CHECK_EQ_INT(ESP_OK, setup_submit_handler(&request));
    TEST_CHECK(strcmp(fake.response_status, "202 Accepted") != 0);
    TEST_CHECK_EQ_U64(0U, (uint64_t)g_esp_restart_calls);
}

int main(void) {
    test_setup_state_valid_exactly_two_fields();
    test_setup_state_reflects_configured_device_name();
    test_setup_state_not_found_after_provisioning();
    test_setup_submit_success_restarts_device();
    test_setup_submit_failure_does_not_restart_device();

    puts("web server setup route tests passed");
    return 0;
}
