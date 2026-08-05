#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_core_sequence.h"
#include "test_assert.h"

#define MAX_CALLS 64U
#define CALL_BYTES 32U

typedef enum {
    FAIL_NONE = 0,
    FAIL_BOOT_INDICATOR,
    FAIL_PROVISIONING_INIT,
    FAIL_PROVISIONING_LOAD,
    FAIL_BOOTSTRAP,
    FAIL_STORAGE_MOUNT,
    FAIL_AUTH_INIT,
    FAIL_USB_INIT,
    FAIL_EXECUTOR_INIT,
    FAIL_CONTROLS_INIT,
    FAIL_WIFI_START,
    FAIL_HTTP_START,
    FAIL_READY_INDICATOR
} failure_stage_t;

typedef struct {
    char calls[MAX_CALLS][CALL_BYTES];
    size_t call_count;
    provisioning_config_t provisioning;
    provisioning_bootstrap_t bootstrap;
    app_core_nvs_result_t nvs_result;
    failure_stage_t failure_stage;
    bool storage_owned;
    bool fail_wifi_stop;
    size_t cleanup_failure_logs;
    size_t manufacturing_logs;
    web_server_config_t observed_web;
} fixture_t;

static void record(fixture_t *fixture, const char *name) {
    TEST_CHECK(fixture != NULL);
    TEST_CHECK(name != NULL);
    TEST_CHECK(fixture->call_count < MAX_CALLS);
    TEST_CHECK(snprintf(fixture->calls[fixture->call_count], CALL_BYTES, "%s", name) >= 0);
    ++fixture->call_count;
}

static size_t count_call(const fixture_t *fixture, const char *name) {
    size_t count = 0U;
    for (size_t index = 0U; index < fixture->call_count; ++index) {
        if (strcmp(fixture->calls[index], name) == 0) {
            ++count;
        }
    }
    return count;
}

static void expect_calls(const fixture_t *fixture, const char *const *expected, size_t count) {
    TEST_CHECK_EQ_U64(count, fixture->call_count);
    for (size_t index = 0U; index < count; ++index) {
        TEST_CHECK_EQ_STRING(expected[index], fixture->calls[index]);
    }
}

static void reset_fixture(fixture_t *fixture, bool provisioned) {
    memset(fixture, 0, sizeof(*fixture));
    fixture->nvs_result = APP_CORE_NVS_OK;
    fixture->provisioning = (provisioning_config_t){
        .schema_version = APP_SCHEMA_VERSION,
        .revision = provisioned ? 4U : 0U,
        .credential_version = provisioned ? 2U : 0U,
        .provisioned = provisioned,
        .require_physical_confirmation = true,
        .always_select_package = true,
    };
    strcpy(fixture->provisioning.ap_ssid, "Macro Keyboard");
    strcpy(fixture->provisioning.ap_passphrase, "correct-horse-battery");
    fixture->provisioning.password_record.iterations = AUTH_PBKDF2_ITERATIONS;
    strcpy(fixture->bootstrap.device_id, "102030A0B0C0");
    strcpy(fixture->bootstrap.ap_ssid, "ESP32-Macro-A0B0C0");
    strcpy(fixture->bootstrap.ap_passphrase, "0665630870D7FE643BA4B540");
    strcpy(fixture->bootstrap.setup_code, "45175C9BB39D8BE5FC7EF773");
}

static app_error_code_t stage_result(const fixture_t *fixture, failure_stage_t stage) {
    return fixture->failure_stage == stage ? APP_ERROR_INTERNAL : APP_ERROR_NONE;
}

static app_core_nvs_result_t fake_nvs_init(void *context) {
    fixture_t *fixture = context;
    record(fixture, "nvs");
    return fixture->nvs_result;
}

static app_error_code_t fake_provisioning_init(void *context) {
    fixture_t *fixture = context;
    record(fixture, "provisioning_init");
    return stage_result(fixture, FAIL_PROVISIONING_INIT);
}

static app_error_code_t fake_provisioning_load(void *context,
                                               provisioning_config_t *out_configuration) {
    fixture_t *fixture = context;
    record(fixture, "provisioning_load");
    const app_error_code_t result = stage_result(fixture, FAIL_PROVISIONING_LOAD);
    if (result == APP_ERROR_NONE) {
        *out_configuration = fixture->provisioning;
    }
    return result;
}

