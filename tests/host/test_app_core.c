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
#define RANDOM_PLAN_CAPACITY 8U

typedef struct {
    app_core_log_type_t type;
    app_error_code_t primary_error;
    app_error_code_t secondary_error;
    char stage[32U];
    char ssid[64U];
    char ap_passphrase[APP_CORE_DEVELOPMENT_PASSWORD_BYTES + 1U];
    char web_password[APP_CORE_DEVELOPMENT_PASSWORD_BYTES + 1U];
} recorded_log_t;

typedef struct {
    fake_call_log_t calls;
    app_core_nvs_result_t nvs_result;
    app_error_code_t storage_mount_result;
    app_error_code_t storage_recover_result;
    app_error_code_t repository_result;
    app_error_code_t auth_result;
    app_error_code_t usb_result;
    app_error_code_t executor_result;
    app_error_code_t controls_result;
    app_error_code_t password_result;
    app_error_code_t wifi_result;
    app_error_code_t http_result;
    app_error_code_t http_stop_result;
    app_error_code_t wifi_stop_result;
    app_error_code_t storage_unmount_result;
    device_indicator_state_t indicator_failure_state;
    app_error_code_t indicator_failure_result;
    size_t random_fail_on;
    app_error_code_t random_failure_result;
    uint8_t random_plan[RANDOM_PLAN_CAPACITY];
    size_t random_plan_count;
    size_t random_call_count;
    size_t secure_zero_count;
    recorded_log_t logs[RECORDED_LOG_CAPACITY];
    size_t log_count;
    char wifi_ssid[64U];
    char wifi_passphrase[APP_CORE_DEVELOPMENT_PASSWORD_BYTES + 1U];
    web_server_config_t observed_web_configuration;
} app_core_fixture_t;

