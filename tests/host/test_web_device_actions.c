#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "device_settings_v2.h"
#include "test_assert.h"
#include "web_device_actions.h"

#define TEST_BODY_CAPACITY 1024U

typedef struct {
    app_v2_device_settings_t current;
    app_error_code_t settings_read_error;
    app_error_code_t password_verify_error;
    app_error_code_t restart_error;
    app_error_code_t reset_settings_error;
    app_error_code_t factory_reset_error;
    bool password_matches;
    unsigned int settings_read_calls;
    unsigned int password_verify_calls;
    unsigned int restart_calls;
    unsigned int reset_settings_calls;
    unsigned int factory_reset_calls;
    char observed_password[256];
} fake_t;

static app_error_code_t fake_settings_read(void *context, app_v2_device_settings_t *out_settings) {
    fake_t *fake = context;
    ++fake->settings_read_calls;
    if (fake->settings_read_error != APP_ERROR_NONE) {
        return fake->settings_read_error;
    }
    *out_settings = fake->current;
    return APP_ERROR_NONE;
}

static app_error_code_t fake_password_verify(void *context, const char *password,
                                             size_t password_length,
                                             const app_v2_device_settings_t *settings,
                                             bool *out_matches) {
    fake_t *fake = context;
    (void)settings;
    ++fake->password_verify_calls;
    TEST_CHECK(password_length < sizeof(fake->observed_password));
    memcpy(fake->observed_password, password, password_length);
    fake->observed_password[password_length] = '\0';
    if (fake->password_verify_error != APP_ERROR_NONE) {
        return fake->password_verify_error;
    }
    *out_matches = fake->password_matches;
    return APP_ERROR_NONE;
}

static app_error_code_t fake_restart(void *context) {
    fake_t *fake = context;
    ++fake->restart_calls;
    return fake->restart_error;
}

static app_error_code_t fake_reset_settings(void *context) {
    fake_t *fake = context;
    ++fake->reset_settings_calls;
    return fake->reset_settings_error;
}

static app_error_code_t fake_factory_reset(void *context) {
    fake_t *fake = context;
    ++fake->factory_reset_calls;
    return fake->factory_reset_error;
}

static web_device_actions_ops_t operations(fake_t *fake) {
    return (web_device_actions_ops_t){
        .context = fake,
        .settings_read = fake_settings_read,
        .password_verify = fake_password_verify,
        .restart = fake_restart,
        .reset_settings = fake_reset_settings,
        .factory_reset = fake_factory_reset,
    };
}

static void build_body(char *buffer, size_t capacity, const char *json) {
    const int written = snprintf(buffer, capacity, "%s", json);
    TEST_CHECK(written > 0 && (size_t)written < capacity);
}

static bool buffer_all_zero(const char *buffer, size_t length) {
    for (size_t index = 0U; index < length; ++index) {
        if (buffer[index] != '\0') {
            return false;
        }
    }
    return true;
}

/* ---------------------------------------------------------------------- */
/* POST /api/v1/device/restart                                             */
/* ---------------------------------------------------------------------- */

static void test_restart_success(void) {
    fake_t fake = {0};
    const web_device_actions_ops_t ops = operations(&fake);

    const web_device_restart_outcome_t outcome = web_device_restart_handle(&ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_RESTART_OK, outcome.result);
    TEST_CHECK_EQ_U64(1U, fake.restart_calls);
}

static void test_restart_backend_failure(void) {
    fake_t fake = {.restart_error = APP_ERROR_INTERNAL};
    const web_device_actions_ops_t ops = operations(&fake);

    const web_device_restart_outcome_t outcome = web_device_restart_handle(&ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_RESTART_BACKEND_UNAVAILABLE, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, outcome.detail);
}

static void test_restart_null_ops(void) {
    TEST_CHECK_EQ_INT(WEB_DEVICE_RESTART_INTERNAL, web_device_restart_handle(NULL).result);
}

/* ---------------------------------------------------------------------- */
/* POST /api/v1/device/reset-settings                                      */
/* ---------------------------------------------------------------------- */

static void test_reset_settings_success(void) {
    fake_t fake = {0};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"confirmation\":\"RESET SETTINGS\"}");

    const web_device_reset_settings_outcome_t outcome =
        web_device_reset_settings_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_RESET_SETTINGS_OK, outcome.result);
    TEST_CHECK_EQ_U64(1U, fake.reset_settings_calls);
    TEST_CHECK(buffer_all_zero(body, sizeof(body)));
}