static app_error_code_t fake_bootstrap(void *context, provisioning_bootstrap_t *out_bootstrap) {
    fixture_t *fixture = context;
    record(fixture, "bootstrap");
    const app_error_code_t result = stage_result(fixture, FAIL_BOOTSTRAP);
    if (result == APP_ERROR_NONE) {
        *out_bootstrap = fixture->bootstrap;
    }
    return result;
}

static app_error_code_t fake_storage_mount(void *context) {
    fixture_t *fixture = context;
    record(fixture, "storage_mount");
    const app_error_code_t result = stage_result(fixture, FAIL_STORAGE_MOUNT);
    if (result == APP_ERROR_NONE) {
        fixture->storage_owned = true;
    }
    return result;
}

static app_error_code_t fake_auth_init(void *context) {
    fixture_t *fixture = context;
    record(fixture, "auth_init");
    return stage_result(fixture, FAIL_AUTH_INIT);
}

static app_error_code_t fake_usb_init(void *context) {
    fixture_t *fixture = context;
    record(fixture, "usb_init");
    return stage_result(fixture, FAIL_USB_INIT);
}

static app_error_code_t fake_executor_init(void *context) {
    fixture_t *fixture = context;
    record(fixture, "executor_init");
    return stage_result(fixture, FAIL_EXECUTOR_INIT);
}

static app_error_code_t fake_controls_init(void *context) {
    fixture_t *fixture = context;
    record(fixture, "controls_init");
    return stage_result(fixture, FAIL_CONTROLS_INIT);
}

static app_error_code_t fake_wifi_start(void *context, const char *ssid, const char *passphrase) {
    fixture_t *fixture = context;
    TEST_CHECK(ssid != NULL);
    TEST_CHECK(passphrase != NULL);
    record(fixture, "wifi_start");
    return stage_result(fixture, FAIL_WIFI_START);
}

static app_error_code_t fake_http_start(void *context, const web_server_config_t *configuration) {
    fixture_t *fixture = context;
    record(fixture, "http_start");
    fixture->observed_web = *configuration;
    return stage_result(fixture, FAIL_HTTP_START);
}

static app_error_code_t fake_http_stop(void *context) {
    record(context, "http_stop");
    return APP_ERROR_NONE;
}

static app_error_code_t fake_wifi_stop(void *context) {
    fixture_t *fixture = context;
    record(fixture, "wifi_stop");
    return fixture->fail_wifi_stop ? APP_ERROR_IO : APP_ERROR_NONE;
}

#define DEFINE_STOP(name)                                                                          \
    static app_error_code_t fake_##name(void *context) {                                           \
        record(context, #name);                                                                    \
        return APP_ERROR_NONE;                                                                     \
    }

DEFINE_STOP(auth_deinit)
DEFINE_STOP(usb_deinit)
DEFINE_STOP(executor_deinit)
DEFINE_STOP(controls_deinit)
DEFINE_STOP(provisioning_deinit)
DEFINE_STOP(nvs_deinit)

static app_error_code_t fake_storage_unmount(void *context) {
    fixture_t *fixture = context;
    record(fixture, "storage_unmount");
    fixture->storage_owned = false;
    return APP_ERROR_NONE;
}

static bool false_owner(void *context) {
    (void)context;
    return false;
}

static bool storage_owner(void *context) {
    const fixture_t *fixture = context;
    return fixture->storage_owned;
}

static app_error_code_t fake_indicator(void *context, device_indicator_state_t state) {
    fixture_t *fixture = context;
    const char *name = "indicator_fatal";
    if (state == DEVICE_INDICATOR_BOOTING) {
        name = "indicator_booting";
    } else if (state == DEVICE_INDICATOR_READY) {
        name = "indicator_ready";
    }
    record(fixture, name);
    if (state == DEVICE_INDICATOR_BOOTING) {
        return stage_result(fixture, FAIL_BOOT_INDICATOR);
    }
    if (state == DEVICE_INDICATOR_READY) {
        return stage_result(fixture, FAIL_READY_INDICATOR);
    }
    return APP_ERROR_NONE;
}

