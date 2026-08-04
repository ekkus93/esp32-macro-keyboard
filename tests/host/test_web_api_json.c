#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "app_limits_v2.h"
#include "test_assert.h"
#include "web_api_json.h"

static void expect_revision_invalid(const char *json, size_t length) {
    uint32_t revision = UINT32_MAX;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_expected_revision(json, length, &revision));
    TEST_CHECK_EQ_U64(0U, revision);
}

static void test_expected_revision_valid(void) {
    uint32_t revision = 0U;
    static const char valid[] = "{\"expectedRevision\":7}";
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_api_json_parse_expected_revision(valid, sizeof(valid) - 1U, &revision));
    TEST_CHECK_EQ_U64(7U, revision);

    static const char maximum[] = "{\"expectedRevision\":4294967295}";
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_api_json_parse_expected_revision(maximum, sizeof(maximum) - 1U,
                                                              &revision));
    TEST_CHECK_EQ_U64(UINT32_MAX, revision);
}

static void test_expected_revision_invalid_documents(void) {
    static const char valid[] = "{\"expectedRevision\":7}";
    expect_revision_invalid(NULL, 0U);
    expect_revision_invalid("", 0U);
    expect_revision_invalid(valid, (size_t)APP_V2_JSON_BODY_MAX_BYTES + 1U);

    static const char unknown[] = "{\"expectedRevision\":7,\"extra\":true}";
    expect_revision_invalid(unknown, sizeof(unknown) - 1U);
    static const char duplicate[] = "{\"expectedRevision\":7,\"expectedRevision\":8}";
    expect_revision_invalid(duplicate, sizeof(duplicate) - 1U);
    static const char missing[] = "{}";
    expect_revision_invalid(missing, sizeof(missing) - 1U);
    static const char array[] = "[7]";
    expect_revision_invalid(array, sizeof(array) - 1U);
    static const char trailing[] = "{\"expectedRevision\":7}x";
    expect_revision_invalid(trailing, sizeof(trailing) - 1U);
    static const char escaped_nul[] = "{\"expectedRevision\":7,\"x\":\"\\u0000\"}";
    expect_revision_invalid(escaped_nul, sizeof(escaped_nul) - 1U);
}

static void test_expected_revision_invalid_values(void) {
    static const char zero[] = "{\"expectedRevision\":0}";
    expect_revision_invalid(zero, sizeof(zero) - 1U);
    static const char negative[] = "{\"expectedRevision\":-1}";
    expect_revision_invalid(negative, sizeof(negative) - 1U);
    static const char fraction[] = "{\"expectedRevision\":1.5}";
    expect_revision_invalid(fraction, sizeof(fraction) - 1U);
    static const char overflow[] = "{\"expectedRevision\":4294967296}";
    expect_revision_invalid(overflow, sizeof(overflow) - 1U);
    static const char string[] = "{\"expectedRevision\":\"7\"}";
    expect_revision_invalid(string, sizeof(string) - 1U);

    static const char valid[] = "{\"expectedRevision\":7}";
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_expected_revision(valid, sizeof(valid) - 1U, NULL));
}

static void expect_settings_invalid(const char *json, size_t length) {
    provisioning_settings_t settings;
    memset(&settings, 0xa5, sizeof(settings));
    uint32_t revision = UINT32_MAX;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_settings_update(json, length, &settings, &revision));
    TEST_CHECK_EQ_U64(0U, revision);
    static const provisioning_settings_t zero = {0};
    TEST_CHECK_EQ_BUFFER(&zero, &settings, sizeof(settings));
}

static void test_settings_update_valid(void) {
    provisioning_settings_t settings = {0};
    uint32_t revision = 0U;
    static const char valid[] =
        "{\"expectedRevision\":3,\"requirePhysicalConfirmation\":true,"
        "\"alwaysSelectPackage\":false}";
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_api_json_parse_settings_update(valid, sizeof(valid) - 1U, &settings,
                                                            &revision));
    TEST_CHECK_EQ_U64(3U, revision);
    TEST_CHECK_EQ_U64(APP_SCHEMA_VERSION, settings.schema_version);
    TEST_CHECK_EQ_U64(3U, settings.revision);
    TEST_CHECK(settings.require_physical_confirmation);
    TEST_CHECK(!settings.always_select_package);

    static const char inverse[] =
        "{\"alwaysSelectPackage\":true,\"expectedRevision\":9,"
        "\"requirePhysicalConfirmation\":false}";
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_api_json_parse_settings_update(inverse, sizeof(inverse) - 1U, &settings,
                                                            &revision));
    TEST_CHECK_EQ_U64(9U, revision);
    TEST_CHECK(!settings.require_physical_confirmation);
    TEST_CHECK(settings.always_select_package);
}

static void test_settings_update_invalid(void) {
    expect_settings_invalid(NULL, 0U);
    expect_settings_invalid("", 0U);

    static const char missing[] =
        "{\"expectedRevision\":3,\"requirePhysicalConfirmation\":true}";
    expect_settings_invalid(missing, sizeof(missing) - 1U);
    static const char unknown[] =
        "{\"expectedRevision\":3,\"requirePhysicalConfirmation\":true,"
        "\"alwaysSelectPackage\":false,\"extra\":true}";
    expect_settings_invalid(unknown, sizeof(unknown) - 1U);
    static const char duplicate[] =
        "{\"expectedRevision\":3,\"requirePhysicalConfirmation\":true,"
        "\"alwaysSelectPackage\":false,\"alwaysSelectPackage\":true}";
    expect_settings_invalid(duplicate, sizeof(duplicate) - 1U);
    static const char wrong_confirmation[] =
        "{\"expectedRevision\":3,\"requirePhysicalConfirmation\":1,"
        "\"alwaysSelectPackage\":false}";
    expect_settings_invalid(wrong_confirmation, sizeof(wrong_confirmation) - 1U);
    static const char wrong_select[] =
        "{\"expectedRevision\":3,\"requirePhysicalConfirmation\":true,"
        "\"alwaysSelectPackage\":0}";
    expect_settings_invalid(wrong_select, sizeof(wrong_select) - 1U);
    static const char invalid_revision[] =
        "{\"expectedRevision\":0,\"requirePhysicalConfirmation\":true,"
        "\"alwaysSelectPackage\":false}";
    expect_settings_invalid(invalid_revision, sizeof(invalid_revision) - 1U);
    static const char trailing[] =
        "{\"expectedRevision\":3,\"requirePhysicalConfirmation\":true,"
        "\"alwaysSelectPackage\":false}x";
    expect_settings_invalid(trailing, sizeof(trailing) - 1U);

    provisioning_settings_t settings = {0};
    uint32_t revision = 0U;
    static const char valid[] =
        "{\"expectedRevision\":3,\"requirePhysicalConfirmation\":true,"
        "\"alwaysSelectPackage\":false}";
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_settings_update(valid, sizeof(valid) - 1U, NULL,
                                                            &revision));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_settings_update(valid, sizeof(valid) - 1U, &settings,
                                                            NULL));
}

int main(void) {
    test_expected_revision_valid();
    test_expected_revision_invalid_documents();
    test_expected_revision_invalid_values();
    test_settings_update_valid();
    test_settings_update_invalid();
    return 0;
}
