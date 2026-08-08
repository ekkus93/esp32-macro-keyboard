#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "cJSON.h"
#include "test_assert.h"
#include "web_status_limits.h"

static web_status_snapshot_t valid_snapshot(void) {
    return (web_status_snapshot_t){
        .provisioned = true,
        .device_name = "Desk Macro Keyboard",
        .firmware_version = "0.2.0",
        .build_id = "git-abcdef0",
        .uptime_ms = 123456U,
        .usb_state = "ready",
        .access_point_state = "running",
        .access_point_ssid = "MacroKeyboard",
        .access_point_client_count = 1U,
        .station_configured = false,
        .station_state = "disabled",
        .station_ssid = NULL,
        .station_ipv4 = NULL,
        .storage_state = "ready",
        .storage_total_bytes = 524288U,
        .storage_used_bytes = 4096U,
        .storage_remaining_bytes = 520192U,
        .storage_blob_count = 3U,
        .send_present = false,
        .send_state = NULL,
    };
}

static void test_matches_spec_example(void) {
    const web_status_snapshot_t snapshot = valid_snapshot();
    char *json = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_status_response_json(&snapshot, &json));
    TEST_CHECK(json != NULL);

    cJSON *root = cJSON_Parse(json);
    TEST_CHECK(root != NULL);
    /* No v1 "ok"/"data" envelope: the response is the flat object itself. */
    TEST_CHECK(cJSON_GetObjectItemCaseSensitive(root, "ok") == NULL);
    TEST_CHECK(cJSON_GetObjectItemCaseSensitive(root, "data") == NULL);

    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "provisioned")));
    TEST_CHECK_EQ_STRING("Desk Macro Keyboard",
                         cJSON_GetObjectItemCaseSensitive(root, "deviceName")->valuestring);
    TEST_CHECK_EQ_STRING("0.2.0",
                         cJSON_GetObjectItemCaseSensitive(root, "firmwareVersion")->valuestring);
    TEST_CHECK_EQ_STRING("git-abcdef0",
                         cJSON_GetObjectItemCaseSensitive(root, "buildId")->valuestring);
    TEST_CHECK_EQ_U64(123456U,
                      (uint64_t)cJSON_GetObjectItemCaseSensitive(root, "uptimeMs")->valuedouble);

    const cJSON *usb = cJSON_GetObjectItemCaseSensitive(root, "usb");
    TEST_CHECK_EQ_STRING("ready", cJSON_GetObjectItemCaseSensitive(usb, "state")->valuestring);

    const cJSON *access_point = cJSON_GetObjectItemCaseSensitive(root, "accessPoint");
    TEST_CHECK_EQ_STRING("running",
                         cJSON_GetObjectItemCaseSensitive(access_point, "state")->valuestring);
    TEST_CHECK_EQ_STRING("MacroKeyboard",
                         cJSON_GetObjectItemCaseSensitive(access_point, "ssid")->valuestring);
    TEST_CHECK_EQ_U64(
        1U, (uint64_t)cJSON_GetObjectItemCaseSensitive(access_point, "clientCount")->valuedouble);

    const cJSON *station = cJSON_GetObjectItemCaseSensitive(root, "station");
    TEST_CHECK(cJSON_IsFalse(cJSON_GetObjectItemCaseSensitive(station, "configured")));
    TEST_CHECK_EQ_STRING("disabled",
                         cJSON_GetObjectItemCaseSensitive(station, "state")->valuestring);
    TEST_CHECK(cJSON_IsNull(cJSON_GetObjectItemCaseSensitive(station, "ssid")));
    TEST_CHECK(cJSON_IsNull(cJSON_GetObjectItemCaseSensitive(station, "ipv4")));

    const cJSON *storage = cJSON_GetObjectItemCaseSensitive(root, "storage");
    TEST_CHECK_EQ_STRING("ready", cJSON_GetObjectItemCaseSensitive(storage, "state")->valuestring);
    TEST_CHECK_EQ_U64(
        524288U, (uint64_t)cJSON_GetObjectItemCaseSensitive(storage, "totalBytes")->valuedouble);
    TEST_CHECK_EQ_U64(
        4096U, (uint64_t)cJSON_GetObjectItemCaseSensitive(storage, "usedBytes")->valuedouble);
    TEST_CHECK_EQ_U64(
        520192U,
        (uint64_t)cJSON_GetObjectItemCaseSensitive(storage, "remainingBytes")->valuedouble);
    TEST_CHECK_EQ_U64(
        3U, (uint64_t)cJSON_GetObjectItemCaseSensitive(storage, "blobCount")->valuedouble);

    const cJSON *send = cJSON_GetObjectItemCaseSensitive(root, "send");
    TEST_CHECK(cJSON_IsFalse(cJSON_GetObjectItemCaseSensitive(send, "present")));
    TEST_CHECK(cJSON_IsNull(cJSON_GetObjectItemCaseSensitive(send, "state")));

    /* Secrets and repository data never appear. */
    char *reprint = cJSON_PrintUnformatted(root);
    TEST_CHECK(strstr(reprint, "password") == NULL);
    TEST_CHECK(strstr(reprint, "passphrase") == NULL);
    TEST_CHECK(strstr(reprint, "macro") == NULL);
    TEST_CHECK(strstr(reprint, "package") == NULL);
    cJSON_free(reprint);

    cJSON_Delete(root);
    free(json);
}

