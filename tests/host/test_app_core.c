#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_core_sequence.h"
#include "fake_call_log.h"
#include "test_assert.h"

#define RECORDED_LOG_CAPACITY 64U

typedef struct {
    app_core_log_type_t type;
    app_error_code_t primary_error;
    app_error_code_t cleanup_error;
    bool cleanup_incomplete;
    uint32_t operation_id;
    char stage[32U];
    char ssid[WIFI_AP_SSID_MAX_BYTES + 1U];
    char ap_passphrase[WIFI_AP_PASSPHRASE_MAX_BYTES + 1U];
    char setup_code[PROVISIONING_SETUP_SECRET_BUFFER_BYTES];
} recorded_log_t;

typedef struct {
    fake_call_log_t calls;
    app_core_nvs_result_t nvs_result;
    app_error_code_t provisioning_init_result;
    app_error_code_t provisioning_load_result;
    app_error_code_t bootstrap_result;
    app_error_code_t storage_mount_result;
    app_error_code_t storage_recover_result;
    app_error_code_t repository_result;
    app_error_code_t auth_result;
    app_error_code_t usb_result;
    app_error_code_t executor_result;
    app_error_code_t controls_result;
    app_error_code_t wifi_result;
    app_error_code_t http_result;
    app_error_code_t http_stop_result;
    app_error_code_t wifi_stop_result;
    app_error_code_t storage_unmount_result;
    app_error_code_t repository_deinit_result;
    app_error_code_t auth_deinit_result;
    app_error_code_t usb_deinit_result;
    app_error_code_t executor_deinit_result;
    app_error_code_t controls_deinit_result;
    app_error_code_t provisioning_deinit_result;
    app_error_code_t nvs_deinit_result;
    bool http_owns_resources;
    bool wifi_owns_resources;
    bool storage_owns_mount;
    bool provisioning_owns_resources;
    device_indicator_state_t indicator_failure_state;
    app_error_code_t indicator_failure_result;
    provisioning_config_t provisioning;
    provisioning_bootstrap_t bootstrap;
    web_server_config_t observed_web_configuration;
    char wifi_ssid[WIFI_AP_SSID_MAX_BYTES + 1U];
    char wifi_passphrase[WIFI_AP_PASSPHRASE_MAX_BYTES + 1U];
    size_t secure_zero_count;
    recorded_log_t logs[RECORDED_LOG_CAPACITY];
    size_t log_count;
} app_core_fixture_t;

static void copy_text(char *destination, size_t destination_size, const char *source) {
    TEST_CHECK(destination != NULL);
    TEST_CHECK(destination_size > 0U);
    if (source == NULL) {
        destination[0] = '\0';
        return;
    }
    const int written = snprintf(destination, destination_size, "%s", source);
    TEST_CHECK(written >= 0);
    TEST_CHECK((size_t)written < destination_size);
}

static const char *indicator_call_name(device_indicator_state_t indicator) {
    switch (indicator) {
    case DEVICE_INDICATOR_BOOTING:
        return "indicator_booting";
    case DEVICE_INDICATOR_READY:
        return "indicator_ready";
    case DEVICE_INDICATOR_EXECUTING:
        return "indicator_executing";
    case DEVICE_INDICATOR_DEGRADED:
        return "indicator_degraded";
    case DEVICE_INDICATOR_FATAL:
        return "indicator_fatal";
    default:
        return "indicator_unknown";
    }
}

static void record_call(app_core_fixture_t *fixture, const char *name) {
    TEST_CHECK(fixture != NULL);
    TEST_CHECK(name != NULL);
    TEST_CHECK(!fake_call_log_record(&fixture->calls, name, 0U, 0U));
}

static size_t call_count(const app_core_fixture_t *fixture, const char *name) {
    size_t count = 0U;
    for (size_t index = 0U; index < fixture->calls.call_count; ++index) {
        const fake_call_t *call = fake_call_log_at(&fixture->calls, index);
        if (call != NULL && strcmp(call->name, name) == 0) {
            ++count;
        }
    }
    return count;
}

static size_t first_call_index(const app_core_fixture_t *fixture, const char *name) {
    for (size_t index = 0U; index < fixture->calls.call_count; ++index) {
        const fake_call_t *call = fake_call_log_at(&fixture->calls, index);
        if (call != NULL && strcmp(call->name, name) == 0) {
            return index;
        }
    }
    return SIZE_MAX;
}

static void assert_order(const app_core_fixture_t *fixture, const char *before, const char *after) {
    const size_t before_index = first_call_index(fixture, before);
    const size_t after_index = first_call_index(fixture, after);
    TEST_CHECK(before_index != SIZE_MAX);
    TEST_CHECK(after_index != SIZE_MAX);
    TEST_CHECK(before_index < after_index);
}