static void fake_zero(void *context, void *memory, size_t length) {
    record(context, "secure_zero");
    memset(memory, 0, length);
}

static void fake_log(void *context, const app_core_log_event_t *event) {
    fixture_t *fixture = context;
    TEST_CHECK(event != NULL);
    if (event->type == APP_CORE_LOG_CLEANUP_FAILED) {
        ++fixture->cleanup_failure_logs;
    } else if (event->type == APP_CORE_LOG_MANUFACTURING_CREDENTIALS) {
        ++fixture->manufacturing_logs;
    }
}

static app_core_ops_t operations(fixture_t *fixture) {
    return (app_core_ops_t){
        .context = fixture,
        .nvs_init = fake_nvs_init,
        .provisioning_init = fake_provisioning_init,
        .provisioning_load = fake_provisioning_load,
        .bootstrap_derive = fake_bootstrap,
        .storage_mount = fake_storage_mount,
        .auth_init = fake_auth_init,
        .usb_init = fake_usb_init,
        .executor_init = fake_executor_init,
        .controls_init = fake_controls_init,
        .wifi_start = fake_wifi_start,
        .http_start = fake_http_start,
        .http_stop = fake_http_stop,
        .wifi_stop = fake_wifi_stop,
        .storage_unmount = fake_storage_unmount,
        .auth_deinit = fake_auth_deinit,
        .usb_deinit = fake_usb_deinit,
        .executor_deinit = fake_executor_deinit,
        .controls_deinit = fake_controls_deinit,
        .provisioning_deinit = fake_provisioning_deinit,
        .nvs_deinit = fake_nvs_deinit,
        .http_owns_resources = false_owner,
        .wifi_owns_resources = false_owner,
        .storage_owns_mount = storage_owner,
        .provisioning_owns_resources = false_owner,
        .set_indicator = fake_indicator,
        .secure_zero = fake_zero,
        .log_event = fake_log,
    };
}

static void test_normal_start_has_no_repository_stage(void) {
    fixture_t fixture;
    reset_fixture(&fixture, true);
    app_core_ops_t ops = operations(&fixture);
    const app_core_policy_t policy = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_core_sequence_start(&ops, &policy));
    static const char *const expected[] = {
        "indicator_booting",
        "nvs",
        "provisioning_init",
        "provisioning_load",
        "storage_mount",
        "auth_init",
        "usb_init",
        "executor_init",
        "controls_init",
        "wifi_start",
        "http_start",
        "secure_zero",
        "indicator_ready",
    };
    expect_calls(&fixture, expected, sizeof(expected) / sizeof(expected[0]));
    TEST_CHECK_EQ_INT(WEB_SERVER_MODE_NORMAL, fixture.observed_web.mode);
    TEST_CHECK(fixture.observed_web.login_enabled);
}

static void test_setup_start_excludes_usb_and_executor(void) {
    fixture_t fixture;
    reset_fixture(&fixture, false);
    app_core_ops_t ops = operations(&fixture);
    const app_core_policy_t policy = {.manufacturing_provisioning_enabled = true};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_core_sequence_start(&ops, &policy));
    TEST_CHECK_EQ_U64(0U, count_call(&fixture, "usb_init"));
    TEST_CHECK_EQ_U64(0U, count_call(&fixture, "executor_init"));
    TEST_CHECK_EQ_U64(1U, count_call(&fixture, "bootstrap"));
    TEST_CHECK_EQ_U64(1U, fixture.manufacturing_logs);
    TEST_CHECK_EQ_INT(WEB_SERVER_MODE_SETUP, fixture.observed_web.mode);
    TEST_CHECK(!fixture.observed_web.login_enabled);
}

static void expect_stage_failure(bool provisioned, failure_stage_t stage) {
    fixture_t fixture;
    reset_fixture(&fixture, provisioned);
    fixture.failure_stage = stage;
    app_core_ops_t ops = operations(&fixture);
    const app_core_policy_t policy = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, app_core_sequence_start(&ops, &policy));
    TEST_CHECK_EQ_U64(1U, count_call(&fixture, "indicator_fatal"));
    TEST_CHECK(count_call(&fixture, "secure_zero") >= 1U);
}