static void copy_text(char *destination, size_t destination_size, const char *source)
{
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

static const char *indicator_call_name(device_indicator_state_t indicator)
{
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

static void record_call(app_core_fixture_t *fixture, const char *name)
{
    TEST_CHECK(fixture != NULL);
    TEST_CHECK(name != NULL);
    TEST_CHECK(!fake_call_log_record(&fixture->calls, name, 0U, 0U));
}

static size_t call_count(const app_core_fixture_t *fixture, const char *name)
{
    size_t count = 0U;
    for (size_t index = 0U; index < fixture->calls.call_count; ++index) {
        const fake_call_t *call = fake_call_log_at(&fixture->calls, index);
        if (call != NULL && strcmp(call->name, name) == 0) {
            ++count;
        }
    }
    return count;
}

static size_t first_call_index(const app_core_fixture_t *fixture, const char *name)
{
    for (size_t index = 0U; index < fixture->calls.call_count; ++index) {
        const fake_call_t *call = fake_call_log_at(&fixture->calls, index);
        if (call != NULL && strcmp(call->name, name) == 0) {
            return index;
        }
    }
    return SIZE_MAX;
}

static void reset_fixture(app_core_fixture_t *fixture)
{
    TEST_CHECK(fixture != NULL);
    memset(fixture, 0, sizeof(*fixture));
    fake_call_log_reset(&fixture->calls);
    fixture->nvs_result = APP_CORE_NVS_OK;
    fixture->indicator_failure_state = (device_indicator_state_t)-1;
    fixture->indicator_failure_result = APP_ERROR_INTERNAL;
    fixture->random_failure_result = APP_ERROR_INTERNAL;
    fixture->random_plan[0] = 0U;
    fixture->random_plan[1] = 1U;
    fixture->random_plan_count = 2U;
}

static app_core_nvs_result_t fake_nvs_init(void *context)
{
    app_core_fixture_t *fixture = context;
    record_call(fixture, "nvs");
    return fixture->nvs_result;
}

static app_error_code_t fake_storage_mount(void *context)
{
    app_core_fixture_t *fixture = context;
    record_call(fixture, "storage_mount");
    return fixture->storage_mount_result;
}

static app_error_code_t fake_storage_recover(void *context)
{
    app_core_fixture_t *fixture = context;
    record_call(fixture, "storage_recover");
    return fixture->storage_recover_result;
}

static app_error_code_t fake_repository_init(void *context)
{
    app_core_fixture_t *fixture = context;
    record_call(fixture, "repository_init");
    return fixture->repository_result;
}

static app_error_code_t fake_auth_init(void *context)
{
    app_core_fixture_t *fixture = context;
    record_call(fixture, "auth_init");
    return fixture->auth_result;
}

static app_error_code_t fake_usb_init(void *context)
{
    app_core_fixture_t *fixture = context;
    record_call(fixture, "usb_init");
    return fixture->usb_result;
}

static app_error_code_t fake_executor_init(void *context)
{
    app_core_fixture_t *fixture = context;
    record_call(fixture, "executor_init");
    return fixture->executor_result;
}

static app_error_code_t fake_controls_init(void *context)
{
    app_core_fixture_t *fixture = context;
    record_call(fixture, "controls_init");
    return fixture->controls_result;
}

static app_error_code_t fake_random_fill(void *context, uint8_t *output, size_t length)
{
    app_core_fixture_t *fixture = context;
    record_call(fixture, "random_fill");
    ++fixture->random_call_count;
    if (fixture->random_fail_on != 0U &&
        fixture->random_call_count == fixture->random_fail_on) {
        return fixture->random_failure_result;
    }
    TEST_CHECK(output != NULL || length == 0U);
    size_t plan_index = fixture->random_call_count - 1U;
    if (fixture->random_plan_count == 0U) {
        plan_index = 0U;
    } else if (plan_index >= fixture->random_plan_count) {
        plan_index = fixture->random_plan_count - 1U;
    }
    const uint8_t value = fixture->random_plan_count == 0U
                              ? 0U
                              : fixture->random_plan[plan_index];
    memset(output, value, length);
    return APP_ERROR_NONE;
}

static app_error_code_t fake_password_create(void *context,
                                             const char *password,
                                             size_t password_length,
                                             auth_password_record_t *out_record)
{
    app_core_fixture_t *fixture = context;
    record_call(fixture, "password_create");
    TEST_CHECK(password != NULL);
    TEST_CHECK_EQ_U64(APP_CORE_DEVELOPMENT_PASSWORD_BYTES, password_length);
    TEST_CHECK_EQ_U64(password_length, strlen(password));
    TEST_CHECK(out_record != NULL);
    if (fixture->password_result == APP_ERROR_NONE) {
        memset(out_record, 0, sizeof(*out_record));
        out_record->iterations = 120000U;
        out_record->salt[0] = 0x11U;
        out_record->hash[0] = 0x22U;
    }
    return fixture->password_result;
}

static app_error_code_t fake_wifi_start(void *context,
                                        const char *ssid,
                                        const char *passphrase)
{
    app_core_fixture_t *fixture = context;
    record_call(fixture, "wifi_start");
    copy_text(fixture->wifi_ssid, sizeof(fixture->wifi_ssid), ssid);
    copy_text(fixture->wifi_passphrase,
              sizeof(fixture->wifi_passphrase),
              passphrase);
    return fixture->wifi_result;
}

static app_error_code_t fake_http_start(void *context,
                                        const web_server_config_t *configuration)
{
    app_core_fixture_t *fixture = context;
    record_call(fixture, "http_start");
    TEST_CHECK(configuration != NULL);
    fixture->observed_web_configuration = *configuration;
    return fixture->http_result;
}

static app_error_code_t fake_http_stop(void *context)
{
    app_core_fixture_t *fixture = context;
    record_call(fixture, "http_stop");
    return fixture->http_stop_result;
}

static app_error_code_t fake_wifi_stop(void *context)
{
    app_core_fixture_t *fixture = context;
    record_call(fixture, "wifi_stop");
    return fixture->wifi_stop_result;
}

static app_error_code_t fake_storage_unmount(void *context)
{
    app_core_fixture_t *fixture = context;
    record_call(fixture, "storage_unmount");
    return fixture->storage_unmount_result;
}

static app_error_code_t fake_set_indicator(void *context,
                                           device_indicator_state_t indicator)
{
    app_core_fixture_t *fixture = context;
    record_call(fixture, indicator_call_name(indicator));
    return indicator == fixture->indicator_failure_state
               ? fixture->indicator_failure_result
               : APP_ERROR_NONE;
}

static void fake_secure_zero(void *context, void *memory, size_t length)
{
    app_core_fixture_t *fixture = context;
    record_call(fixture, "secure_zero");
    TEST_CHECK(memory != NULL || length == 0U);
    if (memory != NULL) {
        memset(memory, 0, length);
    }
    ++fixture->secure_zero_count;
}

static void fake_log_event(void *context, const app_core_log_event_t *event)
{
    app_core_fixture_t *fixture = context;
    TEST_CHECK(event != NULL);
    TEST_CHECK(fixture->log_count < RECORDED_LOG_CAPACITY);
    recorded_log_t *record = &fixture->logs[fixture->log_count++];
    memset(record, 0, sizeof(*record));
    record->type = event->type;
    record->primary_error = event->primary_error;
    record->secondary_error = event->secondary_error;
    copy_text(record->stage, sizeof(record->stage), event->stage);
    copy_text(record->ssid, sizeof(record->ssid), event->ssid);
    copy_text(record->ap_passphrase,
              sizeof(record->ap_passphrase),
              event->ap_passphrase);
    copy_text(record->web_password,
              sizeof(record->web_password),
              event->web_password);
}

static app_core_ops_t make_operations(app_core_fixture_t *fixture)
{
    return (app_core_ops_t){
        .context = fixture,
        .nvs_init = fake_nvs_init,
        .storage_mount = fake_storage_mount,
        .storage_recover = fake_storage_recover,
        .repository_init = fake_repository_init,
        .auth_init = fake_auth_init,
        .usb_init = fake_usb_init,
        .executor_init = fake_executor_init,
        .controls_init = fake_controls_init,
        .random_fill = fake_random_fill,
        .password_create = fake_password_create,
        .wifi_start = fake_wifi_start,
        .http_start = fake_http_start,
        .http_stop = fake_http_stop,
        .wifi_stop = fake_wifi_stop,
        .storage_unmount = fake_storage_unmount,
        .set_indicator = fake_set_indicator,
        .secure_zero = fake_secure_zero,
        .log_event = fake_log_event,
    };
}

static app_core_policy_t development_policy(void)
{
    return (app_core_policy_t){
        .development_provisioning_enabled = true,
        .development_ssid = "ESP32-Macro-Setup",
    };
}

static size_t log_count(const app_core_fixture_t *fixture, app_core_log_type_t type)
{
    size_t count = 0U;
    for (size_t index = 0U; index < fixture->log_count; ++index) {
        if (fixture->logs[index].type == type) {
            ++count;
        }
    }
    return count;
}

static const recorded_log_t *first_log(const app_core_fixture_t *fixture,
                                       app_core_log_type_t type)
{
    for (size_t index = 0U; index < fixture->log_count; ++index) {
        if (fixture->logs[index].type == type) {
            return &fixture->logs[index];
        }
    }
    return NULL;
}

static void assert_order(const app_core_fixture_t *fixture,
                         const char *before,
                         const char *after)
{
    const size_t before_index = first_call_index(fixture, before);
    const size_t after_index = first_call_index(fixture, after);
    TEST_CHECK(before_index != SIZE_MAX);
    TEST_CHECK(after_index != SIZE_MAX);
    TEST_CHECK(before_index < after_index);
}

static void test_nvs_mapping(void)
{
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, app_core_map_nvs_result(APP_CORE_NVS_OK));
    TEST_CHECK_EQ_INT(APP_ERROR_STORAGE_CORRUPT,
                      app_core_map_nvs_result(APP_CORE_NVS_NO_FREE_PAGES));
    TEST_CHECK_EQ_INT(APP_ERROR_STORAGE_CORRUPT,
                      app_core_map_nvs_result(APP_CORE_NVS_NEW_VERSION_FOUND));
    TEST_CHECK_EQ_INT(APP_ERROR_STORAGE_UNAVAILABLE,
                      app_core_map_nvs_result(APP_CORE_NVS_OTHER_FAILURE));
    TEST_CHECK_EQ_INT(APP_ERROR_STORAGE_UNAVAILABLE,
                      app_core_map_nvs_result((app_core_nvs_result_t)99));
}

static void test_invalid_arguments_and_missing_callbacks(void)
{
    app_core_fixture_t fixture;
    reset_fixture(&fixture);
    app_core_ops_t operations = make_operations(&fixture);
    app_core_policy_t policy = development_policy();

    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      app_core_sequence_start(NULL, &policy));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      app_core_sequence_start(&operations, NULL));
    policy.development_ssid = NULL;
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      app_core_sequence_start(&operations, &policy));
    policy.development_ssid = "";
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      app_core_sequence_start(&operations, &policy));

