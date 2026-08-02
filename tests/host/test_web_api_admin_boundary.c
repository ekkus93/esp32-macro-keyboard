#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "storage.h"
#include "storage_incidents.h"
#include "storage_package.h"
#include "storage_repository.h"
#include "test_assert.h"
#include "web_api_admin_boundary.h"
#include "web_api_core.h"
#include "web_api_handlers.h"
#include "web_api_response.h"

static const char BACKUP_DOCUMENT[] =
    "{\"schema_version\":1,\"package_type\":\"backup\",\"sets\":[],"
    "\"macros\":[]}";

static storage_mount_state_t mount_state;
static app_error_code_t backup_result;
static storage_package_failure_t backup_failure;
static app_error_code_t restore_result;
static size_t measured_user_bytes;
static app_error_code_t measure_result;
static storage_restore_report_t restore_report;
static char restore_body[256U];
static size_t restore_body_length;

storage_mount_state_t storage_mount_state(void) {
    return mount_state;
}

app_error_code_t storage_package_export_backup_detail(char **out_data, size_t *out_length,
                                                      storage_package_failure_t *out_failure,
                                                      storage_package_skip_report_t *out_skipped) {
    (void)out_skipped;
    if (out_failure != NULL) {
        memset(out_failure, 0, sizeof(*out_failure));
    }
    if (out_data == NULL || out_length == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_data = NULL;
    *out_length = 0U;
    if (backup_result != APP_ERROR_NONE) {
        if (out_failure != NULL) {
            *out_failure = backup_failure;
        }
        return backup_result;
    }
    const size_t length = sizeof(BACKUP_DOCUMENT) - 1U;
    char *copy = malloc(length + 1U);
    if (copy == NULL) {
        return APP_ERROR_INTERNAL;
    }
    memcpy(copy, BACKUP_DOCUMENT, length + 1U);
    *out_data = copy;
    *out_length = length;
    return APP_ERROR_NONE;
}

app_error_code_t storage_package_restore_backup(const char *data, size_t length,
                                                storage_restore_report_t *out_report) {
    restore_body_length = 0U;
    restore_body[0] = '\0';
    if (data != NULL && length < sizeof(restore_body)) {
        memcpy(restore_body, data, length);
        restore_body[length] = '\0';
        restore_body_length = length;
    }
    if (out_report != NULL) {
        *out_report = restore_report;
    }
    return restore_result;
}

/* The admin boundary reads measured storage use for the diagnostics snapshot
   (SPEC 10.7); this stands in for the repository. */
app_error_code_t storage_repository_measure_user_data(const app_uuid_t *exclude_set_id,
                                                      size_t *out_bytes) {
    (void)exclude_set_id;
    if (out_bytes == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_bytes = measured_user_bytes;
    return measure_result;
}

void storage_package_free(char *data) {
    free(data);
}

static web_api_call_t call_for(web_api_route_t route, web_api_method_t method, const char *body) {
    return (web_api_call_t){
        .method = method,
        .path = {.route = route},
        .body = body,
        .body_length = body == NULL ? 0U : strlen(body),
    };
}

static void reset_fixture(void) {
    mount_state = (storage_mount_state_t){
        .web_mounted = true,
        .data_mounted = true,
    };
    backup_result = APP_ERROR_NONE;
    memset(&backup_failure, 0, sizeof(backup_failure));
    restore_result = APP_ERROR_NONE;
    measured_user_bytes = 0U;
    measure_result = APP_ERROR_NONE;
    memset(&restore_report, 0, sizeof(restore_report));
    restore_body[0] = '\0';
    restore_body_length = 0U;
}

static void assert_absent(const char *body, const char *secret) {
    TEST_CHECK(body != NULL);
    TEST_CHECK(strstr(body, secret) == NULL);
}

static void test_storage_snapshot_is_redacted_and_not_verified(void) {
    reset_fixture();
    const web_api_call_t call =
        call_for(WEB_API_ROUTE_DIAGNOSTICS_STORAGE, WEB_API_METHOD_GET, NULL);
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_admin_boundary_handle(&call, &response));
    TEST_CHECK_EQ_U64(200U, response.status);
    TEST_CHECK(strstr(response.body, "\"verified\":false") != NULL);
    TEST_CHECK(strstr(response.body, "\"webMounted\":true") != NULL);
    TEST_CHECK(strstr(response.body, "\"dataMounted\":true") != NULL);
    assert_absent(response.body, "password");
    assert_absent(response.body, "passphrase");
    assert_absent(response.body, "token");
    assert_absent(response.body, "csrf");
    assert_absent(response.body, "source");
    assert_absent(response.body, "salt");
    assert_absent(response.body, "hash");
    web_api_response_free(&response);
}

static void test_storage_check_never_reports_false_success(void) {
    reset_fixture();
    const web_api_call_t call =
        call_for(WEB_API_ROUTE_DIAGNOSTICS_STORAGE_CHECK, WEB_API_METHOD_POST, NULL);
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_admin_boundary_handle(&call, &response));
    TEST_CHECK_EQ_U64(503U, response.status);
    TEST_CHECK(strstr(response.body, "Phase 19 diagnostics service") != NULL);
    TEST_CHECK(strstr(response.body, "\"ok\":false") != NULL);
    TEST_CHECK(strstr(response.body, "\"verified\":true") == NULL);
    web_api_response_free(&response);
}