static void initialize_bootstrap(provisioning_bootstrap_t *bootstrap) {
    TEST_CHECK_EQ_INT(
        12, snprintf(bootstrap->device_id, sizeof(bootstrap->device_id), "%s", "102030A0B0C0"));
    TEST_CHECK_EQ_INT(
        18, snprintf(bootstrap->ap_ssid, sizeof(bootstrap->ap_ssid), "%s", "ESP32-Macro-A0B0C0"));
    TEST_CHECK_EQ_INT(24, snprintf(bootstrap->ap_passphrase, sizeof(bootstrap->ap_passphrase), "%s",
                                   "0665630870D7FE643BA4B540"));
    TEST_CHECK_EQ_INT(24, snprintf(bootstrap->setup_code, sizeof(bootstrap->setup_code), "%s",
                                   "45175C9BB39D8BE5FC7EF773"));
}

static void configure_normal_provisioning(provisioning_config_t *configuration) {
    *configuration = (provisioning_config_t){
        .schema_version = APP_SCHEMA_VERSION,
        .revision = 4U,
        .credential_version = 2U,
        .provisioned = true,
        .require_physical_confirmation = true,
        .always_select_package = true,
    };
    TEST_CHECK_EQ_INT(14, snprintf(configuration->ap_ssid, sizeof(configuration->ap_ssid), "%s",
                                   "Macro Keyboard"));
    TEST_CHECK_EQ_INT(21,
                      snprintf(configuration->ap_passphrase, sizeof(configuration->ap_passphrase),
                               "%s", "correct-horse-battery"));
    memset(configuration->password_record.salt, 0x11, sizeof(configuration->password_record.salt));
    memset(configuration->password_record.hash, 0x22, sizeof(configuration->password_record.hash));
    configuration->password_record.iterations = AUTH_PBKDF2_ITERATIONS;
}

static void reset_fixture(app_core_fixture_t *fixture) {
    TEST_CHECK(fixture != NULL);
    memset(fixture, 0, sizeof(*fixture));
    fake_call_log_reset(&fixture->calls);
    fixture->nvs_result = APP_CORE_NVS_OK;
    fixture->indicator_failure_state = (device_indicator_state_t)-1;
    fixture->indicator_failure_result = APP_ERROR_INTERNAL;
    fixture->provisioning = (provisioning_config_t){
        .schema_version = APP_SCHEMA_VERSION,
        .revision = 0U,
        .credential_version = 0U,
        .provisioned = false,
        .require_physical_confirmation = true,
        .always_select_package = true,
    };
    initialize_bootstrap(&fixture->bootstrap);
}

static app_core_nvs_result_t fake_nvs_init(void *context) {
    app_core_fixture_t *fixture = context;
    record_call(fixture, "nvs");
    return fixture->nvs_result;
}

static app_error_code_t fake_provisioning_init(void *context) {
    app_core_fixture_t *fixture = context;
    record_call(fixture, "provisioning_init");
    return fixture->provisioning_init_result;
}

static app_error_code_t fake_provisioning_load(void *context,
                                               provisioning_config_t *out_configuration) {
    app_core_fixture_t *fixture = context;
    record_call(fixture, "provisioning_load");
    if (fixture->provisioning_load_result == APP_ERROR_NONE) {
        *out_configuration = fixture->provisioning;
    }
    return fixture->provisioning_load_result;
}

static app_error_code_t fake_bootstrap_derive(void *context,
                                              provisioning_bootstrap_t *out_bootstrap) {
    app_core_fixture_t *fixture = context;
    record_call(fixture, "bootstrap_derive");
    if (fixture->bootstrap_result == APP_ERROR_NONE) {
        *out_bootstrap = fixture->bootstrap;
    }
    return fixture->bootstrap_result;
}

static app_error_code_t fake_storage_mount(void *context) {
    app_core_fixture_t *fixture = context;
    record_call(fixture, "storage_mount");
    return fixture->storage_mount_result;
}

static app_error_code_t fake_storage_recover(void *context) {
    app_core_fixture_t *fixture = context;
    record_call(fixture, "storage_recover");
    return fixture->storage_recover_result;
}

static app_error_code_t fake_repository_init(void *context) {
    app_core_fixture_t *fixture = context;
    record_call(fixture, "repository_init");
    return fixture->repository_result;
}

static app_error_code_t fake_auth_init(void *context) {
    app_core_fixture_t *fixture = context;
    record_call(fixture, "auth_init");
    return fixture->auth_result;
}

static app_error_code_t fake_usb_init(void *context) {
    app_core_fixture_t *fixture = context;
    record_call(fixture, "usb_init");
    return fixture->usb_result;
}

static app_error_code_t fake_executor_init(void *context) {
    app_core_fixture_t *fixture = context;
    record_call(fixture, "executor_init");
    return fixture->executor_result;
}

static app_error_code_t fake_controls_init(void *context) {
    app_core_fixture_t *fixture = context;
    record_call(fixture, "controls_init");
    return fixture->controls_result;
}

