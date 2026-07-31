#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "storage.h"
#include "storage_package.h"
#include "test_assert.h"
#include "web_api_admin_boundary.h"
#include "web_api_core.h"
#include "web_api_handlers.h"
#include "web_api_response.h"

static const char BACKUP_DOCUMENT[] =
    "{\"schema_version\":1,\"package_type\":\"backup\",\"sets\":[],"
    "\"macros\":[],\"global_macros\":[],\"procedures\":[],\"progress\":[]}";

static storage_mount_state_t mount_state;
static storage_quarantine_list_t quarantine_list;
static app_error_code_t quarantine_result;
static app_error_code_t backup_result;
static app_error_code_t restore_result;
static bool backup_include_progress;
static char restore_body[256U];
static size_t restore_body_length;

storage_mount_state_t storage_mount_state(void) {
    return mount_state;
}

app_error_code_t storage_quarantine_list(storage_quarantine_list_t *out_list) {
    if (out_list == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_list = quarantine_list;
    return quarantine_result;
}

app_error_code_t storage_package_export_backup(bool include_progress, char **out_data,
                                                size_t *out_length) {
    backup_include_progress = include_progress;
    if (out_data == NULL || out_length == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_data = NULL;
    *out_length = 0U;
    if (backup_result != APP_ERROR_NONE) {
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

app_error_code_t storage_package_restore_backup(const char *data, size_t length) {
    restore_body_length = 0U;
    restore_body[0] = '\0';
    if (data != NULL && length < sizeof(restore_body)) {
        memcpy(restore_body, data, length);
        restore_body[length] = '\0';
        restore_body_length = length;
    }
    return restore_result;
}

void storage_package_free(char *data) {
    free(data);
}

static web_api_call_t call_for(web_api_route_t route, web_api_method_t method,
                               const char *body) {
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
    quarantine_list = (storage_quarantine_list_t){
        .count = 2U,
        .damaged_count = 1U,
    };
    quarantine_result = APP_ERROR_NONE;
    backup_result = APP_ERROR_NONE;
    restore_result = APP_ERROR_NONE;
    backup_include_progress = false;
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
    TEST_CHECK(strstr(response.body, "\"quarantineCount\":2") != NULL);
    TEST_CHECK(strstr(response.body, "\"damagedQuarantineCount\":1") != NULL);
    assert_absent(response.body, "password");
    assert_absent(response.body, "passphrase");
    assert_absent(response.body, "token");
    assert_absent(response.body, "csrf");
    assert_absent(response.body, "source");
    assert_absent(response.body, "salt");
    assert_absent(response.body, "hash");
    web_api_response_free(&response);
}

static void test_storage_snapshot_failure_is_visible(void) {
    reset_fixture();
    quarantine_result = APP_ERROR_IO;
    const web_api_call_t call =
        call_for(WEB_API_ROUTE_DIAGNOSTICS_STORAGE, WEB_API_METHOD_GET, NULL);
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_admin_boundary_handle(&call, &response));
    TEST_CHECK_EQ_U64(500U, response.status);
    TEST_CHECK(strstr(response.body, "storage health unavailable") != NULL);
    TEST_CHECK(strstr(response.body, "\"ok\":false") != NULL);
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
    TEST_CHECK(backup_include_progress);
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

static void test_restore_failure_is_visible(void) {
    reset_fixture();
    restore_result = APP_ERROR_INVALID_ARGUMENT;
    const web_api_call_t call =
        call_for(WEB_API_ROUTE_RESTORE, WEB_API_METHOD_POST, "{}");
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_admin_boundary_handle(&call, &response));
    TEST_CHECK_EQ_U64(422U, response.status);
    TEST_CHECK(strstr(response.body, "restore failed") != NULL);
    TEST_CHECK(strstr(response.body, "\"ok\":false") != NULL);
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
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_admin_boundary_handle(&call, NULL));
    const web_api_call_t unknown = call_for(WEB_API_ROUTE_SETTINGS, WEB_API_METHOD_GET, NULL);
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND,
                         web_api_admin_boundary_handle(&unknown, &response));
    TEST_CHECK(response.body == NULL);
}

int main(void) {
    test_storage_snapshot_is_redacted_and_not_verified();
    test_storage_snapshot_failure_is_visible();
    test_storage_check_never_reports_false_success();
    test_backup_returns_raw_validated_package();
    test_backup_failure_is_visible();
    test_restore_delegates_complete_package();
    test_restore_failure_is_visible();
    test_remaining_package_boundaries_are_explicit();
    test_invalid_boundary_inputs();
    return 0;
}
