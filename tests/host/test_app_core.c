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

typedef struct {
    char calls[MAX_CALLS][CALL_BYTES];
    size_t call_count;
    provisioning_config_t provisioning;
    provisioning_bootstrap_t bootstrap;
    app_error_code_t storage_result;
    app_error_code_t http_result;
    bool storage_owned;
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
    fixture->provisioning = (provisioning_config_t){
        .schema_version = APP_SCHEMA_VERSION,
        .revision = provisioned ? 4U : 0U,
        .credential_version = provisioned ? 2U : 0U,
        .provisioned = provisioned,
        .require_physical_confirmation = true,
        .always_select_package = true,
    };
    TEST_CHECK(snprintf(fixture->provisioning.ap_ssid,
                        sizeof(fixture->provisioning.ap_ssid), "%s", "Macro Keyboard") >= 0);
    TEST_CHECK(snprintf(fixture->provisioning.ap_passphrase,
                        sizeof(fixture->provisioning.ap_passphrase), "%s",
                        "correct-horse-battery") >= 0);
    fixture->provisioning.password_record.iterations = AUTH_PBKDF2_ITERATIONS;
    TEST_CHECK(snprintf(fixture->bootstrap.device_id, sizeof(fixture->bootstrap.device_id), "%s",
                        "102030A0B0C0") >= 0);
    TEST_CHECK(snprintf(fixture->bootstrap.ap_ssid, sizeof(fixture->bootstrap.ap_ssid), "%s",
                        "ESP32-Macro-A0B0C0") >= 0);
    TEST_CHECK(snprintf(fixture->bootstrap.ap_passphrase,
                        sizeof(fixture->bootstrap.ap_passphrase), "%s",
                        "0665630870D7FE643BA4B540") >= 0);
    TEST_CHECK(snprintf(fixture->bootstrap.setup_code, sizeof(fixture->bootstrap.setup_code), "%s",
                        "45175C9BB39D8BE5FC7EF773") >= 0);
}

static app_core_nvs_result_t fake_nvs_init(void *context) {
    record(context, "nvs");
    return APP_CORE_NVS_OK;
}

static app_error_code_t fake_provisioning_init(void *context) {
    record(context, "provisioning_init");
    return APP_ERROR_NONE;
}

static app_error_code_t fake_provisioning_load(void *context,
                                               provisioning_config_t *out_configuration) {
    fixture_t *fixture = context;
    record(fixture, "provisioning_load");
    *out_configuration = fixture->provisioning;
    return APP_ERROR_NONE;
}

static app_error_code_t fake_bootstrap(void *context,
                                       provisioning_bootstrap_t *out_bootstrap) {
    fixture_t *fixture = context;
    record(fixture, "bootstrap");
    *out_bootstrap = fixture->bootstrap;
    return APP_ERROR_NONE;
}

static app_error_code_t fake_storage_mount(void *context) {
    fixture_t *fixture = context;
    record(fixture, "storage_mount");
    if (fixture->storage_result == APP_ERROR_NONE) {
        fixture->storage_owned = true;
    }
    return fixture->storage_result;
}

#define DEFINE_INIT(name)                                                                          \
    static app_error_code_t fake_##name(void *context) {                                           \
        record(context, #name);                                                                    \
        return APP_ERROR_NONE;                                                                     \
    }

DEFINE_INIT(auth_init)
DEFINE_INIT(usb_init)
DEFINE_INIT(executor_init)
DEFINE_INIT(controls_init)

static app_error_code_t fake_wifi_start(void *context, const char *ssid, const char *passphrase) {
    fixture_t *fixture = context;
    TEST_CHECK(ssid != NULL);
    TEST_CHECK(passphrase != NULL);
    record(fixture, "wifi_start");
    return APP_ERROR_NONE;
}

static app_error_code_t fake_http_start(void *context,
                                        const web_server_config_t *configuration) {
    fixture_t *fixture = context;
    record(fixture, "http_start");
    fixture->observed_web = *configuration;
    return fixture->http_result;
}

