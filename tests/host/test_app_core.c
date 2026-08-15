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
    FAIL_SETTINGS_INIT,
    FAIL_SETTINGS_READ,
    FAIL_BOOTSTRAP,
    FAIL_SETUP_CODE,
    FAIL_SETUP_CODE_DISPLAY,
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
    app_v2_device_settings_t settings;
    provisioning_bootstrap_t bootstrap;
    char setup_code[APP_V2_SETUP_CODE_BUFFER_BYTES];
    app_core_nvs_result_t nvs_result;
    app_error_code_t settings_init_result;
    failure_stage_t failure_stage;
    bool storage_owned;
    bool fail_wifi_stop;
    size_t cleanup_failure_logs;
    size_t setup_code_displays;
    char displayed_setup_code[APP_V2_SETUP_CODE_BUFFER_BYTES];
    web_server_config_t observed_web;
    bool observed_station_configured;
    char observed_ap_ssid[APP_V2_WIFI_SSID_MAX_BYTES + 1U];
    char observed_station_ssid[APP_V2_WIFI_SSID_MAX_BYTES + 1U];
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

static void expect_call(const fixture_t *fixture, size_t index, const char *expected) {
    TEST_CHECK(index < fixture->call_count);
    TEST_CHECK_EQ_STRING(expected, fixture->calls[index]);
}

static void reset_fixture(fixture_t *fixture, bool provisioned) {
    memset(fixture, 0, sizeof(*fixture));
    fixture->nvs_result = APP_CORE_NVS_OK;
    fixture->settings.provisioned = provisioned;
    fixture->settings.credential_version = provisioned ? APP_V2_CREDENTIAL_VERSION : 0U;
    fixture->settings.password_algorithm_version =
        provisioned ? APP_V2_PASSWORD_ALGORITHM_VERSION : 0U;
    fixture->settings.password_iterations = provisioned ? AUTH_PBKDF2_ITERATIONS : 0U;
    fixture->settings.require_serial_confirmation = true;
    memcpy(fixture->settings.device_name, "ESP32 Macro Keyboard", sizeof("ESP32 Macro Keyboard"));
    memcpy(fixture->settings.ap_ssid, "Macro Keyboard", sizeof("Macro Keyboard"));
    memcpy(fixture->settings.ap_passphrase, "correct-horse-battery",
           sizeof("correct-horse-battery"));
    if (provisioned) {
        fixture->settings.station_configured = true;
        memcpy(fixture->settings.station_ssid, "Office WiFi", sizeof("Office WiFi"));
        memcpy(fixture->settings.station_passphrase, "station-secret", sizeof("station-secret"));
    }
    memcpy(fixture->bootstrap.ap_ssid, "ESP32-Macro-A0B0C0", sizeof("ESP32-Macro-A0B0C0"));
    memcpy(fixture->bootstrap.ap_passphrase, "0665630870D7FE643BA4B540",
           sizeof("0665630870D7FE643BA4B540"));
    memcpy(fixture->setup_code, "12345678", sizeof("12345678"));
}

static app_error_code_t stage_result(const fixture_t *fixture, failure_stage_t stage) {
    return fixture->failure_stage == stage ? APP_ERROR_INTERNAL : APP_ERROR_NONE;
}

static app_core_nvs_result_t fake_nvs_init(void *context) {
    fixture_t *fixture = context;
    record(fixture, "nvs");
    return fixture->nvs_result;
}

static app_error_code_t fake_settings_init(void *context) {
    fixture_t *fixture = context;
    record(fixture, "settings_init");
    if (fixture->settings_init_result != APP_ERROR_NONE) {
        return fixture->settings_init_result;
    }
    return stage_result(fixture, FAIL_SETTINGS_INIT);
}