static void test_backup_returns_raw_validated_package(void) {
    reset_fixture();
    const web_api_call_t call = call_for(WEB_API_ROUTE_BACKUP, WEB_API_METHOD_GET, NULL);
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_admin_boundary_handle(&call, &response));
    TEST_CHECK_EQ_U64(200U, response.status);
    TEST_CHECK_EQ_U64(sizeof(BACKUP_DOCUMENT) - 1U, response.body_length);
    TEST_CHECK_EQ_STRING(BACKUP_DOCUMENT, response.body);
    TEST_CHECK(strstr(response.body, "\"ok\":true") == NULL);
    web_api_response_free(&response);
}

static void test_backup_failure_is_visible(void) {
    reset_fixture();
    backup_result = APP_ERROR_STORAGE_FULL;
    const web_api_call_t call = call_for(WEB_API_ROUTE_BACKUP, WEB_API_METHOD_GET, NULL);
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_admin_boundary_handle(&call, &response));
    TEST_CHECK_EQ_U64(507U, response.status);
    TEST_CHECK(strstr(response.body, "backup unavailable") != NULL);
    TEST_CHECK(strstr(response.body, "\"ok\":false") != NULL);
    web_api_response_free(&response);
}

/* The whole point of the failure detail: the response must say which object
 * blocked the backup, or the user has nothing to act on. */
static void test_backup_failure_names_the_offending_macro(void) {
    reset_fixture();
    backup_result = APP_ERROR_MACRO_SYNTAX;
    backup_failure.kind = STORAGE_PACKAGE_OBJECT_MACRO;
    backup_failure.has_object_id = true;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_parse("71cc0195-1111-4111-8111-111111111111",
                                                        &backup_failure.object_id));
    backup_failure.has_set_id = true;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_parse("15c7daee-2222-4222-8222-222222222222",
                                                        &backup_failure.set_id));
    const web_api_call_t call = call_for(WEB_API_ROUTE_BACKUP, WEB_API_METHOD_GET, NULL);
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_admin_boundary_handle(&call, &response));
    TEST_CHECK_EQ_U64(422U, response.status);
    TEST_CHECK(strstr(response.body, "71cc0195-1111-4111-8111-111111111111") != NULL);
    TEST_CHECK(strstr(response.body, "15c7daee-2222-4222-8222-222222222222") != NULL);
    TEST_CHECK(strstr(response.body, "macro") != NULL);
    web_api_response_free(&response);
}

/* When nothing could be identified the message must stay the plain one rather
 * than claim a partial identity. */
static void test_backup_failure_without_detail_stays_plain(void) {
    reset_fixture();
    backup_result = APP_ERROR_STORAGE_FULL;
    const web_api_call_t call = call_for(WEB_API_ROUTE_BACKUP, WEB_API_METHOD_GET, NULL);
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_admin_boundary_handle(&call, &response));
    TEST_CHECK(strstr(response.body, "backup unavailable") != NULL);
    TEST_CHECK(strstr(response.body, "could not be read") == NULL);
    web_api_response_free(&response);
}

/* SPEC 10.7: the client must be able to see how much room is left before it
 * tries a write, so the snapshot publishes measured use alongside the budget. */
