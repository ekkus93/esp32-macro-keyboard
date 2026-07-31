#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_assert.h"
#include "web_setup_json.h"

#define TEST_BODY_BYTES 512U
#define VALID_DOCUMENT_ZERO_CALLS 12U

typedef struct {
    size_t calls;
    size_t bytes;
} zero_fixture_t;

static void fake_secure_zero(void *context, void *memory, size_t size) {
    zero_fixture_t *fixture = context;
    memset(memory, 0, size);
    ++fixture->calls;
    fixture->bytes += size;
}

static web_setup_json_ops_t operations(zero_fixture_t *fixture) {
    return (web_setup_json_ops_t){
        .context = fixture,
        .secure_zero = fake_secure_zero,
    };
}

static const char *valid_json(void) {
    return "{"
           "\"setupCode\":\"45175C9BB39D8BE5FC7EF773\","
           "\"apSsid\":\"Macro Keyboard\","
           "\"apPassphrase\":\"correct-horse-battery\","
           "\"administratorPassword\":\"admin-password-strong\","
           "\"requirePhysicalConfirmation\":true,"
           "\"alwaysSelectSet\":false"
           "}";
}

static void assert_zero(const void *memory, size_t size) {
    const unsigned char *bytes = memory;
    for (size_t index = 0U; index < size; ++index) {
        TEST_CHECK_EQ_INT(0, bytes[index]);
    }
}

static void test_valid_document(void) {
    zero_fixture_t fixture = {0};
    const web_setup_json_ops_t ops = operations(&fixture);
    char body[TEST_BODY_BYTES] = {0};
    TEST_CHECK(snprintf(body, sizeof(body), "%s", valid_json()) > 0);
    web_setup_submission_t submission;
    memset(&submission, 0xaa, sizeof(submission));

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_setup_json_parse(body, sizeof(body), &ops, &submission));
    assert_zero(body, sizeof(body));
    TEST_CHECK_EQ_STRING("45175C9BB39D8BE5FC7EF773", submission.setup_code);
    TEST_CHECK_EQ_STRING("Macro Keyboard", submission.ap_ssid);
    TEST_CHECK_EQ_STRING("correct-horse-battery", submission.ap_passphrase);
    TEST_CHECK_EQ_STRING("admin-password-strong", submission.administrator_password);
    TEST_CHECK(submission.require_physical_confirmation);
    TEST_CHECK(!submission.always_select_set);
    TEST_CHECK_EQ_U64(VALID_DOCUMENT_ZERO_CALLS, fixture.calls);
    TEST_CHECK(fixture.bytes > sizeof(body));
}

static void expect_invalid(const char *json) {
    zero_fixture_t fixture = {0};
    const web_setup_json_ops_t ops = operations(&fixture);
    char body[TEST_BODY_BYTES] = {0};
    TEST_CHECK(snprintf(body, sizeof(body), "%s", json) > 0);
    web_setup_submission_t submission;
    memset(&submission, 0xaa, sizeof(submission));

    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_setup_json_parse(body, sizeof(body), &ops, &submission));
    assert_zero(body, sizeof(body));
    assert_zero(&submission, sizeof(submission));
    TEST_CHECK(fixture.calls >= 2U);
}

static void test_strict_document_and_schema(void) {
    char value[TEST_BODY_BYTES];

    TEST_CHECK(snprintf(value, sizeof(value), "%s trailing", valid_json()) > 0);
    expect_invalid(value);

    expect_invalid("{\"setupCode\":\"45175C9BB39D8BE5FC7EF773\"}");

    TEST_CHECK(snprintf(value, sizeof(value), "{\"unexpected\":true,%s", valid_json() + 1) > 0);
    expect_invalid(value);

    TEST_CHECK(snprintf(value, sizeof(value), "{\"setupCode\":\"45175C9BB39D8BE5FC7EF773\",%s",
                        valid_json() + 1) > 0);
    expect_invalid(value);

    TEST_CHECK(snprintf(value, sizeof(value), "%s", valid_json()) > 0);
    char *boolean = strstr(value, "\"alwaysSelectSet\":false");
    TEST_CHECK(boolean != NULL);
    memcpy(boolean, "\"alwaysSelectSet\":\"no\"  ", strlen("\"alwaysSelectSet\":false"));
    expect_invalid(value);

    TEST_CHECK(snprintf(value, sizeof(value), "%s", valid_json()) > 0);
    char *password = strstr(value, "admin-password-strong");
    TEST_CHECK(password != NULL);
    memcpy(password, "admin\\u0000passwordxx", strlen("admin-password-strong"));
    expect_invalid(value);
}

static void test_bounds_and_arguments(void) {
    char long_ssid[WIFI_AP_SSID_MAX_BYTES + 2U];
    memset(long_ssid, 'S', sizeof(long_ssid) - 1U);
    long_ssid[sizeof(long_ssid) - 1U] = '\0';
    char value[TEST_BODY_BYTES];
    TEST_CHECK(snprintf(value, sizeof(value),
                        "{"
                        "\"setupCode\":\"45175C9BB39D8BE5FC7EF773\","
                        "\"apSsid\":\"%s\","
                        "\"apPassphrase\":\"correct-horse-battery\","
                        "\"administratorPassword\":\"admin-password-strong\","
                        "\"requirePhysicalConfirmation\":true,"
                        "\"alwaysSelectSet\":false"
                        "}",
                        long_ssid) > 0);
    expect_invalid(value);

    zero_fixture_t fixture = {0};
    web_setup_json_ops_t ops = operations(&fixture);
    char body[TEST_BODY_BYTES];
    memset(body, 'X', sizeof(body));
    web_setup_submission_t submission;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_setup_json_parse(body, sizeof(body), &ops, &submission));
    assert_zero(body, sizeof(body));
    assert_zero(&submission, sizeof(submission));

    TEST_CHECK(snprintf(body, sizeof(body), "%s", valid_json()) > 0);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_setup_json_parse(body, sizeof(body), &ops, NULL));
    assert_zero(body, sizeof(body));

    ops.secure_zero = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_setup_json_parse(NULL, 0U, &ops, &submission));
}

int main(void) {
    test_valid_document();
    test_strict_document_and_schema();
    test_bounds_and_arguments();
    puts("web setup JSON tests passed");
    return EXIT_SUCCESS;
}
