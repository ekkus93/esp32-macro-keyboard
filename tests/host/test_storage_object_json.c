#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "macro_model.h"
#include "storage_repository_objects_json.h"
#include "test_assert.h"

static app_uuid_t uuid_value(uint32_t value) {
    char text[APP_UUID_BUFFER_LENGTH];
    TEST_CHECK_EQ_INT(36, snprintf(text, sizeof(text), "%08x-0000-4000-8000-%012x", value, value));
    app_uuid_t uuid = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_parse(text, &uuid));
    return uuid;
}

static macro_t set_macro(void) {
    macro_t macro = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = uuid_value(1U),
        .revision = 1U,
        .set_id = uuid_value(2U),
        .source = "TYPE hello",
        .source_length = 10U,
        .key_press_ms = 8U,
        .inter_key_ms = 15U,
    };
    TEST_CHECK_EQ_INT(5, snprintf(macro.name, sizeof(macro.name), "%s", "Hello"));
    return macro;
}

static void test_macro_round_trip(void) {
    macro_t input = set_macro();
    char *json = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_serialize_macro_json(&input, &json, &length));
    TEST_CHECK(json != NULL);
    macro_t output = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_parse_macro_json(json, length, &output));
    TEST_CHECK_EQ_UUID(&input.id, &output.id);
    TEST_CHECK_EQ_UUID(&input.set_id, &output.set_id);
    TEST_CHECK_EQ_STRING(input.name, output.name);
    TEST_CHECK_EQ_STRING(input.source, output.source);
    TEST_CHECK_EQ_U64(input.key_press_ms, output.key_press_ms);
    macro_model_free_macro(&output);
    cJSON_free(json);
}

static void test_macro_rejects_noncanonical_json(void) {
    static const char unknown[] =
        "{\"schema_version\":1,\"id\":\"00000001-0000-4000-8000-000000000001\","
        "\"revision\":1,\"scope\":\"global\",\"name\":\"x\",\"source\":\"\","
        "\"key_press_ms\":8,\"inter_key_ms\":15,\"extra\":1}";
    macro_t output = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         storage_repository_parse_macro_json(unknown, strlen(unknown), &output));

    static const char duplicate[] =
        "{\"schema_version\":1,\"id\":\"00000001-0000-4000-8000-000000000001\","
        "\"revision\":1,\"scope\":\"global\",\"name\":\"x\",\"name\":\"y\","
        "\"source\":\"\",\"key_press_ms\":8,\"inter_key_ms\":15}";
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, storage_repository_parse_macro_json(
                                                        duplicate, strlen(duplicate), &output));

    static const char trailing[] =
        "{\"schema_version\":1,\"id\":\"00000001-0000-4000-8000-000000000001\","
        "\"revision\":1,\"scope\":\"global\",\"name\":\"x\",\"source\":\"\","
        "\"key_press_ms\":8,\"inter_key_ms\":15}garbage";
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         storage_repository_parse_macro_json(trailing, strlen(trailing), &output));

    static const char embedded_nul[] =
        "{\"schema_version\":1,\"id\":\"00000001-0000-4000-8000-000000000001\","
        "\"revision\":1,\"scope\":\"global\",\"name\":\"x\\u0000hidden\","
        "\"source\":\"\",\"key_press_ms\":8,\"inter_key_ms\":15}";
    TEST_CHECK_APP_ERROR(
        APP_ERROR_STORAGE_CORRUPT,
        storage_repository_parse_macro_json(embedded_nul, strlen(embedded_nul), &output));

    static const char wrong_scope[] =
        "{\"schema_version\":1,\"id\":\"00000001-0000-4000-8000-000000000001\","
        "\"revision\":1,\"scope\":\"set\",\"name\":\"x\",\"source\":\"\","
        "\"key_press_ms\":8,\"inter_key_ms\":15}";
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, storage_repository_parse_macro_json(
                                                        wrong_scope, strlen(wrong_scope), &output));
}

static void test_order_round_trip(void) {
    char *json = NULL;
    size_t length = 0U;
    storage_uuid_order_t order = {.count = 2U};
    order.ids[0] = uuid_value(1U);
    order.ids[1] = uuid_value(2U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_serialize_order_json(&order, 2U, &json, &length));
    storage_uuid_order_t parsed = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_parse_order_json(json, length, &parsed, 2U));
    TEST_CHECK_EQ_U64(2U, parsed.count);
    TEST_CHECK_EQ_UUID(&order.ids[1], &parsed.ids[1]);
    cJSON_free(json);

    order.ids[1] = order.ids[0];
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_repository_serialize_order_json(&order, 2U, &json, &length));
}

int main(void) {
    test_macro_round_trip();
    test_macro_rejects_noncanonical_json();
    test_order_round_trip();
    puts("storage object JSON tests passed");
    return EXIT_SUCCESS;
}
