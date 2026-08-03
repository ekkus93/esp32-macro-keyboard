#include "web_api_handlers.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "macro_limits.h"
#include "macro_model.h"
#include "provisioning.h"
#include "storage_object_json.h"
#include "storage_package.h"
#include "storage_repository.h"
#include "web_api_core.h"
#include "web_api_handler_common.h"
#include "web_api_json.h"
#include "web_api_response.h"
#include "web_http_status.h"

#define WEB_SET_DELETE_RESPONSE_BYTES 80U
#define WEB_SET_DUPLICATE_NAME_BYTES (APP_NAME_MAX_BYTES + 1U)

static app_error_code_t respond_result(web_api_response_t *response, app_error_code_t result,
                                       const char *message) {
    return web_api_handler_error(response, result, message, NULL);
}

static app_error_code_t send_package(web_api_response_t *response, unsigned int status,
                                     const macro_package_t *set) {
    char *json = NULL;
    app_error_code_t result = web_api_handler_package_json(set, &json);
    if (result == APP_ERROR_NONE) {
        result = web_api_handler_success_json(response, status, json);
    }
    web_api_handler_json_free(json);
    return result;
}

static app_error_code_t handle_package_collection(const web_api_call_t *call,
                                                  web_api_response_t *response) {
    if (call->method == WEB_API_METHOD_GET) {
        /* Heap-allocated, not a stack local: storage_package_list_t is far larger
         * than the httpd task stack (see web_server_lifecycle.c), so a stack
         * local here overflows it and panics the device on every request. */
        storage_package_list_t *list = calloc(1U, sizeof(*list));
        if (list == NULL) {
            return respond_result(response, APP_ERROR_INTERNAL, "could not list packages");
        }
        app_error_code_t result = storage_package_list(list);
        char *json = NULL;
        if (result == APP_ERROR_NONE) {
            result = web_api_handler_package_list_json(list, &json);
        }
        if (result == APP_ERROR_NONE) {
            result = web_api_handler_success_json(response, WEB_HTTP_STATUS_OK, json);
        } else {
            const app_error_code_t response_result =
                respond_result(response, result, "could not list packages");
            if (response_result != APP_ERROR_NONE) {
                result = response_result;
            } else {
                result = APP_ERROR_NONE;
            }
        }
        web_api_handler_json_free(json);
        free(list);
        return result;
    }

    macro_package_t set = {0};
    app_error_code_t result =
        web_api_json_parse_package_resource(call->body, call->body_length, &set);
    if (result == APP_ERROR_NONE && set.revision != 1U) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    if (result == APP_ERROR_NONE) {
        result = storage_package_create(&set);
    }
    if (result == APP_ERROR_NONE) {
        macro_package_t committed = {0};
        result = storage_package_read(&set.id, &committed);
        if (result == APP_ERROR_NONE) {
            return send_package(response, WEB_HTTP_STATUS_CREATED, &committed);
        }
    }
    return respond_result(response, result, "could not create package");
}

static app_error_code_t handle_package_item(const web_api_call_t *call,
                                            web_api_response_t *response) {
    if (call->method == WEB_API_METHOD_GET) {
        macro_package_t set = {0};
        const app_error_code_t result = storage_package_read(&call->path.set_id, &set);
        return result == APP_ERROR_NONE ? send_package(response, WEB_HTTP_STATUS_OK, &set)
                                        : respond_result(response, result, "package not available");
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
        macro_package_t replacement = {0};
        if (result == APP_ERROR_NONE) {
            result = web_api_json_parse_package_resource(mutation.resource_json,
                                                         mutation.resource_length, &replacement);
        }
        if (result == APP_ERROR_NONE && !app_uuid_equal(&replacement.id, &call->path.set_id)) {
            result = APP_ERROR_INVALID_ARGUMENT;
        }
        macro_package_t committed = {0};
        if (result == APP_ERROR_NONE) {
            result = storage_package_update(&replacement, mutation.expected_revision, &committed);
        }
        web_api_json_free_resource_mutation(&mutation);
        return result == APP_ERROR_NONE
                   ? send_package(response, WEB_HTTP_STATUS_OK, &committed)
                   : respond_result(response, result, "could not update package");
    }

    uint32_t expected_revision = 0U;
    app_error_code_t result =
        web_api_json_parse_expected_revision(call->body, call->body_length, &expected_revision);
    if (result == APP_ERROR_NONE) {
        result = storage_package_delete(&call->path.set_id, expected_revision);
    }
    if (result != APP_ERROR_NONE) {
        return respond_result(response, result, "could not delete package");
    }
    char data[WEB_SET_DELETE_RESPONSE_BYTES];
    const int length =
        snprintf(data, sizeof(data), "{\"deleted\":true,\"id\":\"%s\"}", call->path.set_id.value);
    if (length < 0 || (size_t)length >= sizeof(data)) {
        return APP_ERROR_INTERNAL;
    }
    return web_api_handler_success_json(response, WEB_HTTP_STATUS_OK, data);
}

