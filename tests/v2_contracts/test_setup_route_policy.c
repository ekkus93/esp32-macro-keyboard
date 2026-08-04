#include <stdio.h>
#include <string.h>

#include "setup_route_policy_v2.h"

static int failures = 0;

static void expect_string(const char *name, const char *actual, const char *expected) {
    if (strcmp(actual, expected) != 0) {
        (void)fprintf(stderr, "[FAIL] %s differs\n", name);
        ++failures;
    }
}

static void expect_u16(const char *name, uint16_t actual, uint16_t expected) {
    if (actual != expected) {
        (void)fprintf(stderr, "[FAIL] %s differs\n", name);
        ++failures;
    }
}

int main(void) {
    expect_string("route path", APP_V2_SETUP_ROUTE_PATH, "/api/v1/setup");
    expect_string("GET method", APP_V2_SETUP_STATE_METHOD, "GET");
    expect_string("POST method", APP_V2_SETUP_SUBMIT_METHOD, "POST");
    expect_string("authentication", APP_V2_SETUP_AUTHENTICATION, "none");
    expect_string("JSON content type", APP_V2_SETUP_JSON_CONTENT_TYPE, "application/json");
    expect_string("POST body limit", APP_V2_SETUP_POST_BODY_LIMIT_NAME, "jsonBodyMaxBytes");

    if (APP_V2_SETUP_UNPROVISIONED_ROUTE_COUNT != UINT8_C(2) ||
        APP_V2_SETUP_OTHER_API_ROUTES_UNAVAILABLE != UINT8_C(1)) {
        (void)fprintf(stderr, "[FAIL] unprovisioned route surface differs\n");
        ++failures;
    }

    expect_u16("unprovisioned GET status", APP_V2_SETUP_GET_UNPROVISIONED_STATUS, UINT16_C(200));
    expect_u16("unprovisioned POST status", APP_V2_SETUP_POST_UNPROVISIONED_STATUS, UINT16_C(202));
    expect_u16("provisioned GET status", APP_V2_SETUP_GET_PROVISIONED_STATUS, UINT16_C(404));
    expect_u16("provisioned POST status", APP_V2_SETUP_POST_PROVISIONED_STATUS, UINT16_C(409));

    if (failures != 0) {
        (void)fprintf(stderr, "%d setup route policy test(s) failed\n", failures);
        return 1;
    }
    (void)printf("setup route policy contract passed\n");
    return 0;
}
