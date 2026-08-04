#ifndef MACRO_MODEL_H
#define MACRO_MODEL_H

#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "macro_limits.h"

/* Execution-source validation shared by the parser and its host tests. Phase 2
 * removed the firmware-owned package and macro repository objects; this module
 * intentionally contains no persisted resource model. */
app_error_code_t macro_model_validate_revision(uint32_t revision);
app_error_code_t macro_model_validate_text(const char *text, size_t length, size_t maximum);

#endif
