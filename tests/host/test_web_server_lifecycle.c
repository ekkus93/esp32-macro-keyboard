/* Live route-registration test for web_server_lifecycle.c (TODO_V2 V2-051's
 * "Test the complete unprovisioned/provisioned route-access matrix").
 *
 * Every other route test in this codebase (test_web_server_blob*.c,
 * test_web_server_status_limits_route.c, test_web_server_send_route.c,
 * test_web_server_setup_route.c, test_web_server_administration_route.c)
 * calls one fixed-URI handler function directly against a fake httpd
 * request/response (fakes/fake_httpd.c) -- that proves what each handler
 * *does*, but none of them proves what URIs/methods are actually *reachable*
 * in each provisioning mode, because none of them ever calls
 * web_server_start()/web_server_lifecycle.c's normal_routes[]/setup_routes[]
 * tables. That gap was previously closed only structurally, by
 * scripts/check-setup-route-isolation.sh parsing web_server_lifecycle.c's
 * source text with a regex -- a real, useful guard, but not an executed
 * test, and not proof that ESP-IDF's own registration/dispatch (as opposed
 * to the array literal) resolves each route the way the source implies.
 *
 * This file closes that gap one level up from the source-parsing approach:
 * fakes/fake_httpd_router.c is a small, faithful port of ESP-IDF v5.5.5's
 * real httpd_register_uri_handler()/httpd_find_uri_handler()/
 * httpd_uri_match_wildcard() (components/esp_http_server/src/httpd_uri.c),
 * so the real, unmodified web_server_start() literally registers the real,
 * unmodified normal_routes[]/setup_routes[] tables into it via the real
 * httpd_start()/httpd_register_uri_handler() call sites, and this file then
 * asks the fake router which handler -- if any -- a given uri/method
 * resolves to, exactly the way ESP-IDF's own dispatcher would. web_server_
 * async_start()/web_server_async_stop() (FreeRTOS-backed, not host-linkable)
 * are the only two symbols web_server_lifecycle.c calls that this file fakes
 * outside of the httpd layer; every route handler symbol
 * (status_handler(), blob_list_handler(), ...) is defined below as a stand-in
 * that is never called (TEST_CHECK(false) guards each one) -- only its
 * address is compared against what the fake router resolved, and the actual
 * handler *behavior* for every one of these routes is already covered
 * elsewhere (see the file list above), so re-testing behavior here would be
 * duplication, not new coverage.
 *
 * SPEC_V2 12.3: "An unprovisioned device exposes only GET /api/v1/setup,
 * POST /api/v1/setup, and the static assets required for the setup UI. Every
 * other /api/v1 route is unavailable while the device is unprovisioned."
 * SPEC_V2 13.4 / 18.1#37: after provisioning, GET /api/v1/setup returns 404
 * and POST /api/v1/setup returns 409 -- both already verified at the
 * live-handler level by test_web_server_administration_route.c; this file
 * only proves that provisioned-mode GET/POST /api/v1/setup resolve to
 * api_handler() (the generic "/api/v1/" wildcard, which is what implements
 * that 404/409), not that either status code is produced. */

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "factory_reset_state.h"
#include "fake_httpd_router.h"
#include "test_assert.h"
#include "web_server.h"
#include "web_server_internal.h"

static factory_reset_state_t g_factory_reset_state = FACTORY_RESET_STATE_NONE;
static app_error_code_t g_factory_reset_state_read_result = APP_ERROR_NONE;
static bool g_reset_settings_restart_required;
static unsigned int g_reset_gate_error_calls;
static unsigned int g_reset_gate_status;
static app_error_code_t g_reset_gate_code;
static const char *g_reset_gate_message;

