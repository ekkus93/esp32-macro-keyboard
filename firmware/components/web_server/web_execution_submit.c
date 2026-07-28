#include "web_execution_submit.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_executor.h"
#include "macro_model.h"
#include "macro_parser.h"
#include "storage_repository.h"

static bool operations_valid(const web_execution_ops_t *operations) {
    return operations != NULL && operations->macro_read != NULL &&
           operations->procedure_read != NULL && operations->compile != NULL &&
           operations->plan_free != NULL && operations->uuid_generate != NULL &&
           operations->submit != NULL;
}

static bool request_valid(const web_execution_submit_request_t *request) {
    if (request == NULL || !app_uuid_is_valid_string(request->set_id.value) ||
        !app_uuid_is_valid_string(request->macro_id.value) || request->macro_revision == 0U) {
        return false;
    }
    if (!request->has_procedure_context) {
        return true;
    }
    return app_uuid_is_valid_string(request->procedure_id.value) &&
           app_uuid_is_valid_string(request->step_id.value);
}

static bool procedure_context_matches(const procedure_t *procedure,
                                      const web_execution_submit_request_t *request) {
    if (procedure == NULL || request == NULL ||
        !app_uuid_equal(&procedure->set_id, &request->set_id)) {
        return false;
    }
    for (size_t index = 0U; index < procedure->step_count; ++index) {
        const procedure_step_t *step = &procedure->steps[index];
        if (app_uuid_equal(&step->id, &request->step_id)) {
            return step->type == PROCEDURE_STEP_MACRO && step->has_macro_id &&
                   app_uuid_equal(&step->macro_id, &request->macro_id);
        }
    }
    return false;
}

static app_error_code_t load_macro_snapshot(const web_execution_submit_request_t *request,
                                            const web_execution_ops_t *operations,
                                            macro_t *out_macro) {
    storage_macro_location_t location = {
        .scope = MACRO_SCOPE_SET,
        .has_set_id = true,
        .set_id = request->set_id,
    };
    app_error_code_t result =
        operations->macro_read(operations->context, &location, &request->macro_id, out_macro);
    if (result != APP_ERROR_NOT_FOUND) {
        return result;
    }
    location = (storage_macro_location_t){
        .scope = MACRO_SCOPE_GLOBAL,
        .has_set_id = false,
    };
    return operations->macro_read(operations->context, &location, &request->macro_id, out_macro);
}

static app_error_code_t validate_procedure_context(const web_execution_submit_request_t *request,
                                                   const web_execution_ops_t *operations) {
    if (!request->has_procedure_context) {
        return APP_ERROR_NONE;
    }
    procedure_t procedure = {0};
    const app_error_code_t result = operations->procedure_read(
        operations->context, &request->set_id, &request->procedure_id, &procedure);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    const bool matches = procedure_context_matches(&procedure, request);
    macro_model_free_procedure(&procedure);
    return matches ? APP_ERROR_NONE : APP_ERROR_INVALID_ARGUMENT;
}

app_error_code_t web_execution_submit_persisted(const web_execution_submit_request_t *request,
                                                const web_execution_ops_t *operations,
                                                web_execution_accepted_t *out_accepted,
                                                macro_parse_error_t *out_parse_error) {
    if (out_accepted != NULL) {
        memset(out_accepted, 0, sizeof(*out_accepted));
    }
    if (out_parse_error != NULL) {
        memset(out_parse_error, 0, sizeof(*out_parse_error));
    }
    if (!request_valid(request) || !operations_valid(operations) || out_accepted == NULL ||
        out_parse_error == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    app_error_code_t result = validate_procedure_context(request, operations);
    if (result != APP_ERROR_NONE) {
        return result;
    }

    macro_t macro = {0};
    result = load_macro_snapshot(request, operations, &macro);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (macro.revision != request->macro_revision ||
        (macro.scope == MACRO_SCOPE_SET &&
         (!macro.has_set_id || !app_uuid_equal(&macro.set_id, &request->set_id)))) {
        macro_model_free_macro(&macro);
        return APP_ERROR_CONFLICT;
    }

    const macro_compile_options_t compile_options = {
        .key_press_ms = macro.key_press_ms,
        .inter_key_ms = macro.inter_key_ms,
    };
    macro_plan_t plan = {0};
    bool plan_owned = false;
    result = operations->compile(operations->context, macro.source, macro.source_length,
                                 &compile_options, &plan, out_parse_error);
    if (result == APP_ERROR_NONE) {
        plan_owned = true;
    }
    if (result != APP_ERROR_NONE) {
        macro_model_free_macro(&macro);
        return result;
    }

    app_uuid_t execution_id = {0};
    result = operations->uuid_generate(operations->context, &execution_id);
    if (result == APP_ERROR_NONE) {
        macro_execution_request_t execution = {
            .execution_id = execution_id,
            .set_id = request->set_id,
            .macro_id = request->macro_id,
            .macro_revision = request->macro_revision,
            .key_press_ms = macro.key_press_ms,
            .inter_key_ms = macro.inter_key_ms,
            .plan = plan,
        };
        result = operations->submit(operations->context, &execution);
        if (result == APP_ERROR_NONE) {
            plan_owned = false;
            out_accepted->execution_id = execution_id;
            out_accepted->action_count = plan.action_count;
            out_accepted->estimated_duration_ms = plan.estimated_duration_ms;
        }
    }

    if (plan_owned) {
        operations->plan_free(operations->context, &plan);
    }
    macro_model_free_macro(&macro);
    return result;
}
