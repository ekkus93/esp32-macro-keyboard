#ifndef MACRO_MODEL_H
#define MACRO_MODEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_limits.h"

/* Keyboard-layout code buffer, e.g. "en-US" plus a terminator. */
#define MACRO_KEYBOARD_LAYOUT_BYTES 6U

typedef enum {
    PROCEDURE_STEP_MACRO = 0,
    PROCEDURE_STEP_INSTRUCTION,
    PROCEDURE_STEP_CHECKPOINT
} procedure_step_type_t;

typedef struct {
    uint32_t schema_version;
    app_uuid_t id;
    uint32_t revision;
    /* A set is a name and an ordered list of macros (SPEC 12.1). Set order
     * lives in index.json's ordered set_ids and nowhere else. */
    char name[APP_NAME_MAX_BYTES + 1U];
} macro_set_t;

typedef struct {
    uint32_t schema_version;
    app_uuid_t id;
    uint32_t revision;
    /* Every macro belongs to exactly one set (SPEC §7.2). There is no global or
     * shared macro library, so this is always populated. */
    app_uuid_t set_id;
    char name[APP_MACRO_NAME_MAX_BYTES + 1U];
    char *source;
    size_t source_length;
    bool favorite;
    uint32_t key_press_ms;
    uint32_t inter_key_ms;
} macro_t;

typedef struct {
    app_uuid_t id;
    procedure_step_type_t type;
    char title[APP_STEP_TITLE_MAX_BYTES + 1U];
    bool required;
    bool auto_complete_on_success;
    bool has_macro_id;
    app_uuid_t macro_id;
    char *body;
    size_t body_length;
} procedure_step_t;

typedef struct {
    uint32_t schema_version;
    app_uuid_t id;
    uint32_t revision;
    app_uuid_t set_id;
    char name[APP_PROCEDURE_NAME_MAX_BYTES + 1U];
    char description[APP_DESCRIPTION_MAX_BYTES + 1U];
    procedure_step_t *steps;
    size_t step_count;
    int32_t sort_order;
} procedure_t;

typedef struct {
    uint32_t schema_version;
    app_uuid_t set_id;
    app_uuid_t procedure_id;
    uint32_t procedure_revision;
    app_uuid_t current_step_id;
    app_uuid_t completed_step_ids[APP_STEPS_PER_PROCEDURE_MAX];
    size_t completed_step_count;
    app_uuid_t skipped_step_ids[APP_STEPS_PER_PROCEDURE_MAX];
    size_t skipped_step_count;
} procedure_progress_t;

app_error_code_t macro_model_validate_revision(uint32_t revision);
app_error_code_t macro_model_validate_text(const char *text, size_t length, size_t maximum);
void macro_model_free_macro(macro_t *macro);
void macro_model_free_procedure(procedure_t *procedure);

#endif