app_error_code_t factory_reset_state_read(factory_reset_state_t *out_state) {
    if (out_state == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (g_factory_reset_state_read_result != APP_ERROR_NONE) {
        return g_factory_reset_state_read_result;
    }
    *out_state = g_factory_reset_state;
    return APP_ERROR_NONE;
}

bool device_controls_reset_settings_restart_required(void);

bool device_controls_reset_settings_restart_required(void) {
    return g_reset_settings_restart_required;
}

esp_err_t web_api_send_status_error(httpd_req_t *request, unsigned int status,
                                    app_error_code_t code, const char *message) {
    TEST_CHECK(request != NULL);
    ++g_reset_gate_error_calls;
    g_reset_gate_status = status;
    g_reset_gate_code = code;
    g_reset_gate_message = message;
    return ESP_OK;
}

/* ---------------------------------------------------------------------- *
 * Handler stand-ins. web_server_lifecycle.c's route tables reference these
 * symbols by address to build httpd_uri_t entries; this file never invokes
 * any of them through the fake router (it only compares addresses), so each
 * one below is guarded to fail loudly if that assumption is ever wrong.
 * ---------------------------------------------------------------------- */

esp_err_t status_handler(httpd_req_t *request) {
    (void)request;
    TEST_CHECK(false);
    return ESP_FAIL;
}

esp_err_t limits_handler(httpd_req_t *request) {
    (void)request;
    TEST_CHECK(false);
    return ESP_FAIL;
}

esp_err_t login_handler(httpd_req_t *request) {
    (void)request;
    TEST_CHECK(false);
    return ESP_FAIL;
}

esp_err_t logout_handler(httpd_req_t *request) {
    (void)request;
    TEST_CHECK(false);
    return ESP_FAIL;
}

esp_err_t blob_list_handler(httpd_req_t *request) {
    (void)request;
    TEST_CHECK(false);
    return ESP_FAIL;
}

esp_err_t blob_create_handler(httpd_req_t *request) {
    (void)request;
    TEST_CHECK(false);
    return ESP_FAIL;
}

esp_err_t blob_load_handler(httpd_req_t *request) {
    (void)request;
    TEST_CHECK(false);
    return ESP_FAIL;
}

esp_err_t blob_delete_handler(httpd_req_t *request) {
    (void)request;
    TEST_CHECK(false);
    return ESP_FAIL;
}

esp_err_t send_create_handler(httpd_req_t *request) {
    (void)request;
    TEST_CHECK(false);
    return ESP_FAIL;
}

esp_err_t send_get_handler(httpd_req_t *request) {
    (void)request;
    TEST_CHECK(false);
    return ESP_FAIL;
}

esp_err_t send_cancel_handler(httpd_req_t *request) {
    (void)request;
    TEST_CHECK(false);
    return ESP_FAIL;
}

esp_err_t api_handler(httpd_req_t *request) {
    (void)request;
    TEST_CHECK(false);
    return ESP_FAIL;
}

esp_err_t static_handler(httpd_req_t *request) {
    (void)request;
    TEST_CHECK(false);
    return ESP_FAIL;
}

esp_err_t setup_state_handler(httpd_req_t *request) {
    (void)request;
    TEST_CHECK(false);
    return ESP_FAIL;
}

esp_err_t setup_submit_handler(httpd_req_t *request) {
    (void)request;
    TEST_CHECK(false);
    return ESP_FAIL;
}

/* web_server_async_start()/web_server_async_stop() are the only two
 * non-httpd symbols web_server_lifecycle.c calls that are FreeRTOS-backed
 * (web_server_async.c creates a real queue/task/semaphore) and therefore not
 * host-linkable -- no first-party route table content depends on their
 * result beyond web_server_start()/web_server_stop()'s own success/failure,
 * so they simply succeed here. */
app_error_code_t web_server_async_start(void) {
    return APP_ERROR_NONE;
}

app_error_code_t web_server_async_stop(void) {
    return APP_ERROR_NONE;
}

/* ---------------------------------------------------------------------- *
 * server_configuration/server_lifecycle/setup_session are declared extern in
 * web_server.h/web_server_internal.h and normally defined by
 * web_server_common.c; this file defines them directly instead of linking
 * that translation unit in, which would drag in auth_session_validate() and
 * the JSON/body-auth adapter helpers this file has no use for.
 * ---------------------------------------------------------------------- */

web_server_config_t server_configuration;
web_adapter_lifecycle_t server_lifecycle;
app_v2_setup_session_t setup_session;

typedef struct {
    const char *uri;
    httpd_method_t method;
    fake_httpd_route_result_t expected_result;
    esp_err_t (*expected_handler)(httpd_req_t *);
} route_expectation_t;

static void reset_all(void) {
    fake_httpd_router_reset();
    server_configuration = (web_server_config_t){0};
    server_lifecycle = (web_adapter_lifecycle_t){0};
    setup_session = (app_v2_setup_session_t){0};
    g_factory_reset_state = FACTORY_RESET_STATE_NONE;
    g_factory_reset_state_read_result = APP_ERROR_NONE;
    g_reset_settings_restart_required = false;
    g_reset_gate_error_calls = 0U;
    g_reset_gate_status = 0U;
    g_reset_gate_code = APP_ERROR_NONE;
    g_reset_gate_message = NULL;
}

static web_server_config_t make_setup_config(void) {
    web_server_config_t configuration = {0};
    configuration.mode = WEB_SERVER_MODE_SETUP;
    configuration.login_enabled = false;
    (void)snprintf(configuration.setup_device_name, sizeof(configuration.setup_device_name), "%s",
                   "Matrix Test Keyboard");
    (void)snprintf(configuration.setup_code, sizeof(configuration.setup_code), "%s", "12345678");
    return configuration;
}

static web_server_config_t make_normal_config(void) {
    web_server_config_t configuration = {0};
    configuration.mode = WEB_SERVER_MODE_NORMAL;
    configuration.login_enabled = true;
    return configuration;
}

static void assert_route_matrix(const route_expectation_t *expectations, size_t count) {
    for (size_t index = 0U; index < count; ++index) {
        const route_expectation_t *expectation = &expectations[index];
        esp_err_t (*resolved_handler)(httpd_req_t *) = NULL;
        const fake_httpd_route_result_t result =
            fake_httpd_router_resolve(expectation->uri, expectation->method, &resolved_handler);
        TEST_CHECK_EQ_INT(expectation->expected_result, result);
        if (expectation->expected_result == FAKE_HTTPD_ROUTE_FOUND &&
            expectation->expected_handler != NULL) {
            TEST_CHECK(resolved_handler == expectation->expected_handler);
        }
    }
}

/* -------------------------------------------------------------------------
 * Unprovisioned (WEB_SERVER_MODE_SETUP)
 * ---------------------------------------------------------------------- */

static void test_unprovisioned_route_surface(void) {
    reset_all();
    web_server_config_t configuration = make_setup_config();
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, web_server_start(&configuration));
    TEST_CHECK(fake_httpd_router_started());
    /* setup_routes[] has exactly 3 entries -- see web_server_lifecycle.c. */
    TEST_CHECK_EQ_INT(3, fake_httpd_router_registered_count());

    static const route_expectation_t expectations[] = {
        {"/api/v1/setup", HTTP_GET, FAKE_HTTPD_ROUTE_FOUND, setup_state_handler},
        {"/api/v1/setup", HTTP_POST, FAKE_HTTPD_ROUTE_FOUND, setup_submit_handler},
        {"/", HTTP_GET, FAKE_HTTPD_ROUTE_FOUND, static_handler},
        {"/index.html", HTTP_GET, FAKE_HTTPD_ROUTE_FOUND, static_handler},
        /* /api/v1/setup is registered for GET and POST only -- DELETE/PUT
         * must resolve as "uri present, method not allowed" (405), not
         * "route absent" (404). */
        {"/api/v1/setup", HTTP_DELETE, FAKE_HTTPD_ROUTE_METHOD_NOT_ALLOWED, NULL},
        {"/api/v1/setup", HTTP_PUT, FAKE_HTTPD_ROUTE_METHOD_NOT_ALLOWED, NULL},
        /* setup_routes[]'s only entries are the two exact-match /api/v1/setup
         * registrations above and a single GET "/wildcard" catch-all -- there is no
         * dedicated /api/v1 wildcard in setup mode at all, unlike normal
         * mode's generic api_handler() registration. That means: every GET
         * request that is not exactly "/api/v1/setup" -- including a longer
         * /api/v1/setup/... path and every other /api/v1 route -- resolves to
         * static_handler() (the same catch-all "/", "/index.html", ... use),
         * not to "no route matched". static_handler() itself then 404s any
         * path with no corresponding file on the static filesystem (see
         * web_server_static.c's open_result == APP_ERROR_NOT_FOUND branch) --
         * so SPEC_V2 12.3's "every other /api/v1 route is unavailable while
         * unprovisioned" still holds (no API handler ever runs for these
         * paths), it is just implemented as a static-file 404 rather than a
         * routing-layer 404. Verifying the httpd_uri_t table alone (as
         * scripts/check-setup-route-isolation.sh does) cannot show this --
         * only actually registering the table and resolving requests against
         * it, as this test does, can. */
        {"/api/v1/setup/extra", HTTP_GET, FAKE_HTTPD_ROUTE_FOUND, static_handler},
        {"/api/v1/status", HTTP_GET, FAKE_HTTPD_ROUTE_FOUND, static_handler},
        {"/api/v1/limits", HTTP_GET, FAKE_HTTPD_ROUTE_FOUND, static_handler},
        {"/api/v1/auth/session", HTTP_GET, FAKE_HTTPD_ROUTE_FOUND, static_handler},
        {"/api/v1/blob", HTTP_GET, FAKE_HTTPD_ROUTE_FOUND, static_handler},
        {"/api/v1/blob/abc123", HTTP_GET, FAKE_HTTPD_ROUTE_FOUND, static_handler},
        {"/api/v1/send", HTTP_GET, FAKE_HTTPD_ROUTE_FOUND, static_handler},
        {"/api/v1/settings", HTTP_GET, FAKE_HTTPD_ROUTE_FOUND, static_handler},
        {"/api/v1/diagnostics", HTTP_GET, FAKE_HTTPD_ROUTE_FOUND, static_handler},
        /* Non-GET methods have no dedicated registration outside the two
         * exact /api/v1/setup entries above, but they are still not
         * "route absent": the GET-only wildcard's *uri* pattern ("/wildcard")
         * matches every path regardless of method, so ESP-IDF's real
         * dispatch algorithm (ported into fake_httpd_router.c -- ESP-IDF's
         * own httpd_find_uri_handler(), ported verbatim) records that as a
         * uri match with a method mismatch and reports 405 Method Not
         * Allowed, never 404 Not Found. Concretely: no request whose path
         * starts with "/" can ever resolve as FAKE_HTTPD_ROUTE_NOT_FOUND in
         * either provisioning mode's route table, because both tables end in
         * a GET catch-all whose uri pattern matches unconditionally -- a
         * genuinely absent route always reads as "wrong method", never
         * "wrong path". This is exactly the kind of characteristic that only
         * running the real dispatch algorithm against the real tables (as
         * this test does), not reading the array literal, can show. */
        {"/api/v1/auth/login", HTTP_POST, FAKE_HTTPD_ROUTE_METHOD_NOT_ALLOWED, NULL},
        {"/api/v1/auth/logout", HTTP_POST, FAKE_HTTPD_ROUTE_METHOD_NOT_ALLOWED, NULL},
        {"/api/v1/blob", HTTP_POST, FAKE_HTTPD_ROUTE_METHOD_NOT_ALLOWED, NULL},
        {"/api/v1/blob/abc123", HTTP_DELETE, FAKE_HTTPD_ROUTE_METHOD_NOT_ALLOWED, NULL},
        {"/api/v1/send", HTTP_POST, FAKE_HTTPD_ROUTE_METHOD_NOT_ALLOWED, NULL},
        {"/api/v1/send", HTTP_DELETE, FAKE_HTTPD_ROUTE_METHOD_NOT_ALLOWED, NULL},
        {"/api/v1/settings", HTTP_PUT, FAKE_HTTPD_ROUTE_METHOD_NOT_ALLOWED, NULL},
        {"/api/v1/settings/change-password", HTTP_POST, FAKE_HTTPD_ROUTE_METHOD_NOT_ALLOWED, NULL},
        {"/api/v1/device/restart", HTTP_POST, FAKE_HTTPD_ROUTE_METHOD_NOT_ALLOWED, NULL},
        {"/api/v1/device/reset-settings", HTTP_POST, FAKE_HTTPD_ROUTE_METHOD_NOT_ALLOWED, NULL},
        {"/api/v1/device/factory-reset", HTTP_POST, FAKE_HTTPD_ROUTE_METHOD_NOT_ALLOWED, NULL},
    };
    assert_route_matrix(expectations, sizeof(expectations) / sizeof(expectations[0]));

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, web_server_stop());
    TEST_CHECK(!fake_httpd_router_started());
    TEST_CHECK(!web_server_owns_resources());
}

