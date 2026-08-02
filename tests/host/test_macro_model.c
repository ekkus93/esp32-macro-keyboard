#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "macro_model.h"
#include "test_assert.h"

static char *duplicate_bytes(const char *text, size_t length) {
    TEST_CHECK(text != NULL || length == 0U);
    char *copy = malloc(length + 1U);
    TEST_CHECK(copy != NULL);
    if (length != 0U) {
        memcpy(copy, text, length);
    }
    copy[length] = '\0';
    return copy;
}

static char *duplicate_text(const char *text) {
    TEST_CHECK(text != NULL);
    return duplicate_bytes(text, strlen(text));
}

static void test_revision_boundaries(void) {
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT, macro_model_validate_revision(0U));
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, macro_model_validate_revision(1U));
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, macro_model_validate_revision(UINT32_MAX));
}

static void test_text_null_and_empty_policy(void) {
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, macro_model_validate_text(NULL, 0U, 0U));
    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      macro_model_validate_text(NULL, 0U, APP_MACRO_SOURCE_MAX_BYTES));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT, macro_model_validate_text(NULL, 1U, 1U));
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, macro_model_validate_text("", 0U, 0U));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT, macro_model_validate_text("a", 1U, 0U));
}

static void test_text_exact_limits(void) {
    char *text = malloc(APP_MACRO_SOURCE_MAX_BYTES + 1U);
    TEST_CHECK(text != NULL);
    memset(text, 'x', APP_MACRO_SOURCE_MAX_BYTES + 1U);

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, macro_model_validate_text(text, APP_MACRO_SOURCE_MAX_BYTES,
                                                                APP_MACRO_SOURCE_MAX_BYTES));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      macro_model_validate_text(text, APP_MACRO_SOURCE_MAX_BYTES + 1U,
                                                APP_MACRO_SOURCE_MAX_BYTES));
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, macro_model_validate_text(text, 1U, SIZE_MAX));
    free(text);
}

static void test_text_embedded_nul_and_byte_policy(void) {
    static const char embedded_nul[] = {'a', 'b', '\0', 'c'};
    TEST_CHECK_EQ_INT(
        APP_ERROR_INVALID_ARGUMENT,
        macro_model_validate_text(embedded_nul, sizeof(embedded_nul), sizeof(embedded_nul)));
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, macro_model_validate_text(embedded_nul, 2U, 2U));

    static const char trailing_nul[] = {'a', 'b', '\0'};
    TEST_CHECK_EQ_INT(
        APP_ERROR_INVALID_ARGUMENT,
        macro_model_validate_text(trailing_nul, sizeof(trailing_nul), sizeof(trailing_nul)));

    static const char non_ascii_utf8[] = "\xc3\xa9";
    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      macro_model_validate_text(non_ascii_utf8, sizeof(non_ascii_utf8) - 1U,
                                                sizeof(non_ascii_utf8) - 1U));

    static const char arbitrary_nonzero_bytes[] = {(char)0xff, (char)0x80};
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, macro_model_validate_text(arbitrary_nonzero_bytes,
                                                                sizeof(arbitrary_nonzero_bytes),
                                                                sizeof(arbitrary_nonzero_bytes)));
}

static void test_macro_cleanup_is_idempotent(void) {
    macro_model_free_macro(NULL);

    macro_t macro = {
        .schema_version = APP_SCHEMA_VERSION,
        .revision = 7U,
        .source = duplicate_text("macro source"),
        .source_length = strlen("macro source"),
        .favorite = true,
        .key_press_ms = 8U,
        .inter_key_ms = 15U,
    };
    macro_model_free_macro(&macro);
    TEST_CHECK(macro.source == NULL);
    TEST_CHECK_EQ_U64(0U, macro.source_length);
    TEST_CHECK_EQ_U64(APP_SCHEMA_VERSION, macro.schema_version);
    TEST_CHECK_EQ_U64(7U, macro.revision);
    TEST_CHECK(macro.favorite);
    TEST_CHECK_EQ_U64(8U, macro.key_press_ms);
    TEST_CHECK_EQ_U64(15U, macro.inter_key_ms);

    macro_model_free_macro(&macro);
    TEST_CHECK(macro.source == NULL);
    TEST_CHECK_EQ_U64(0U, macro.source_length);

    macro.source = duplicate_bytes("", 0U);
    macro.source_length = 0U;
    macro_model_free_macro(&macro);
    TEST_CHECK(macro.source == NULL);
    TEST_CHECK_EQ_U64(0U, macro.source_length);
}

static void test_procedure_cleanup_is_idempotent(void) {
    macro_model_free_procedure(NULL);

    procedure_t procedure = {
        .schema_version = APP_SCHEMA_VERSION,
        .steps = calloc(3U, sizeof(*procedure.steps)),
        .step_count = 3U,
        .revision = 7U,
        .sort_order = -5,
    };
    TEST_CHECK(procedure.steps != NULL);
    procedure.steps[0].body = duplicate_text("first");
    procedure.steps[0].body_length = strlen("first");
    procedure.steps[1].body = NULL;
    procedure.steps[1].body_length = 0U;
    procedure.steps[2].body = duplicate_text("third");
    procedure.steps[2].body_length = strlen("third");

    macro_model_free_procedure(&procedure);
    TEST_CHECK(procedure.steps == NULL);
    TEST_CHECK_EQ_U64(0U, procedure.step_count);
    TEST_CHECK_EQ_U64(APP_SCHEMA_VERSION, procedure.schema_version);
    TEST_CHECK_EQ_U64(7U, procedure.revision);
    TEST_CHECK_EQ_INT(-5, procedure.sort_order);

    macro_model_free_procedure(&procedure);
    TEST_CHECK(procedure.steps == NULL);
    TEST_CHECK_EQ_U64(0U, procedure.step_count);
}

static void test_partial_procedure_cleanup(void) {
    procedure_t without_array = {
        .steps = NULL,
        .step_count = APP_STEPS_PER_PROCEDURE_MAX,
    };
    macro_model_free_procedure(&without_array);
    TEST_CHECK(without_array.steps == NULL);
    TEST_CHECK_EQ_U64(0U, without_array.step_count);

    procedure_t partial = {
        .steps = calloc(APP_STEPS_PER_PROCEDURE_MAX, sizeof(*partial.steps)),
        .step_count = APP_STEPS_PER_PROCEDURE_MAX,
        .revision = UINT32_MAX,
    };
    TEST_CHECK(partial.steps != NULL);
    partial.steps[0].body = duplicate_text("first");
    partial.steps[0].body_length = strlen("first");
    partial.steps[APP_STEPS_PER_PROCEDURE_MAX - 1U].body = duplicate_text("last");
    partial.steps[APP_STEPS_PER_PROCEDURE_MAX - 1U].body_length = strlen("last");

    macro_model_free_procedure(&partial);
    TEST_CHECK(partial.steps == NULL);
    TEST_CHECK_EQ_U64(0U, partial.step_count);
    TEST_CHECK_EQ_U64(UINT32_MAX, partial.revision);
}

int main(void) {
    test_revision_boundaries();
    test_text_null_and_empty_policy();
    test_text_exact_limits();
    test_text_embedded_nul_and_byte_policy();
    test_macro_cleanup_is_idempotent();
    test_procedure_cleanup_is_idempotent();
    test_partial_procedure_cleanup();
    puts("macro model tests passed");
    return EXIT_SUCCESS;
}