static app_error_code_t fake_wifi_start(void *context, const char *ssid, const char *passphrase) {
    app_core_fixture_t *fixture = context;
    record_call(fixture, "wifi_start");
    copy_text(fixture->wifi_ssid, sizeof(fixture->wifi_ssid), ssid);
    copy_text(fixture->wifi_passphrase, sizeof(fixture->wifi_passphrase), passphrase);
    return fixture->wifi_result;
}

static app_error_code_t fake_http_start(void *context, const web_server_config_t *configuration) {
    app_core_fixture_t *fixture = context;
    record_call(fixture, "http_start");
    TEST_CHECK(configuration != NULL);
    fixture->observed_web_configuration = *configuration;
    return fixture->http_result;
}

#define DEFINE_TEARDOWN_FAKE(name, field)                                                          \
    static app_error_code_t fake_##name(void *context) {                                           \
        app_core_fixture_t *fixture = context;                                                     \
        record_call(fixture, #name);                                                               \
        return fixture->field;                                                                     \
    }

DEFINE_TEARDOWN_FAKE(http_stop, http_stop_result)
DEFINE_TEARDOWN_FAKE(wifi_stop, wifi_stop_result)
DEFINE_TEARDOWN_FAKE(storage_unmount, storage_unmount_result)
DEFINE_TEARDOWN_FAKE(repository_deinit, repository_deinit_result)
DEFINE_TEARDOWN_FAKE(auth_deinit, auth_deinit_result)
DEFINE_TEARDOWN_FAKE(usb_deinit, usb_deinit_result)
DEFINE_TEARDOWN_FAKE(executor_deinit, executor_deinit_result)
DEFINE_TEARDOWN_FAKE(controls_deinit, controls_deinit_result)
DEFINE_TEARDOWN_FAKE(provisioning_deinit, provisioning_deinit_result)
DEFINE_TEARDOWN_FAKE(nvs_deinit, nvs_deinit_result)

#undef DEFINE_TEARDOWN_FAKE

static bool fake_http_owns_resources(void *context) {
    const app_core_fixture_t *fixture = context;
    return fixture->http_owns_resources;
}

static bool fake_wifi_owns_resources(void *context) {
    const app_core_fixture_t *fixture = context;
    return fixture->wifi_owns_resources;
}

static bool fake_storage_owns_mount(void *context) {
    const app_core_fixture_t *fixture = context;
    return fixture->storage_owns_mount;
}

static bool fake_provisioning_owns_resources(void *context) {
    const app_core_fixture_t *fixture = context;
    return fixture->provisioning_owns_resources;
}

static app_error_code_t fake_set_indicator(void *context, device_indicator_state_t indicator) {
    app_core_fixture_t *fixture = context;
    record_call(fixture, indicator_call_name(indicator));
    return indicator == fixture->indicator_failure_state ? fixture->indicator_failure_result
                                                         : APP_ERROR_NONE;
}

static void fake_secure_zero(void *context, void *memory, size_t length) {
    app_core_fixture_t *fixture = context;
    record_call(fixture, "secure_zero");
    TEST_CHECK(memory != NULL || length == 0U);
    if (memory != NULL) {
        memset(memory, 0, length);
    }
    ++fixture->secure_zero_count;
}

static void fake_log_event(void *context, const app_core_log_event_t *event) {
    app_core_fixture_t *fixture = context;
    TEST_CHECK(event != NULL);
    TEST_CHECK(fixture->log_count < RECORDED_LOG_CAPACITY);
    recorded_log_t *record = &fixture->logs[fixture->log_count++];
    memset(record, 0, sizeof(*record));
    record->type = event->type;
    record->primary_error = event->primary_error;
    record->cleanup_error = event->cleanup_error;
    record->cleanup_incomplete = event->cleanup_incomplete;
    record->operation_id = event->operation_id;
    copy_text(record->stage, sizeof(record->stage), event->stage);
    copy_text(record->ssid, sizeof(record->ssid), event->ssid);
    copy_text(record->ap_passphrase, sizeof(record->ap_passphrase), event->ap_passphrase);
    copy_text(record->setup_code, sizeof(record->setup_code), event->setup_code);
}

static app_core_ops_t make_operations(app_core_fixture_t *fixture) {
    return (app_core_ops_t){
        .context = fixture,
        .nvs_init = fake_nvs_init,
        .provisioning_init = fake_provisioning_init,
        .provisioning_load = fake_provisioning_load,
        .bootstrap_derive = fake_bootstrap_derive,
        .storage_mount = fake_storage_mount,
        .storage_recover = fake_storage_recover,
        .repository_init = fake_repository_init,
        .auth_init = fake_auth_init,
        .usb_init = fake_usb_init,
        .executor_init = fake_executor_init,
        .controls_init = fake_controls_init,
        .wifi_start = fake_wifi_start,
        .http_start = fake_http_start,
        .http_stop = fake_http_stop,
        .wifi_stop = fake_wifi_stop,
        .storage_unmount = fake_storage_unmount,
        .repository_deinit = fake_repository_deinit,
        .auth_deinit = fake_auth_deinit,
        .usb_deinit = fake_usb_deinit,
        .executor_deinit = fake_executor_deinit,
        .controls_deinit = fake_controls_deinit,
        .provisioning_deinit = fake_provisioning_deinit,
        .nvs_deinit = fake_nvs_deinit,
        .http_owns_resources = fake_http_owns_resources,
        .wifi_owns_resources = fake_wifi_owns_resources,
        .storage_owns_mount = fake_storage_owns_mount,
        .provisioning_owns_resources = fake_provisioning_owns_resources,
        .set_indicator = fake_set_indicator,
        .secure_zero = fake_secure_zero,
        .log_event = fake_log_event,
    };
}