static void test_storage_snapshot_publishes_remaining_space(void) {
    reset_fixture();
    measured_user_bytes = 100U * 1024U;
    const web_api_call_t call =
        call_for(WEB_API_ROUTE_DIAGNOSTICS_STORAGE, WEB_API_METHOD_GET, NULL);
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_admin_boundary_handle(&call, &response));
    TEST_CHECK_EQ_U64(200U, response.status);
    TEST_CHECK(strstr(response.body, "\"usedBytes\":102400") != NULL);
    TEST_CHECK(strstr(response.body, "\"totalBytes\":491520") != NULL);
    TEST_CHECK(strstr(response.body, "\"remainingBytes\":389120") != NULL);
    /* SPEC 20.3: what boot recovery cleaned up and what was thrown away. */
    TEST_CHECK(strstr(response.body, "\"temporariesRemovedAtBoot\":0") != NULL);
    TEST_CHECK(strstr(response.body, "\"discardedObjects\":[]") != NULL);
    web_api_response_free(&response);
}

/* A discarded object is reported with its path AND its error (SPEC 13.6). */
static void test_storage_snapshot_reports_discarded_objects(void) {
    reset_fixture();
    storage_incidents_reset();
    storage_incident_record_discard("/data/sets/broken.json", APP_ERROR_STORAGE_CORRUPT);
    const web_api_call_t call =
        call_for(WEB_API_ROUTE_DIAGNOSTICS_STORAGE, WEB_API_METHOD_GET, NULL);
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_admin_boundary_handle(&call, &response));
    TEST_CHECK(strstr(response.body, "\"discardedObjectCount\":1") != NULL);
    TEST_CHECK(strstr(response.body, "/data/sets/broken.json") != NULL);
    TEST_CHECK(strstr(response.body, app_error_code_string(APP_ERROR_STORAGE_CORRUPT)) != NULL);
    web_api_response_free(&response);
    storage_incidents_reset();
}

/* Remaining space is clamped at zero rather than underflowing into a huge
 * number when measured use somehow exceeds the budget. */
static void test_storage_snapshot_clamps_remaining_at_zero(void) {
    reset_fixture();
    measured_user_bytes = 600U * 1024U;
    const web_api_call_t call =
        call_for(WEB_API_ROUTE_DIAGNOSTICS_STORAGE, WEB_API_METHOD_GET, NULL);
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_admin_boundary_handle(&call, &response));
    TEST_CHECK(strstr(response.body, "\"remainingBytes\":0") != NULL);
    web_api_response_free(&response);
}

static void test_restore_delegates_complete_package(void) {
    reset_fixture();
    const web_api_call_t call =
        call_for(WEB_API_ROUTE_RESTORE, WEB_API_METHOD_POST, BACKUP_DOCUMENT);
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_admin_boundary_handle(&call, &response));
    TEST_CHECK_EQ_U64(200U, response.status);
    TEST_CHECK_EQ_U64(sizeof(BACKUP_DOCUMENT) - 1U, restore_body_length);
    TEST_CHECK_EQ_STRING(BACKUP_DOCUMENT, restore_body);
    TEST_CHECK(strstr(response.body, "\"restored\":true") != NULL);
    TEST_CHECK(strstr(response.body, "\"reloadRequired\":true") != NULL);
    web_api_response_free(&response);
}

/* A restore that never reached the write loop has no per-set outcomes, so it is
 * reported as an ordinary error. */
static void test_restore_failure_is_visible(void) {
    reset_fixture();
    restore_result = APP_ERROR_INVALID_ARGUMENT;
    const web_api_call_t call = call_for(WEB_API_ROUTE_RESTORE, WEB_API_METHOD_POST, "{}");
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_admin_boundary_handle(&call, &response));
    TEST_CHECK_EQ_U64(422U, response.status);
    TEST_CHECK(strstr(response.body, "restore failed") != NULL);
    TEST_CHECK(strstr(response.body, "\"ok\":false") != NULL);
    web_api_response_free(&response);
}

/* SPEC 13.5, 17: a partial restore enumerates which sets landed and which did
 * not, and MUST NOT be reported as 200. */