static void test_startup_failure_matrix(void) {
    expect_stage_failure(true, FAIL_BOOT_INDICATOR);
    expect_stage_failure(true, FAIL_PROVISIONING_INIT);
    expect_stage_failure(true, FAIL_PROVISIONING_LOAD);
    expect_stage_failure(true, FAIL_STORAGE_MOUNT);
    expect_stage_failure(true, FAIL_AUTH_INIT);
    expect_stage_failure(true, FAIL_USB_INIT);
    expect_stage_failure(true, FAIL_EXECUTOR_INIT);
    expect_stage_failure(true, FAIL_CONTROLS_INIT);
    expect_stage_failure(true, FAIL_WIFI_START);
    expect_stage_failure(true, FAIL_HTTP_START);
    expect_stage_failure(true, FAIL_READY_INDICATOR);
    expect_stage_failure(false, FAIL_BOOTSTRAP);
    expect_stage_failure(false, FAIL_AUTH_INIT);
    expect_stage_failure(false, FAIL_CONTROLS_INIT);
    expect_stage_failure(false, FAIL_WIFI_START);
    expect_stage_failure(false, FAIL_HTTP_START);
}

static void test_nvs_failure_cleanup(void) {
    fixture_t fixture;
    reset_fixture(&fixture, true);
    fixture.nvs_result = APP_CORE_NVS_NO_FREE_PAGES;
    app_core_ops_t ops = operations(&fixture);
    const app_core_policy_t policy = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, app_core_sequence_start(&ops, &policy));
    TEST_CHECK_EQ_U64(0U, count_call(&fixture, "nvs_deinit"));
    TEST_CHECK_EQ_U64(1U, count_call(&fixture, "indicator_fatal"));
}

static void test_cleanup_failure_is_reported(void) {
    fixture_t fixture;
    reset_fixture(&fixture, true);
    fixture.failure_stage = FAIL_HTTP_START;
    fixture.fail_wifi_stop = true;
    app_core_ops_t ops = operations(&fixture);
    const app_core_policy_t policy = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, app_core_sequence_start(&ops, &policy));
    TEST_CHECK_EQ_U64(1U, count_call(&fixture, "wifi_stop"));
    TEST_CHECK_EQ_U64(1U, fixture.cleanup_failure_logs);
}

static void test_invalid_inputs_and_operation_table(void) {
    fixture_t fixture;
    reset_fixture(&fixture, true);
    app_core_ops_t ops = operations(&fixture);
    const app_core_policy_t policy = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, app_core_sequence_start(NULL, &policy));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, app_core_sequence_start(&ops, NULL));

    app_core_ops_t invalid = ops;
    invalid.nvs_init = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, app_core_sequence_start(&invalid, &policy));
    invalid = ops;
    invalid.storage_mount = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, app_core_sequence_start(&invalid, &policy));
    invalid = ops;
    invalid.http_start = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, app_core_sequence_start(&invalid, &policy));
    invalid = ops;
    invalid.log_event = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, app_core_sequence_start(&invalid, &policy));
    TEST_CHECK_EQ_U64(0U, fixture.call_count);
}

static void test_nvs_mapping(void) {
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_core_map_nvs_result(APP_CORE_NVS_OK));
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         app_core_map_nvs_result(APP_CORE_NVS_NO_FREE_PAGES));
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         app_core_map_nvs_result(APP_CORE_NVS_NEW_VERSION_FOUND));
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE,
                         app_core_map_nvs_result(APP_CORE_NVS_OTHER_FAILURE));
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE,
                         app_core_map_nvs_result((app_core_nvs_result_t)99));
}

int main(void) {
    test_nvs_mapping();
    test_invalid_inputs_and_operation_table();
    test_normal_start_has_no_repository_stage();
    test_setup_start_excludes_usb_and_executor();
    test_startup_failure_matrix();
    test_nvs_failure_cleanup();
    test_cleanup_failure_is_reported();
    puts("app core tests passed");
    return EXIT_SUCCESS;
}