static app_core_policy_t production_policy(void) {
    return (app_core_policy_t){
        .manufacturing_provisioning_enabled = false,
    };
}

static size_t log_count(const app_core_fixture_t *fixture, app_core_log_type_t type) {
    size_t count = 0U;
    for (size_t index = 0U; index < fixture->log_count; ++index) {
        if (fixture->logs[index].type == type) {
            ++count;
        }
    }
    return count;
}

static const recorded_log_t *first_log(const app_core_fixture_t *fixture,
                                       app_core_log_type_t type) {
    for (size_t index = 0U; index < fixture->log_count; ++index) {
        if (fixture->logs[index].type == type) {
            return &fixture->logs[index];
        }
    }
    return NULL;
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

static void test_invalid_arguments_and_missing_callbacks(void) {
    app_core_fixture_t fixture;
    reset_fixture(&fixture);
    app_core_ops_t operations = make_operations(&fixture);
    const app_core_policy_t policy = production_policy();
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, app_core_sequence_start(NULL, &policy));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, app_core_sequence_start(&operations, NULL));

#define CHECK_MISSING_CALLBACK(member)                                                             \
    do {                                                                                           \
        reset_fixture(&fixture);                                                                   \
        operations = make_operations(&fixture);                                                    \
        operations.member = NULL;                                                                  \
        TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,                                           \
                             app_core_sequence_start(&operations, &policy));                       \
        TEST_CHECK_EQ_U64(0U, fixture.calls.call_count);                                           \
    } while (0)

    CHECK_MISSING_CALLBACK(nvs_init);
    CHECK_MISSING_CALLBACK(provisioning_init);
    CHECK_MISSING_CALLBACK(provisioning_load);
    CHECK_MISSING_CALLBACK(bootstrap_derive);
    CHECK_MISSING_CALLBACK(storage_mount);
    CHECK_MISSING_CALLBACK(storage_recover);
    CHECK_MISSING_CALLBACK(repository_init);
    CHECK_MISSING_CALLBACK(auth_init);
    CHECK_MISSING_CALLBACK(usb_init);
    CHECK_MISSING_CALLBACK(executor_init);
    CHECK_MISSING_CALLBACK(controls_init);
    CHECK_MISSING_CALLBACK(wifi_start);
    CHECK_MISSING_CALLBACK(http_start);
    CHECK_MISSING_CALLBACK(http_stop);
    CHECK_MISSING_CALLBACK(wifi_stop);
    CHECK_MISSING_CALLBACK(storage_unmount);
    CHECK_MISSING_CALLBACK(repository_deinit);
    CHECK_MISSING_CALLBACK(auth_deinit);
    CHECK_MISSING_CALLBACK(usb_deinit);
    CHECK_MISSING_CALLBACK(executor_deinit);
    CHECK_MISSING_CALLBACK(controls_deinit);
    CHECK_MISSING_CALLBACK(provisioning_deinit);
    CHECK_MISSING_CALLBACK(nvs_deinit);
    CHECK_MISSING_CALLBACK(http_owns_resources);
    CHECK_MISSING_CALLBACK(wifi_owns_resources);
    CHECK_MISSING_CALLBACK(storage_owns_mount);
    CHECK_MISSING_CALLBACK(provisioning_owns_resources);
    CHECK_MISSING_CALLBACK(set_indicator);
    CHECK_MISSING_CALLBACK(secure_zero);
    CHECK_MISSING_CALLBACK(log_event);

#undef CHECK_MISSING_CALLBACK
}

