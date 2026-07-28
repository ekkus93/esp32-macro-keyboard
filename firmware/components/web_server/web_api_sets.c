#include "web_api_handlers.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_model.h"
#include "provisioning.h"
#include "storage_object_json.h"
#include "storage_repository.h"
#include "web_api_core.h"
#include "web_api_handler_common.h"
#include "web_api_json.h"
#include "web_api_response.h"
#include "web_http_status.h"

#define WEB_SET_DELETE_RESPONSE_BYTES 80U

static app_error_code_t respond_result(web_api_response_t *response, app_error_code_t result,
                                       const char *message) {
    return web_api_handler_error(response, result, message, NULL);
}

static app_error_code_t send_set(web_api_response_t *response, unsigned int status,
                                 const macro_set_t *set) {
    char *json = NULL;
    app_error_code_t result = web_api_handler_set_json(set, &json);
    if (result == APP_ERROR_NONE) {
        result = web_api_handler_success_json(response, status, json);
    }
    web_api_handler_json_free(json);
    return result;
}

static app_error_code_t handle_set_collection(const web_api_call_t *call,
                                              web_api_response_t *response) {
    if (call->method == WEB_API_METHOD_GET) {
        storage_set_list_t list = {0};
        app_error_code_t result = storage_set_list(&list);
        char *json = NULL;
        if (result == APP_ERROR_NONE) {
            result = web_api_handler_set_list_json(&list, &json);
        }
        if (result == APP_ERROR_NONE) {
            result = web_api_handler_success_json(response, WEB_HTTP_STATUS_OK, json);
        } else {
            const app_error_code_t response_result =
                respond_result(response, result, "could not list sets");
            if (response_result != APP_ERROR_NONE) {
                result = response_result;
            } else {
                result = APP_ERROR_NONE;
            }
        }
        web_api_handler_json_free(json);
        return result;
    }

    macro_set_t set = {0};
    app_error_code_t result =
        storage_repository_parse_set_json(call->body, call->body_length, &set);
    if (result == APP_ERROR_NONE && set.revision != 1U) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    if (result == APP_ERROR_NONE) {
        result = storage_set_create(&set);
    }
    if (result == APP_ERROR_NONE) {
        macro_set_t committed = {0};
        result = storage_set_read(&set.id, &committed);
        if (result == APP_ERROR_NONE) {
            return send_set(response, WEB_HTTP_STATUS_CREATED, &committed);
        }
    }
    return respond_result(response, result, "could not create set");
}

static app_error_code_t handle_set_item(const web_api_call_t *call, web_api_response_t *response) {
    if (call->method == WEB_API_METHOD_GET) {
        macro_set_t set = {0};
        const app_error_code_t result = storage_set_read(&call->path.set_id, &set);
        return result == APP_ERROR_NONE ? send_set(response, WEB_HTTP_STATUS_OK, &set)
                                        : respond_result(response, result, "set not available");
    }
    if (call->method == WEB_API_METHOD_PUT) {
        web_api_resource_mutation_t mutation = {0};
        app_error_code_t result = web_api_json_parse_resource_mutation(
            call->body,
            &(web_api_resource_parse_limits_t){
                .body_length = call->body_length,
                .maximum_resource_length = STORAGE_SET_FILE_MAX_BYTES,
            },
            &mutation);
        macro_set_t replacement = {0};
        if (result == APP_ERROR_NONE) {
            result = storage_repository_parse_set_json(mutation.resource_json,
                                                       mutation.resource_length, &replacement);
        }
        if (result == APP_ERROR_NONE && !app_uuid_equal(&replacement.id, &call->path.set_id)) {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
        macro_set_t committed = {0};
        if (result == APP_ERROR_NONE) {
            result = storage_set_update(&replacement, mutation.expected_revision, &committed);
        }
        web_api_json_free_resource_mutation(&mutation);
        return result == APP_ERROR_NONE ? send_set(response, WEB_HTTP_STATUS_OK, &committed)
                                        : respond_result(response, result, "could not update set");
    }

    uint32_t expected_revision = 0U;
    app_error_code_t result =
        web_api_json_parse_expected_revision(call->body, call->body_length, &expected_revision);
    if (result == APP_ERROR_NONE) {
        result = storage_set_delete(&call->path.set_id, expected_revision);
    }
    if (result != APP_ERROR_NONE) {
        return respond_result(response, result, "could not delete set");
    }
    char data[WEB_SET_DELETE_RESPONSE_BYTES];
    const int length =
        snprintf(data, sizeof(data), "{\"deleted\":true,\"id\":\"%s\"}", call->path.set_id.value);
    if (length < 0 || (size_t)length >= sizeof(data)) {
        return APP_ERROR_INTERNAL;
    }
    return web_api_handler_success_json(response, WEB_HTTP_STATUS_OK, data);
}

static app_error_code_t handle_select(const web_api_call_t *call, web_api_response_t *response) {
    uint32_t expected_revision = 0U;
    app_error_code_t result =
        web_api_json_parse_expected_revision(call->body, call->body_length, &expected_revision);
    macro_set_t set = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_set_read(&call->path.set_id, &set);
    }
    provisioning_settings_t settings = {0};
    if (result == APP_ERROR_NONE) {
        result = provisioning_settings_read(&settings);
    }
    provisioning_settings_t committed = {0};
    if (result == APP_ERROR_NONE) {
        settings.has_active_set = true;
        settings.active_set_id = call->path.set_id;
        result = provisioning_settings_update(&settings, expected_revision, &committed);
    }
    if (result != APP_ERROR_NONE) {
        return respond_result(response, result, "could not select set");
    }
    char *json = NULL;
    result = web_api_handler_settings_json(&committed, &json);
    if (result == APP_ERROR_NONE) {
        result = web_api_handler_success_json(response, WEB_HTTP_STATUS_OK, json);
    }
    web_api_handler_json_free(json);
    return result;
}

static app_error_code_t unavailable(web_api_response_t *response, const char *operation) {
    return web_api_handler_error(response, APP_ERROR_STORAGE_UNAVAILABLE, operation, NULL);
}

app_error_code_t web_api_handle_sets(const web_api_call_t *call, web_api_response_t *response) {
    if (call == NULL || response == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    switch (call->path.route) {
    case WEB_API_ROUTE_SETS:
        return handle_set_collection(call, response);
    case WEB_API_ROUTE_SET:
        return handle_set_item(call, response);
    case WEB_API_ROUTE_SET_SELECT:
        return handle_select(call, response);
    case WEB_API_ROUTE_SET_DUPLICATE:
        return unavailable(response, "set duplication requires the Phase 18 transaction service");
    case WEB_API_ROUTE_SET_EXPORT:
        return unavailable(response, "set export requires the Phase 18 package service");
    case WEB_API_ROUTE_SET_IMPORT:
        return unavailable(response, "set import requires the Phase 18 package service");
    default:
        return APP_ERROR_NOT_FOUND;
    }
}