static void test_send_present_carries_state(void) {
    web_status_snapshot_t snapshot = valid_snapshot();
    snapshot.send_present = true;
    snapshot.send_state = "running";
    char *json = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_status_response_json(&snapshot, &json));
    cJSON *root = cJSON_Parse(json);
    const cJSON *send = cJSON_GetObjectItemCaseSensitive(root, "send");
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(send, "present")));
    TEST_CHECK_EQ_STRING("running", cJSON_GetObjectItemCaseSensitive(send, "state")->valuestring);
    cJSON_Delete(root);
    free(json);
}

static void test_station_configured_carries_ssid_and_ip(void) {
    web_status_snapshot_t snapshot = valid_snapshot();
    snapshot.station_configured = true;
    snapshot.station_state = "unknown";
    snapshot.station_ssid = "OfficeWiFi";
    snapshot.station_ipv4 = "192.168.1.42";
    char *json = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_status_response_json(&snapshot, &json));
    cJSON *root = cJSON_Parse(json);
    const cJSON *station = cJSON_GetObjectItemCaseSensitive(root, "station");
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(station, "configured")));
    TEST_CHECK_EQ_STRING("OfficeWiFi",
                         cJSON_GetObjectItemCaseSensitive(station, "ssid")->valuestring);
    TEST_CHECK_EQ_STRING("192.168.1.42",
                         cJSON_GetObjectItemCaseSensitive(station, "ipv4")->valuestring);
    cJSON_Delete(root);
    free(json);
}

static void test_invalid_arguments_rejected(void) {
    char *json = (char *)0x1;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, web_status_response_json(NULL, &json));
    TEST_CHECK(json == NULL);

    const web_status_snapshot_t snapshot = valid_snapshot();
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, web_status_response_json(&snapshot, NULL));

    web_status_snapshot_t missing_name = valid_snapshot();
    missing_name.device_name = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_status_response_json(&missing_name, &json));

    web_status_snapshot_t send_present_without_state = valid_snapshot();
    send_present_without_state.send_present = true;
    send_present_without_state.send_state = NULL;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_status_response_json(&send_present_without_state, &json));
}

int main(void) {
    test_matches_spec_example();
    test_send_present_carries_state();
    test_station_configured_carries_ssid_and_ip();
    test_invalid_arguments_rejected();
    puts("web status/limits tests passed");
    return EXIT_SUCCESS;
}