/* -------------------------------------------------------------------------
 * Provisioned (WEB_SERVER_MODE_NORMAL)
 * ---------------------------------------------------------------------- */

static void test_provisioned_route_surface(void) {
    reset_all();
    web_server_config_t configuration = make_normal_config();
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, web_server_start(&configuration));
    TEST_CHECK(fake_httpd_router_started());
    /* normal_routes[] has exactly 16 entries -- see web_server_lifecycle.c. */
    TEST_CHECK_EQ_INT(16, fake_httpd_router_registered_count());

    static const route_expectation_t expectations[] = {
        {"/api/v1/status", HTTP_GET, FAKE_HTTPD_ROUTE_FOUND, NULL},
        {"/api/v1/limits", HTTP_GET, FAKE_HTTPD_ROUTE_FOUND, NULL},
        {"/api/v1/auth/login", HTTP_POST, FAKE_HTTPD_ROUTE_FOUND, NULL},
        {"/api/v1/auth/logout", HTTP_POST, FAKE_HTTPD_ROUTE_FOUND, NULL},
        {"/api/v1/blob", HTTP_GET, FAKE_HTTPD_ROUTE_FOUND, NULL},
        {"/api/v1/blob", HTTP_POST, FAKE_HTTPD_ROUTE_FOUND, NULL},
        {"/api/v1/blob/abc123", HTTP_GET, FAKE_HTTPD_ROUTE_FOUND, NULL},
        {"/api/v1/blob/abc123", HTTP_DELETE, FAKE_HTTPD_ROUTE_FOUND, NULL},
        {"/api/v1/send", HTTP_POST, FAKE_HTTPD_ROUTE_FOUND, NULL},
        {"/api/v1/send", HTTP_GET, FAKE_HTTPD_ROUTE_FOUND, NULL},
        {"/api/v1/send", HTTP_DELETE, FAKE_HTTPD_ROUTE_FOUND, NULL},
        /* Deliberately absent from normal_routes[] (see
         * web_server_lifecycle.c's comment) -- falls through to the generic
         * "/api/v1/" wildcard, answered by api_handler()'s WEB_API_ROUTE_SETUP
         * case rather than by setup_state_handler()/setup_submit_handler(). */
        {"/api/v1/setup", HTTP_GET, FAKE_HTTPD_ROUTE_FOUND, NULL},
        {"/api/v1/setup", HTTP_POST, FAKE_HTTPD_ROUTE_FOUND, NULL},
        /* Every other /api/v1 route not given its own dedicated registration
         * falls through to the same generic wildcard. */
        {"/api/v1/settings", HTTP_GET, FAKE_HTTPD_ROUTE_FOUND, NULL},
        {"/api/v1/settings", HTTP_PUT, FAKE_HTTPD_ROUTE_FOUND, NULL},
        {"/api/v1/auth/session", HTTP_GET, FAKE_HTTPD_ROUTE_FOUND, NULL},
        {"/api/v1/device/restart", HTTP_POST, FAKE_HTTPD_ROUTE_FOUND, NULL},
        {"/api/v1/device/reset-settings", HTTP_POST, FAKE_HTTPD_ROUTE_FOUND, NULL},
        {"/api/v1/device/factory-reset", HTTP_POST, FAKE_HTTPD_ROUTE_FOUND, NULL},
        {"/api/v1/settings/change-password", HTTP_POST, FAKE_HTTPD_ROUTE_FOUND, NULL},
        {"/api/v1/diagnostics", HTTP_GET, FAKE_HTTPD_ROUTE_FOUND, NULL},
        {"/", HTTP_GET, FAKE_HTTPD_ROUTE_FOUND, static_handler},
        {"/index.html", HTTP_GET, FAKE_HTTPD_ROUTE_FOUND, static_handler},
        /* No dedicated wildcard exists outside the "/api/v1/" prefix for
         * mutating methods -- but the trailing GET-only static fallback's uri
         * pattern still matches "/other" (it matches every path), so this
         * resolves as "uri found, method not allowed" (405), the same
         * always-405-never-404 characteristic documented in
         * test_unprovisioned_route_surface() above, not as route-absent. */
        {"/other", HTTP_POST, FAKE_HTTPD_ROUTE_METHOD_NOT_ALLOWED, NULL},
        {"/other", HTTP_PUT, FAKE_HTTPD_ROUTE_METHOD_NOT_ALLOWED, NULL},
        {"/other", HTTP_DELETE, FAKE_HTTPD_ROUTE_METHOD_NOT_ALLOWED, NULL},
    };
    assert_route_matrix(expectations, sizeof(expectations) / sizeof(expectations[0]));

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, web_server_stop());
    TEST_CHECK(!fake_httpd_router_started());
    TEST_CHECK(!web_server_owns_resources());
}

