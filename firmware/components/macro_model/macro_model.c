#include "macro_model.h"

#include <stdlib.h>
#include <string.h>

app_error_code_t macro_model_validate_revision(uint32_t revision)
{
    return revision == 0U ? APP_ERROR_INVALID_ARGUMENT : APP_ERROR_NONE;
}

app_error_code_t macro_model_validate_text(const char *text,
                                           size_t length,
                                           size_t maximum)
{
    if ((text == NULL && length != 0U) || length > maximum) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (text != NULL && memchr(text, '\0', length) != NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return APP_ERROR_NONE;
}

void macro_model_free_macro(macro_t *macro)
{
    if (macro == NULL) {
        return;
    }
    free(macro->source);
    macro->source = NULL;
    macro->source_length = 0U;
}

void macro_model_free_procedure(procedure_t *procedure)
{
    if (procedure == NULL) {
        return;
    }
    if (procedure->steps != NULL) {
        for (size_t index = 0U; index < procedure->step_count; ++index) {
            free(procedure->steps[index].body);
            procedure->steps[index].body = NULL;
            procedure->steps[index].body_length = 0U;
        }
    }
    free(procedure->steps);
    procedure->steps = NULL;
    procedure->step_count = 0U;
}
