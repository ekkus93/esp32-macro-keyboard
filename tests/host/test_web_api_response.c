#include <stddef.h>
#include <string.h>

#include "app_error.h"
#include "test_assert.h"
#include "web_api_response.h"

static void test_success_envelope(void) {
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_api_response_success(&response, 201U, "{\"id\":\"abc\"}"));
    TEST_CHECK_EQ_U64(201U, response.status);
    TEST_CHECK(response.body_length == strlen(response.body));
    TEST_CHECK(strstr(response.body, "\"ok\":true") != NULL);
    TEST_CHECK(strstr(response.body, "\"id\":\"abc\"") != NULL);
    web_api_response_free(&response);
    TEST_CHECK(response.body == NULL);
    TEST_CHECK_EQ_U64(0U, response.body_length);
}

static void test_error_envelope(void) {
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        web_api_response_error(&response, 409U, APP_ERROR_CONFLICT, "stale revision",
                               "{\"expectedRevision\":3,\"actualRevision\":4}"));
    TEST_CHECK_EQ_U64(409U, response.status);
    TEST_CHECK(strstr(response.body, "\"ok\":false") != NULL);
    TEST_CHECK(strstr(response.body, "\"code\":\"conflict\"") != NULL);
    TEST_CHECK(strstr(response.body, "\"actualRevision\":4") != NULL);
    web_api_response_free(&response);
}

static void test_invalid_payload_rejected(void) {
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_response_success(&response, 500U, "{}"));
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         web_api_response_success(&response, 200U, "not-json"));
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         web_api_response_error(&response, 400U, APP_ERROR_INVALID_ARGUMENT,
                                                "invalid", "not-json"));
}

int main(void) {
    test_success_envelope();
    test_error_envelope();
    test_invalid_payload_rejected();
    return 0;
}
