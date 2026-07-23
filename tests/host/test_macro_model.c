#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "macro_model.h"
#include "test_assert.h"

static char *duplicate_text(const char *text)
{
    TEST_CHECK(text != NULL);
    const size_t length = strlen(text);
    char *copy = malloc(length + 1U);
    TEST_CHECK(copy != NULL);
    memcpy(copy, text, length + 1U);
    return copy;
}

static void test_revision_boundaries(void)
{
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      macro_model_validate_revision(0U));
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, macro_model_validate_revision(1U));
    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      macro_model_validate_revision(UINT32_MAX));
}

static void test_text_boundaries(void)
{
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, macro_model_validate_text(NULL, 0U, 0U));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      macro_model_validate_text(NULL, 1U, 1U));
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, macro_model_validate_text("", 0U, 0U));
    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      macro_model_validate_text("abcd", 4U, 4U));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      macro_model_validate_text("abcd", 4U, 3U));

    static const char embedded_nul[] = {'a', 'b', '\0', 'c'};
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      macro_model_validate_text(embedded_nul,
                                                sizeof(embedded_nul),
                                                sizeof(embedded_nul)));
    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      macro_model_validate_text(embedded_nul, 2U, 2U));
}

static void test_macro_cleanup_is_idempotent(void)
{
    macro_model_free_macro(NULL);

    macro_t macro = {
        .source = duplicate_text("macro source"),
        .source_length = strlen("macro source"),
        .has_set_id = true,
        .favorite = true,
    };
    macro_model_free_macro(&macro);
    TEST_CHECK(macro.source == NULL);
    TEST_CHECK_EQ_U64(0U, macro.source_length);
    TEST_CHECK(macro.has_set_id);
    TEST_CHECK(macro.favorite);

    macro_model_free_macro(&macro);
    TEST_CHECK(macro.source == NULL);
    TEST_CHECK_EQ_U64(0U, macro.source_length);
}

static void test_procedure_cleanup_is_idempotent(void)
{
    macro_model_free_procedure(NULL);

    procedure_t procedure = {
        .steps = calloc(3U, sizeof(*procedure.steps)),
        .step_count = 3U,
        .revision = 7U,
    };
    TEST_CHECK(procedure.steps != NULL);
    procedure.steps[0].body = duplicate_text("first");
    procedure.steps[0].body_length = strlen("first");
    procedure.steps[2].body = duplicate_text("third");
    procedure.steps[2].body_length = strlen("third");

    macro_model_free_procedure(&procedure);
    TEST_CHECK(procedure.steps == NULL);
    TEST_CHECK_EQ_U64(0U, procedure.step_count);
    TEST_CHECK_EQ_U64(7U, procedure.revision);

    macro_model_free_procedure(&procedure);
    TEST_CHECK(procedure.steps == NULL);
    TEST_CHECK_EQ_U64(0U, procedure.step_count);
}

static void test_partial_procedure_without_step_array(void)
{
    procedure_t procedure = {
        .steps = NULL,
        .step_count = APP_STEPS_PER_PROCEDURE_MAX,
    };
    macro_model_free_procedure(&procedure);
    TEST_CHECK(procedure.steps == NULL);
    TEST_CHECK_EQ_U64(0U, procedure.step_count);
}

int main(void)
{
    test_revision_boundaries();
    test_text_boundaries();
    test_macro_cleanup_is_idempotent();
    test_procedure_cleanup_is_idempotent();
    test_partial_procedure_without_step_array();
    puts("macro model tests passed");
    return EXIT_SUCCESS;
}
