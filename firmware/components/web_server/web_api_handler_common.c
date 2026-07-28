#include "web_api_handler_common.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "cJSON.h"
#include "macro_model.h"
#include "provisioning.h"
#include "storage.h"
#include "storage_object_json.h"
#include "storage_repository.h"
#include "web_api_core.h"
#include "web_api_response.h"

static app_error_code_t finish_json(cJSON *root, char **out_json) {
    if (out_json != NULL) {
        *out_json = NULL;
    }
    if (root == NULL || out_json == NULL) {
        cJSON_Delete(root);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (json == NULL || strlen(json) > WEB_API_RESPONSE_MAX_BYTES) {
        cJSON_free(json);
        return APP_ERROR_INTERNAL;
    }
    *out_json = json;
    return APP_ERROR_NONE;
}

static cJSON *parse_serialized(char *json, size_t length) {
    if (json == NULL) {
        return NULL;
    }
    const char *parse_end = NULL;
    cJSON *item = cJSON_ParseWithLengthOpts(json, length, &parse_end, false);
    cJSON_free(json);
    if (item == NULL || parse_end == NULL || (size_t)(parse_end - json) != length) {
        cJSON_Delete(item);
        return NULL;
    }
    return item;
}

app_error_code_t web_api_handler_error(web_api_response_t *response, app_error_code_t error,
                                       const char *message, const char *details_json) {
    return web_api_response_error(response, web_api_http_status_for_error(error), error, message,
                                  details_json);
}

app_error_code_t web_api_handler_success_json(web_api_response_t *response, unsigned int status,
                                              const char *data_json) {
    return web_api_response_success(response, status, data_json);
}

app_error_code_t web_api_handler_set_json(const macro_set_t *set, char **out_json) {
    size_t length = 0U;
    return set == NULL ? APP_ERROR_INVALID_ARGUMENT
                       : storage_repository_serialize_set_json(set, out_json, &length);
}

static cJSON *set_summary(const macro_set_t *set) {
    cJSON *item = cJSON_CreateObject();
    if (item == NULL || !cJSON_AddNumberToObject(item, "schema_version", set->schema_version) ||
        !cJSON_AddStringToObject(item, "id", set->id.value) ||
        !cJSON_AddNumberToObject(item, "revision", set->revision) ||
        !cJSON_AddStringToObject(item, "name", set->name) ||
        !cJSON_AddStringToObject(item, "description", set->description) ||
        !cJSON_AddStringToObject(item, "manufacturer", set->manufacturer) ||
        !cJSON_AddStringToObject(item, "model", set->model) ||
        !cJSON_AddStringToObject(item, "board", set->board) ||
        !cJSON_AddStringToObject(item, "keyboard_layout", set->keyboard_layout) ||
        !cJSON_AddNumberToObject(item, "sort_order", set->sort_order)) {
        cJSON_Delete(item);
        return NULL;
    }
    return item;
}

app_error_code_t web_api_handler_set_list_json(const storage_set_list_t *list, char **out_json) {
    if (list == NULL || out_json == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *array = cJSON_CreateArray();
    if (array == NULL) {
        return APP_ERROR_INTERNAL;
    }
    for (size_t index = 0U; index < list->count; ++index) {
        cJSON *item = set_summary(&list->items[index]);
        if (item == NULL || !cJSON_AddItemToArray(array, item)) {
            cJSON_Delete(item);
            cJSON_Delete(array);
            return APP_ERROR_INTERNAL;
        }
    }
    return finish_json(array, out_json);
}

app_error_code_t web_api_handler_macro_json(const macro_t *macro, char **out_json) {
    size_t length = 0U;
    return macro == NULL ? APP_ERROR_INVALID_ARGUMENT
                         : storage_repository_serialize_macro_json(macro, out_json, &length);
}

static const char *macro_scope_string(macro_scope_t scope) {
    return scope == MACRO_SCOPE_GLOBAL ? "global" : "set";
}

static cJSON *macro_summary(const macro_t *macro) {
    cJSON *item = cJSON_CreateObject();
    if (item == NULL || !cJSON_AddNumberToObject(item, "schema_version", macro->schema_version) ||
        !cJSON_AddStringToObject(item, "id", macro->id.value) ||
        !cJSON_AddNumberToObject(item, "revision", macro->revision) ||
        !cJSON_AddStringToObject(item, "scope", macro_scope_string(macro->scope)) ||
        !cJSON_AddStringToObject(item, "name", macro->name) ||
        !cJSON_AddBoolToObject(item, "favorite", macro->favorite) ||
        !cJSON_AddNumberToObject(item, "key_press_ms", macro->key_press_ms) ||
        !cJSON_AddNumberToObject(item, "inter_key_ms", macro->inter_key_ms) ||
        !cJSON_AddNumberToObject(item, "source_bytes", macro->source_length)) {
        cJSON_Delete(item);
        return NULL;
    }
    if (macro->has_set_id && !cJSON_AddStringToObject(item, "set_id", macro->set_id.value)) {
        cJSON_Delete(item);
        return NULL;
    }
    return item;
}

app_error_code_t web_api_handler_macro_list_json(const storage_macro_list_t *list,
                                                 char **out_json) {
    if (list == NULL || out_json == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *array = cJSON_CreateArray();
    if (array == NULL) {
        return APP_ERROR_INTERNAL;
    }
    for (size_t index = 0U; index < list->count; ++index) {
        cJSON *item = macro_summary(&list->items[index]);
        if (item == NULL || !cJSON_AddItemToArray(array, item)) {
            cJSON_Delete(item);
            cJSON_Delete(array);
            return APP_ERROR_INTERNAL;
        }
    }
    return finish_json(array, out_json);
}

app_error_code_t web_api_handler_procedure_json(const procedure_t *procedure, char **out_json) {
    size_t length = 0U;
    return procedure == NULL
               ? APP_ERROR_INVALID_ARGUMENT
               : storage_repository_serialize_procedure_json(procedure, out_json, &length);
}

static cJSON *procedure_summary(const procedure_t *procedure) {
    cJSON *item = cJSON_CreateObject();
    if (item == NULL ||
        !cJSON_AddNumberToObject(item, "schema_version", procedure->schema_version) ||
        !cJSON_AddStringToObject(item, "id", procedure->id.value) ||
        !cJSON_AddNumberToObject(item, "revision", procedure->revision) ||
        !cJSON_AddStringToObject(item, "set_id", procedure->set_id.value) ||
        !cJSON_AddStringToObject(item, "name", procedure->name) ||
        !cJSON_AddStringToObject(item, "description", procedure->description) ||
        !cJSON_AddNumberToObject(item, "step_count", procedure->step_count) ||
        !cJSON_AddNumberToObject(item, "sort_order", procedure->sort_order)) {
        cJSON_Delete(item);
        return NULL;
    }
    return item;
}

app_error_code_t web_api_handler_procedure_list_json(const storage_procedure_list_t *list,
                                                     char **out_json) {
    if (list == NULL || out_json == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *array = cJSON_CreateArray();
    if (array == NULL) {
        return APP_ERROR_INTERNAL;
    }
    for (size_t index = 0U; index < list->count; ++index) {
        cJSON *item = procedure_summary(&list->items[index]);
        if (item == NULL || !cJSON_AddItemToArray(array, item)) {
            cJSON_Delete(item);
            cJSON_Delete(array);
            return APP_ERROR_INTERNAL;
        }
    }
    return finish_json(array, out_json);
}

app_error_code_t web_api_handler_progress_json(const storage_progress_snapshot_t *snapshot,
                                               char **out_json) {
    if (snapshot == NULL || out_json == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char *progress_json = NULL;
    size_t progress_length = 0U;
    app_error_code_t result = storage_repository_serialize_progress_json(
        &snapshot->progress, &progress_json, &progress_length);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    cJSON *progress = parse_serialized(progress_json, progress_length);
    cJSON *root = cJSON_CreateObject();
    if (progress == NULL || root == NULL ||
        !cJSON_AddStringToObject(root, "status",
                                snapshot->status == STORAGE_PROGRESS_STATUS_STALE ? "stale"
                                                                                 : "current") ||
        !cJSON_AddNumberToObject(root, "currentProcedureRevision",
                                snapshot->current_procedure_revision) ||
        !cJSON_AddItemToObject(root, "progress", progress)) {
        cJSON_Delete(progress);
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    return finish_json(root, out_json);
}

app_error_code_t web_api_handler_settings_json(const provisioning_settings_t *settings,
                                               char **out_json) {
    if (settings == NULL || out_json == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = cJSON_CreateObject();
    if (root == NULL || !cJSON_AddNumberToObject(root, "schemaVersion", settings->schema_version) ||
        !cJSON_AddNumberToObject(root, "revision", settings->revision) ||
        !cJSON_AddBoolToObject(root, "requirePhysicalConfirmation",
                              settings->require_physical_confirmation) ||
        !cJSON_AddBoolToObject(root, "alwaysSelectSet", settings->always_select_set)) {
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    const bool added = settings->has_active_set
                           ? cJSON_AddStringToObject(root, "activeSetId",
                                                    settings->active_set_id.value) != NULL
                           : cJSON_AddNullToObject(root, "activeSetId") != NULL;
    if (!added) {
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    return finish_json(root, out_json);
}

app_error_code_t web_api_handler_quarantine_json(const storage_quarantine_list_t *list,
                                                 char **out_json) {
    if (list == NULL || out_json == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = cJSON_CreateObject();
    cJSON *items = cJSON_CreateArray();
    if (root == NULL || items == NULL ||
        !cJSON_AddNumberToObject(root, "damagedCount", list->damaged_count) ||
        !cJSON_AddItemToObject(root, "items", items)) {
        cJSON_Delete(items);
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    for (size_t index = 0U; index < list->count; ++index) {
        const storage_quarantine_entry_t *entry = &list->items[index];
        cJSON *item = cJSON_CreateObject();
        if (item == NULL || !cJSON_AddStringToObject(item, "id", entry->id.value) ||
            !cJSON_AddStringToObject(item, "sourcePath", entry->source_path) ||
            !cJSON_AddStringToObject(item, "evidencePath", entry->evidence_path) ||
            !cJSON_AddStringToObject(item, "reason", entry->reason) ||
            !cJSON_AddItemToArray(items, item)) {
            cJSON_Delete(item);
            cJSON_Delete(root);
            return APP_ERROR_INTERNAL;
        }
    }
    return finish_json(root, out_json);
}

void web_api_handler_json_free(char *json) {
    cJSON_free(json);
}
