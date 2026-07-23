#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fake_wifi_backend.h"
#include "test_assert.h"
#include "wifi_ap_state.h"

typedef struct {
    fake_wifi_backend_t backend;
    wifi_ap_status_t status;
    wifi_ap_runtime_config_t configuration;
} wifi_fixture_t;

static void reset_fixture(wifi_fixture_t *fixture)
{
    TEST_CHECK(fixture != NULL);
    memset(fixture, 0, sizeof(*fixture));
    fake_wifi_backend_reset(&fixture->backend);
    fixture->status.state = WIFI_AP_STOPPED;
}

static app_error_code_t operation_result(wifi_fixture_t *fixture,
                                         fake_wifi_operation_t operation)
{
    return (app_error_code_t)fake_wifi_backend_call(&fixture->backend, operation);
}

static wifi_ap_status_t fake_status_get(void *context)
{
    const wifi_fixture_t *fixture = context;
    TEST_CHECK(fixture != NULL);
    return fixture->status;
}

static void fake_status_set(void *context, const wifi_ap_status_t *status)
{
    wifi_fixture_t *fixture = context;
    TEST_CHECK(fixture != NULL);
    TEST_CHECK(status != NULL);
    fixture->status = *status;
}

static app_error_code_t fake_netif_init(void *context)
{
    return operation_result(context, FAKE_WIFI_NETIF_INIT);
}

static wifi_ap_event_loop_result_t fake_event_loop_create(void *context)
{
    wifi_fixture_t *fixture = context;
    const int result = fake_wifi_backend_call(&fixture->backend, FAKE_WIFI_EVENT_LOOP);
    if (result == 0) {
        return WIFI_AP_EVENT_LOOP_CREATED;
    }
    return result == 1 ? WIFI_AP_EVENT_LOOP_ALREADY_EXISTS
                       : WIFI_AP_EVENT_LOOP_FAILED;
}

static app_error_code_t fake_netif_create(void *context)
{
    return operation_result(context, FAKE_WIFI_CREATE_AP);
}

static app_error_code_t fake_wifi_init(void *context)
{
    return operation_result(context, FAKE_WIFI_INIT);
}

static app_error_code_t fake_handler_register(void *context)
{
    return operation_result(context, FAKE_WIFI_REGISTER_HANDLER);
}

static app_error_code_t fake_set_mode_ap(void *context)
{
    return operation_result(context, FAKE_WIFI_SET_MODE);
}

static app_error_code_t fake_set_config(
    void *context,
    const wifi_ap_runtime_config_t *configuration)
{
    wifi_fixture_t *fixture = context;
    TEST_CHECK(configuration != NULL);
    fixture->configuration = *configuration;
    fake_wifi_backend_capture_config(&fixture->backend,
                                     configuration->ssid,
                                     configuration->passphrase,
                                     configuration->maximum_clients);
    return operation_result(fixture, FAKE_WIFI_SET_CONFIG);
}

static app_error_code_t fake_wifi_start(void *context)
{
    return operation_result(context, FAKE_WIFI_START);
}

static app_error_code_t fake_wifi_stop(void *context)
{
    return operation_result(context, FAKE_WIFI_STOP);
}

static app_error_code_t fake_handler_unregister(void *context)
{
    return operation_result(context, FAKE_WIFI_UNREGISTER_HANDLER);
}

static app_error_code_t fake_wifi_deinit(void *context)
{
    return operation_result(context, FAKE_WIFI_DEINIT);
}

static app_error_code_t fake_netif_destroy(void *context)
{
    return operation_result(context, FAKE_WIFI_DESTROY_AP);
}

static wifi_ap_ops_t make_operations(wifi_fixture_t *fixture)
{
    return (wifi_ap_ops_t){
        .context = fixture,
        .status_get = fake_status_get,
        .status_set = fake_status_set,
        .netif_init = fake_netif_init,
        .event_loop_create = fake_event_loop_create,
        .netif_create = fake_netif_create,
        .wifi_init = fake_wifi_init,
        .handler_register = fake_handler_register,
        .set_mode_ap = fake_set_mode_ap,
        .set_config = fake_set_config,
        .wifi_start = fake_wifi_start,
        .wifi_stop = fake_wifi_stop,
        .handler_unregister = fake_handler_unregister,
        .wifi_deinit = fake_wifi_deinit,
        .netif_destroy = fake_netif_destroy,
    };
}

