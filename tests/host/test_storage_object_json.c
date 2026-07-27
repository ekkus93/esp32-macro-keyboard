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
        .scope = MACRO_SCOPE_SET,
        .has_set_id = true,
        .set_id = uuid_value(2U),
        .source = "TYPE hello",
        .source_length = 10U,
        .favorite = true,
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
    TEST_CHECK(output.favorite);
    TEST_CHECK_EQ_U64(input.key_press_ms, output.key_press_ms);
    macro_model_free_macro(&output);
    cJSON_free(json);

    macro_t global = input;
    global.scope = MACRO_SCOPE_GLOBAL;
    global.has_set_id = false;
    memset(&global.set_id, 0, sizeof(global.set_id));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_serialize_macro_json(&global, &json, &length));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_parse_macro_json(json, length, &output));
    TEST_CHECK(output.scope == MACRO_SCOPE_GLOBAL);
    TEST_CHECK(!output.has_set_id);
    macro_model_free_macro(&output);
    cJSON_free(json);
}

static void test_macro_rejects_noncanonical_json(void) {
    static const char unknown[] =
        "{\"schema_version\":1,\"id\":\"00000001-0000-4000-8000-000000000001\","
        "\"revision\":1,\"scope\":\"global\",\"name\":\"x\",\"source\":\"\","
        "\"favorite\":false,\"key_press_ms\":8,\"inter_key_ms\":15,\"extra\":1}";
    macro_t output = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         storage_repository_parse_macro_json(unknown, strlen(unknown), &output));

    static const char duplicate[] =
        "{\"schema_version\":1,\"id\":\"00000001-0000-4000-8000-000000000001\","
        "\"revision\":1,\"scope\":\"global\",\"name\":\"x\",\"name\":\"y\","
        "\"source\":\"\",\"favorite\":false,\"key_press_ms\":8,\"inter_key_ms\":15}";
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         storage_repository_parse_macro_json(duplicate, strlen(duplicate), &output));

    static const char trailing[] =
        "{\"schema_version\":1,\"id\":\"00000001-0000-4000-8000-000000000001\","
        "\"revision\":1,\"scope\":\"global\",\"name\":\"x\",\"source\":\"\","
        "\"favorite\":false,\"key_press_ms\":8,\"inter_key_ms\":15}garbage";
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         storage_repository_parse_macro_json(trailing, strlen(trailing), &output));

    static const char embedded_nul[] =
        "{\"schema_version\":1,\"id\":\"00000001-0000-4000-8000-000000000001\","
        "\"revision\":1,\"scope\":\"global\",\"name\":\"x\\u0000hidden\","
        "\"source\":\"\",\"favorite\":false,\"key_press_ms\":8,\"inter_key_ms\":15}";
    TEST_CHECK_APP_ERROR(
        APP_ERROR_STORAGE_CORRUPT,
        storage_repository_parse_macro_json(embedded_nul, strlen(embedded_nul), &output));

    static const char wrong_scope[] =
        "{\"schema_version\":1,\"id\":\"00000001-0000-4000-8000-000000000001\","
        "\"revision\":1,\"scope\":\"set\",\"name\":\"x\",\"source\":\"\","
        "\"favorite\":false,\"key_press_ms\":8,\"inter_key_ms\":15}";
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         storage_repository_parse_macro_json(wrong_scope, strlen(wrong_scope),
                                                             &output));
}

static procedure_t procedure_value(void) {
    procedure_t procedure = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = uuid_value(10U),
        .revision = 3U,
        .set_id = uuid_value(2U),
        .step_count = 2U,
        .sort_order = -4,
    };
    TEST_CHECK_EQ_INT(9, snprintf(procedure.name, sizeof(procedure.name), "%s", "Provision"));
    TEST_CHECK_EQ_INT(4, snprintf(procedure.description, sizeof(procedure.description), "%s", "Test"));
    procedure.steps = calloc(procedure.step_count, sizeof(*procedure.steps));
    TEST_CHECK(procedure.steps != NULL);
    procedure.steps[0] = (procedure_step_t){
        .id = uuid_value(11U),
        .type = PROCEDURE_STEP_MACRO,
        .required = true,
        .auto_complete_on_success = true,
        .has_macro_id = true,
        .macro_id = uuid_value(1U),
    };
    TEST_CHECK_EQ_INT(4, snprintf(procedure.steps[0].title,
                                  sizeof(procedure.steps[0].title), "%s", "Type"));
    procedure.steps[1] = (procedure_step_t){
        .id = uuid_value(12U),
        .type = PROCEDURE_STEP_CHECKPOINT,
        .required = true,
        .body = strdup("Confirm output"),
        .body_length = 14U,
    };
    TEST_CHECK(procedure.steps[1].body != NULL);
    TEST_CHECK_EQ_INT(7, snprintf(procedure.steps[1].title,
                                  sizeof(procedure.steps[1].title), "%s", "Confirm"));
    return procedure;
}

static void test_procedure_round_trip(void) {
    procedure_t input = procedure_value();
    char *json = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_serialize_procedure_json(&input, &json, &length));
    procedure_t output = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_parse_procedure_json(json, length, &output));
    TEST_CHECK_EQ_U64(2U, output.step_count);
    TEST_CHECK(output.steps[0].type == PROCEDURE_STEP_MACRO);
    TEST_CHECK_EQ_UUID(&input.steps[0].macro_id, &output.steps[0].macro_id);
    TEST_CHECK(output.steps[1].type == PROCEDURE_STEP_CHECKPOINT);
    TEST_CHECK_EQ_STRING("Confirm output", output.steps[1].body);
    macro_model_free_procedure(&output);
    macro_model_free_procedure(&input);
    cJSON_free(json);
}

static void test_procedure_rejects_duplicate_steps(void) {
    procedure_t input = procedure_value();
    input.steps[1].id = input.steps[0].id;
    char *json = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_repository_serialize_procedure_json(&input, &json, &length));
    TEST_CHECK(json == NULL);
    macro_model_free_procedure(&input);
}

static procedure_progress_t progress_value(void) {
    procedure_progress_t progress = {
        .schema_version = APP_SCHEMA_VERSION,
        .set_id = uuid_value(2U),
        .procedure_id = uuid_value(10U),
        .procedure_revision = 3U,
        .current_step_id = uuid_value(12U),
        .completed_step_count = 1U,
        .skipped_step_count = 1U,
    };
    progress.completed_step_ids[0] = uuid_value(11U);
    progress.skipped_step_ids[0] = uuid_value(13U);
    return progress;
}

static void test_progress_and_order_round_trip(void) {
    procedure_progress_t progress = progress_value();
    char *json = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_serialize_progress_json(&progress, &json, &length));
    procedure_progress_t output = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_parse_progress_json(json, length, &output));
    TEST_CHECK_EQ_UUID(&progress.current_step_id, &output.current_step_id);
    TEST_CHECK_EQ_U64(1U, output.completed_step_count);
    cJSON_free(json);

    progress.skipped_step_ids[0] = progress.completed_step_ids[0];
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_repository_serialize_progress_json(&progress, &json, &length));

    storage_uuid_order_t order = {.count = 2U};
    order.ids[0] = uuid_value(1U);
    order.ids[1] = uuid_value(2U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_serialize_order_json(&order, 2U, &json, &length));
    storage_uuid_order_t parsed = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_parse_order_json(json, length, 2U, &parsed));
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
    test_procedure_round_trip();
    test_procedure_rejects_duplicate_steps();
    test_progress_and_order_round_trip();
    puts("storage object JSON tests passed");
    return EXIT_SUCCESS;
}
