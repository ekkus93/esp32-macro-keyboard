#ifndef WEB_EXECUTION_SUBMIT_H
#define WEB_EXECUTION_SUBMIT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_executor.h"
#include "macro_model.h"
#include "macro_parser.h"
#include "storage_repository.h"

typedef struct {
    app_uuid_t set_id;
    app_uuid_t macro_id;
    uint32_t macro_revision;
    bool has_procedure_context;
    app_uuid_t procedure_id;
    app_uuid_t step_id;
} web_execution_submit_request_t;

typedef struct {
    app_uuid_t execution_id;
    size_t action_count;
    uint32_t estimated_duration_ms;
} web_execution_accepted_t;

typedef app_error_code_t (*web_execution_macro_read_fn)(void *context,
                                                        const storage_macro_location_t *location,
                                                        const app_uuid_t *macro_id,
                                                        macro_t *out_macro);
typedef app_error_code_t (*web_execution_procedure_read_fn)(void *context, const app_uuid_t *set_id,
                                                            const app_uuid_t *procedure_id,
                                                            procedure_t *out_procedure);
typedef app_error_code_t (*web_execution_compile_fn)(void *context, const char *source,
                                                     size_t source_length,
                                                     const macro_compile_options_t *options,
                                                     macro_plan_t *out_plan,
                                                     macro_parse_error_t *out_error);
typedef void (*web_execution_plan_free_fn)(void *context, macro_plan_t *plan);
typedef app_error_code_t (*web_execution_uuid_generate_fn)(void *context, app_uuid_t *out_uuid);
typedef app_error_code_t (*web_execution_submit_fn)(void *context,
                                                    macro_execution_request_t *request);

typedef struct {
    void *context;
    web_execution_macro_read_fn macro_read;
    web_execution_procedure_read_fn procedure_read;
    web_execution_compile_fn compile;
    web_execution_plan_free_fn plan_free;
    web_execution_uuid_generate_fn uuid_generate;
    web_execution_submit_fn submit;
} web_execution_ops_t;

app_error_code_t web_execution_submit_persisted(const web_execution_submit_request_t *request,
                                                const web_execution_ops_t *operations,
                                                web_execution_accepted_t *out_accepted,
                                                macro_parse_error_t *out_parse_error);

#endif