static app_error_code_t fake_settings_read(void *context, app_v2_device_settings_t *out_settings) {
    fixture_t *fixture = context;
    record(fixture, "settings_read");
    const app_error_code_t result = stage_result(fixture, FAIL_SETTINGS_READ);
    if (result == APP_ERROR_NONE) {
        *out_settings = fixture->settings;
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

static app_error_code_t fake_setup_code(void *context,
                                        char out_code[APP_V2_SETUP_CODE_BUFFER_BYTES]) {
    fixture_t *fixture = context;
    record(fixture, "setup_code");
    const app_error_code_t result = stage_result(fixture, FAIL_SETUP_CODE);
    if (result == APP_ERROR_NONE) {
        memcpy(out_code, fixture->setup_code, sizeof(fixture->setup_code));
    }
    return result;
}

static app_error_code_t fake_show_setup_code(void *context, const char *setup_code) {
    fixture_t *fixture = context;
    record(fixture, "show_setup_code");
    if (setup_code == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const app_error_code_t result = stage_result(fixture, FAIL_SETUP_CODE_DISPLAY);
    if (result == APP_ERROR_NONE) {
        ++fixture->setup_code_displays;
        TEST_CHECK(snprintf(fixture->displayed_setup_code, sizeof(fixture->displayed_setup_code),
                            "%s", setup_code) >= 0);
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

static app_error_code_t fake_wifi_start(void *context,
                                        const app_core_wifi_configuration_t *configuration) {
    fixture_t *fixture = context;
    TEST_CHECK(configuration != NULL);
    TEST_CHECK(configuration->ap_ssid != NULL);
    TEST_CHECK(configuration->ap_passphrase != NULL);
    record(fixture, "wifi_start");
    fixture->observed_station_configured = configuration->station_configured;
    TEST_CHECK(snprintf(fixture->observed_ap_ssid, sizeof(fixture->observed_ap_ssid), "%s",
                        configuration->ap_ssid) >= 0);
    if (configuration->station_configured) {
        TEST_CHECK(configuration->station_ssid != NULL);
        TEST_CHECK(configuration->station_passphrase != NULL);
        TEST_CHECK(snprintf(fixture->observed_station_ssid, sizeof(fixture->observed_station_ssid),
                            "%s", configuration->station_ssid) >= 0);
    }
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
DEFINE_STOP(settings_deinit)
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
    }
}

static app_core_ops_t operations(fixture_t *fixture) {
    return (app_core_ops_t){
        .context = fixture,
        .nvs_init = fake_nvs_init,
        .settings_init = fake_settings_init,
        .settings_read = fake_settings_read,
        .bootstrap_derive = fake_bootstrap,
        .setup_code_generate = fake_setup_code,
        .show_setup_code = fake_show_setup_code,
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
        .settings_deinit = fake_settings_deinit,
        .nvs_deinit = fake_nvs_deinit,
        .http_owns_resources = false_owner,
        .wifi_owns_resources = false_owner,
        .storage_owns_mount = storage_owner,
        .set_indicator = fake_indicator,
        .secure_zero = fake_zero,
        .log_event = fake_log,
    };
}

static void test_normal_start_uses_v2_settings(void) {
    fixture_t fixture;
    reset_fixture(&fixture, true);
    app_core_ops_t ops = operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_core_sequence_start(&ops));
    TEST_CHECK_EQ_U64(13U, fixture.call_count);
    expect_call(&fixture, 0U, "indicator_booting");
    expect_call(&fixture, 1U, "nvs");
    expect_call(&fixture, 2U, "settings_init");
    expect_call(&fixture, 3U, "settings_read");
    expect_call(&fixture, 4U, "storage_mount");
    expect_call(&fixture, 5U, "auth_init");
    expect_call(&fixture, 6U, "usb_init");
    expect_call(&fixture, 7U, "executor_init");
    expect_call(&fixture, 8U, "controls_init");
    expect_call(&fixture, 9U, "wifi_start");
    expect_call(&fixture, 10U, "http_start");
    expect_call(&fixture, 11U, "secure_zero");
    expect_call(&fixture, 12U, "indicator_ready");
    TEST_CHECK_EQ_INT(WEB_SERVER_MODE_NORMAL, fixture.observed_web.mode);
    TEST_CHECK(fixture.observed_web.login_enabled);
    TEST_CHECK(fixture.observed_web.require_physical_confirmation);
    TEST_CHECK_EQ_STRING("Macro Keyboard", fixture.observed_ap_ssid);
    TEST_CHECK(fixture.observed_station_configured);
    TEST_CHECK_EQ_STRING("Office WiFi", fixture.observed_station_ssid);
    TEST_CHECK_EQ_U64(AUTH_PBKDF2_ITERATIONS, fixture.observed_web.password_record.iterations);
    TEST_CHECK_EQ_U64(0U, count_call(&fixture, "bootstrap"));
    TEST_CHECK_EQ_U64(0U, count_call(&fixture, "setup_code"));
}

static void test_setup_start_uses_bootstrap_ap_and_random_code(void) {
    fixture_t fixture;
    reset_fixture(&fixture, false);
    app_core_ops_t ops = operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_core_sequence_start(&ops));
    TEST_CHECK_EQ_U64(0U, count_call(&fixture, "usb_init"));
    TEST_CHECK_EQ_U64(0U, count_call(&fixture, "executor_init"));
    TEST_CHECK_EQ_U64(0U, count_call(&fixture, "controls_init"));
    TEST_CHECK_EQ_U64(1U, count_call(&fixture, "bootstrap"));
    TEST_CHECK_EQ_U64(1U, count_call(&fixture, "setup_code"));
    TEST_CHECK_EQ_U64(1U, fixture.setup_code_displays);
    TEST_CHECK_EQ_STRING("12345678", fixture.displayed_setup_code);
    TEST_CHECK_EQ_INT(WEB_SERVER_MODE_SETUP, fixture.observed_web.mode);
    TEST_CHECK(!fixture.observed_web.login_enabled);
    TEST_CHECK_EQ_STRING("ESP32 Macro Keyboard", fixture.observed_web.setup_device_name);
    TEST_CHECK_EQ_STRING("12345678", fixture.observed_web.setup_code);
    TEST_CHECK_EQ_STRING("ESP32-Macro-A0B0C0", fixture.observed_ap_ssid);
    TEST_CHECK(!fixture.observed_station_configured);
}

static void expect_stage_failure(bool provisioned, failure_stage_t stage) {
    fixture_t fixture;
    reset_fixture(&fixture, provisioned);
    fixture.failure_stage = stage;
    app_core_ops_t ops = operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, app_core_sequence_start(&ops));
    TEST_CHECK_EQ_U64(1U, count_call(&fixture, "indicator_fatal"));
    TEST_CHECK(count_call(&fixture, "secure_zero") >= 1U);
}

static void test_startup_failure_matrix(void) {
    expect_stage_failure(true, FAIL_BOOT_INDICATOR);
    expect_stage_failure(true, FAIL_SETTINGS_INIT);
    expect_stage_failure(true, FAIL_SETTINGS_READ);
    expect_stage_failure(true, FAIL_STORAGE_MOUNT);
    expect_stage_failure(true, FAIL_AUTH_INIT);
    expect_stage_failure(true, FAIL_USB_INIT);
    expect_stage_failure(true, FAIL_EXECUTOR_INIT);
    expect_stage_failure(true, FAIL_CONTROLS_INIT);
    expect_stage_failure(true, FAIL_WIFI_START);
    expect_stage_failure(true, FAIL_HTTP_START);
    expect_stage_failure(true, FAIL_READY_INDICATOR);
    expect_stage_failure(false, FAIL_BOOTSTRAP);
    expect_stage_failure(false, FAIL_SETUP_CODE);
    expect_stage_failure(false, FAIL_SETUP_CODE_DISPLAY);
    expect_stage_failure(false, FAIL_AUTH_INIT);
    expect_stage_failure(false, FAIL_WIFI_START);
    expect_stage_failure(false, FAIL_HTTP_START);
}

static void test_factory_reset_pending_blocks_all_runtime_startup(void) {
    fixture_t fixture;
    reset_fixture(&fixture, true);
    fixture.settings_init_result = APP_ERROR_RESET_RECOVERY_REQUIRED;
    app_core_ops_t ops = operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_RESET_RECOVERY_REQUIRED, app_core_sequence_start(&ops));
    TEST_CHECK_EQ_U64(1U, count_call(&fixture, "settings_init"));
    TEST_CHECK_EQ_U64(0U, count_call(&fixture, "settings_read"));
    TEST_CHECK_EQ_U64(0U, count_call(&fixture, "storage_mount"));
    TEST_CHECK_EQ_U64(0U, count_call(&fixture, "auth_init"));
    TEST_CHECK_EQ_U64(0U, count_call(&fixture, "usb_init"));
    TEST_CHECK_EQ_U64(0U, count_call(&fixture, "executor_init"));
    TEST_CHECK_EQ_U64(0U, count_call(&fixture, "controls_init"));
    TEST_CHECK_EQ_U64(0U, count_call(&fixture, "wifi_start"));
    TEST_CHECK_EQ_U64(0U, count_call(&fixture, "http_start"));
    TEST_CHECK_EQ_U64(1U, count_call(&fixture, "nvs_deinit"));
    TEST_CHECK_EQ_U64(1U, count_call(&fixture, "indicator_fatal"));
}

