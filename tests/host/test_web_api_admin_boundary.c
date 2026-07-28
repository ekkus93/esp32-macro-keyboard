#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "app_error.h"
#include "storage.h"
#include "test_assert.h"
#include "web_api_admin_boundary.h"
#include "web_api_core.h"
#include "web_api_response.h"

static storage_mount_state_t mount_state;
static storage_quarantine_list_t quarantine_list;
static app_error_code_t quarantine_result;

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
}

static void assert_absent(const char *body, const char *secret) {
    TEST_CHECK(body != NULL);
    TEST_CHECK(strstr(body, secret) == NULL);
}

static void test_storage_snapshot_is_redacted_and_not_verified(void) {
    reset_fixture();
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        web_api_admin_boundary_handle(WEB_API_ROUTE_DIAGNOSTICS_STORAGE, &response));
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
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        web_api_admin_boundary_handle(WEB_API_ROUTE_DIAGNOSTICS_STORAGE, &response));
    TEST_CHECK_EQ_U64(500U, response.status);
    TEST_CHECK(strstr(response.body, "storage health unavailable") != NULL);
    TEST_CHECK(strstr(response.body, "\"ok\":false") != NULL);
    web_api_response_free(&response);
}

static void test_storage_check_never_reports_false_success(void) {
    reset_fixture();
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        web_api_admin_boundary_handle(WEB_API_ROUTE_DIAGNOSTICS_STORAGE_CHECK, &response));
    TEST_CHECK_EQ_U64(503U, response.status);
    TEST_CHECK(strstr(response.body, "Phase 19 diagnostics service") != NULL);
    TEST_CHECK(strstr(response.body, "\"ok\":false") != NULL);
    TEST_CHECK(strstr(response.body, "\"verified\":true") == NULL);
    web_api_response_free(&response);
}

static void test_package_boundaries_are_explicit(void) {
    static const web_api_route_t routes[] = {
        WEB_API_ROUTE_SET_EXPORT,
        WEB_API_ROUTE_SET_IMPORT,
        WEB_API_ROUTE_BACKUP,
        WEB_API_ROUTE_RESTORE,
    };
    for (size_t index = 0U; index < sizeof(routes) / sizeof(routes[0]); ++index) {
        web_api_response_t response = {0};
        TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                             web_api_admin_boundary_handle(routes[index], &response));
        TEST_CHECK_EQ_U64(503U, response.status);
        TEST_CHECK(strstr(response.body, "Phase 18 transaction service") != NULL);
        TEST_CHECK(strstr(response.body, "\"ok\":false") != NULL);
        web_api_response_free(&response);
    }
}

static void test_invalid_boundary_inputs(void) {
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        web_api_admin_boundary_handle(WEB_API_ROUTE_DIAGNOSTICS_STORAGE, NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND,
                         web_api_admin_boundary_handle(WEB_API_ROUTE_SETTINGS, &response));
    TEST_CHECK(response.body == NULL);
}

int main(void) {
    test_storage_snapshot_is_redacted_and_not_verified();
    test_storage_snapshot_failure_is_visible();
    test_storage_check_never_reports_false_success();
    test_package_boundaries_are_explicit();
    test_invalid_boundary_inputs();
    return 0;
}