static size_t call_count(const wifi_fixture_t *fixture, const char *name)
{
    size_t count = 0U;
    for (size_t index = 0U; index < fixture->backend.calls.call_count; ++index) {
        const fake_call_t *call = fake_call_log_at(&fixture->backend.calls, index);
        if (call != NULL && strcmp(call->name, name) == 0) {
            ++count;
        }
    }
    return count;
}

static size_t call_index(const wifi_fixture_t *fixture, const char *name)
{
    for (size_t index = 0U; index < fixture->backend.calls.call_count; ++index) {
        const fake_call_t *call = fake_call_log_at(&fixture->backend.calls, index);
        if (call != NULL && strcmp(call->name, name) == 0) {
            return index;
        }
    }
    return SIZE_MAX;
}

static void assert_order(const wifi_fixture_t *fixture,
                         const char *before,
                         const char *after)
{
    const size_t before_index = call_index(fixture, before);
    const size_t after_index = call_index(fixture, after);
    TEST_CHECK(before_index != SIZE_MAX);
    TEST_CHECK(after_index != SIZE_MAX);
    TEST_CHECK(before_index < after_index);
}

static void initialize_engine(wifi_fixture_t *fixture, wifi_ap_engine_t *engine)
{
    const wifi_ap_ops_t operations = make_operations(fixture);
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, wifi_ap_engine_init(engine, &operations));
    TEST_CHECK_EQ_INT(WIFI_AP_STOPPED, fixture->status.state);
    TEST_CHECK_EQ_U64(0U, fixture->status.client_count);
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, fixture->status.last_error);
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, fixture->status.cleanup_error);
}

static void start_successfully(wifi_fixture_t *fixture, wifi_ap_engine_t *engine)
{
    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      wifi_ap_engine_start(engine,
                                           "ESP32-Macro-Setup",
                                           "correct horse battery"));
    TEST_CHECK_EQ_INT(WIFI_AP_STARTING, fixture->status.state);
}

static void test_operation_validation(void)
{
    wifi_fixture_t fixture;
    reset_fixture(&fixture);
    wifi_ap_engine_t engine;
    wifi_ap_ops_t operations = make_operations(&fixture);
    TEST_CHECK(wifi_ap_ops_is_valid(&operations));
    TEST_CHECK(!wifi_ap_ops_is_valid(NULL));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      wifi_ap_engine_init(NULL, &operations));

#define CHECK_MISSING(member)                                                          \
    do {                                                                               \
        operations = make_operations(&fixture);                                        \
        operations.member = NULL;                                                      \
        TEST_CHECK(!wifi_ap_ops_is_valid(&operations));                                \
        TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,                                  \
                          wifi_ap_engine_init(&engine, &operations));                   \
    } while (0)

    CHECK_MISSING(status_get);
    CHECK_MISSING(status_set);
    CHECK_MISSING(netif_init);
    CHECK_MISSING(event_loop_create);
    CHECK_MISSING(netif_create);
    CHECK_MISSING(wifi_init);
    CHECK_MISSING(handler_register);
    CHECK_MISSING(set_mode_ap);
    CHECK_MISSING(set_config);
    CHECK_MISSING(wifi_start);
    CHECK_MISSING(wifi_stop);
    CHECK_MISSING(handler_unregister);
    CHECK_MISSING(wifi_deinit);
    CHECK_MISSING(netif_destroy);

#undef CHECK_MISSING
}