static void test_reset_settings_wrong_confirmation_rejected(void) {
    fake_t fake = {0};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"confirmation\":\"reset settings\"}");

    const web_device_reset_settings_outcome_t outcome =
        web_device_reset_settings_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_RESET_SETTINGS_INVALID_CONFIRMATION, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.reset_settings_calls);
}

static void test_reset_settings_unknown_field_rejected(void) {
    fake_t fake = {0};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"confirmation\":\"RESET SETTINGS\",\"adminPassword\":\"whatever\"}");

    const web_device_reset_settings_outcome_t outcome =
        web_device_reset_settings_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_RESET_SETTINGS_INVALID_BODY, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.reset_settings_calls);
}

static void test_reset_settings_missing_field_rejected(void) {
    fake_t fake = {0};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{}");

    const web_device_reset_settings_outcome_t outcome =
        web_device_reset_settings_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_RESET_SETTINGS_INVALID_BODY, outcome.result);
}

static void test_reset_settings_body_without_nul_terminator_rejected(void) {
    fake_t fake = {0};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    memset(body, 'x', sizeof(body));

    const web_device_reset_settings_outcome_t outcome =
        web_device_reset_settings_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_RESET_SETTINGS_INVALID_BODY, outcome.result);
    TEST_CHECK(buffer_all_zero(body, sizeof(body)));
}

static void test_reset_settings_embedded_nul_escape_rejected(void) {
    fake_t fake = {0};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"confirmation\":\"A\\u0000B\"}");

    const web_device_reset_settings_outcome_t outcome =
        web_device_reset_settings_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_RESET_SETTINGS_INVALID_BODY, outcome.result);
    TEST_CHECK(buffer_all_zero(body, sizeof(body)));
}

static void test_reset_settings_malformed_json_rejected(void) {
    fake_t fake = {0};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"confirmation\":");

    const web_device_reset_settings_outcome_t outcome =
        web_device_reset_settings_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_RESET_SETTINGS_INVALID_BODY, outcome.result);
}

static void test_reset_settings_trailing_content_rejected(void) {
    fake_t fake = {0};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"confirmation\":\"RESET SETTINGS\"}garbage");

    const web_device_reset_settings_outcome_t outcome =
        web_device_reset_settings_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_RESET_SETTINGS_INVALID_BODY, outcome.result);
}

static void test_reset_settings_non_object_top_level_rejected(void) {
    fake_t fake = {0};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "[]");

    const web_device_reset_settings_outcome_t outcome =
        web_device_reset_settings_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_RESET_SETTINGS_INVALID_BODY, outcome.result);
}

static void test_reset_settings_confirmation_wrong_type_rejected(void) {
    fake_t fake = {0};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"confirmation\":123}");

    const web_device_reset_settings_outcome_t outcome =
        web_device_reset_settings_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_RESET_SETTINGS_INVALID_BODY, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.reset_settings_calls);
}

static void test_reset_settings_null_ops_wipes_body(void) {
    web_device_actions_ops_t ops = {0}; /* reset_settings == NULL */
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"confirmation\":\"RESET SETTINGS\"}");

    const web_device_reset_settings_outcome_t outcome =
        web_device_reset_settings_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_RESET_SETTINGS_INTERNAL, outcome.result);
    TEST_CHECK(buffer_all_zero(body, sizeof(body)));
}

static void test_reset_settings_zero_capacity_body_wiped(void) {
    fake_t fake = {0};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"confirmation\":\"RESET SETTINGS\"}");

    const web_device_reset_settings_outcome_t outcome =
        web_device_reset_settings_handle(body, 0U, &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_RESET_SETTINGS_INTERNAL, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.reset_settings_calls);
}

static void test_reset_settings_backend_failure(void) {
    fake_t fake = {.reset_settings_error = APP_ERROR_STORAGE_UNAVAILABLE};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"confirmation\":\"RESET SETTINGS\"}");

    const web_device_reset_settings_outcome_t outcome =
        web_device_reset_settings_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_RESET_SETTINGS_BACKEND_UNAVAILABLE, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, outcome.detail);
}

/* ---------------------------------------------------------------------- */
/* POST /api/v1/device/factory-reset                                       */
/* ---------------------------------------------------------------------- */

