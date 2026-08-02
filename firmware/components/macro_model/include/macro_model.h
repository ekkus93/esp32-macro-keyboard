#ifndef MACRO_MODEL_H
#define MACRO_MODEL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_limits.h"

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
    uint32_t key_press_ms;
    uint32_t inter_key_ms;
} macro_t;

app_error_code_t macro_model_validate_revision(uint32_t revision);
app_error_code_t macro_model_validate_text(const char *text, size_t length, size_t maximum);
void macro_model_free_macro(macro_t *macro);

#endif