static void test_credentials_and_configuration(void)
{
    wifi_fixture_t fixture;
    reset_fixture(&fixture);
    wifi_ap_engine_t engine;
    initialize_engine(&fixture, &engine);

    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      wifi_ap_engine_start(&engine, NULL, "123456789012"));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      wifi_ap_engine_start(&engine, "", "123456789012"));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      wifi_ap_engine_start(&engine, "ssid", NULL));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      wifi_ap_engine_start(&engine, "ssid", "12345678901"));

    char long_ssid[WIFI_AP_SSID_MAX_BYTES + 2U];
    memset(long_ssid, 's', sizeof(long_ssid) - 1U);
    long_ssid[sizeof(long_ssid) - 1U] = '\0';
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      wifi_ap_engine_start(&engine,
                                           long_ssid,
                                           "123456789012"));

    char long_passphrase[WIFI_AP_PASSPHRASE_MAX_BYTES + 2U];
    memset(long_passphrase, 'p', sizeof(long_passphrase) - 1U);
    long_passphrase[sizeof(long_passphrase) - 1U] = '\0';
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      wifi_ap_engine_start(&engine, "ssid", long_passphrase));
    TEST_CHECK_EQ_U64(0U, fixture.backend.calls.call_count);

    char maximum_ssid[WIFI_AP_SSID_MAX_BYTES + 1U];
    memset(maximum_ssid, 's', sizeof(maximum_ssid) - 1U);
    maximum_ssid[sizeof(maximum_ssid) - 1U] = '\0';
    char maximum_passphrase[WIFI_AP_PASSPHRASE_MAX_BYTES + 1U];
    memset(maximum_passphrase, 'p', sizeof(maximum_passphrase) - 1U);
    maximum_passphrase[sizeof(maximum_passphrase) - 1U] = '\0';

    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      wifi_ap_engine_start(&engine,
                                           maximum_ssid,
                                           maximum_passphrase));
    TEST_CHECK_EQ_U64(WIFI_AP_SSID_MAX_BYTES,
                      fixture.configuration.ssid_length);
    TEST_CHECK_EQ_STRING(maximum_ssid, fixture.configuration.ssid);
    TEST_CHECK_EQ_STRING(maximum_passphrase, fixture.configuration.passphrase);
    TEST_CHECK_EQ_INT(WIFI_AP_DEFAULT_CHANNEL, fixture.configuration.channel);
    TEST_CHECK_EQ_INT(WIFI_AP_MAX_CLIENTS,
                      fixture.configuration.maximum_clients);
    TEST_CHECK(fixture.configuration.wpa2_wpa3_psk);
    TEST_CHECK(fixture.configuration.pmf_required);
    TEST_CHECK_EQ_INT('\0',
                      fixture.configuration.ssid[WIFI_AP_SSID_MAX_BYTES]);
    TEST_CHECK_EQ_INT('\0',
                      fixture.configuration
                          .passphrase[WIFI_AP_PASSPHRASE_MAX_BYTES]);
    TEST_CHECK_EQ_INT(APP_ERROR_CONFLICT,
                      wifi_ap_engine_start(&engine,
                                           "second",
                                           "123456789012"));
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, wifi_ap_engine_stop(&engine));
}

static void test_minimum_credentials_and_existing_event_loop(void)
{
    wifi_fixture_t fixture;
    reset_fixture(&fixture);
    fake_wifi_backend_set_result(&fixture.backend, FAKE_WIFI_EVENT_LOOP, 1);
    wifi_ap_engine_t engine;
    initialize_engine(&fixture, &engine);
    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      wifi_ap_engine_start(&engine, "s", "123456789012"));
    TEST_CHECK_EQ_STRING("s", fixture.configuration.ssid);
    TEST_CHECK_EQ_STRING("123456789012", fixture.configuration.passphrase);
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, wifi_ap_engine_stop(&engine));
}

static void test_events_and_client_saturation(void)
{
    wifi_fixture_t fixture;
    reset_fixture(&fixture);
    wifi_ap_engine_t engine;
    initialize_engine(&fixture, &engine);
    start_successfully(&fixture, &engine);

    wifi_ap_engine_handle_event(&engine, WIFI_AP_EVENT_STARTED);
    TEST_CHECK_EQ_INT(WIFI_AP_READY, fixture.status.state);
    for (size_t index = 0U; index < WIFI_AP_MAX_CLIENTS + 3U; ++index) {
        wifi_ap_engine_handle_event(&engine, WIFI_AP_EVENT_CLIENT_CONNECTED);
    }
    TEST_CHECK_EQ_U64(WIFI_AP_MAX_CLIENTS, fixture.status.client_count);
    for (size_t index = 0U; index < WIFI_AP_MAX_CLIENTS + 3U; ++index) {
        wifi_ap_engine_handle_event(&engine,
                                    WIFI_AP_EVENT_CLIENT_DISCONNECTED);
    }
    TEST_CHECK_EQ_U64(0U, fixture.status.client_count);

    const wifi_ap_status_t before_unknown = fixture.status;
    wifi_ap_engine_handle_event(&engine, WIFI_AP_EVENT_UNKNOWN);
    TEST_CHECK_EQ_BUFFER(&before_unknown, &fixture.status, sizeof(before_unknown));

    wifi_ap_engine_handle_event(&engine, WIFI_AP_EVENT_STOPPED);
    TEST_CHECK_EQ_INT(WIFI_AP_STOPPED, fixture.status.state);
    TEST_CHECK_EQ_U64(0U, fixture.status.client_count);
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, wifi_ap_engine_stop(&engine));

    wifi_ap_engine_handle_event(NULL, WIFI_AP_EVENT_STARTED);
}