/* Selection is a repository write (SPEC 12.3): it records the active set in the
 * index beside the set order, so deleting the active set clears it in one atomic
 * write. The response still carries settings, because the client wants the whole
 * operational state back in one round trip.
 *
 * There is deliberately NO expectedRevision. Selection used to be a settings
 * update and carried the settings revision; Phase 4b moved the active set out of
 * settings, which left that parameter parsed but never compared against
 * anything -- while the route still rejected a body without it. Selection is
 * idempotent and has no revision the client holds, so the body is now empty.
 * Found by the hardware harness, which sent `{}` and got 422. */
static app_error_code_t handle_select(const web_api_call_t *call, web_api_response_t *response) {
    macro_package_t set = {0};
    app_error_code_t result = storage_package_read(&call->path.set_id, &set);
    if (result == APP_ERROR_NONE) {
        result = storage_package_select(&call->path.set_id);
    }
    provisioning_settings_t committed = {0};
    if (result == APP_ERROR_NONE) {
        result = provisioning_settings_read(&committed);
    }
    if (result != APP_ERROR_NONE) {
        return respond_result(response, result, "could not select package");
    }
    char *json = NULL;
    result = web_api_handler_settings_json(&committed, &json);
    if (result == APP_ERROR_NONE) {
        result = web_api_handler_success_json(response, WEB_HTTP_STATUS_OK, json);
    }
    web_api_handler_json_free(json);
    return result;
}

