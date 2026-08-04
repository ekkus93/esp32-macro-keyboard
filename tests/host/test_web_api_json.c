#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "test_assert.h"
#include "web_api_json.h"

static void test_expected_revision(void) {
    uint32_t revision = 0U;
    static const char valid[] = "{\"expectedRevision\":7}";
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_api_json_parse_expected_revision(valid, sizeof(valid) - 1U, &revision));
    TEST_CHECK_EQ_U64(7U, revision);

    static const char unknown[] = "{\"expectedRevision\":7,\"extra\":true}";
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_expected_revision(unknown, sizeof(unknown) - 1U,
                                                              &revision));
    static const char zero[] = "{\"expectedRevision\":0}";
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_expected_revision(zero, sizeof(zero) - 1U, &revision));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_expected_revision(valid, sizeof(valid) - 1U, NULL));
}

static void test_settings_update(void) {
    provisioning_settings_t settings = {0};
    uint32_t revision = 0U;
    static const char valid[] =
        "{\"expectedRevision\":3,\"requirePhysicalConfirmation\":true,"
        "\"alwaysSelectPackage\":false}";
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_api_json_parse_settings_update(valid, sizeof(valid) - 1U, &settings,
                                                            &revision));
    TEST_CHECK_EQ_U64(3U, revision);
    TEST_CHECK(settings.require_physical_confirmation);
    TEST_CHECK(!settings.always_select_package);

    static const char missing[] =
        "{\"expectedRevision\":3,\"requirePhysicalConfirmation\":true}";
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_settings_update(missing, sizeof(missing) - 1U, &settings,
                                                            &revision));
    static const char wrong_type[] =
        "{\"expectedRevision\":3,\"requirePhysicalConfirmation\":1,"
        "\"alwaysSelectPackage\":false}";
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_settings_update(wrong_type, sizeof(wrong_type) - 1U,
                                                            &settings, &revision));
    static const char trailing[] =
        "{\"expectedRevision\":3,\"requirePhysicalConfirmation\":true,"
        "\"alwaysSelectPackage\":false}x";
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_settings_update(trailing, sizeof(trailing) - 1U,
                                                            &settings, &revision));
}

int main(void) {
    test_expected_revision();
    test_settings_update();
    return 0;
}