typedef struct {
    fake_wifi_operation_t operation;
    int fake_result;
    app_error_code_t expected_error;
    const char *forbidden_later_call;
} start_failure_case_t;

static void test_start_failure_matrix(void)
{
    static const start_failure_case_t cases[] = {
        {FAKE_WIFI_NETIF_INIT,
         (int)APP_ERROR_STORAGE_UNAVAILABLE,
         APP_ERROR_STORAGE_UNAVAILABLE,
         "wifi_event_loop"},
        {FAKE_WIFI_EVENT_LOOP, 2, APP_ERROR_INTERNAL, "wifi_create_ap"},
        {FAKE_WIFI_CREATE_AP,
         (int)APP_ERROR_INTERNAL,
         APP_ERROR_INTERNAL,
         "wifi_init"},
        {FAKE_WIFI_INIT,
         (int)APP_ERROR_INTERNAL,
         APP_ERROR_INTERNAL,
         "wifi_register_handler"},
        {FAKE_WIFI_REGISTER_HANDLER,
         (int)APP_ERROR_INTERNAL,
         APP_ERROR_INTERNAL,
         "wifi_set_mode"},
        {FAKE_WIFI_SET_MODE,
         (int)APP_ERROR_IO,
         APP_ERROR_IO,
         "wifi_capture_config"},
        {FAKE_WIFI_SET_CONFIG,
         (int)APP_ERROR_IO,
         APP_ERROR_IO,
         "wifi_start"},
        {FAKE_WIFI_START, (int)APP_ERROR_IO, APP_ERROR_IO, NULL},
    };

    for (size_t index = 0U; index < (sizeof(cases) / sizeof(cases[0])); ++index) {
        wifi_fixture_t fixture;
        reset_fixture(&fixture);
        fake_wifi_backend_set_result(&fixture.backend,
                                     cases[index].operation,
                                     cases[index].fake_result);
        wifi_ap_engine_t engine;
        initialize_engine(&fixture, &engine);

        TEST_CHECK_EQ_INT(cases[index].expected_error,
                          wifi_ap_engine_start(&engine,
                                               "ssid",
                                               "123456789012"));
        TEST_CHECK_EQ_INT(WIFI_AP_ERROR, fixture.status.state);
        TEST_CHECK_EQ_INT(cases[index].expected_error,
                          fixture.status.last_error);
        TEST_CHECK_EQ_INT(APP_ERROR_NONE, fixture.status.cleanup_error);
        if (cases[index].forbidden_later_call != NULL) {
            TEST_CHECK_EQ_U64(0U,
                              call_count(&fixture,
                                         cases[index].forbidden_later_call));
        }

        if (cases[index].operation == FAKE_WIFI_INIT) {
            TEST_CHECK_EQ_U64(1U, call_count(&fixture, "wifi_destroy_ap"));
        }
        if (cases[index].operation == FAKE_WIFI_REGISTER_HANDLER) {
            assert_order(&fixture, "wifi_deinit", "wifi_destroy_ap");
        }
        if (cases[index].operation == FAKE_WIFI_SET_MODE ||
            cases[index].operation == FAKE_WIFI_SET_CONFIG) {
            assert_order(&fixture,
                         "wifi_unregister_handler",
                         "wifi_deinit");
            assert_order(&fixture, "wifi_deinit", "wifi_destroy_ap");
        }
        if (cases[index].operation == FAKE_WIFI_START) {
            assert_order(&fixture, "wifi_stop", "wifi_unregister_handler");
            assert_order(&fixture,
                         "wifi_unregister_handler",
                         "wifi_deinit");
            assert_order(&fixture, "wifi_deinit", "wifi_destroy_ap");
        }

        TEST_CHECK_EQ_INT(APP_ERROR_NONE, wifi_ap_engine_stop(&engine));
        TEST_CHECK_EQ_INT(WIFI_AP_STOPPED, fixture.status.state);
    }
}