static app_error_code_t parse_package_duplicate(const web_api_call_t *call,
                                                app_uuid_t *out_duplicate_id,
                                                uint32_t *out_expected_revision, char *out_name,
                                                size_t out_name_size) {
    if (call == NULL || out_duplicate_id == NULL || out_expected_revision == NULL ||
        out_name == NULL || out_name_size == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_duplicate_id, 0, sizeof(*out_duplicate_id));
    *out_expected_revision = 0U;
    out_name[0] = '\0';
    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts(call->body, call->body_length, &parse_end, false);
    bool identifier_seen = false;
    bool name_seen = false;
    bool revision_seen = false;
    bool valid =
        root != NULL && parse_end == call->body + call->body_length && cJSON_IsObject(root);
    for (const cJSON *item = valid ? root->child : NULL; item != NULL; item = item->next) {
        if (item->string != NULL && strcmp(item->string, "id") == 0 && !identifier_seen &&
            cJSON_IsString(item) && item->valuestring != NULL &&
            app_uuid_parse(item->valuestring, out_duplicate_id) == APP_ERROR_NONE) {
            identifier_seen = true;
        } else if (item->string != NULL && strcmp(item->string, "name") == 0 && !name_seen &&
                   cJSON_IsString(item) && item->valuestring != NULL) {
            const size_t length = strlen(item->valuestring);
            if (length == 0U || length > APP_NAME_MAX_BYTES || length >= out_name_size) {
                valid = false;
                break;
            }
            memcpy(out_name, item->valuestring, length + 1U);
            name_seen = true;
        } else if (item->string != NULL && strcmp(item->string, "expectedRevision") == 0 &&
                   !revision_seen && cJSON_IsNumber(item) && item->valuedouble >= 1.0 &&
                   item->valuedouble <= (double)UINT32_MAX &&
                   item->valuedouble == (double)item->valueint) {
            *out_expected_revision = (uint32_t)item->valuedouble;
            revision_seen = true;
        } else {
            valid = false;
            break;
        }
    }
    cJSON_Delete(root);
    valid = valid && identifier_seen && name_seen && revision_seen;
    if (!valid) {
        memset(out_duplicate_id, 0, sizeof(*out_duplicate_id));
        *out_expected_revision = 0U;
        out_name[0] = '\0';
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t handle_duplicate(const web_api_call_t *call, web_api_response_t *response) {
    app_uuid_t duplicate_id = {0};
    uint32_t expected_revision = 0U;
    char duplicate_name[WEB_SET_DUPLICATE_NAME_BYTES] = {0};
    app_error_code_t result = parse_package_duplicate(call, &duplicate_id, &expected_revision,
                                                      duplicate_name, sizeof(duplicate_name));
    macro_package_t duplicate = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_package_duplicate(&call->path.set_id, expected_revision, &duplicate_id,
                                           duplicate_name, &duplicate);
    }
    return result == APP_ERROR_NONE
               ? send_package(response, WEB_HTTP_STATUS_CREATED, &duplicate)
               : respond_result(response, result, "could not duplicate package");
}

static app_error_code_t handle_package_reorder(const web_api_call_t *call,
                                               web_api_response_t *response) {
    storage_uuid_order_t order = {0};
    app_error_code_t result = web_api_json_parse_uuid_order(call->body,
                                                            &(web_api_order_parse_limits_t){
                                                                .body_length = call->body_length,
                                                                .maximum_count = APP_MACRO_SETS_MAX,
                                                            },
                                                            &order);
    if (result == APP_ERROR_NONE) {
        result = storage_package_reorder(order.ids, order.count);
    }
    /* Heap-allocated for the same reason as handle_package_collection() above. */
    storage_package_list_t *committed = calloc(1U, sizeof(*committed));
    if (committed == NULL) {
        return respond_result(response, APP_ERROR_INTERNAL, "could not reorder packages");
    }
    char *json = NULL;
    if (result == APP_ERROR_NONE) {
        result = storage_package_list(committed);
    }
    if (result == APP_ERROR_NONE) {
        result = web_api_handler_package_list_json(committed, &json);
    }
    if (result == APP_ERROR_NONE) {
        result = web_api_handler_success_json(response, WEB_HTTP_STATUS_OK, json);
    } else {
        const app_error_code_t encoded =
            respond_result(response, result, "could not reorder packages");
        result = encoded == APP_ERROR_NONE ? APP_ERROR_NONE : encoded;
    }
    web_api_handler_json_free(json);
    free(committed);
    return result;
}

typedef struct {
    app_uuid_t target_package_id;
    uint32_t expected_revision;
    char *package_json;
    size_t package_length;
} web_package_import_request_t;

static void free_package_import_request(web_package_import_request_t *request) {
    if (request == NULL) {
        return;
    }
    cJSON_free(request->package_json);
    memset(request, 0, sizeof(*request));
}

typedef struct {
    bool target;
    bool revision;
    bool package;
} set_import_seen_t;

static app_error_code_t parse_package_import_item(const cJSON *item,
                                                  web_package_import_request_t *out_request,
                                                  set_import_seen_t *seen) {
    if (item->string != NULL && strcmp(item->string, "targetPackageId") == 0 && !seen->target &&
        cJSON_IsString(item) && item->valuestring != NULL &&
        app_uuid_parse(item->valuestring, &out_request->target_package_id) == APP_ERROR_NONE) {
        seen->target = true;
        return APP_ERROR_NONE;
    }
    if (item->string != NULL && strcmp(item->string, "expectedRevision") == 0 && !seen->revision &&
        cJSON_IsNumber(item) && item->valuedouble >= 1.0 &&
        item->valuedouble <= (double)UINT32_MAX) {
        const uint32_t revision = (uint32_t)item->valuedouble;
        if ((double)revision != item->valuedouble) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        out_request->expected_revision = revision;
        seen->revision = true;
        return APP_ERROR_NONE;
    }
    if (item->string != NULL && strcmp(item->string, "package") == 0 && !seen->package &&
        cJSON_IsObject(item)) {
        out_request->package_json = cJSON_PrintUnformatted(item);
        if (out_request->package_json == NULL) {
            return APP_ERROR_INTERNAL;
        }
        out_request->package_length = strlen(out_request->package_json);
        if (out_request->package_length == 0U ||
            out_request->package_length > APP_IMPORT_PACKAGE_MAX_BYTES) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        seen->package = true;
        return APP_ERROR_NONE;
    }
    return APP_ERROR_INVALID_ARGUMENT;
}

static app_error_code_t parse_package_import(const web_api_call_t *call,
                                             web_package_import_request_t *out_request) {
    if (call == NULL || out_request == NULL || call->body == NULL || call->body_length == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_request, 0, sizeof(*out_request));
    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts(call->body, call->body_length, &parse_end, false);
    set_import_seen_t seen = {0};
    app_error_code_t result =
        root != NULL && parse_end == call->body + call->body_length && cJSON_IsObject(root)
            ? APP_ERROR_NONE
            : APP_ERROR_INVALID_ARGUMENT;
    for (const cJSON *item = result == APP_ERROR_NONE ? root->child : NULL; item != NULL;
         item = item->next) {
        result = parse_package_import_item(item, out_request, &seen);
        if (result != APP_ERROR_NONE) {
            break;
        }
    }
    cJSON_Delete(root);
    if (result == APP_ERROR_NONE && (!seen.target || !seen.revision || !seen.package)) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    if (result != APP_ERROR_NONE) {
        free_package_import_request(out_request);
    }
    return result;
}

static app_error_code_t handle_import(const web_api_call_t *call, web_api_response_t *response) {
    web_package_import_request_t request = {0};
    app_error_code_t result = parse_package_import(call, &request);
    macro_package_t committed = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_package_replace(&request.target_package_id, request.expected_revision,
                                         request.package_json, request.package_length, &committed);
    }
    free_package_import_request(&request);
    return result == APP_ERROR_NONE ? send_package(response, WEB_HTTP_STATUS_OK, &committed)
                                    : respond_result(response, result, "could not replace package");
}

