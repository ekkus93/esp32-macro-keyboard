#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_assert.h"
#include "wifi_ap.h"
#include "wifi_ap_station.h"

#define FAKE_MAX_RESULTS 8U

typedef struct {
    wifi_station_status_t status;
    app_error_code_t attempt_results[FAKE_MAX_RESULTS];
    size_t attempt_result_count;
    size_t attempt_calls;
    char last_ssid[64];
    char last_password[64];
    uint32_t last_timeout_ms;
} station_fixture_t;

static void reset_fixture(station_fixture_t *fixture) {
    memset(fixture, 0, sizeof(*fixture));
    fixture->status.state = WIFI_STATION_DISABLED;
}

static wifi_station_status_t fake_status_get(void *context) {
    const station_fixture_t *fixture = context;
    TEST_CHECK(fixture != NULL);
    return fixture->status;
}

static void fake_status_set(void *context, const wifi_station_status_t *status) {
    station_fixture_t *fixture = context;
    TEST_CHECK(fixture != NULL);
    TEST_CHECK(status != NULL);
    fixture->status = *status;
}

static app_error_code_t fake_connect_attempt(void *context, const char *ssid, const char *password,
                                             uint32_t timeout_ms, char *out_ip,
                                             size_t out_ip_size) {
    station_fixture_t *fixture = context;
    TEST_CHECK(fixture != NULL);
    TEST_CHECK(fixture->attempt_calls < FAKE_MAX_RESULTS);
    snprintf(fixture->last_ssid, sizeof(fixture->last_ssid), "%s", ssid);
    snprintf(fixture->last_password, sizeof(fixture->last_password), "%s", password);
    fixture->last_timeout_ms = timeout_ms;
    const app_error_code_t result = fixture->attempt_results[fixture->attempt_calls];
    ++fixture->attempt_calls;
    if (result == APP_ERROR_NONE) {
        TEST_CHECK(out_ip_size >= 8U);
        snprintf(out_ip, out_ip_size, "192.0.2.%zu", fixture->attempt_calls);
    }
    return result;
}

static wifi_ap_station_ops_t make_operations(station_fixture_t *fixture) {
    return (wifi_ap_station_ops_t){
        .context = fixture,
        .status_get = fake_status_get,
        .status_set = fake_status_set,
        .connect_attempt = fake_connect_attempt,
    };
}

static void test_operation_validation(void) {
    station_fixture_t fixture;
    reset_fixture(&fixture);
    wifi_ap_station_ops_t operations = make_operations(&fixture);
    TEST_CHECK(wifi_ap_station_ops_is_valid(&operations));
    TEST_CHECK(!wifi_ap_station_ops_is_valid(NULL));

    char ip[WIFI_STA_IP_STRING_BYTES];
    operations.status_get = NULL;
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        wifi_ap_station_connect(&operations, "ssid", "password", 1000U, ip, sizeof(ip)));

    operations = make_operations(&fixture);
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        wifi_ap_station_connect(&operations, NULL, "password", 1000U, ip, sizeof(ip)));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         wifi_ap_station_connect(&operations, "ssid", NULL, 1000U, ip, sizeof(ip)));
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        wifi_ap_station_connect(&operations, "ssid", "password", 1000U, NULL, sizeof(ip)));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         wifi_ap_station_connect(&operations, "ssid", "password", 1000U, ip, 0U));
    TEST_CHECK_EQ_U64(0U, fixture.attempt_calls);
}

/* SPEC_V2.md §12.1: a station join failure must not become an unbounded retry
 * loop. Every attempt fails; the engine gives up after exactly
 * WIFI_STATION_MAX_ATTEMPTS calls and reports WIFI_STATION_FAILED. */