/* -------------------------------------------------------------------------
 * The provisioning transition itself: the same lifecycle object moving from
 * setup mode to normal mode, exactly as a real device does across the
 * setup-completion reboot, proving the route surface actually swaps rather
 * than accumulating or leaking entries between the two starts.
 * ---------------------------------------------------------------------- */

static void test_route_surface_swaps_across_provisioning_transition(void) {
    reset_all();
    web_server_config_t setup_configuration = make_setup_config();
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, web_server_start(&setup_configuration));

    esp_err_t (*resolved_handler)(httpd_req_t *) = NULL;
    TEST_CHECK_EQ_INT(FAKE_HTTPD_ROUTE_FOUND,
                      fake_httpd_router_resolve("/api/v1/setup", HTTP_GET, &resolved_handler));
    TEST_CHECK(resolved_handler == setup_state_handler);
    /* GET /api/v1/status while unprovisioned is not wired to the real status
     * API at all -- it falls through to the same static-file catch-all as
     * "/"/"/index.html" (see test_unprovisioned_route_surface()'s header
     * comment for why), which is the point of contrast this test exists to
     * make: the identical uri/method resolves to a completely different
     * handler depending on provisioning state. */
    TEST_CHECK_EQ_INT(FAKE_HTTPD_ROUTE_FOUND,
                      fake_httpd_router_resolve("/api/v1/status", HTTP_GET, &resolved_handler));
    TEST_CHECK(resolved_handler == static_handler);

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, web_server_stop());
    /* fake_httpd_router_reset() is not called here on purpose: httpd_stop()
     * (called by web_server_stop()) is what a real device relies on to clear
     * the prior registration set, so this test leaves that to the real
     * production code path, not to test setup between cases. */
    TEST_CHECK(!fake_httpd_router_started());
    TEST_CHECK_EQ_INT(0, fake_httpd_router_registered_count());

    server_configuration = (web_server_config_t){0};
    server_lifecycle = (web_adapter_lifecycle_t){0};
    setup_session = (app_v2_setup_session_t){0};
    web_server_config_t normal_configuration = make_normal_config();
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, web_server_start(&normal_configuration));

    resolved_handler = NULL;
    TEST_CHECK_EQ_INT(FAKE_HTTPD_ROUTE_FOUND,
                      fake_httpd_router_resolve("/api/v1/status", HTTP_GET, &resolved_handler));
    esp_err_t (*normal_status_handler)(httpd_req_t *) = resolved_handler;
    TEST_CHECK(normal_status_handler != NULL);
    TEST_CHECK(normal_status_handler != static_handler);
    /* Setup mode's dedicated registration is gone -- provisioned mode
     * answers the same path through the reset-guarded generic API wildcard.
     * The guard wrapper is intentionally translation-unit private, so prove
     * the route moved off both setup/static handlers and onto a distinct
     * normal-API handler rather than coupling this test to a private symbol. */
    TEST_CHECK_EQ_INT(FAKE_HTTPD_ROUTE_FOUND,
                      fake_httpd_router_resolve("/api/v1/setup", HTTP_GET, &resolved_handler));
    TEST_CHECK(resolved_handler != NULL);
    TEST_CHECK(resolved_handler != setup_state_handler);
    TEST_CHECK(resolved_handler != static_handler);
    TEST_CHECK(resolved_handler != normal_status_handler);

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, web_server_stop());
}