static void test_nvs_failure_cleanup(void) {
    fixture_t fixture;
    reset_fixture(&fixture, true);
    fixture.nvs_result = APP_CORE_NVS_NO_FREE_PAGES;
    app_core_ops_t ops = operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, app_core_sequence_start(&ops));
    TEST_CHECK_EQ_U64(0U, count_call(&fixture, "nvs_deinit"));
    TEST_CHECK_EQ_U64(1U, count_call(&fixture, "indicator_fatal"));
}

static void test_cleanup_failure_is_reported(void) {
    fixture_t fixture;
    reset_fixture(&fixture, true);
    fixture.failure_stage = FAIL_HTTP_START;
    fixture.fail_wifi_stop = true;
    app_core_ops_t ops = operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, app_core_sequence_start(&ops));
    TEST_CHECK_EQ_U64(1U, count_call(&fixture, "wifi_stop"));
    TEST_CHECK_EQ_U64(1U, fixture.cleanup_failure_logs);
}

static void test_invalid_inputs_and_operation_table(void) {
    fixture_t fixture;
    reset_fixture(&fixture, true);
    app_core_ops_t ops = operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, app_core_sequence_start(NULL));

    app_core_ops_t invalid = ops;
    invalid.nvs_init = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, app_core_sequence_start(&invalid));
    invalid = ops;
    invalid.settings_read = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, app_core_sequence_start(&invalid));
    invalid = ops;
    invalid.setup_code_generate = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, app_core_sequence_start(&invalid));
    invalid = ops;
    invalid.http_start = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, app_core_sequence_start(&invalid));
    invalid = ops;
    invalid.log_event = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, app_core_sequence_start(&invalid));
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
    test_normal_start_uses_v2_settings();
    test_setup_start_uses_bootstrap_ap_and_random_code();
    test_startup_failure_matrix();
    test_factory_reset_pending_blocks_all_runtime_startup();
    test_nvs_failure_cleanup();
    test_cleanup_failure_is_reported();
    puts("app core tests passed");
    return EXIT_SUCCESS;
}
