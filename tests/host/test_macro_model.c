#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "macro_model.h"
#include "test_assert.h"

static void test_revision_boundaries(void) {
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_model_validate_revision(0U));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_model_validate_revision(1U));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_model_validate_revision(UINT32_MAX));
}

static void test_text_policy(void) {
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_model_validate_text(NULL, 0U, 0U));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_model_validate_text(NULL, 1U, 1U));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_model_validate_text("", 0U, 0U));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, macro_model_validate_text("a", 1U, 0U));

    char *text = malloc(APP_MACRO_SOURCE_MAX_BYTES + 1U);
    TEST_CHECK(text != NULL);
    memset(text, 'x', APP_MACRO_SOURCE_MAX_BYTES + 1U);
    const app_error_code_t maximum_result =
        macro_model_validate_text(text, APP_MACRO_SOURCE_MAX_BYTES, APP_MACRO_SOURCE_MAX_BYTES);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, maximum_result);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         macro_model_validate_text(text, APP_MACRO_SOURCE_MAX_BYTES + 1U,
                                                   APP_MACRO_SOURCE_MAX_BYTES));
    free(text);

    static const char embedded_nul[] = {'a', 'b', '\0', 'c'};
    const app_error_code_t embedded_nul_result =
        macro_model_validate_text(embedded_nul, sizeof(embedded_nul), sizeof(embedded_nul));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, embedded_nul_result);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, macro_model_validate_text(embedded_nul, 2U, 2U));
}

int main(void) {
    test_revision_boundaries();
    test_text_policy();
    puts("macro model tests passed");
    return EXIT_SUCCESS;
}