#define CHECK_MISSING_CALLBACK(member)                                                  \
    do {                                                                                \
        reset_fixture(&fixture);                                                        \
        operations = make_operations(&fixture);                                         \
        policy = development_policy();                                                  \
        operations.member = NULL;                                                       \
        TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,                                   \
                          app_core_sequence_start(&operations, &policy));                \
        TEST_CHECK_EQ_U64(0U, fixture.calls.call_count);                                 \
    } while (0)

    CHECK_MISSING_CALLBACK(nvs_init);
    CHECK_MISSING_CALLBACK(storage_mount);
    CHECK_MISSING_CALLBACK(storage_recover);
    CHECK_MISSING_CALLBACK(repository_init);
    CHECK_MISSING_CALLBACK(auth_init);
    CHECK_MISSING_CALLBACK(usb_init);
    CHECK_MISSING_CALLBACK(executor_init);
    CHECK_MISSING_CALLBACK(controls_init);
    CHECK_MISSING_CALLBACK(random_fill);
    CHECK_MISSING_CALLBACK(password_create);
    CHECK_MISSING_CALLBACK(wifi_start);
    CHECK_MISSING_CALLBACK(http_start);
    CHECK_MISSING_CALLBACK(http_stop);
    CHECK_MISSING_CALLBACK(wifi_stop);
    CHECK_MISSING_CALLBACK(storage_unmount);
    CHECK_MISSING_CALLBACK(set_indicator);
    CHECK_MISSING_CALLBACK(secure_zero);
    CHECK_MISSING_CALLBACK(log_event);

