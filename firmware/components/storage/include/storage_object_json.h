#ifndef STORAGE_OBJECT_JSON_H
#define STORAGE_OBJECT_JSON_H

#include <stddef.h>

#include "app_error.h"
#include "macro_model.h"

#define STORAGE_SET_FILE_MAX_BYTES 4096U
#define STORAGE_MACRO_FILE_MAX_BYTES 8192U
#define STORAGE_PROCEDURE_FILE_MAX_BYTES (256U * 1024U)
#define STORAGE_PROGRESS_FILE_MAX_BYTES (32U * 1024U)
#define STORAGE_ORDER_FILE_MAX_BYTES 8192U
/* An order list holds macro, procedure, or macro-set identifiers - never
 * procedure steps - so it only has to fit the largest of those three limits.
 * This was APP_STEPS_PER_PROCEDURE_MAX (200), which no caller can fill: every
 * call site bounds the count by one of the three limits asserted below. The
 * over-provisioning mattered because storage_uuid_order_t is used as a stack
 * local throughout the storage layer, where it was ~8 KB against FreeRTOS task
 * stacks of a few KiB (see scripts/check-stack-usage.sh). */
#define STORAGE_ORDER_MAX_IDS APP_MACROS_PER_SET_MAX
/* Split into two assertions on purpose: the two limits below currently share a
 * value, so combining them into one expression is literally redundant and
 * clang-tidy rejects it. Separate assertions stay meaningful if either limit
 * changes independently later. */
_Static_assert(STORAGE_ORDER_MAX_IDS >= APP_PROCEDURES_PER_SET_MAX,
               "STORAGE_ORDER_MAX_IDS must fit a procedure order list");
_Static_assert(STORAGE_ORDER_MAX_IDS >= APP_MACRO_SETS_MAX,
               "STORAGE_ORDER_MAX_IDS must fit a macro-set order list");

typedef struct {
    app_uuid_t ids[STORAGE_ORDER_MAX_IDS];
    size_t count;
} storage_uuid_order_t;

app_error_code_t storage_repository_parse_set_json(const char *data, size_t length,
                                                   macro_set_t *out_set);
app_error_code_t storage_repository_serialize_set_json(const macro_set_t *set, char **out_json,
                                                       size_t *out_length);
app_error_code_t storage_repository_parse_macro_json(const char *data, size_t length,
                                                     macro_t *out_macro);
app_error_code_t storage_repository_serialize_macro_json(const macro_t *macro, char **out_json,
                                                         size_t *out_length);
app_error_code_t storage_repository_parse_procedure_json(const char *data, size_t length,
                                                         procedure_t *out_procedure);
app_error_code_t storage_repository_serialize_procedure_json(const procedure_t *procedure,
                                                             char **out_json, size_t *out_length);
app_error_code_t storage_repository_parse_progress_json(const char *data, size_t length,
                                                        procedure_progress_t *out_progress);
app_error_code_t storage_repository_serialize_progress_json(const procedure_progress_t *progress,
                                                            char **out_json, size_t *out_length);
app_error_code_t storage_repository_parse_order_json(const char *data, size_t length,
                                                     storage_uuid_order_t *out_order,
                                                     size_t maximum_count);
app_error_code_t storage_repository_serialize_order_json(const storage_uuid_order_t *order,
                                                         size_t maximum_count, char **out_json,
                                                         size_t *out_length);

#endif