static void test_bounded_retry_exhausted(void) {
    station_fixture_t fixture;
    reset_fixture(&fixture);
    for (size_t index = 0U; index < WIFI_STATION_MAX_ATTEMPTS; ++index) {
        fixture.attempt_results[index] = APP_ERROR_TIMEOUT;
    }
    fixture.attempt_result_count = WIFI_STATION_MAX_ATTEMPTS;
    const wifi_ap_station_ops_t operations = make_operations(&fixture);

    char ip[WIFI_STA_IP_STRING_BYTES] = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_TIMEOUT,
                         wifi_ap_station_connect(&operations, "OfficeWiFi", "correct horse battery",
                                                 1500U, ip, sizeof(ip)));
    TEST_CHECK_EQ_U64(WIFI_STATION_MAX_ATTEMPTS, fixture.attempt_calls);
    TEST_CHECK_EQ_INT(WIFI_STATION_FAILED, fixture.status.state);
    TEST_CHECK_APP_ERROR(APP_ERROR_TIMEOUT, fixture.status.last_error);
    TEST_CHECK_EQ_U64(WIFI_STATION_MAX_ATTEMPTS, fixture.status.attempt_count);
    TEST_CHECK(strcmp(fixture.last_ssid, "OfficeWiFi") == 0);
    TEST_CHECK(strcmp(fixture.last_password, "correct horse battery") == 0);
    TEST_CHECK_EQ_U64(1500U, fixture.last_timeout_ms);
}

/* Success on the very first attempt: no retries spent, status lands on
 * WIFI_STATION_CONNECTED, and the IP the attempt reported is returned. */
static void test_first_attempt_success(void) {
    station_fixture_t fixture;
    reset_fixture(&fixture);
    fixture.attempt_results[0] = APP_ERROR_NONE;
    const wifi_ap_station_ops_t operations = make_operations(&fixture);

    char ip[WIFI_STA_IP_STRING_BYTES] = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, wifi_ap_station_connect(&operations, "ssid", "password12",
                                                                 1000U, ip, sizeof(ip)));
    TEST_CHECK_EQ_U64(1U, fixture.attempt_calls);
    TEST_CHECK_EQ_INT(WIFI_STATION_CONNECTED, fixture.status.state);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, fixture.status.last_error);
    TEST_CHECK_EQ_U64(1U, fixture.status.attempt_count);
    TEST_CHECK(strcmp(ip, "192.0.2.1") == 0);
}

/* Success on the last permitted attempt: prior failures are absorbed and the
 * final status is still WIFI_STATION_CONNECTED, not WIFI_STATION_FAILED. */
static void test_success_on_last_attempt(void) {
    station_fixture_t fixture;
    reset_fixture(&fixture);
    for (size_t index = 0U; index + 1U < WIFI_STATION_MAX_ATTEMPTS; ++index) {
        fixture.attempt_results[index] = APP_ERROR_INTERNAL;
    }
    fixture.attempt_results[WIFI_STATION_MAX_ATTEMPTS - 1U] = APP_ERROR_NONE;
    const wifi_ap_station_ops_t operations = make_operations(&fixture);

    char ip[WIFI_STA_IP_STRING_BYTES] = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, wifi_ap_station_connect(&operations, "ssid", "password12",
                                                                 1000U, ip, sizeof(ip)));
    TEST_CHECK_EQ_U64(WIFI_STATION_MAX_ATTEMPTS, fixture.attempt_calls);
    TEST_CHECK_EQ_INT(WIFI_STATION_CONNECTED, fixture.status.state);
    TEST_CHECK_EQ_U64(WIFI_STATION_MAX_ATTEMPTS, fixture.status.attempt_count);
}

/* The status accessor defaults to WIFI_STATION_DISABLED until a connect is
 * ever attempted -- diagnostics must not report "connecting" for a device
 * that never had a station network configured. */
static void test_disabled_until_first_attempt(void) {
    station_fixture_t fixture;
    reset_fixture(&fixture);
    TEST_CHECK_EQ_INT(WIFI_STATION_DISABLED, fixture.status.state);
    TEST_CHECK_EQ_U64(0U, fixture.attempt_calls);
}

int main(void) {
    test_operation_validation();
    test_bounded_retry_exhausted();
    test_first_attempt_success();
    test_success_on_last_attempt();
    test_disabled_until_first_attempt();
    puts("Wi-Fi station bounded-retry tests passed");
    return EXIT_SUCCESS;
}