#define DEFINE_STOP(name)                                                                          \
    static app_error_code_t fake_##name(void *context) {                                           \
        record(context, #name);                                                                    \
        return APP_ERROR_NONE;                                                                     \
    }

DEFINE_STOP(http_stop)
DEFINE_STOP(wifi_stop)
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
    record(fixture, state == DEVICE_INDICATOR_BOOTING
                        ? "indicator_booting"
                        : state == DEVICE_INDICATOR_READY ? "indicator_ready" : "indicator_fatal");
    return APP_ERROR_NONE;
}

static void fake_zero(void *context, void *memory, size_t length) {
    record(context, "secure_zero");
    memset(memory, 0, length);
}

static void fake_log(void *context, const app_core_log_event_t *event) {
    (void)context;
    TEST_CHECK(event != NULL);
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
        "indicator_booting", "nvs",          "provisioning_init", "provisioning_load",
        "storage_mount",     "auth_init",    "usb_init",          "executor_init",
        "controls_init",     "wifi_start",  "http_start",        "secure_zero",
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
    const app_core_policy_t policy = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_core_sequence_start(&ops, &policy));
    TEST_CHECK_EQ_U64(0U, count_call(&fixture, "usb_init"));
    TEST_CHECK_EQ_U64(0U, count_call(&fixture, "executor_init"));
    TEST_CHECK_EQ_U64(1U, count_call(&fixture, "bootstrap"));
    TEST_CHECK_EQ_INT(WEB_SERVER_MODE_SETUP, fixture.observed_web.mode);
    TEST_CHECK(!fixture.observed_web.login_enabled);
}

static void test_failure_cleans_owned_services(void) {
    fixture_t fixture;
    reset_fixture(&fixture, true);
    fixture.http_result = APP_ERROR_INTERNAL;
    app_core_ops_t ops = operations(&fixture);
    const app_core_policy_t policy = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, app_core_sequence_start(&ops, &policy));
    TEST_CHECK_EQ_U64(1U, count_call(&fixture, "wifi_stop"));
    TEST_CHECK_EQ_U64(1U, count_call(&fixture, "controls_deinit"));
    TEST_CHECK_EQ_U64(1U, count_call(&fixture, "executor_deinit"));
    TEST_CHECK_EQ_U64(1U, count_call(&fixture, "usb_deinit"));
    TEST_CHECK_EQ_U64(1U, count_call(&fixture, "auth_deinit"));
    TEST_CHECK_EQ_U64(1U, count_call(&fixture, "storage_unmount"));
    TEST_CHECK_EQ_U64(1U, count_call(&fixture, "provisioning_deinit"));
    TEST_CHECK_EQ_U64(1U, count_call(&fixture, "nvs_deinit"));
    TEST_CHECK_EQ_U64(1U, count_call(&fixture, "indicator_fatal"));
}

static void test_invalid_inputs(void) {
    fixture_t fixture;
    reset_fixture(&fixture, true);
    app_core_ops_t ops = operations(&fixture);
    const app_core_policy_t policy = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, app_core_sequence_start(NULL, &policy));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, app_core_sequence_start(&ops, NULL));
    ops.storage_mount = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, app_core_sequence_start(&ops, &policy));
    TEST_CHECK_EQ_U64(0U, fixture.call_count);
}

static void test_nvs_mapping(void) {
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_core_map_nvs_result(APP_CORE_NVS_OK));
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         app_core_map_nvs_result(APP_CORE_NVS_NO_FREE_PAGES));
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE,
                         app_core_map_nvs_result(APP_CORE_NVS_OTHER_FAILURE));
}

int main(void) {
    test_nvs_mapping();
    test_invalid_inputs();
    test_normal_start_has_no_repository_stage();
    test_setup_start_excludes_usb_and_executor();
    test_failure_cleans_owned_services();
    puts("app core tests passed");
    return EXIT_SUCCESS;
}