static void test_setup_success_isolated_from_normal_services(void) {
    app_core_fixture_t fixture;
    reset_fixture(&fixture);
    app_core_ops_t operations = make_operations(&fixture);
    const app_core_policy_t policy = production_policy();
    static const char *const expected[] = {
        "indicator_booting", "nvs",        "provisioning_init", "provisioning_load",
        "storage_mount",     "auth_init",  "controls_init",     "bootstrap_derive",
        "wifi_start",        "http_start", "secure_zero",       "indicator_ready",
    };
    fake_call_log_set_strict(&fixture.calls, true);
    for (size_t index = 0U; index < sizeof(expected) / sizeof(expected[0]); ++index) {
        fake_call_log_expect(&fixture.calls, expected[index]);
    }

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_core_sequence_start(&operations, &policy));
    fake_call_log_verify(&fixture.calls);
    TEST_CHECK_EQ_U64(0U, call_count(&fixture, "storage_recover"));
    TEST_CHECK_EQ_U64(0U, call_count(&fixture, "repository_init"));
    TEST_CHECK_EQ_U64(0U, call_count(&fixture, "usb_init"));
    TEST_CHECK_EQ_U64(0U, call_count(&fixture, "executor_init"));
    TEST_CHECK_EQ_U64(1U, call_count(&fixture, "secure_zero"));
    TEST_CHECK_EQ_STRING(fixture.bootstrap.ap_ssid, fixture.wifi_ssid);
    TEST_CHECK_EQ_STRING(fixture.bootstrap.ap_passphrase, fixture.wifi_passphrase);
    TEST_CHECK_EQ_INT(WEB_SERVER_MODE_SETUP, fixture.observed_web_configuration.mode);
    TEST_CHECK(!fixture.observed_web_configuration.login_enabled);
    TEST_CHECK_EQ_STRING(fixture.bootstrap.device_id,
                         fixture.observed_web_configuration.setup_device_id);
    TEST_CHECK_EQ_STRING(fixture.bootstrap.setup_code,
                         fixture.observed_web_configuration.setup_code);
    TEST_CHECK(fixture.observed_web_configuration.setup_physical_confirmation_required);
    TEST_CHECK(!fixture.observed_web_configuration.setup_manufacturing_bypass);
    TEST_CHECK_EQ_U64(1U, log_count(&fixture, APP_CORE_LOG_PROVISIONING_REQUIRED));
    TEST_CHECK_EQ_U64(0U, log_count(&fixture, APP_CORE_LOG_MANUFACTURING_CREDENTIALS));
}

static void test_manufacturing_mode_logs_once_and_bypasses_confirmation(void) {
    app_core_fixture_t fixture;
    reset_fixture(&fixture);
    app_core_ops_t operations = make_operations(&fixture);
    const app_core_policy_t policy = {
        .manufacturing_provisioning_enabled = true,
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_core_sequence_start(&operations, &policy));
    TEST_CHECK(fixture.observed_web_configuration.setup_manufacturing_bypass);
    TEST_CHECK_EQ_U64(1U, log_count(&fixture, APP_CORE_LOG_MANUFACTURING_CREDENTIALS));
    const recorded_log_t *credentials = first_log(&fixture, APP_CORE_LOG_MANUFACTURING_CREDENTIALS);
    TEST_CHECK(credentials != NULL);
    TEST_CHECK_EQ_STRING(fixture.bootstrap.ap_ssid, credentials->ssid);
    TEST_CHECK_EQ_STRING(fixture.bootstrap.ap_passphrase, credentials->ap_passphrase);
    TEST_CHECK_EQ_STRING(fixture.bootstrap.setup_code, credentials->setup_code);
}

static void test_normal_success_uses_persisted_credentials(void) {
    app_core_fixture_t fixture;
    reset_fixture(&fixture);
    configure_normal_provisioning(&fixture.provisioning);
    app_core_ops_t operations = make_operations(&fixture);
    const app_core_policy_t policy = production_policy();
    static const char *const expected[] = {
        "indicator_booting", "nvs",           "provisioning_init",
        "provisioning_load", "storage_mount", "storage_recover",
        "repository_init",   "auth_init",     "usb_init",
        "executor_init",     "controls_init", "wifi_start",
        "http_start",        "secure_zero",   "indicator_ready",
    };
    fake_call_log_set_strict(&fixture.calls, true);
    for (size_t index = 0U; index < sizeof(expected) / sizeof(expected[0]); ++index) {
        fake_call_log_expect(&fixture.calls, expected[index]);
    }

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_core_sequence_start(&operations, &policy));
    fake_call_log_verify(&fixture.calls);
    TEST_CHECK_EQ_U64(0U, call_count(&fixture, "bootstrap_derive"));
    TEST_CHECK_EQ_STRING(fixture.provisioning.ap_ssid, fixture.wifi_ssid);
    TEST_CHECK_EQ_STRING(fixture.provisioning.ap_passphrase, fixture.wifi_passphrase);
    TEST_CHECK_EQ_INT(WEB_SERVER_MODE_NORMAL, fixture.observed_web_configuration.mode);
    TEST_CHECK(fixture.observed_web_configuration.login_enabled);
    TEST_CHECK_EQ_U64(AUTH_PBKDF2_ITERATIONS,
                      fixture.observed_web_configuration.password_record.iterations);
    TEST_CHECK_EQ_U64(0U, log_count(&fixture, APP_CORE_LOG_PROVISIONING_REQUIRED));
}