#undef CHECK_MISSING_CALLBACK
}

static void test_success_order_and_distinct_credentials(void)
{
    app_core_fixture_t fixture;
    reset_fixture(&fixture);
    app_core_ops_t operations = make_operations(&fixture);
    const app_core_policy_t policy = development_policy();

    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      app_core_sequence_start(&operations, &policy));

    static const char *const expected[] = {
        "indicator_booting",
        "nvs",
        "storage_mount",
        "storage_recover",
        "repository_init",
        "auth_init",
        "usb_init",
        "executor_init",
        "controls_init",
        "random_fill",
        "secure_zero",
        "random_fill",
        "secure_zero",
        "password_create",
        "wifi_start",
        "http_start",
        "secure_zero",
        "secure_zero",
        "secure_zero",
        "indicator_ready",
    };
    TEST_CHECK_EQ_U64(sizeof(expected) / sizeof(expected[0]),
                      fixture.calls.call_count);
    for (size_t index = 0U; index < (sizeof(expected) / sizeof(expected[0])); ++index) {
        const fake_call_t *call = fake_call_log_at(&fixture.calls, index);
        TEST_CHECK(call != NULL);
        TEST_CHECK_EQ_STRING(expected[index], call->name);
    }

    TEST_CHECK_EQ_STRING(policy.development_ssid, fixture.wifi_ssid);
    TEST_CHECK(strlen(fixture.wifi_passphrase) == APP_CORE_DEVELOPMENT_PASSWORD_BYTES);
    TEST_CHECK(fixture.observed_web_configuration.login_enabled);
    TEST_CHECK(fixture.observed_web_configuration.password_record.iterations == 120000U);
    TEST_CHECK_EQ_U64(1U, log_count(&fixture, APP_CORE_LOG_DEVELOPMENT_CREDENTIALS));
    const recorded_log_t *credentials =
        first_log(&fixture, APP_CORE_LOG_DEVELOPMENT_CREDENTIALS);
    TEST_CHECK(credentials != NULL);
    TEST_CHECK_EQ_STRING(policy.development_ssid, credentials->ssid);
    TEST_CHECK(strcmp(credentials->ap_passphrase, credentials->web_password) != 0);
    TEST_CHECK_EQ_STRING(credentials->ap_passphrase, fixture.wifi_passphrase);
    TEST_CHECK_EQ_U64(0U, call_count(&fixture, "http_stop"));
    TEST_CHECK_EQ_U64(0U, call_count(&fixture, "wifi_stop"));
    TEST_CHECK_EQ_U64(0U, call_count(&fixture, "storage_unmount"));
}

