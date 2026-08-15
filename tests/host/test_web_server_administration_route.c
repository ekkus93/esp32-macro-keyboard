/* Live end-to-end HTTP tests for the administration route group (TODO_V2
 * V2-057's remaining items, see
 * docs/implementation-v2/V2_057_FULL_HTTP_CONTRACT_MATRIX_2026-08-09.md's
 * "What remains open" section): GET /api/v1/auth/session,
 * POST /api/v1/device/restart, GET/PUT /api/v1/settings,
 * POST /api/v1/settings/change-password,
 * POST /api/v1/device/reset-settings, POST /api/v1/device/factory-reset,
 * GET/POST /api/v1/setup (provisioned-mode conflict), and
 * GET /api/v1/diagnostics.
 *
 * Unlike test_web_api_administration.c (which calls
 * web_api_handle_administration() directly with a hand-built web_api_call_t)
 * and test_web_request_policy.c (which calls web_request_policy_evaluate()
 * directly), this drives the real api_handler() -- web_server_api.c's httpd
 * URI handler for the generic "/api/v1" wildcard -- against a fake
 * esp_http_server.h (fakes/esp_http_server_stub, fakes/fake_httpd.c), the
 * same technique test_web_server_blob.c/test_web_server_send_route.c use.
 * That proves a request actually flows through
 * method_from_request() -> web_api_parse_path() ->
 * web_request_policy_evaluate() -> web_api_dispatch() ->
 * web_api_handle_administration() -> the exact same pipeline a real device
 * uses, not just the last step of it.
 *
 * server_configuration.require_physical_confirmation is left false for
 * every test below. With it false, web_api_request_requires_worker()
 * (web_server_api.c) returns false for every route in this group, so
 * api_handler() always takes the synchronous
 * web_api_handle_call() path under test rather than
 * web_server_async_dispatch()'s FreeRTOS worker queue, which is not
 * host-linkable. The confirmation-required=true path (which would route
 * restart/reset-settings/factory-reset/change-password through the async
 * worker and device_controls_wait_for_confirmation()) is therefore NOT
 * exercised here -- that is a distinct, FreeRTOS-dependent code path
 * (web_server_async.c) out of this track's scope; policy_confirm()'s
 * decision of *whether* confirmation is required for a route is already
 * covered by test_web_request_policy.c and test_web_api_core.c.
 *
 * diagnostics (GET /api/v1/diagnostics) is now included too: the prior
 * track's report framed the eight subsystem-health snapshot functions
 * web_diagnostics_handle()'s collect_diagnostics() (web_server_diagnostics.c)
 * calls as needing "host stand-ins ... none of which exist yet", but reading
 * each one's own module comment shows six of them
 * (app_lifecycle_health_snapshot(), storage_health_snapshot(),
 * auth_health_snapshot(), usb_health_snapshot(), executor_health_snapshot(),
 * http_health_snapshot()) are already portable C with no ESP-IDF dependency
 * -- they link in real, unfaked, the same way this file's own
 * device_controls_logic.c-backed device_controls_health_derive_state() and
 * wifi_ap_state.c-backed wifi_ap_health_derive_state() do. Only two
 * (device_controls_get_health(), wifi_ap_get_status()) live in
 * ESP-IDF/FreeRTOS-bound files and need fakes, both provided below using the
 * same narrow-substitution technique already used for this file's other
 * device_controls.h entry points. web_diagnostics_handle() itself, and the
 * esp_heap_caps.h/esp_system.h entry points collect_diagnostics() calls
 * (esp_reset_reason(), esp_get_free_heap_size(), esp_get_minimum_free_heap_size(),
 * heap_caps_get_largest_free_block()), are real too -- see
 * fakes/esp_idf_misc_stub/esp_system.h and the new
 * fakes/esp_idf_misc_stub/esp_heap_caps.h for those two headers' stand-ins.
 * Diagnostics' own JSON composition (schema, exact status codes,
 * secret-sentinel absence, bounded-output/corrupt-input handling) keeps its
 * existing deep, dedicated coverage in
 * test_web_server_adapter_diagnostics_json.inc; the tests added here prove
 * only that a live request reaches that composition through the same
 * policy/dispatch pipeline every other route in this file already proves,
 * plus one representative backend-failure mapping. */

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
#include "esp_app_desc.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "executor_health.h"
#include "factory_reset_state.h"
#include "fake_httpd.h"
#include "http_health.h"
#include "macro_executor.h"
#include "storage.h"
#include "storage_blob.h"
#include "test_assert.h"
#include "test_examples_fixture.h"
#include "usb_keyboard.h"
#include "web_api_core.h"
#include "web_api_response.h"
#include "web_diagnostics.h"
#include "web_server.h"
#include "web_server_internal.h"
#include "web_server_password_record.h"
#include "wifi_ap.h"
/* The fixture defines the fakes and helpers the suites below depend on, so it
 * must be included first; the blank line keeps it in its own include block so
 * include sorting cannot reorder it after the alphabetized suites. */
#include "admin_route_test_fixture.inc"

#include "admin_route_diagnostics_tests.inc"
#include "admin_route_reset_tests.inc"
#include "admin_route_session_tests.inc"
#include "admin_route_settings_tests.inc"

int main(void) {
    test_session_get_valid();
    test_session_get_unauthorized_without_cookie();
    test_session_get_unauthorized_expired_session();

    test_restart_post_valid();
    test_restart_post_unauthorized_without_cookie();

    test_settings_get_valid();
    test_settings_get_rejects_oversized_request_id();
    test_settings_put_valid();
    test_settings_put_valid_matches_example();
    test_settings_put_unauthorized_expired_session();

    test_change_password_post_valid();
    test_change_password_post_committed_invalidation_failure_is_explicit();
    test_change_password_post_busy_before_commit_keeps_cookie();
    test_change_password_post_incorrect_current_password();

    test_reset_settings_post_valid();
    test_reset_settings_post_wrong_confirmation();

    test_factory_reset_post_valid();
    test_factory_reset_post_precommit_failure_is_not_accepted();
    test_factory_reset_post_cleanup_failure_stays_accepted_for_recovery();
    test_factory_reset_post_incorrect_password();

    test_setup_get_not_found_when_provisioned();
    test_setup_post_conflict_when_provisioned();

    test_diagnostics_get_valid();
    test_diagnostics_get_pending_reset_reports_recovery();
    test_diagnostics_release_fault_degrades_executor_subsystem();
    test_diagnostics_async_failure_degrades_existing_http_subsystem();
    test_diagnostics_get_unauthorized_without_cookie();
    test_diagnostics_get_unauthorized_expired_session();
    test_diagnostics_get_blob_scan_failure_maps_to_503();

    puts("web server administration route tests passed");
    return 0;
}
