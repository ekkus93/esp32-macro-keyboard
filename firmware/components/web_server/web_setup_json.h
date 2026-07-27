#ifndef WEB_SETUP_JSON_H
#define WEB_SETUP_JSON_H

#include <stddef.h>

#include "app_error.h"
#include "web_setup_core.h"

typedef struct {
    void *context;
    void (*secure_zero)(void *context, void *memory, size_t size);
} web_setup_json_ops_t;

app_error_code_t web_setup_json_parse(
    char *body,
    size_t body_capacity,
    const web_setup_json_ops_t *operations,
    web_setup_submission_t *out_submission);

#endif