typedef struct {
    const char *uri;
    httpd_method_t method;
} guarded_route_t;

static void assert_pending_route_is_denied(const guarded_route_t *route) {
    esp_err_t (*resolved_handler)(httpd_req_t *) = NULL;
    TEST_CHECK_EQ_INT(FAKE_HTTPD_ROUTE_FOUND,
                      fake_httpd_router_resolve(route->uri, route->method, &resolved_handler));
    TEST_CHECK(resolved_handler != NULL);

    httpd_req_t request = {0};
    const unsigned int before = g_reset_gate_error_calls;
    TEST_CHECK_EQ_INT(ESP_OK, resolved_handler(&request));
    TEST_CHECK_EQ_U64((uint64_t)before + 1U, g_reset_gate_error_calls);
    TEST_CHECK_EQ_U64(503U, g_reset_gate_status);
    TEST_CHECK_APP_ERROR(APP_ERROR_RESET_RECOVERY_REQUIRED, g_reset_gate_code);
    TEST_CHECK_EQ_STRING("factory reset recovery in progress", g_reset_gate_message);
}

static void test_pending_factory_reset_denies_every_normal_api_route(void) {
    reset_all();
    g_factory_reset_state = FACTORY_RESET_STATE_PENDING;
    web_server_config_t configuration = make_normal_config();
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, web_server_start(&configuration));

    static const guarded_route_t routes[] = {
        {"/api/v1/status", HTTP_GET},
        {"/api/v1/limits", HTTP_GET},
        {"/api/v1/auth/login", HTTP_POST},
        {"/api/v1/auth/logout", HTTP_POST},
        {"/api/v1/blob", HTTP_GET},
        {"/api/v1/blob", HTTP_POST},
        {"/api/v1/blob/00000000000000000001", HTTP_GET},
        {"/api/v1/blob/00000000000000000001", HTTP_DELETE},
        {"/api/v1/send", HTTP_POST},
        {"/api/v1/send", HTTP_GET},
        {"/api/v1/send", HTTP_DELETE},
        {"/api/v1/diagnostics", HTTP_GET},
        {"/api/v1/settings/change-password", HTTP_POST},
        {"/api/v1/settings", HTTP_PUT},
        {"/api/v1/unknown", HTTP_DELETE},
    };
    for (size_t index = 0U; index < sizeof(routes) / sizeof(routes[0]); ++index) {
        assert_pending_route_is_denied(&routes[index]);
    }

    esp_err_t (*static_route)(httpd_req_t *) = NULL;
    TEST_CHECK_EQ_INT(FAKE_HTTPD_ROUTE_FOUND,
                      fake_httpd_router_resolve("/", HTTP_GET, &static_route));
    TEST_CHECK(static_route == static_handler);

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, web_server_stop());
}

