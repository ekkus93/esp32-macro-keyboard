#include "web_api_handlers.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "macro_limits.h"
#include "macro_model.h"
#include "macro_parser.h"
#include "storage_object_json.h"
#include "storage_repository.h"
#include "web_api_core.h"
#include "web_api_handler_common.h"
#include "web_api_json.h"
#include "web_api_response.h"
#include "web_http_status.h"

#define WEB_MACRO_DELETE_RESPONSE_BYTES 80U
#define WEB_MACRO_VALIDATION_DETAILS_BYTES 192U
#define WEB_MACRO_VALIDATION_RESPONSE_BYTES 160U

static storage_macro_location_t location_for_call(const web_api_call_t *call) {
    if (call->path.route == WEB_API_ROUTE_GLOBAL_MACROS ||
        call->path.route == WEB_API_ROUTE_GLOBAL_MACRO ||
        call->path.route == WEB_API_ROUTE_GLOBAL_MACRO_VALIDATE ||
        call->path.route == WEB_API_ROUTE_GLOBAL_MACRO_DUPLICATE ||
        call->path.route == WEB_API_ROUTE_GLOBAL_MACROS_REORDER) {
        return (storage_macro_location_t){.scope = MACRO_SCOPE_GLOBAL};
    }
    return (storage_macro_location_t){
        .scope = MACRO_SCOPE_SET,
        .has_set_id = true,
        .set_id = call->path.set_id,
    };
}

static app_error_code_t respond_error(web_api_response_t *response, app_error_code_t result,
                                      const char *message, const char *details) {
    return web_api_handler_error(response, result, message, details);
}

static app_error_code_t send_macro(web_api_response_t *response, unsigned int status,
                                   const macro_t *macro) {
    char *json = NULL;
    app_error_code_t result = web_api_handler_macro_json(macro, &json);
    if (result == APP_ERROR_NONE) {
        result = web_api_handler_success_json(response, status, json);
    }
    web_api_handler_json_free(json);
    return result;
}

static bool macro_matches_path(const macro_t *macro, const storage_macro_location_t *location,
                               const app_uuid_t *path_id) {
    if (macro == NULL || location == NULL || path_id == NULL ||
        !app_uuid_equal(&macro->id, path_id) || macro->scope != location->scope) {
        return false;
    }
    return location->scope == MACRO_SCOPE_GLOBAL
               ? !macro->has_set_id
               : macro->has_set_id && app_uuid_equal(&macro->set_id, &location->set_id);
}

static app_error_code_t handle_collection(const web_api_call_t *call,
                                          web_api_response_t *response) {
    const storage_macro_location_t location = location_for_call(call);
    if (call->method == WEB_API_METHOD_GET) {
        storage_macro_list_t list = {0};
        app_error_code_t result = storage_macro_list(&location, &list);
        char *json = NULL;
        if (result == APP_ERROR_NONE) {
            result = web_api_handler_macro_list_json(&list, &json);
        }
        storage_macro_list_free(&list);
        if (result == APP_ERROR_NONE) {
            result = web_api_handler_success_json(response, WEB_HTTP_STATUS_OK, json);
        } else {
            const app_error_code_t encoded =
                respond_error(response, result, "could not list macros", NULL);
            result = encoded == APP_ERROR_NONE ? APP_ERROR_NONE : encoded;
        }
        web_api_handler_json_free(json);
        return result;
    }

    macro_t macro = {0};
    app_error_code_t result =
        web_api_json_parse_macro_resource(call->body, call->body_length, &macro);
    if (result == APP_ERROR_NONE &&
        (macro.revision != 1U || macro.scope != location.scope ||
         (location.scope == MACRO_SCOPE_SET &&
          (!macro.has_set_id || !app_uuid_equal(&macro.set_id, &location.set_id))) ||
         (location.scope == MACRO_SCOPE_GLOBAL && macro.has_set_id))) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    if (result == APP_ERROR_NONE) {
        result = storage_macro_create(&location, &macro);
    }
    macro_t committed = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_macro_read(&location, &macro.id, &committed);
    }
    macro_model_free_macro(&macro);
    if (result != APP_ERROR_NONE) {
        return respond_error(response, result, "could not create macro", NULL);
    }
    result = send_macro(response, WEB_HTTP_STATUS_CREATED, &committed);
    macro_model_free_macro(&committed);
    return result;
}