static void test_factory_reset_success(void) {
    fake_t fake = {.password_matches = true};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"adminPassword\":\"example-admin-password\",\"confirmation\":\"FACTORY RESET\"}");

    const web_device_factory_reset_outcome_t outcome =
        web_device_factory_reset_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_FACTORY_RESET_OK, outcome.result);
    TEST_CHECK_EQ_STRING("example-admin-password", fake.observed_password);
    TEST_CHECK_EQ_U64(1U, fake.factory_reset_calls);
    TEST_CHECK(buffer_all_zero(body, sizeof(body)));
}

static void test_factory_reset_wrong_confirmation_rejected_before_password_check(void) {
    fake_t fake = {.password_matches = true};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"adminPassword\":\"example-admin-password\",\"confirmation\":\"factory reset\"}");

    const web_device_factory_reset_outcome_t outcome =
        web_device_factory_reset_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_FACTORY_RESET_INVALID_CONFIRMATION, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_read_calls);
    TEST_CHECK_EQ_U64(0U, fake.password_verify_calls);
    TEST_CHECK_EQ_U64(0U, fake.factory_reset_calls);
}

static void test_factory_reset_wrong_password_rejected(void) {
    fake_t fake = {.password_matches = false};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"adminPassword\":\"wrong-password\",\"confirmation\":\"FACTORY RESET\"}");

    const web_device_factory_reset_outcome_t outcome =
        web_device_factory_reset_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_FACTORY_RESET_INCORRECT_PASSWORD, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.factory_reset_calls);
}

static void test_factory_reset_unknown_field_rejected(void) {
    fake_t fake = {.password_matches = true};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"adminPassword\":\"example-admin-password\",\"confirmation\":\"FACTORY "
               "RESET\",\"extra\":true}");

    const web_device_factory_reset_outcome_t outcome =
        web_device_factory_reset_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_FACTORY_RESET_INVALID_BODY, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_read_calls);
}

static void test_factory_reset_missing_field_rejected(void) {
    fake_t fake = {.password_matches = true};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"confirmation\":\"FACTORY RESET\"}");

    const web_device_factory_reset_outcome_t outcome =
        web_device_factory_reset_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_FACTORY_RESET_INVALID_BODY, outcome.result);
}

static void test_factory_reset_settings_read_backend_unavailable(void) {
    fake_t fake = {.password_matches = true, .settings_read_error = APP_ERROR_STORAGE_UNAVAILABLE};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"adminPassword\":\"example-admin-password\",\"confirmation\":\"FACTORY RESET\"}");

    const web_device_factory_reset_outcome_t outcome =
        web_device_factory_reset_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_FACTORY_RESET_BACKEND_UNAVAILABLE, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, outcome.detail);
    TEST_CHECK_EQ_U64(0U, fake.factory_reset_calls);
}

static void test_factory_reset_malformed_json_rejected(void) {
    fake_t fake = {.password_matches = true};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"adminPassword\":");

    const web_device_factory_reset_outcome_t outcome =
        web_device_factory_reset_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_FACTORY_RESET_INVALID_BODY, outcome.result);
}

static void test_factory_reset_admin_password_wrong_type_rejected(void) {
    fake_t fake = {.password_matches = true};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"adminPassword\":123,\"confirmation\":\"FACTORY RESET\"}");

    const web_device_factory_reset_outcome_t outcome =
        web_device_factory_reset_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_FACTORY_RESET_INVALID_BODY, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_read_calls);
}

static void test_factory_reset_null_ops_wipes_body(void) {
    web_device_actions_ops_t ops = {0}; /* every callback missing */
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"adminPassword\":\"example-admin-password\",\"confirmation\":\"FACTORY RESET\"}");

    const web_device_factory_reset_outcome_t outcome =
        web_device_factory_reset_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_FACTORY_RESET_INTERNAL, outcome.result);
    TEST_CHECK(buffer_all_zero(body, sizeof(body)));
}

static void test_factory_reset_zero_capacity_body_rejected(void) {
    fake_t fake = {.password_matches = true};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"adminPassword\":\"example-admin-password\",\"confirmation\":\"FACTORY RESET\"}");

    const web_device_factory_reset_outcome_t outcome =
        web_device_factory_reset_handle(body, 0U, &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_FACTORY_RESET_INTERNAL, outcome.result);
    TEST_CHECK_EQ_U64(0U, fake.settings_read_calls);
}