typedef struct {
    app_uuid_t new_package_id;
    char *package_json;
    size_t package_length;
} web_package_import_new_request_t;

static void free_package_import_new_request(web_package_import_new_request_t *request) {
    if (request == NULL) {
        return;
    }
    cJSON_free(request->package_json);
    memset(request, 0, sizeof(*request));
}

typedef struct {
    bool new_package_id;
    bool package;
} set_import_new_seen_t;

static app_error_code_t parse_package_import_new_item(const cJSON *item,
                                                      web_package_import_new_request_t *out_request,
                                                      set_import_new_seen_t *seen) {
    if (item->string != NULL && strcmp(item->string, "newPackageId") == 0 &&
        !seen->new_package_id && cJSON_IsString(item) && item->valuestring != NULL &&
        app_uuid_parse(item->valuestring, &out_request->new_package_id) == APP_ERROR_NONE) {
        seen->new_package_id = true;
        return APP_ERROR_NONE;
    }
    if (item->string != NULL && strcmp(item->string, "package") == 0 && !seen->package &&
        cJSON_IsObject(item)) {
        out_request->package_json = cJSON_PrintUnformatted(item);
        if (out_request->package_json == NULL) {
            return APP_ERROR_INTERNAL;
        }
        out_request->package_length = strlen(out_request->package_json);
        if (out_request->package_length == 0U ||
            out_request->package_length > APP_IMPORT_PACKAGE_MAX_BYTES) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        seen->package = true;
        return APP_ERROR_NONE;
    }
    return APP_ERROR_INVALID_ARGUMENT;
}

static app_error_code_t parse_package_import_new(const web_api_call_t *call,
                                                 web_package_import_new_request_t *out_request) {
    if (call == NULL || out_request == NULL || call->body == NULL || call->body_length == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_request, 0, sizeof(*out_request));
    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts(call->body, call->body_length, &parse_end, false);
    set_import_new_seen_t seen = {0};
    app_error_code_t result =
        root != NULL && parse_end == call->body + call->body_length && cJSON_IsObject(root)
            ? APP_ERROR_NONE
            : APP_ERROR_INVALID_ARGUMENT;
    for (const cJSON *item = result == APP_ERROR_NONE ? root->child : NULL; item != NULL;
         item = item->next) {
        result = parse_package_import_new_item(item, out_request, &seen);
        if (result != APP_ERROR_NONE) {
            break;
        }
    }
    cJSON_Delete(root);
    if (result == APP_ERROR_NONE && (!seen.new_package_id || !seen.package)) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    if (result != APP_ERROR_NONE) {
        free_package_import_new_request(out_request);
    }
    return result;
}

static app_error_code_t handle_import_new(const web_api_call_t *call,
                                          web_api_response_t *response) {
    web_package_import_new_request_t request = {0};
    app_error_code_t result = parse_package_import_new(call, &request);
    macro_package_t committed = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_package_import(&request.new_package_id, request.package_json,
                                        request.package_length, &committed);
    }
    free_package_import_new_request(&request);
    return result == APP_ERROR_NONE
               ? send_package(response, WEB_HTTP_STATUS_CREATED, &committed)
               : respond_result(response, result, "could not import package as new");
}

static app_error_code_t handle_export(const web_api_call_t *call, web_api_response_t *response) {
    char *package_json = NULL;
    size_t package_length = 0U;
    app_error_code_t result =
        storage_package_export(&call->path.set_id, &package_json, &package_length);
    if (result != APP_ERROR_NONE) {
        return respond_result(response, result, "could not export package");
    }
    result = web_api_response_take_json(response, WEB_HTTP_STATUS_OK, package_json, package_length);
    if (result != APP_ERROR_NONE) {
        storage_package_free(package_json);
    }
    return result;
}

app_error_code_t web_api_handle_packages(const web_api_call_t *call, web_api_response_t *response) {
    if (call == NULL || response == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    switch (call->path.route) {
    case WEB_API_ROUTE_SETS:
        return handle_package_collection(call, response);
    case WEB_API_ROUTE_SETS_ORDER:
        return handle_package_reorder(call, response);
    case WEB_API_ROUTE_SET:
        return handle_package_item(call, response);
    case WEB_API_ROUTE_SET_SELECT:
        return handle_select(call, response);
    case WEB_API_ROUTE_SET_DUPLICATE:
        return handle_duplicate(call, response);
    case WEB_API_ROUTE_SET_EXPORT:
        return handle_export(call, response);
    case WEB_API_ROUTE_SET_IMPORT:
        return handle_import(call, response);
    case WEB_API_ROUTE_SET_IMPORT_NEW:
        return handle_import_new(call, response);
    default:
        return APP_ERROR_NOT_FOUND;
    }
}