static void test_partial_restore_reports_per_set_outcomes(void) {
    reset_fixture();
    restore_report.count = 2U;
    restore_report.written = 1U;
    restore_report.failed = 1U;
    restore_report.first_failure = APP_ERROR_STORAGE_FULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_parse("11111111-1111-4111-8111-111111111111",
                                                        &restore_report.items[0].set_id));
    restore_report.items[0].result = APP_ERROR_NONE;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_parse("22222222-2222-4222-8222-222222222222",
                                                        &restore_report.items[1].set_id));
    restore_report.items[1].result = APP_ERROR_STORAGE_FULL;
    restore_result = APP_ERROR_STORAGE_FULL;

    const web_api_call_t call =
        call_for(WEB_API_ROUTE_RESTORE, WEB_API_METHOD_POST, BACKUP_DOCUMENT);
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_admin_boundary_handle(&call, &response));

    /* Not 200, and the status reflects the actual fault rather than a generic
     * 500. */
    TEST_CHECK_EQ_U64(507U, response.status);
    TEST_CHECK(strstr(response.body, "\"ok\":false") != NULL);
    TEST_CHECK(strstr(response.body, "restore did not write every set") != NULL);
    TEST_CHECK(strstr(response.body, "\"restored\":false") != NULL);
    TEST_CHECK(strstr(response.body, "\"setsRestored\":1") != NULL);
    TEST_CHECK(strstr(response.body, "\"setsFailed\":1") != NULL);
    TEST_CHECK(strstr(response.body, "11111111-1111-4111-8111-111111111111") != NULL);
    TEST_CHECK(strstr(response.body, "22222222-2222-4222-8222-222222222222") != NULL);
    web_api_response_free(&response);
}

/* A complete restore still enumerates the sets, so a client never has to guess
 * whether an empty list means "none" or "not reported". */
static void test_complete_restore_enumerates_sets(void) {
    reset_fixture();
    restore_report.count = 1U;
    restore_report.written = 1U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_parse("33333333-3333-4333-8333-333333333333",
                                                        &restore_report.items[0].set_id));
    const web_api_call_t call =
        call_for(WEB_API_ROUTE_RESTORE, WEB_API_METHOD_POST, BACKUP_DOCUMENT);
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_admin_boundary_handle(&call, &response));
    TEST_CHECK_EQ_U64(200U, response.status);
    TEST_CHECK(strstr(response.body, "\"setsFailed\":0") != NULL);
    TEST_CHECK(strstr(response.body, "33333333-3333-4333-8333-333333333333") != NULL);
    web_api_response_free(&response);
}

static void test_remaining_package_boundaries_are_explicit(void) {
    static const web_api_route_t routes[] = {
        WEB_API_ROUTE_SET_EXPORT,
        WEB_API_ROUTE_SET_IMPORT,
    };
    for (size_t index = 0U; index < sizeof(routes) / sizeof(routes[0]); ++index) {
        const web_api_call_t call = call_for(routes[index], WEB_API_METHOD_GET, NULL);
        web_api_response_t response = {0};
        TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_admin_boundary_handle(&call, &response));
        TEST_CHECK_EQ_U64(503U, response.status);
        TEST_CHECK(strstr(response.body, "Phase 18 transaction service") != NULL);
        TEST_CHECK(strstr(response.body, "\"ok\":false") != NULL);
        web_api_response_free(&response);
    }
}

static void test_invalid_boundary_inputs(void) {
    const web_api_call_t call =
        call_for(WEB_API_ROUTE_DIAGNOSTICS_STORAGE, WEB_API_METHOD_GET, NULL);
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_admin_boundary_handle(NULL, &response));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, web_api_admin_boundary_handle(&call, NULL));
    const web_api_call_t unknown = call_for(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_GET, NULL);
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, web_api_admin_boundary_handle(&unknown, &response));
    TEST_CHECK(response.body == NULL);
}

int main(void) {
    test_storage_snapshot_is_redacted_and_not_verified();
    test_storage_check_never_reports_false_success();
    test_backup_returns_raw_validated_package();
    test_backup_failure_is_visible();
    test_backup_failure_names_the_offending_macro();
    test_backup_failure_without_detail_stays_plain();
    test_storage_snapshot_publishes_remaining_space();
    test_storage_snapshot_clamps_remaining_at_zero();
    test_storage_snapshot_reports_discarded_objects();
    test_restore_delegates_complete_package();
    test_restore_failure_is_visible();
    test_partial_restore_reports_per_set_outcomes();
    test_complete_restore_enumerates_sets();
    test_remaining_package_boundaries_are_explicit();
    test_invalid_boundary_inputs();
    return 0;
}
