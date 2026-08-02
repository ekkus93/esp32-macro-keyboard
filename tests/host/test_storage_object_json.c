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

/* The set file is the unit of storage now (SPEC 12.1): its macros live inline,
 * in the array order that IS the user's order. The order-file round trip this
 * replaced tested a JSON shape that no longer exists. */
/* SPEC 12: "All persistent objects MUST contain: `schema_version`; stable ID;
 * revision number". The round trip is where that is enforceable -- a field the
 * writer omits or the reader ignores shows up as a document that does not come
 * back equal to what went in. */
static void test_set_document_round_trip(void) {
    /* set_macro()'s source is a string literal, so these are not freed. */
    macro_t macros[2] = {0};
    macros[0] = set_macro();
    macros[1] = set_macro();
    macros[1].id = uuid_value(7U);
    TEST_CHECK(snprintf(macros[1].name, sizeof(macros[1].name), "%s", "Second") > 0);

    const storage_set_document_t input = {
        .set = {.schema_version = APP_SCHEMA_VERSION,
                .id = uuid_value(3U),
                .revision = 4U,
                .name = "Round trip"},
        .macros = macros,
        .macro_count = 2U,
    };

    char *json = NULL;
    size_t length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_document_serialize(&input, &json, &length));
    /* A stored macro carries no set_id: the file it is in identifies the set
     * (SPEC 12.2). */
    TEST_CHECK(strstr(json, "set_id") == NULL);

    storage_set_document_t output = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_document_parse(json, length, &output));
    TEST_CHECK_EQ_U64(2U, output.macro_count);
    TEST_CHECK_EQ_UUID(&input.set.id, &output.set.id);
    TEST_CHECK_EQ_STRING("Round trip", output.set.name);
    /* Order is preserved exactly, because array position is the order. */
    TEST_CHECK_EQ_UUID(&macros[0].id, &output.macros[0].id);
    TEST_CHECK_EQ_UUID(&macros[1].id, &output.macros[1].id);
    /* Parsing stamps each macro with the set it was read from. */
    TEST_CHECK_EQ_UUID(&input.set.id, &output.macros[0].set_id);
    storage_set_document_free(&output);
    cJSON_free(json);
}

/* A set file that repeats a macro id is corrupt: nothing downstream can address
 * either copy unambiguously. */
static void test_set_document_rejects_duplicate_macro_ids(void) {
    static const char duplicated[] =
        "{\"schema_version\":1,\"id\":\"00000001-0000-4000-8000-000000000003\","
        "\"revision\":1,\"name\":\"Dup\",\"macros\":["
        "{\"schema_version\":1,\"id\":\"00000001-0000-4000-8000-000000000009\","
        "\"revision\":1,\"name\":\"a\",\"source\":\"a\",\"key_press_ms\":8,"
        "\"inter_key_ms\":15},"
        "{\"schema_version\":1,\"id\":\"00000001-0000-4000-8000-000000000009\","
        "\"revision\":1,\"name\":\"b\",\"source\":\"b\",\"key_press_ms\":8,"
        "\"inter_key_ms\":15}]}";
    storage_set_document_t output = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT,
                         storage_set_document_parse(duplicated, sizeof(duplicated) - 1U, &output));
    storage_set_document_free(&output);
}

/* A stored macro that carries set_id is not a stored macro, it is a package
 * entry in the wrong container (SPEC 12.2). */
static void test_set_document_rejects_stored_macro_with_set_id(void) {
    static const char with_set_id[] =
        "{\"schema_version\":1,\"id\":\"00000001-0000-4000-8000-000000000003\","
        "\"revision\":1,\"name\":\"Dup\",\"macros\":["
        "{\"schema_version\":1,\"id\":\"00000001-0000-4000-8000-000000000009\","
        "\"revision\":1,\"set_id\":\"00000001-0000-4000-8000-000000000003\","
        "\"name\":\"a\",\"source\":\"a\",\"key_press_ms\":8,\"inter_key_ms\":15}]}";
    storage_set_document_t output = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_STORAGE_CORRUPT,
        storage_set_document_parse(with_set_id, sizeof(with_set_id) - 1U, &output));
    storage_set_document_free(&output);
}

int main(void) {
    test_macro_round_trip();
    test_macro_rejects_noncanonical_json();
    test_set_document_round_trip();
    test_set_document_rejects_duplicate_macro_ids();
    test_set_document_rejects_stored_macro_with_set_id();
    puts("storage object JSON tests passed");
    return EXIT_SUCCESS;
}