static void test_start_cleanup_failure_is_visible_and_retriable(void)
{
    wifi_fixture_t fixture;
    reset_fixture(&fixture);
    fake_wifi_backend_set_result(&fixture.backend,
                                 FAKE_WIFI_SET_MODE,
                                 (int)APP_ERROR_IO);
    fake_wifi_backend_set_result(&fixture.backend,
                                 FAKE_WIFI_UNREGISTER_HANDLER,
                                 (int)APP_ERROR_INTERNAL);
    wifi_ap_engine_t engine;
    initialize_engine(&fixture, &engine);

    TEST_CHECK_EQ_INT(APP_ERROR_IO,
                      wifi_ap_engine_start(&engine,
                                           "ssid",
                                           "123456789012"));
    TEST_CHECK_EQ_INT(WIFI_AP_ERROR, fixture.status.state);
    TEST_CHECK_EQ_INT(APP_ERROR_IO, fixture.status.last_error);
    TEST_CHECK_EQ_INT(APP_ERROR_INTERNAL, fixture.status.cleanup_error);
    TEST_CHECK(engine.handler_registered);
    TEST_CHECK(engine.wifi_initialized);
    TEST_CHECK(engine.netif_created);

    fake_wifi_backend_set_result(&fixture.backend,
                                 FAKE_WIFI_UNREGISTER_HANDLER,
                                 (int)APP_ERROR_NONE);
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, wifi_ap_engine_stop(&engine));
    TEST_CHECK_EQ_INT(WIFI_AP_STOPPED, fixture.status.state);
    TEST_CHECK(!engine.handler_registered);
    TEST_CHECK(!engine.wifi_initialized);
    TEST_CHECK(!engine.netif_created);
}

static void test_stop_failure_matrix_and_retry(void)
{
    static const fake_wifi_operation_t cleanup_operations[] = {
        FAKE_WIFI_STOP,
        FAKE_WIFI_UNREGISTER_HANDLER,
        FAKE_WIFI_DEINIT,
        FAKE_WIFI_DESTROY_AP,
    };
    for (size_t index = 0U;
         index < (sizeof(cleanup_operations) / sizeof(cleanup_operations[0]));
         ++index) {
        wifi_fixture_t fixture;
        reset_fixture(&fixture);
        wifi_ap_engine_t engine;
        initialize_engine(&fixture, &engine);
        start_successfully(&fixture, &engine);
        fake_wifi_backend_set_result(&fixture.backend,
                                     cleanup_operations[index],
                                     (int)APP_ERROR_INTERNAL);

        TEST_CHECK_EQ_INT(APP_ERROR_INTERNAL, wifi_ap_engine_stop(&engine));
        TEST_CHECK_EQ_INT(WIFI_AP_ERROR, fixture.status.state);
        TEST_CHECK_EQ_INT(APP_ERROR_INTERNAL, fixture.status.last_error);
        TEST_CHECK_EQ_INT(APP_ERROR_INTERNAL, fixture.status.cleanup_error);

        fake_wifi_backend_set_result(&fixture.backend,
                                     cleanup_operations[index],
                                     (int)APP_ERROR_NONE);
        TEST_CHECK_EQ_INT(APP_ERROR_NONE, wifi_ap_engine_stop(&engine));
        TEST_CHECK_EQ_INT(WIFI_AP_STOPPED, fixture.status.state);
        TEST_CHECK_EQ_INT(APP_ERROR_NONE, fixture.status.last_error);
        TEST_CHECK_EQ_INT(APP_ERROR_NONE, fixture.status.cleanup_error);
    }
}

static void test_stop_when_already_stopped(void)
{
    wifi_fixture_t fixture;
    reset_fixture(&fixture);
    wifi_ap_engine_t engine;
    initialize_engine(&fixture, &engine);
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, wifi_ap_engine_stop(&engine));
    TEST_CHECK_EQ_U64(0U, fixture.backend.calls.call_count);

    wifi_ap_engine_t uninitialized = {0};
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      wifi_ap_engine_start(&uninitialized,
                                           "ssid",
                                           "123456789012"));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      wifi_ap_engine_stop(&uninitialized));
}

int main(void)
{
    test_operation_validation();
    test_credentials_and_configuration();
    test_minimum_credentials_and_existing_event_loop();
    test_events_and_client_saturation();
    test_start_failure_matrix();
    test_start_cleanup_failure_is_visible_and_retriable();
    test_stop_failure_matrix_and_retry();
    test_stop_when_already_stopped();
    puts("Wi-Fi AP tests passed");
    return EXIT_SUCCESS;
}