static void test_reset_settings_restart_required_denies_normal_api(void) {
    reset_all();
    g_reset_settings_restart_required = true;
    web_server_config_t configuration = make_normal_config();
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, web_server_start(&configuration));

    esp_err_t (*resolved_handler)(httpd_req_t *) = NULL;
    TEST_CHECK_EQ_INT(
        FAKE_HTTPD_ROUTE_FOUND,
        fake_httpd_router_resolve("/api/v1/auth/login", HTTP_POST, &resolved_handler));
    httpd_req_t request = {0};
    TEST_CHECK_EQ_INT(ESP_OK, resolved_handler(&request));
    TEST_CHECK_EQ_U64(503U, g_reset_gate_status);
    TEST_CHECK_APP_ERROR(APP_ERROR_RESET_SETTINGS_INCOMPLETE, g_reset_gate_code);
    TEST_CHECK_EQ_STRING("reset-settings restart required", g_reset_gate_message);

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, web_server_stop());
}

static void test_reset_journal_read_failure_denies_normal_api(void) {
    reset_all();
    g_factory_reset_state_read_result = APP_ERROR_IO;
    web_server_config_t configuration = make_normal_config();
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, web_server_start(&configuration));

    esp_err_t (*resolved_handler)(httpd_req_t *) = NULL;
    TEST_CHECK_EQ_INT(
        FAKE_HTTPD_ROUTE_FOUND,
        fake_httpd_router_resolve("/api/v1/auth/login", HTTP_POST, &resolved_handler));
    httpd_req_t request = {0};
    TEST_CHECK_EQ_INT(ESP_OK, resolved_handler(&request));
    TEST_CHECK_EQ_U64(503U, g_reset_gate_status);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, g_reset_gate_code);
    TEST_CHECK_EQ_STRING("factory reset state unavailable", g_reset_gate_message);

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, web_server_stop());
}

int main(void) {
    test_unprovisioned_route_surface();
    test_provisioned_route_surface();
    test_pending_factory_reset_denies_every_normal_api_route();
    test_reset_settings_restart_required_denies_normal_api();
    test_reset_journal_read_failure_denies_normal_api();
    test_route_surface_swaps_across_provisioning_transition();

    puts("web server lifecycle route matrix tests passed");
    return 0;
}