static void test_normal_degraded_storage_reaches_degraded_indicator(void) {
    app_core_fixture_t fixture;
    reset_fixture(&fixture);
    configure_normal_provisioning(&fixture.provisioning);
    fixture.storage_recover_result = APP_ERROR_STORAGE_CORRUPT;
    app_core_ops_t operations = make_operations(&fixture);
    const app_core_policy_t policy = production_policy();
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_core_sequence_start(&operations, &policy));
    TEST_CHECK_EQ_U64(1U, call_count(&fixture, "indicator_degraded"));
    TEST_CHECK_EQ_U64(0U, call_count(&fixture, "indicator_ready"));
    TEST_CHECK_EQ_U64(1U, log_count(&fixture, APP_CORE_LOG_STORAGE_DEGRADED));
}

typedef enum {
    SETUP_FAILURE_BOOT = 0,
    SETUP_FAILURE_NVS,
    SETUP_FAILURE_PROVISIONING_INIT,
    SETUP_FAILURE_PROVISIONING_LOAD,
    SETUP_FAILURE_STORAGE_MOUNT,
    SETUP_FAILURE_AUTH,
    SETUP_FAILURE_CONTROLS,
    SETUP_FAILURE_BOOTSTRAP,
    SETUP_FAILURE_WIFI,
    SETUP_FAILURE_HTTP,
    SETUP_FAILURE_READY
} setup_failure_t;

static app_error_code_t configure_setup_failure(app_core_fixture_t *fixture,
                                                setup_failure_t point) {
    switch (point) {
    case SETUP_FAILURE_BOOT:
        fixture->indicator_failure_state = DEVICE_INDICATOR_BOOTING;
        fixture->indicator_failure_result = APP_ERROR_IO;
        return APP_ERROR_IO;
    case SETUP_FAILURE_NVS:
        fixture->nvs_result = APP_CORE_NVS_OTHER_FAILURE;
        return APP_ERROR_STORAGE_UNAVAILABLE;
    case SETUP_FAILURE_PROVISIONING_INIT:
        fixture->provisioning_init_result = APP_ERROR_STORAGE_UNAVAILABLE;
        return APP_ERROR_STORAGE_UNAVAILABLE;
    case SETUP_FAILURE_PROVISIONING_LOAD:
        fixture->provisioning_load_result = APP_ERROR_STORAGE_CORRUPT;
        return APP_ERROR_STORAGE_CORRUPT;
    case SETUP_FAILURE_STORAGE_MOUNT:
        fixture->storage_mount_result = APP_ERROR_STORAGE_UNAVAILABLE;
        return APP_ERROR_STORAGE_UNAVAILABLE;
    case SETUP_FAILURE_AUTH:
        fixture->auth_result = APP_ERROR_INTERNAL;
        return APP_ERROR_INTERNAL;
    case SETUP_FAILURE_CONTROLS:
        fixture->controls_result = APP_ERROR_IO;
        return APP_ERROR_IO;
    case SETUP_FAILURE_BOOTSTRAP:
        fixture->bootstrap_result = APP_ERROR_INTERNAL;
        return APP_ERROR_INTERNAL;
    case SETUP_FAILURE_WIFI:
        fixture->wifi_result = APP_ERROR_IO;
        return APP_ERROR_IO;
    case SETUP_FAILURE_HTTP:
        fixture->http_result = APP_ERROR_INTERNAL;
        return APP_ERROR_INTERNAL;
    case SETUP_FAILURE_READY:
        fixture->indicator_failure_state = DEVICE_INDICATOR_READY;
        fixture->indicator_failure_result = APP_ERROR_TIMEOUT;
        return APP_ERROR_TIMEOUT;
    default:
        return APP_ERROR_INTERNAL;
    }
}

static void test_setup_failure_matrix(void) {
    for (setup_failure_t point = SETUP_FAILURE_BOOT; point <= SETUP_FAILURE_READY;
         point = (setup_failure_t)((int)point + 1)) {
        app_core_fixture_t fixture;
        reset_fixture(&fixture);
        const app_error_code_t expected = configure_setup_failure(&fixture, point);
        app_core_ops_t operations = make_operations(&fixture);
        const app_core_policy_t policy = production_policy();
        TEST_CHECK_APP_ERROR(expected, app_core_sequence_start(&operations, &policy));
        TEST_CHECK_EQ_U64(1U, call_count(&fixture, "secure_zero"));
        TEST_CHECK_EQ_U64(1U, call_count(&fixture, "indicator_fatal"));
        TEST_CHECK_EQ_U64(0U, call_count(&fixture, "usb_init"));
        TEST_CHECK_EQ_U64(0U, call_count(&fixture, "executor_init"));
        TEST_CHECK_EQ_U64(0U, call_count(&fixture, "repository_init"));
        if (point == SETUP_FAILURE_WIFI) {
            TEST_CHECK_EQ_U64(0U, call_count(&fixture, "http_start"));
        }
        if (point == SETUP_FAILURE_HTTP || point == SETUP_FAILURE_READY) {
            TEST_CHECK_EQ_U64(1U, call_count(&fixture, "wifi_stop"));
        }
        if (point == SETUP_FAILURE_READY) {
            TEST_CHECK_EQ_U64(1U, call_count(&fixture, "http_stop"));
            assert_order(&fixture, "http_stop", "wifi_stop");
            assert_order(&fixture, "wifi_stop", "controls_deinit");
            assert_order(&fixture, "controls_deinit", "auth_deinit");
            assert_order(&fixture, "auth_deinit", "storage_unmount");
            assert_order(&fixture, "storage_unmount", "provisioning_deinit");
            assert_order(&fixture, "provisioning_deinit", "nvs_deinit");
        }
    }
}

