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
    /* SPEC_V2 13's success responses are the flat object itself -- no v1
     * "ok"/"data" envelope. */
    TEST_CHECK(strstr(response.body, "\"ok\"") == NULL);
    TEST_CHECK(strstr(response.body, "\"data\"") == NULL);
    TEST_CHECK(strstr(response.body, "\"id\":\"abc\"") != NULL);
    TEST_CHECK(response.body_free != NULL);
    web_api_response_free(&response);
    TEST_CHECK(response.body == NULL);
    TEST_CHECK_EQ_U64(0U, response.body_length);
    TEST_CHECK(response.body_free == NULL);
}

static void test_raw_package_response(void) {
    static const char package[] =
        "{\"schema_version\":1,\"package_type\":\"package\",\"packages\":[],\"macros\":[],"
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
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_api_response_error(&response, &(web_api_error_spec_t){
                                                               .status = WEB_HTTP_STATUS_CONFLICT,
                                                               .code = APP_ERROR_CONFLICT,
                                                               .message = "stale revision",
                                                               .field = "deviceName",
                                                           }));
    TEST_CHECK_EQ_U64(WEB_HTTP_STATUS_CONFLICT, response.status);
    /* SPEC_V2 13.2's exact error envelope: {"error":{"code","message",
     * "field"}} -- no top-level "ok" key, no "details" sub-object. */
    TEST_CHECK(strstr(response.body, "\"ok\"") == NULL);
    TEST_CHECK(strstr(response.body, "\"details\"") == NULL);
    TEST_CHECK(strstr(response.body, "\"error\":{") != NULL);
    TEST_CHECK(strstr(response.body, "\"code\":\"conflict\"") != NULL);
    TEST_CHECK(strstr(response.body, "\"message\":\"stale revision\"") != NULL);
    TEST_CHECK(strstr(response.body, "\"field\":\"deviceName\"") != NULL);
    web_api_response_free(&response);
}

static void test_error_envelope_without_field(void) {
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_api_response_error(&response, &(web_api_error_spec_t){
                                                               .status = WEB_HTTP_STATUS_NOT_FOUND,
                                                               .code = APP_ERROR_NOT_FOUND,
                                                               .message = "route not found",
                                                           }));
    TEST_CHECK(strstr(response.body, "\"field\"") == NULL);
    web_api_response_free(&response);
}

static void test_parser_error_envelope(void) {
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        web_api_response_error(&response, &(web_api_error_spec_t){
                                              .status = WEB_HTTP_STATUS_UNPROCESSABLE_ENTITY,
                                              .code = APP_ERROR_MACRO_SYNTAX,
                                              .message = "Unknown directive.",
                                              .field = "source",
                                              .has_parser_location = true,
                                              .byte_offset = 12U,
                                              .line = 1U,
                                              .column = 13U,
                                          }));
    TEST_CHECK_EQ_U64(WEB_HTTP_STATUS_UNPROCESSABLE_ENTITY, response.status);
    /* SPEC_V2 13.2: parser errors always report code "macro_parse_error",
     * regardless of the underlying app_error_code_t. */
    TEST_CHECK(strstr(response.body, "\"code\":\"macro_parse_error\"") != NULL);
    TEST_CHECK(strstr(response.body, "\"field\":\"source\"") != NULL);
    TEST_CHECK(strstr(response.body, "\"byteOffset\":12") != NULL);
    TEST_CHECK(strstr(response.body, "\"line\":1") != NULL);
    TEST_CHECK(strstr(response.body, "\"column\":13") != NULL);
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
        APP_ERROR_INVALID_ARGUMENT,
        web_api_response_error(&response, &(web_api_error_spec_t){
                                              .status = WEB_HTTP_STATUS_BAD_REQUEST,
                                              .code = APP_ERROR_INVALID_ARGUMENT,
                                              .message = NULL,
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
    test_error_envelope_without_field();
    test_parser_error_envelope();
    test_invalid_payload_rejected();
    return 0;
}
