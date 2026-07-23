#ifndef MACRO_PARSER_H
#define MACRO_PARSER_H

#include <stddef.h>
#include <stdint.h>

#include "app_error.h"

typedef enum {
    MACRO_ACTION_KEY = 0,
    MACRO_ACTION_CHORD,
    MACRO_ACTION_DELAY
} macro_action_type_t;

typedef struct {
    macro_action_type_t type;
    uint8_t modifiers;
    uint8_t usage;
    uint32_t delay_ms;
} macro_action_t;

typedef struct {
    macro_action_t *actions;
    size_t action_count;
    uint32_t estimated_duration_ms;
} macro_plan_t;

typedef struct {
    uint32_t key_press_ms;
    uint32_t inter_key_ms;
} macro_compile_options_t;

typedef struct {
    app_error_code_t code;
    size_t byte_offset;
    size_t line;
    size_t column;
    char message[96U];
} macro_parse_error_t;

app_error_code_t macro_compile(const char *source,
                               size_t source_length,
                               const macro_compile_options_t *options,
                               macro_plan_t *out_plan,
                               macro_parse_error_t *out_error);
void macro_plan_free(macro_plan_t *plan);

#endif