static void test_factory_reset_verify_backend_unavailable(void) {
    fake_t fake = {.password_verify_error = APP_ERROR_INTERNAL};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"adminPassword\":\"example-admin-password\",\"confirmation\":\"FACTORY RESET\"}");

    const web_device_factory_reset_outcome_t outcome =
        web_device_factory_reset_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_FACTORY_RESET_BACKEND_UNAVAILABLE, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, outcome.detail);
    TEST_CHECK_EQ_U64(0U, fake.factory_reset_calls);
}

static void test_factory_reset_backend_failure(void) {
    fake_t fake = {.password_matches = true, .factory_reset_error = APP_ERROR_INTERNAL};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body),
               "{\"adminPassword\":\"example-admin-password\",\"confirmation\":\"FACTORY RESET\"}");

    const web_device_factory_reset_outcome_t outcome =
        web_device_factory_reset_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_FACTORY_RESET_BACKEND_UNAVAILABLE, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, outcome.detail);
}

/* ---------------------------------------------------------------------- */
/* Response JSON                                                           */
/* ---------------------------------------------------------------------- */

static void test_restart_accepted_json_null_out_json_rejected(void) {
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, web_device_restart_accepted_json(NULL));
}

static void test_reset_accepted_json_null_out_json_rejected(void) {
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_device_reset_accepted_json(false, true, NULL));
}

static void test_restart_accepted_json(void) {
    char *json = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_device_restart_accepted_json(&json));
    TEST_CHECK(strstr(json, "\"accepted\":true") != NULL);
    TEST_CHECK(strstr(json, "\"connectionWillClose\":true") != NULL);
    TEST_CHECK(strstr(json, "\"reprovisioningRequired\":false") != NULL);
    free(json);
}

static void test_reset_accepted_json_reset_settings_shape(void) {
    char *json = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_device_reset_accepted_json(false, true, &json));
    TEST_CHECK(strstr(json, "\"connectionWillClose\":true") != NULL);
    TEST_CHECK(strstr(json, "\"reprovisioningRequired\":false") != NULL);
    TEST_CHECK(strstr(json, "\"repositoryBlobsPreserved\":true") != NULL);
    free(json);
}

static void test_reset_accepted_json_factory_reset_shape(void) {
    char *json = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_device_reset_accepted_json(true, false, &json));
    TEST_CHECK(strstr(json, "\"connectionWillClose\":true") != NULL);
    TEST_CHECK(strstr(json, "\"reprovisioningRequired\":true") != NULL);
    TEST_CHECK(strstr(json, "\"repositoryBlobsPreserved\":false") != NULL);
    free(json);
}

int main(void) {
    test_restart_success();
    test_restart_backend_failure();
    test_restart_null_ops();
    test_reset_settings_success();
    test_reset_settings_wrong_confirmation_rejected();
    test_reset_settings_unknown_field_rejected();
    test_reset_settings_missing_field_rejected();
    test_reset_settings_body_without_nul_terminator_rejected();
    test_reset_settings_embedded_nul_escape_rejected();
    test_reset_settings_malformed_json_rejected();
    test_reset_settings_trailing_content_rejected();
    test_reset_settings_non_object_top_level_rejected();
    test_reset_settings_confirmation_wrong_type_rejected();
    test_reset_settings_null_ops_wipes_body();
    test_reset_settings_zero_capacity_body_wiped();
    test_reset_settings_backend_failure();
    test_factory_reset_success();
    test_factory_reset_wrong_confirmation_rejected_before_password_check();
    test_factory_reset_wrong_password_rejected();
    test_factory_reset_unknown_field_rejected();
    test_factory_reset_missing_field_rejected();
    test_factory_reset_malformed_json_rejected();
    test_factory_reset_admin_password_wrong_type_rejected();
    test_factory_reset_null_ops_wipes_body();
    test_factory_reset_zero_capacity_body_rejected();
    test_factory_reset_verify_backend_unavailable();
    test_factory_reset_settings_read_backend_unavailable();
    test_factory_reset_backend_failure();
    test_restart_accepted_json_null_out_json_rejected();
    test_reset_accepted_json_null_out_json_rejected();
    test_restart_accepted_json();
    test_reset_accepted_json_reset_settings_shape();
    test_reset_accepted_json_factory_reset_shape();
    puts("web device actions tests passed");
    return EXIT_SUCCESS;
}
