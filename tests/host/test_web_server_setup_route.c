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
 * mode POST handler), which this file does not exercise -- only
 * setup_state_handler() (GET) is this file's scope, per TODO_V2 V2-057's
 * remaining setup-state bullet. Compiling web_server_setup.c still requires
 * every symbol setup_submit_handler() references to resolve at link time
 * (device_settings_read/replace, auth_password_create), so narrow stand-ins
 * for those are provided below purely to satisfy the linker; none is called
 * by any test in this file. */

#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "auth.h"
#include "cJSON.h"
#include "device_settings.h"
#include "device_settings_v2.h"
#include "fake_httpd.h"
#include "test_assert.h"
#include "test_examples_fixture.h"
#include "web_server.h"
#include "web_server_internal.h"

/* ---------------------------------------------------------------------- *
 * Test doubles, present only so web_server_setup.c (which also defines
 * setup_submit_handler(), never called here) links -- see the file header
 * comment above.
 * ---------------------------------------------------------------------- */

app_error_code_t auth_session_validate(const char *session_token) {
    (void)session_token;
    /* Never reached: setup_state_handler() performs no authentication. */
    TEST_CHECK(false);
    return APP_ERROR_AUTH_REQUIRED;
}

app_error_code_t device_settings_read(app_v2_device_settings_t *out_settings) {
    (void)out_settings;
    TEST_CHECK(false); /* Never reached: only setup_submit_handler() calls this. */
    return APP_ERROR_INTERNAL;
}

app_error_code_t device_settings_replace(const app_v2_device_settings_t *settings,
                                         bool *out_changed) {
    (void)settings;
    (void)out_changed;
    TEST_CHECK(false); /* Never reached: only setup_submit_handler() calls this. */
    return APP_ERROR_INTERNAL;
}

app_error_code_t auth_password_create(const char *password, size_t password_length,
                                      auth_password_record_t *out_record) {
    (void)password;
    (void)password_length;
    (void)out_record;
    TEST_CHECK(false); /* Never reached: only setup_submit_handler() calls this. */
    return APP_ERROR_INTERNAL;
}

/* ---------------------------------------------------------------------- */

static void reset_fakes(void) {
    server_configuration = (web_server_config_t){0};
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

int main(void) {
    test_setup_state_valid_exactly_two_fields();
    test_setup_state_reflects_configured_device_name();
    test_setup_state_not_found_after_provisioning();

    puts("web server setup route tests passed");
    return 0;
}