typedef enum {
    NORMAL_FAILURE_RECOVERY = 0,
    NORMAL_FAILURE_REPOSITORY,
    NORMAL_FAILURE_AUTH,
    NORMAL_FAILURE_USB,
    NORMAL_FAILURE_EXECUTOR,
    NORMAL_FAILURE_CONTROLS,
    NORMAL_FAILURE_WIFI,
    NORMAL_FAILURE_HTTP
} normal_failure_t;

static app_error_code_t configure_normal_failure(app_core_fixture_t *fixture,
                                                 normal_failure_t point) {
    switch (point) {
    case NORMAL_FAILURE_RECOVERY:
        fixture->storage_recover_result = APP_ERROR_IO;
        return APP_ERROR_IO;
    case NORMAL_FAILURE_REPOSITORY:
        fixture->repository_result = APP_ERROR_STORAGE_CORRUPT;
        return APP_ERROR_STORAGE_CORRUPT;
    case NORMAL_FAILURE_AUTH:
        fixture->auth_result = APP_ERROR_INTERNAL;
        return APP_ERROR_INTERNAL;
    case NORMAL_FAILURE_USB:
        fixture->usb_result = APP_ERROR_USB_NOT_READY;
        return APP_ERROR_USB_NOT_READY;
    case NORMAL_FAILURE_EXECUTOR:
        fixture->executor_result = APP_ERROR_INTERNAL;
        return APP_ERROR_INTERNAL;
    case NORMAL_FAILURE_CONTROLS:
        fixture->controls_result = APP_ERROR_IO;
        return APP_ERROR_IO;
    case NORMAL_FAILURE_WIFI:
        fixture->wifi_result = APP_ERROR_IO;
        return APP_ERROR_IO;
    case NORMAL_FAILURE_HTTP:
        fixture->http_result = APP_ERROR_INTERNAL;
        return APP_ERROR_INTERNAL;
    default:
        return APP_ERROR_INTERNAL;
    }
}

static void test_normal_failure_matrix(void) {
    for (normal_failure_t point = NORMAL_FAILURE_RECOVERY; point <= NORMAL_FAILURE_HTTP;
         point = (normal_failure_t)((int)point + 1)) {
        app_core_fixture_t fixture;
        reset_fixture(&fixture);
        configure_normal_provisioning(&fixture.provisioning);
        const app_error_code_t expected = configure_normal_failure(&fixture, point);
        app_core_ops_t operations = make_operations(&fixture);
        const app_core_policy_t policy = production_policy();
        TEST_CHECK_APP_ERROR(expected, app_core_sequence_start(&operations, &policy));
        TEST_CHECK_EQ_U64(1U, call_count(&fixture, "secure_zero"));
        TEST_CHECK_EQ_U64(1U, call_count(&fixture, "indicator_fatal"));
        TEST_CHECK_EQ_U64(0U, call_count(&fixture, "bootstrap_derive"));
        if (point == NORMAL_FAILURE_WIFI) {
            /* SPEC 15.1: "AP startup failure is a visible fatal network state.
             * The firmware MUST NOT silently continue as though the web
             * application were available." Visible is the indicator_fatal call
             * asserted above; not continuing is this: the HTTP server is never
             * started, so there is no listener to suggest the application is
             * reachable when no network exists to reach it over. */
            TEST_CHECK_EQ_U64(0U, call_count(&fixture, "http_start"));
        }
        if (point == NORMAL_FAILURE_HTTP) {
            TEST_CHECK_EQ_U64(1U, call_count(&fixture, "wifi_stop"));
        }
    }
}

static void test_cleanup_errors_preserve_primary_and_continue(void) {
    app_core_fixture_t fixture;
    reset_fixture(&fixture);
    configure_normal_provisioning(&fixture.provisioning);
    fixture.indicator_failure_state = DEVICE_INDICATOR_READY;
    fixture.indicator_failure_result = APP_ERROR_TIMEOUT;
    fixture.http_stop_result = APP_ERROR_IO;
    fixture.wifi_stop_result = APP_ERROR_STORAGE_UNAVAILABLE;
    fixture.storage_unmount_result = APP_ERROR_STORAGE_CORRUPT;
    app_core_ops_t operations = make_operations(&fixture);
    const app_core_policy_t policy = production_policy();
    TEST_CHECK_APP_ERROR(APP_ERROR_TIMEOUT, app_core_sequence_start(&operations, &policy));
    assert_order(&fixture, "http_stop", "wifi_stop");
    assert_order(&fixture, "wifi_stop", "controls_deinit");
    assert_order(&fixture, "storage_unmount", "provisioning_deinit");
    assert_order(&fixture, "provisioning_deinit", "nvs_deinit");
    const recorded_log_t *cleanup = first_log(&fixture, APP_CORE_LOG_CLEANUP_FAILED);
    TEST_CHECK(cleanup != NULL);
    TEST_CHECK_APP_ERROR(APP_ERROR_TIMEOUT, cleanup->primary_error);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, cleanup->cleanup_error);
    TEST_CHECK(cleanup->cleanup_incomplete);
}