static void test_degraded_storage_reaches_degraded_indicator(void)
{
    app_core_fixture_t fixture;
    reset_fixture(&fixture);
    fixture.storage_recover_result = APP_ERROR_STORAGE_CORRUPT;
    app_core_ops_t operations = make_operations(&fixture);
    const app_core_policy_t policy = development_policy();

    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      app_core_sequence_start(&operations, &policy));
    TEST_CHECK_EQ_U64(1U, call_count(&fixture, "indicator_degraded"));
    TEST_CHECK_EQ_U64(0U, call_count(&fixture, "indicator_ready"));
    TEST_CHECK_EQ_U64(1U, log_count(&fixture, APP_CORE_LOG_STORAGE_DEGRADED));
}

static void test_production_refuses_unprovisioned_network(void)
{
    app_core_fixture_t fixture;
    reset_fixture(&fixture);
    app_core_ops_t operations = make_operations(&fixture);
    const app_core_policy_t policy = {
        .development_provisioning_enabled = false,
        .development_ssid = NULL,
    };

    TEST_CHECK_EQ_INT(APP_ERROR_AUTH_REQUIRED,
                      app_core_sequence_start(&operations, &policy));
    TEST_CHECK_EQ_U64(0U, call_count(&fixture, "random_fill"));
    TEST_CHECK_EQ_U64(0U, call_count(&fixture, "wifi_start"));
    TEST_CHECK_EQ_U64(0U, call_count(&fixture, "http_start"));
    TEST_CHECK_EQ_U64(1U, call_count(&fixture, "storage_unmount"));
    TEST_CHECK_EQ_U64(1U, call_count(&fixture, "indicator_fatal"));
    TEST_CHECK_EQ_U64(1U, log_count(&fixture, APP_CORE_LOG_PROVISIONING_REQUIRED));
}

typedef enum {
    FAILURE_BOOT_INDICATOR = 0,
    FAILURE_NVS,
    FAILURE_STORAGE_MOUNT,
    FAILURE_STORAGE_RECOVERY,
    FAILURE_REPOSITORY,
    FAILURE_AUTH,
    FAILURE_USB,
    FAILURE_EXECUTOR,
    FAILURE_CONTROLS,
    FAILURE_RANDOM_AP,
    FAILURE_RANDOM_WEB,
    FAILURE_PASSWORD,
    FAILURE_WIFI,
    FAILURE_HTTP,
    FAILURE_READY_INDICATOR
} failure_point_t;

