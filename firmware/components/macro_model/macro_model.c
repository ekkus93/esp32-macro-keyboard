#include "macro_model.h"

#include <stdint.h>
#include <string.h>

#include "app_error.h"

app_error_code_t macro_model_validate_revision(uint32_t revision) {
    return revision == 0U ? APP_ERROR_INVALID_ARGUMENT : APP_ERROR_NONE;
}

app_error_code_t macro_model_validate_text(const char *text, size_t length, size_t maximum) {
    if ((text == NULL && length != 0U) || length > maximum) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (text != NULL && memchr(text, '\0', length) != NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return APP_ERROR_NONE;
}
