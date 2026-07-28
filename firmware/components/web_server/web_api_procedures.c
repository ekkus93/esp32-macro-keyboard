#include "web_api_handlers.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_limits.h"
#include "macro_model.h"
#include "storage_object_json.h"
#include "storage_repository.h"
#include "web_api_handler_common.h"
#include "web_api_json.h"
#include "web_api_response.h"

static app_error_code_t respond_error(web_api_response_t *response, app_error_code_t error,
                                      const char *message) {
    return web_api_handler_error(response, error, message, NULL);
}

static app_error_code_t send_procedure(web_api_response_t *response, unsigned int status,
                                       const procedure_t *procedure) {
    char *json = NULL;
    app_error_code_t result = web_api_handler_procedure_json(procedure, &json);
    if (result == APP_ERROR_NONE) {
        result = web_api_handler_success_json(response, status, json);
    }
    web_api_handler_json_free(json);
    return result;
}

static app_error_code_t send_progress(web_api_response_t *response,
                                      const storage_progress_snapshot_t *snapshot) {
    char *json = NULL;
    app_error_code_t result = web_api_handler_progress_json(snapshot, &json);
    if (result == APP_ERROR_NONE) {
        result = web_api_handler_success_json(response, 200U, json);
    }
    web_api_handler_json_free(json);
    return result;
}

static app_error_code_t handle_collection(const web_api_call_t *call,
                                          web_api_response_t *response) {
    if (call->method == WEB_API_METHOD_GET) {
        storage_procedure_list_t list = {0};
        app_error_code_t result = storage_procedure_list(&call->path.set_id, &list);
        char *json = NULL;
        if (result == APP_ERROR_NONE) {
            result = web_api_handler_procedure_list_json(&list, &json);
        }
        storage_procedure_list_free(&list);
        if (result == APP_ERROR_NONE) {
            result = web_api_handler_success_json(response, 200U, json);
        } else {
            const app_error_code_t encoded =
                respond_error(response, result, "could not list procedures");
            result = encoded == APP_ERROR_NONE ? APP_ERROR_NONE : encoded;
        }
        web_api_handler_json_free(json);
        return result;
    }

    procedure_t procedure = {0};
    app_error_code_t result = storage_repository_parse_procedure_json(
        call->body, call->body_length, &procedure);
    if (result == APP_ERROR_NONE &&
        (procedure.revision != 1U || !app_uuid_equal(&procedure.set_id, &call->path.set_id))) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    if (result == APP_ERROR_NONE) {
        result = storage_procedure_create(&call->path.set_id, &procedure);
    }
    procedure_t committed = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_procedure_read(&call->path.set_id, &procedure.id, &committed);
    }
    macro_model_free_procedure(&procedure);
    if (result != APP_ERROR_NONE) {
        return respond_error(response, result, "could not create procedure");
    }
    result = send_procedure(response, 201U, &committed);
    macro_model_free_procedure(&committed);
    return result;
}

