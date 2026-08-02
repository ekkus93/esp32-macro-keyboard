#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "test_assert.h"
#include "web_api_response.h"
#include "web_http_status.h"

static void test_success_envelope(void) {
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        web_api_response_success(&response, WEB_HTTP_STATUS_CREATED, "{\"id\":\"abc\"}"));
    TEST_CHECK_EQ_U64(WEB_HTTP_STATUS_CREATED, response.status);
    TEST_CHECK(response.body_length == strlen(response.body));
    TEST_CHECK(strstr(response.body, "\"ok\":true") != NULL);
    TEST_CHECK(strstr(response.body, "\"id\":\"abc\"") != NULL);
    TEST_CHECK(response.body_free != NULL);
    web_api_response_free(&response);
    TEST_CHECK(response.body == NULL);
    TEST_CHECK_EQ_U64(0U, response.body_length);
    TEST_CHECK(response.body_free == NULL);
}

static void test_raw_package_response(void) {
    static const char package[] =
        "{\"schema_version\":1,\"package_type\":\"set\",\"sets\":[],\"macros\":[],"
        "\"procedures\":[],\"progress\":[]}";
    char *owned = malloc(sizeof(package));
    TEST_CHECK(owned != NULL);
    memcpy(owned, package, sizeof(package));

    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_response_take_json(&response, WEB_HTTP_STATUS_OK,
                                                                    owned, sizeof(package) - 1U));
    TEST_CHECK_EQ_U64(WEB_HTTP_STATUS_OK, response.status);
    TEST_CHECK_EQ_U64(sizeof(package) - 1U, response.body_length);
    TEST_CHECK_EQ_STRING(package, response.body);
    TEST_CHECK(strstr(response.body, "\"ok\":true") == NULL);
    TEST_CHECK(response.body_free != NULL);
    web_api_response_free(&response);
    TEST_CHECK(response.body == NULL);
    TEST_CHECK(response.body_free == NULL);
}

static void test_error_envelope(void) {
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        web_api_response_error(&response,
                               &(web_api_error_spec_t){
                                   .status = WEB_HTTP_STATUS_CONFLICT,
                                   .code = APP_ERROR_CONFLICT,
                                   .message = "stale revision",
                                   .details_json = "{\"expectedRevision\":3,\"actualRevision\":4}",
                               }));
    TEST_CHECK_EQ_U64(WEB_HTTP_STATUS_CONFLICT, response.status);
    TEST_CHECK(strstr(response.body, "\"ok\":false") != NULL);
    TEST_CHECK(strstr(response.body, "\"code\":\"conflict\"") != NULL);
    TEST_CHECK(strstr(response.body, "\"actualRevision\":4") != NULL);
    web_api_response_free(&response);
}

static void test_invalid_payload_rejected(void) {
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        web_api_response_success(&response, WEB_HTTP_STATUS_INTERNAL_SERVER_ERROR, "{}"));
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         web_api_response_success(&response, WEB_HTTP_STATUS_OK, "not-json"));
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INTERNAL,
        web_api_response_error(&response, &(web_api_error_spec_t){
                                              .status = WEB_HTTP_STATUS_BAD_REQUEST,
                                              .code = APP_ERROR_INVALID_ARGUMENT,
                                              .message = "invalid",
                                              .details_json = "not-json",
                                          }));

    char *embedded_nul = malloc(3U);
    TEST_CHECK(embedded_nul != NULL);
    embedded_nul[0] = '{';
    embedded_nul[1] = '\0';
    embedded_nul[2] = '}';
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        web_api_response_take_json(&response, WEB_HTTP_STATUS_OK, embedded_nul, 3U));
    free(embedded_nul);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_response_take_json(&response, WEB_HTTP_STATUS_OK, NULL, 0U));
}

int main(void) {
    test_success_envelope();
    test_raw_package_response();
    test_error_envelope();
    test_invalid_payload_rejected();
    return 0;
}