static app_error_code_t reference_details_json(const storage_reference_list_t *references,
                                               char **out_json) {
    *out_json = NULL;
    cJSON *root = cJSON_CreateObject();
    cJSON *ids = cJSON_CreateArray();
    if (root == NULL || ids == NULL ||
        !cJSON_AddBoolToObject(root, "truncated", references->truncated) ||
        !cJSON_AddItemToObject(root, "procedureIds", ids)) {
        cJSON_Delete(ids);
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    for (size_t index = 0U; index < references->count; ++index) {
        cJSON *identifier = cJSON_CreateString(references->ids[index].value);
        if (identifier == NULL || !cJSON_AddItemToArray(ids, identifier)) {
            cJSON_Delete(identifier);
            cJSON_Delete(root);
            return APP_ERROR_INTERNAL;
        }
    }
    *out_json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return *out_json == NULL ? APP_ERROR_INTERNAL : APP_ERROR_NONE;
}

static app_error_code_t handle_item(const web_api_call_t *call, web_api_response_t *response) {
    const storage_macro_location_t location = location_for_call(call);
    if (call->method == WEB_API_METHOD_GET) {
        macro_t macro = {0};
        app_error_code_t result = storage_macro_read(&location, &call->path.macro_id, &macro);
        if (result == APP_ERROR_NONE) {
            result = send_macro(response, WEB_HTTP_STATUS_OK, &macro);
        } else {
            result = respond_error(response, result, "macro not available", NULL);
        }
        macro_model_free_macro(&macro);
        return result;
    }
    if (call->method == WEB_API_METHOD_PUT) {
        web_api_resource_mutation_t mutation = {0};
        app_error_code_t result = web_api_json_parse_resource_mutation(
            call->body,
            &(web_api_resource_parse_limits_t){
                .body_length = call->body_length,
                .maximum_resource_length = STORAGE_MACRO_FILE_MAX_BYTES,
            },
            &mutation);
        macro_t replacement = {0};
        if (result == APP_ERROR_NONE) {
            result = web_api_json_parse_macro_resource(mutation.resource_json,
                                                       mutation.resource_length, &replacement);
        }
        if (result == APP_ERROR_NONE &&
            !macro_matches_path(&replacement, &location, &call->path.macro_id)) {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
        macro_t committed = {0};
        if (result == APP_ERROR_NONE) {
            result = storage_macro_update(&location, &replacement, mutation.expected_revision,
                                          &committed);
        }
        macro_model_free_macro(&replacement);
        web_api_json_free_resource_mutation(&mutation);
        if (result != APP_ERROR_NONE) {
            return respond_error(response, result, "could not update macro", NULL);
        }
        result = send_macro(response, WEB_HTTP_STATUS_OK, &committed);
        macro_model_free_macro(&committed);
        return result;
    }

    uint32_t expected_revision = 0U;
    app_error_code_t result =
        web_api_json_parse_expected_revision(call->body, call->body_length, &expected_revision);
    storage_reference_list_t references = {0};
    if (result == APP_ERROR_NONE) {
        result =
            storage_macro_delete(&location, &call->path.macro_id, expected_revision, &references);
    }
    if (result == APP_ERROR_CONFLICT && references.count > 0U) {
        char *details = NULL;
        const app_error_code_t details_result = reference_details_json(&references, &details);
        if (details_result != APP_ERROR_NONE) {
            return details_result;
        }
        const app_error_code_t response_result =
            respond_error(response, result, "macro is referenced by procedures", details);
        cJSON_free(details);
        return response_result;
    }
    if (result != APP_ERROR_NONE) {
        return respond_error(response, result, "could not delete macro", NULL);
    }
    char data[WEB_MACRO_DELETE_RESPONSE_BYTES];
    const int length =
        snprintf(data, sizeof(data), "{\"deleted\":true,\"id\":\"%s\"}", call->path.macro_id.value);
    return length < 0 || (size_t)length >= sizeof(data)
               ? APP_ERROR_INTERNAL
               : web_api_handler_success_json(response, WEB_HTTP_STATUS_OK, data);
}

static app_error_code_t handle_reorder(const web_api_call_t *call, web_api_response_t *response) {
    storage_uuid_order_t order = {0};
    app_error_code_t result =
        web_api_json_parse_uuid_order(call->body,
                                      &(web_api_order_parse_limits_t){
                                          .body_length = call->body_length,
                                          .maximum_count = APP_MACROS_PER_SET_MAX,
                                      },
                                      &order);
    const storage_macro_location_t location = location_for_call(call);
    if (result == APP_ERROR_NONE) {
        result = storage_macro_reorder(&location, order.ids, order.count);
    }
    return result == APP_ERROR_NONE
               ? web_api_handler_success_json(response, WEB_HTTP_STATUS_OK, "{\"reordered\":true}")
               : respond_error(response, result, "could not reorder macros", NULL);
}

static app_error_code_t handle_validate(const web_api_call_t *call, web_api_response_t *response) {
    const storage_macro_location_t location = location_for_call(call);
    macro_t candidate = {0};
    app_error_code_t result =
        web_api_json_parse_macro_resource(call->body, call->body_length, &candidate);
    if (result == APP_ERROR_NONE &&
        !macro_matches_path(&candidate, &location, &call->path.macro_id)) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    macro_plan_t plan = {0};
    macro_parse_error_t parse_error = {0};
    if (result == APP_ERROR_NONE) {
        const macro_compile_options_t options = {
            .key_press_ms = candidate.key_press_ms,
            .inter_key_ms = candidate.inter_key_ms,
        };
        result =
            macro_compile(candidate.source, candidate.source_length, &options, &plan, &parse_error);
    }
    macro_model_free_macro(&candidate);
    if (result != APP_ERROR_NONE) {
        char details[WEB_MACRO_VALIDATION_DETAILS_BYTES];
        const int length =
            snprintf(details, sizeof(details), "{\"line\":%lu,\"column\":%lu,\"byteOffset\":%lu}",
                     (unsigned long)parse_error.line, (unsigned long)parse_error.column,
                     (unsigned long)parse_error.byte_offset);
        return length < 0 || (size_t)length >= sizeof(details)
                   ? APP_ERROR_INTERNAL
                   : respond_error(response, result, "macro validation failed", details);
    }
    char data[WEB_MACRO_VALIDATION_RESPONSE_BYTES];
    const int length =
        snprintf(data, sizeof(data),
                 "{\"valid\":true,\"actionCount\":%lu,"
                 "\"estimatedDurationMs\":%lu}",
                 (unsigned long)plan.action_count, (unsigned long)plan.estimated_duration_ms);
    macro_plan_free(&plan);
    return length < 0 || (size_t)length >= sizeof(data)
               ? APP_ERROR_INTERNAL
               : web_api_handler_success_json(response, WEB_HTTP_STATUS_OK, data);
}

static app_error_code_t parse_duplicate(const web_api_call_t *call, app_uuid_t *out_id,
                                        char *out_name, size_t out_name_size) {
    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts(call->body, call->body_length, &parse_end, false);
    bool id_seen = false;
    bool name_seen = false;
    bool valid =
        root != NULL && parse_end == call->body + call->body_length && cJSON_IsObject(root);
    for (const cJSON *item = valid ? root->child : NULL; item != NULL; item = item->next) {
        if (item->string != NULL && strcmp(item->string, "id") == 0 && !id_seen &&
            cJSON_IsString(item) && item->valuestring != NULL &&
            app_uuid_parse(item->valuestring, out_id) == APP_ERROR_NONE) {
            id_seen = true;
        } else if (item->string != NULL && strcmp(item->string, "name") == 0 && !name_seen &&
                   cJSON_IsString(item) && item->valuestring != NULL &&
                   strlen(item->valuestring) <= APP_MACRO_NAME_MAX_BYTES &&
                   strlen(item->valuestring) < out_name_size) {
            memcpy(out_name, item->valuestring, strlen(item->valuestring) + 1U);
            name_seen = true;
        } else {
            valid = false;
            break;
        }
    }
    valid = valid && id_seen && name_seen;
    cJSON_Delete(root);
    return valid ? APP_ERROR_NONE : APP_ERROR_INVALID_ARGUMENT;
}

static app_error_code_t handle_duplicate(const web_api_call_t *call, web_api_response_t *response) {
    app_uuid_t duplicate_id = {0};
    char duplicate_name[APP_MACRO_NAME_MAX_BYTES + 1U] = {0};
    app_error_code_t result =
        parse_duplicate(call, &duplicate_id, duplicate_name, sizeof(duplicate_name));
    const storage_macro_location_t location = location_for_call(call);
    macro_t duplicate = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_macro_duplicate(&location, &call->path.macro_id, &duplicate_id,
                                         duplicate_name, &duplicate);
    }
    if (result != APP_ERROR_NONE) {
        return respond_error(response, result, "could not duplicate macro", NULL);
    }
    result = send_macro(response, WEB_HTTP_STATUS_CREATED, &duplicate);
    macro_model_free_macro(&duplicate);
    return result;
}

app_error_code_t web_api_handle_macros(const web_api_call_t *call, web_api_response_t *response) {
    if (call == NULL || response == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    switch (call->path.route) {
    case WEB_API_ROUTE_SET_MACROS:
    case WEB_API_ROUTE_GLOBAL_MACROS:
        return handle_collection(call, response);
    case WEB_API_ROUTE_SET_MACRO:
    case WEB_API_ROUTE_GLOBAL_MACRO:
        return handle_item(call, response);
    case WEB_API_ROUTE_SET_MACROS_REORDER:
    case WEB_API_ROUTE_GLOBAL_MACROS_REORDER:
        return handle_reorder(call, response);
    case WEB_API_ROUTE_SET_MACRO_VALIDATE:
    case WEB_API_ROUTE_GLOBAL_MACRO_VALIDATE:
        return handle_validate(call, response);
    case WEB_API_ROUTE_SET_MACRO_DUPLICATE:
    case WEB_API_ROUTE_GLOBAL_MACRO_DUPLICATE:
        return handle_duplicate(call, response);
    default:
        return APP_ERROR_NOT_FOUND;
    }
}