static app_error_code_t handle_item(const web_api_call_t *call,
                                    web_api_response_t *response) {
    if (call->method == WEB_API_METHOD_GET) {
        procedure_t procedure = {0};
        app_error_code_t result = storage_procedure_read(
            &call->path.set_id, &call->path.procedure_id, &procedure);
        if (result == APP_ERROR_NONE) {
            result = send_procedure(response, 200U, &procedure);
        } else {
            result = respond_error(response, result, "procedure not available");
        }
        macro_model_free_procedure(&procedure);
        return result;
    }
    if (call->method == WEB_API_METHOD_PUT) {
        web_api_resource_mutation_t mutation = {0};
        app_error_code_t result = web_api_json_parse_resource_mutation(
            call->body, call->body_length, STORAGE_PROCEDURE_FILE_MAX_BYTES, &mutation);
        procedure_t replacement = {0};
        if (result == APP_ERROR_NONE) {
            result = storage_repository_parse_procedure_json(
                mutation.resource_json, mutation.resource_length, &replacement);
        }
        if (result == APP_ERROR_NONE &&
            (!app_uuid_equal(&replacement.id, &call->path.procedure_id) ||
             !app_uuid_equal(&replacement.set_id, &call->path.set_id))) {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
        procedure_t committed = {0};
        if (result == APP_ERROR_NONE) {
            result = storage_procedure_update(&call->path.set_id, &replacement,
                                              mutation.expected_revision, &committed);
        }
        macro_model_free_procedure(&replacement);
        web_api_json_free_resource_mutation(&mutation);
        if (result != APP_ERROR_NONE) {
            return respond_error(response, result, "could not update procedure");
        }
        result = send_procedure(response, 200U, &committed);
        macro_model_free_procedure(&committed);
        return result;
    }

    uint32_t expected_revision = 0U;
    app_error_code_t result = web_api_json_parse_expected_revision(
        call->body, call->body_length, &expected_revision);
    if (result == APP_ERROR_NONE) {
        result = storage_procedure_delete(&call->path.set_id, &call->path.procedure_id,
                                          expected_revision);
    }
    if (result != APP_ERROR_NONE) {
        return respond_error(response, result, "could not delete procedure");
    }
    char data[80U];
    const int length = snprintf(data, sizeof(data), "{\"deleted\":true,\"id\":\"%s\"}",
                                call->path.procedure_id.value);
    return length < 0 || (size_t)length >= sizeof(data)
               ? APP_ERROR_INTERNAL
               : web_api_handler_success_json(response, 200U, data);
}

static app_error_code_t handle_reorder(const web_api_call_t *call,
                                       web_api_response_t *response) {
    storage_uuid_order_t order = {0};
    app_error_code_t result = web_api_json_parse_uuid_order(
        call->body, call->body_length, APP_PROCEDURES_PER_SET_MAX, &order);
    if (result == APP_ERROR_NONE) {
        result = storage_procedure_reorder(&call->path.set_id, order.ids, order.count);
    }
    return result == APP_ERROR_NONE
               ? web_api_handler_success_json(response, 200U, "{\"reordered\":true}")
               : respond_error(response, result, "could not reorder procedures");
}

static storage_procedure_identity_t progress_identity(const web_api_call_t *call) {
    return (storage_procedure_identity_t){
        .set_id = call->path.set_id,
        .procedure_id = call->path.procedure_id,
    };
}

static app_error_code_t handle_progress_resource(const web_api_call_t *call,
                                                 web_api_response_t *response) {
    const storage_procedure_identity_t identity = progress_identity(call);
    if (call->method == WEB_API_METHOD_GET) {
        storage_progress_snapshot_t snapshot = {0};
        const app_error_code_t result = storage_progress_read(&identity, &snapshot);
        return result == APP_ERROR_NONE ? send_progress(response, &snapshot)
                                        : respond_error(response, result,
                                                        "progress not available");
    }
    if (call->method == WEB_API_METHOD_PUT) {
        procedure_progress_t replacement = {0};
        app_error_code_t result = storage_repository_parse_progress_json(
            call->body, call->body_length, &replacement);
        if (result == APP_ERROR_NONE &&
            (!app_uuid_equal(&replacement.set_id, &identity.set_id) ||
             !app_uuid_equal(&replacement.procedure_id, &identity.procedure_id))) {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
        storage_progress_snapshot_t snapshot = {0};
        if (result == APP_ERROR_NONE) {
            result = storage_progress_update(&identity, &replacement, &snapshot);
        }
        return result == APP_ERROR_NONE
                   ? send_progress(response, &snapshot)
                   : respond_error(response, result, "could not update progress");
    }

    uint32_t expected_revision = 0U;
    app_error_code_t result = web_api_json_parse_expected_revision(
        call->body, call->body_length, &expected_revision);
    storage_progress_snapshot_t snapshot = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_progress_reset(&identity, expected_revision, &snapshot);
    }
    return result == APP_ERROR_NONE ? send_progress(response, &snapshot)
                                    : respond_error(response, result,
                                                    "could not reset progress");
}

static bool order_contains(const app_uuid_t *items, size_t count, const app_uuid_t *id) {
    for (size_t index = 0U; index < count; ++index) {
        if (app_uuid_equal(&items[index], id)) {
            return true;
        }
    }
    return false;
}

static void order_remove(app_uuid_t *items, size_t *count, const app_uuid_t *id) {
    for (size_t index = 0U; index < *count; ++index) {
        if (app_uuid_equal(&items[index], id)) {
            if (index + 1U < *count) {
                memmove(&items[index], &items[index + 1U],
                        (*count - index - 1U) * sizeof(items[0]));
            }
            --(*count);
            memset(&items[*count], 0, sizeof(items[0]));
            return;
        }
    }
}

static bool procedure_has_step(const procedure_t *procedure, const app_uuid_t *step_id,
                               size_t *out_index) {
    for (size_t index = 0U; index < procedure->step_count; ++index) {
        if (app_uuid_equal(&procedure->steps[index].id, step_id)) {
            *out_index = index;
            return true;
        }
    }
    return false;
}

static app_error_code_t handle_progress_action(const web_api_call_t *call,
                                               web_api_response_t *response, bool skipped) {
    web_api_progress_action_t action = {0};
    app_error_code_t result = web_api_json_parse_progress_action(
        call->body, call->body_length, skipped, &action);
    const storage_procedure_identity_t identity = progress_identity(call);
    storage_progress_snapshot_t current = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_progress_read(&identity, &current);
    }
    if (result == APP_ERROR_NONE &&
        (current.status != STORAGE_PROGRESS_STATUS_CURRENT ||
         current.current_procedure_revision != action.expected_procedure_revision)) {
        result = APP_ERROR_CONFLICT;
    }
    procedure_t procedure = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_procedure_read(&identity.set_id, &identity.procedure_id, &procedure);
    }
    size_t step_index = 0U;
    if (result == APP_ERROR_NONE && !procedure_has_step(&procedure, &action.step_id, &step_index)) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    procedure_progress_t replacement = current.progress;
    if (result == APP_ERROR_NONE) {
        app_uuid_t *target = skipped ? replacement.skipped_step_ids
                                     : replacement.completed_step_ids;
        size_t *target_count = skipped ? &replacement.skipped_step_count
                                       : &replacement.completed_step_count;
        app_uuid_t *opposite = skipped ? replacement.completed_step_ids
                                       : replacement.skipped_step_ids;
        size_t *opposite_count = skipped ? &replacement.completed_step_count
                                         : &replacement.skipped_step_count;
        order_remove(opposite, opposite_count, &action.step_id);
        if (!order_contains(target, *target_count, &action.step_id)) {
            if (*target_count >= APP_STEPS_PER_PROCEDURE_MAX) {
                result = APP_ERROR_MACRO_LIMIT;
            } else {
                target[*target_count] = action.step_id;
                ++(*target_count);
            }
        }
        if (result == APP_ERROR_NONE && step_index + 1U < procedure.step_count) {
            replacement.current_step_id = procedure.steps[step_index + 1U].id;
        }
    }
    storage_progress_snapshot_t committed = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_progress_update(&identity, &replacement, &committed);
    }
    macro_model_free_procedure(&procedure);
    return result == APP_ERROR_NONE
               ? send_progress(response, &committed)
               : respond_error(response, result,
                               skipped ? "could not skip procedure step"
                                       : "could not complete procedure step");
}

app_error_code_t web_api_handle_procedures(const web_api_call_t *call,
                                           web_api_response_t *response) {
    if (call == NULL || response == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    switch (call->path.route) {
    case WEB_API_ROUTE_SET_PROCEDURES:
        return handle_collection(call, response);
    case WEB_API_ROUTE_SET_PROCEDURE:
        return handle_item(call, response);
    case WEB_API_ROUTE_SET_PROCEDURES_REORDER:
        return handle_reorder(call, response);
    case WEB_API_ROUTE_PROCEDURE_PROGRESS:
        return handle_progress_resource(call, response);
    case WEB_API_ROUTE_PROGRESS_COMPLETE:
        return handle_progress_action(call, response, false);
    case WEB_API_ROUTE_PROGRESS_SKIP:
        return handle_progress_action(call, response, true);
    default:
        return APP_ERROR_NOT_FOUND;
    }
}