static void test_residual_ownership_queries_trigger_cleanup(void) {
    app_core_fixture_t fixture;
    app_core_ops_t operations;
    const app_core_policy_t policy = production_policy();

    reset_fixture(&fixture);
    fixture.provisioning_init_result = APP_ERROR_STORAGE_UNAVAILABLE;
    fixture.provisioning_owns_resources = true;
    operations = make_operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE,
                         app_core_sequence_start(&operations, &policy));
    TEST_CHECK_EQ_U64(1U, call_count(&fixture, "provisioning_deinit"));

    reset_fixture(&fixture);
    fixture.storage_mount_result = APP_ERROR_STORAGE_UNAVAILABLE;
    fixture.storage_owns_mount = true;
    operations = make_operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE,
                         app_core_sequence_start(&operations, &policy));
    TEST_CHECK_EQ_U64(1U, call_count(&fixture, "storage_unmount"));

    reset_fixture(&fixture);
    fixture.wifi_result = APP_ERROR_IO;
    fixture.wifi_owns_resources = true;
    operations = make_operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, app_core_sequence_start(&operations, &policy));
    TEST_CHECK_EQ_U64(1U, call_count(&fixture, "wifi_stop"));

    reset_fixture(&fixture);
    fixture.http_result = APP_ERROR_INTERNAL;
    fixture.http_owns_resources = true;
    operations = make_operations(&fixture);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, app_core_sequence_start(&operations, &policy));
    TEST_CHECK_EQ_U64(1U, call_count(&fixture, "http_stop"));
    TEST_CHECK_EQ_U64(1U, call_count(&fixture, "wifi_stop"));
}

/* SPEC 20.2: logs MUST "avoid passwords, tokens, raw cookie values, setup
 * codes, and macro text that may contain secrets." Startup is where that is
 * hardest, because the sequence is holding exactly those values as it brings
 * subsystems up. The manufacturing banner is the single deliberate exception,
 * gated behind a Kconfig option the production gate rejects. */
/* SPEC 24.4 item: redaction */
static void test_stage_logs_are_redacted(void) {
    app_core_fixture_t fixture;
    reset_fixture(&fixture);
    app_core_ops_t operations = make_operations(&fixture);
    const app_core_policy_t policy = production_policy();
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_core_sequence_start(&operations, &policy));
    for (size_t index = 0U; index < fixture.log_count; ++index) {
        const recorded_log_t *record = &fixture.logs[index];
        if (record->type != APP_CORE_LOG_MANUFACTURING_CREDENTIALS) {
            TEST_CHECK_EQ_STRING("", record->ssid);
            TEST_CHECK_EQ_STRING("", record->ap_passphrase);
            TEST_CHECK_EQ_STRING("", record->setup_code);
        }
        TEST_CHECK_EQ_U64(0U, record->operation_id);
    }
}

static void test_nvs_recovery_states_never_continue(void) {
    const app_core_nvs_result_t results[] = {
        APP_CORE_NVS_NO_FREE_PAGES,
        APP_CORE_NVS_NEW_VERSION_FOUND,
    };
    const app_core_policy_t policy = production_policy();
    for (size_t index = 0U; index < sizeof(results) / sizeof(results[0]); ++index) {
        app_core_fixture_t fixture;
        reset_fixture(&fixture);
        fixture.nvs_result = results[index];
        app_core_ops_t operations = make_operations(&fixture);
        TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                             app_core_sequence_start(&operations, &policy));
        TEST_CHECK_EQ_U64(0U, call_count(&fixture, "provisioning_init"));
        TEST_CHECK_EQ_U64(1U, call_count(&fixture, "indicator_fatal"));
    }
}

int main(void) {
    test_nvs_mapping();
    test_invalid_arguments_and_missing_callbacks();
    test_setup_success_isolated_from_normal_services();
    test_manufacturing_mode_logs_once_and_bypasses_confirmation();
    test_normal_success_uses_persisted_credentials();
    test_normal_degraded_storage_reaches_degraded_indicator();
    test_setup_failure_matrix();
    test_normal_failure_matrix();
    test_cleanup_errors_preserve_primary_and_continue();
    test_residual_ownership_queries_trigger_cleanup();
    test_stage_logs_are_redacted();
    test_nvs_recovery_states_never_continue();
    puts("app core tests passed");
    return EXIT_SUCCESS;
}
