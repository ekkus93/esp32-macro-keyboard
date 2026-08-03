#include "web_api_handlers.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_executor.h"
#include "macro_model.h"
#include "macro_parser.h"
#include "storage_repository.h"
#include "web_api_core.h"
#include "web_api_handler_common.h"
#include "web_api_json.h"
#include "web_api_response.h"
#include "web_execution_route_policy.h"
#include "web_execution_submit.h"
#include "web_http_status.h"

#define WEB_EXECUTION_STATUS_RESPONSE_BYTES 768U
#define WEB_EXECUTION_DETAILS_RESPONSE_BYTES 192U
#include "web_server_internal.h"

static app_error_code_t read_macro(void *context, const app_uuid_t *set_id,
                                   const app_uuid_t *macro_id, macro_t *out_macro) {
    (void)context;
    return storage_macro_read(set_id, macro_id, out_macro);
}

static app_error_code_t compile_macro(void *context, const char *source, size_t source_length,
                                      const macro_compile_options_t *options,
                                      macro_plan_t *out_plan, macro_parse_error_t *out_error) {
    (void)context;
    return macro_compile(source, source_length, options, out_plan, out_error);
}

static void free_plan(void *context, macro_plan_t *plan) {
    (void)context;
    macro_plan_free(plan);
}

static app_error_code_t generate_uuid(void *context, app_uuid_t *out_uuid) {
    (void)context;
    return app_uuid_generate(out_uuid);
}

static app_error_code_t submit_execution(void *context, macro_execution_request_t *request) {
    (void)context;
    return macro_executor_submit(request);
}

static web_execution_ops_t execution_operations(void) {
    return (web_execution_ops_t){
        .context = NULL,
        .macro_read = read_macro,
        .compile = compile_macro,
        .plan_free = free_plan,
        .uuid_generate = generate_uuid,
        .submit = submit_execution,
    };
}

static app_error_code_t execution_status_json(const macro_execution_status_t *status, char *output,
                                              size_t output_size) {
    if (status == NULL || output == NULL || output_size == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const int length = snprintf(
        output, output_size,
        "{\"executionId\":\"%s\",\"packageId\":\"%s\",\"macroId\":\"%s\","
        "\"macroRevision\":%lu,\"state\":\"%s\",\"error\":\"%s\","
        "\"releaseError\":\"%s\",\"actionIndex\":%lu,\"actionCount\":%lu,"
        "\"available\":%s,\"cancellationRequested\":%s,\"acceptedMs\":%lu,"
        "\"startedMs\":%lu,\"completedMs\":%lu,\"currentAction\":\"%s\"}",
        status->execution_id.value, status->set_id.value, status->macro_id.value,
        (unsigned long)status->macro_revision, execution_state_string(status->state),
        app_error_code_string(status->error), app_error_code_string(status->release_error),
        (unsigned long)status->action_index, (unsigned long)status->action_count,
        status->available ? "true" : "false", status->cancellation_requested ? "true" : "false",
        (unsigned long)status->accepted_ms, (unsigned long)status->started_ms,
        (unsigned long)status->completed_ms, status->current_action);
    if (length < 0 || (size_t)length >= output_size) {
        output[0] = '\0';
        return APP_ERROR_INTERNAL;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t send_current(web_api_response_t *response) {
    const macro_execution_status_t status = macro_executor_get_status();
    char data[WEB_EXECUTION_STATUS_RESPONSE_BYTES];
    const app_error_code_t result = execution_status_json(&status, data, sizeof(data));
    return result == APP_ERROR_NONE
               ? web_api_handler_success_json(response, WEB_HTTP_STATUS_OK, data)
               : result;
}

static app_error_code_t send_submission_error(web_api_response_t *response, app_error_code_t error,
                                              const macro_parse_error_t *parse_error) {
    if ((error == APP_ERROR_MACRO_SYNTAX || error == APP_ERROR_MACRO_LIMIT) &&
        parse_error != NULL) {
        char details[WEB_EXECUTION_DETAILS_RESPONSE_BYTES];
        const int length =
            snprintf(details, sizeof(details), "{\"line\":%lu,\"column\":%lu,\"byteOffset\":%lu}",
                     (unsigned long)parse_error->line, (unsigned long)parse_error->column,
                     (unsigned long)parse_error->byte_offset);
        if (length < 0 || (size_t)length >= sizeof(details)) {
            return APP_ERROR_INTERNAL;
        }
        return web_api_handler_error(response, error, "macro compilation failed", details);
    }
    return web_api_handler_error(response, error, "execution submission failed", NULL);
}

static app_error_code_t handle_submit(const web_api_call_t *call, web_api_response_t *response) {
    web_execution_submit_request_t request = {0};
    app_error_code_t result =
        web_api_json_parse_execution_submit(call->body, call->body_length, &request);
    web_execution_accepted_t accepted = {0};
    macro_parse_error_t parse_error = {0};
    if (result == APP_ERROR_NONE) {
        const web_execution_ops_t operations = execution_operations();
        result = web_execution_submit_persisted(&request, &operations, &accepted, &parse_error);
    }
    if (result != APP_ERROR_NONE) {
        return send_submission_error(response, result, &parse_error);
    }
    char data[WEB_EXECUTION_DETAILS_RESPONSE_BYTES];
    const int length = snprintf(data, sizeof(data),
                                "{\"executionId\":\"%s\",\"actionCount\":%lu,"
                                "\"estimatedDurationMs\":%lu}",
                                accepted.execution_id.value, (unsigned long)accepted.action_count,
                                (unsigned long)accepted.estimated_duration_ms);
    return length < 0 || (size_t)length >= sizeof(data)
               ? APP_ERROR_INTERNAL
               : web_api_handler_success_json(response, WEB_HTTP_STATUS_ACCEPTED, data);
}

static app_error_code_t explicit_error(web_api_response_t *response, unsigned int status,
                                       app_error_code_t error, const char *message) {
    return web_api_response_error(response, &(web_api_error_spec_t){
                                                .status = status,
                                                .code = error,
                                                .message = message,
                                            });
}

static app_error_code_t handle_cancel(const web_api_call_t *call, web_api_response_t *response) {
    const macro_execution_status_t status = macro_executor_get_status();
    web_execution_cancel_policy_t policy = {0};
    app_error_code_t result = web_execution_cancel_policy_evaluate(&status, &call->path, &policy);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (!policy.permitted) {
        return explicit_error(response, policy.status, policy.error, policy.message);
    }
    result = macro_executor_cancel();
    const unsigned int http_status = web_api_cancel_http_status(&status, result);
    if (result != APP_ERROR_NONE) {
        return explicit_error(response, http_status, result, "cancellation request failed");
    }
    return web_api_handler_success_json(response, WEB_HTTP_STATUS_ACCEPTED,
                                        "{\"cancelRequested\":true}");
}

app_error_code_t web_api_handle_execution(const web_api_call_t *call,
                                          web_api_response_t *response) {
    if (call == NULL || response == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    switch (call->path.route) {
    case WEB_API_ROUTE_EXECUTIONS:
        return handle_submit(call, response);
    case WEB_API_ROUTE_EXECUTION_CURRENT:
        return send_current(response);
    case WEB_API_ROUTE_EXECUTION_CANCEL:
        return handle_cancel(call, response);
    default:
        return APP_ERROR_NOT_FOUND;
    }
}
