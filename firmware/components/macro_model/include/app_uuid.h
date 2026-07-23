#ifndef APP_UUID_H
#define APP_UUID_H

#include <stdbool.h>
#include "app_error.h"
#include "macro_limits.h"

typedef struct {
    char value[APP_UUID_BUFFER_LENGTH];
} app_uuid_t;

bool app_uuid_is_valid_string(const char *text);
app_error_code_t app_uuid_parse(const char *text, app_uuid_t *out_uuid);
app_error_code_t app_uuid_generate(app_uuid_t *out_uuid);
bool app_uuid_equal(const app_uuid_t *left, const app_uuid_t *right);

#endif
