#ifndef MACRO_PARSER_H
#define MACRO_PARSER_H

#include <stddef.h>
#include <stdint.h>

#include "app_error.h"

#define MACRO_PARSE_MESSAGE_BYTES 96U

typedef enum { MACRO_ACTION_KEY = 0, MACRO_ACTION_CHORD, MACRO_ACTION_DELAY } macro_action_type_t;

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
    char message[MACRO_PARSE_MESSAGE_BYTES];
} macro_parse_error_t;

/* The v2 contract compiler, and the only one: both timing values accept the
 * complete specified range 0 through 10,000 ms, and an over-long source reports
 * the specific limit rather than a generic invalid-argument. It is exercised by
 * the shared C/TypeScript conformance corpus. The v1 `macro_compile` entry point
 * it replaced during the Phase 6 executor migration was deleted 2026-08-16 once
 * that migration was confirmed complete -- production compiles only through
 * this function (web_send.c). */
app_error_code_t macro_compile_v2(const char *source, size_t source_length,
                                  const macro_compile_options_t *options, macro_plan_t *out_plan,
                                  macro_parse_error_t *out_error);

void macro_plan_v2_free(macro_plan_t *plan);

#endif