static app_error_code_t configure_failure(app_core_fixture_t *fixture,
                                          failure_point_t point)
{
    switch (point) {
    case FAILURE_BOOT_INDICATOR:
        fixture->indicator_failure_state = DEVICE_INDICATOR_BOOTING;
        fixture->indicator_failure_result = APP_ERROR_IO;
        return APP_ERROR_IO;
    case FAILURE_NVS:
        fixture->nvs_result = APP_CORE_NVS_OTHER_FAILURE;
        return APP_ERROR_STORAGE_UNAVAILABLE;
    case FAILURE_STORAGE_MOUNT:
        fixture->storage_mount_result = APP_ERROR_STORAGE_UNAVAILABLE;
        return APP_ERROR_STORAGE_UNAVAILABLE;
    case FAILURE_STORAGE_RECOVERY:
        fixture->storage_recover_result = APP_ERROR_IO;
        return APP_ERROR_IO;
    case FAILURE_REPOSITORY:
        fixture->repository_result = APP_ERROR_STORAGE_CORRUPT;
        return APP_ERROR_STORAGE_CORRUPT;
    case FAILURE_AUTH:
        fixture->auth_result = APP_ERROR_AUTH_FAILED;
        return APP_ERROR_AUTH_FAILED;
    case FAILURE_USB:
        fixture->usb_result = APP_ERROR_USB_NOT_READY;
        return APP_ERROR_USB_NOT_READY;
    case FAILURE_EXECUTOR:
        fixture->executor_result = APP_ERROR_INTERNAL;
        return APP_ERROR_INTERNAL;
    case FAILURE_CONTROLS:
        fixture->controls_result = APP_ERROR_IO;
        return APP_ERROR_IO;
    case FAILURE_RANDOM_AP:
        fixture->random_fail_on = 1U;
        fixture->random_failure_result = APP_ERROR_INTERNAL;
        return APP_ERROR_INTERNAL;
    case FAILURE_RANDOM_WEB:
        fixture->random_fail_on = 2U;
        fixture->random_failure_result = APP_ERROR_INTERNAL;
        return APP_ERROR_INTERNAL;
    case FAILURE_PASSWORD:
        fixture->password_result = APP_ERROR_INTERNAL;
        return APP_ERROR_INTERNAL;
    case FAILURE_WIFI:
        fixture->wifi_result = APP_ERROR_IO;
        return APP_ERROR_IO;
    case FAILURE_HTTP:
        fixture->http_result = APP_ERROR_INTERNAL;
        return APP_ERROR_INTERNAL;
    case FAILURE_READY_INDICATOR:
        fixture->indicator_failure_state = DEVICE_INDICATOR_READY;
        fixture->indicator_failure_result = APP_ERROR_IO;
        return APP_ERROR_IO;
    default:
        return APP_ERROR_INTERNAL;
    }
}

static void test_failure_matrix_and_cleanup(void)
{
    for (failure_point_t point = FAILURE_BOOT_INDICATOR;
         point <= FAILURE_READY_INDICATOR;
         point = (failure_point_t)((int)point + 1)) {
        app_core_fixture_t fixture;
        reset_fixture(&fixture);
        const app_error_code_t expected = configure_failure(&fixture, point);
        app_core_ops_t operations = make_operations(&fixture);
        const app_core_policy_t policy = development_policy();

        TEST_CHECK_EQ_INT(expected,
                          app_core_sequence_start(&operations, &policy));
        TEST_CHECK_EQ_U64(1U, call_count(&fixture, "indicator_fatal"));

        const bool storage_owned = point > FAILURE_STORAGE_MOUNT;
        const bool wifi_owned = point > FAILURE_WIFI;
        const bool web_owned = point > FAILURE_HTTP;
        TEST_CHECK_EQ_U64(storage_owned ? 1U : 0U,
                          call_count(&fixture, "storage_unmount"));
        TEST_CHECK_EQ_U64(wifi_owned ? 1U : 0U,
                          call_count(&fixture, "wifi_stop"));
        TEST_CHECK_EQ_U64(web_owned ? 1U : 0U,
                          call_count(&fixture, "http_stop"));

        if (web_owned) {
            assert_order(&fixture, "http_stop", "wifi_stop");
        }
        if (wifi_owned) {
            assert_order(&fixture, "wifi_stop", "storage_unmount");
        }
        if (storage_owned) {
            assert_order(&fixture, "storage_unmount", "indicator_fatal");
        }

        if (point == FAILURE_RANDOM_AP || point == FAILURE_RANDOM_WEB ||
            point == FAILURE_PASSWORD) {
            TEST_CHECK_EQ_U64(
                0U,
                log_count(&fixture, APP_CORE_LOG_DEVELOPMENT_CREDENTIALS));
            TEST_CHECK_EQ_U64(0U, call_count(&fixture, "wifi_start"));
        }
        if (point == FAILURE_WIFI) {
            TEST_CHECK_EQ_U64(0U, call_count(&fixture, "http_start"));
        }
    }
}

static void test_equal_credentials_retry_and_exhaustion(void)
{
    app_core_fixture_t fixture;
    reset_fixture(&fixture);
    fixture.random_plan[0] = 7U;
    fixture.random_plan[1] = 7U;
    fixture.random_plan[2] = 8U;
    fixture.random_plan_count = 3U;
    app_core_ops_t operations = make_operations(&fixture);
    const app_core_policy_t policy = development_policy();

    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      app_core_sequence_start(&operations, &policy));
    TEST_CHECK_EQ_U64(3U, fixture.random_call_count);
    const recorded_log_t *credentials =
        first_log(&fixture, APP_CORE_LOG_DEVELOPMENT_CREDENTIALS);
    TEST_CHECK(credentials != NULL);
    TEST_CHECK(strcmp(credentials->ap_passphrase, credentials->web_password) != 0);

    reset_fixture(&fixture);
    fixture.random_plan[0] = 7U;
    fixture.random_plan_count = 1U;
    operations = make_operations(&fixture);
    TEST_CHECK_EQ_INT(APP_ERROR_INTERNAL,
                      app_core_sequence_start(&operations, &policy));
    TEST_CHECK_EQ_U64(1U + APP_CORE_CREDENTIAL_RETRY_LIMIT,
                      fixture.random_call_count);
    TEST_CHECK_EQ_U64(0U, call_count(&fixture, "password_create"));
    TEST_CHECK_EQ_U64(0U, call_count(&fixture, "wifi_start"));
    TEST_CHECK_EQ_U64(0U,
                      log_count(&fixture, APP_CORE_LOG_DEVELOPMENT_CREDENTIALS));
    TEST_CHECK_EQ_U64(1U, call_count(&fixture, "storage_unmount"));
}

static void test_cleanup_errors_do_not_replace_original(void)
{
    app_core_fixture_t fixture;
    reset_fixture(&fixture);
    fixture.indicator_failure_state = DEVICE_INDICATOR_READY;
    fixture.indicator_failure_result = APP_ERROR_INTERNAL;
    fixture.http_stop_result = APP_ERROR_IO;
    fixture.wifi_stop_result = APP_ERROR_STORAGE_UNAVAILABLE;
    fixture.storage_unmount_result = APP_ERROR_STORAGE_CORRUPT;
    app_core_ops_t operations = make_operations(&fixture);
    const app_core_policy_t policy = development_policy();

    TEST_CHECK_EQ_INT(APP_ERROR_INTERNAL,
                      app_core_sequence_start(&operations, &policy));
    assert_order(&fixture, "http_stop", "wifi_stop");
    assert_order(&fixture, "wifi_stop", "storage_unmount");
    assert_order(&fixture, "storage_unmount", "indicator_fatal");
    TEST_CHECK_EQ_U64(1U, log_count(&fixture, APP_CORE_LOG_CLEANUP_FAILED));
    const recorded_log_t *cleanup =
        first_log(&fixture, APP_CORE_LOG_CLEANUP_FAILED);
    TEST_CHECK(cleanup != NULL);
    TEST_CHECK_EQ_INT(APP_ERROR_INTERNAL, cleanup->primary_error);
    TEST_CHECK_EQ_INT(APP_ERROR_IO, cleanup->secondary_error);
}

static void test_nvs_recovery_states_never_continue(void)
{
    static const app_core_nvs_result_t results[] = {
        APP_CORE_NVS_NO_FREE_PAGES,
        APP_CORE_NVS_NEW_VERSION_FOUND,
    };
    for (size_t index = 0U; index < (sizeof(results) / sizeof(results[0])); ++index) {
        app_core_fixture_t fixture;
        reset_fixture(&fixture);
        fixture.nvs_result = results[index];
        app_core_ops_t operations = make_operations(&fixture);
        const app_core_policy_t policy = development_policy();
        TEST_CHECK_EQ_INT(APP_ERROR_STORAGE_CORRUPT,
                          app_core_sequence_start(&operations, &policy));
        TEST_CHECK_EQ_U64(0U, call_count(&fixture, "storage_mount"));
        TEST_CHECK_EQ_U64(1U, call_count(&fixture, "indicator_fatal"));
    }
}

int main(void)
{
    test_nvs_mapping();
    test_invalid_arguments_and_missing_callbacks();
    test_success_order_and_distinct_credentials();
    test_degraded_storage_reaches_degraded_indicator();
    test_production_refuses_unprovisioned_network();
    test_failure_matrix_and_cleanup();
    test_equal_credentials_retry_and_exhaustion();
    test_cleanup_errors_do_not_replace_original();
    test_nvs_recovery_states_never_continue();
    puts("app core tests passed");
    return EXIT_SUCCESS;
}
