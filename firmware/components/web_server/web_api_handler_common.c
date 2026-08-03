#include "web_api_handler_common.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "cJSON.h"
#include "macro_model.h"
#include "provisioning.h"
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

static bool size_to_json_number(size_t value, double *out_number) {
    if (out_number == NULL || value > UINT32_MAX) {
        return false;
    }
    *out_number = (double)(uint32_t)value;
    return true;
}

app_error_code_t web_api_handler_error(web_api_response_t *response, app_error_code_t error,
                                       const char *message, const char *details_json) {
    return web_api_response_error(response, &(web_api_error_spec_t){
                                                .status = web_api_http_status_for_error(error),
                                                .code = error,
                                                .message = message,
                                                .details_json = details_json,
                                            });
}

app_error_code_t web_api_handler_success_json(web_api_response_t *response, unsigned int status,
                                              const char *data_json) {
    return web_api_response_success(response, status, data_json);
}

app_error_code_t web_api_handler_session_json(char **out_json) {
    if (out_json != NULL) {
        *out_json = NULL;
    }
    if (out_json == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = cJSON_CreateObject();
    if (root == NULL || !cJSON_AddBoolToObject(root, "authenticated", true)) {
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    return finish_json(root, out_json);
}

app_error_code_t web_api_handler_package_json(const macro_package_t *set, char **out_json) {
    size_t length = 0U;
    return set == NULL ? APP_ERROR_INVALID_ARGUMENT
                       : storage_repository_serialize_package_json(set, out_json, &length);
}

static cJSON *set_summary(const macro_package_t *set) {
    cJSON *item = cJSON_CreateObject();
    if (item == NULL || !cJSON_AddNumberToObject(item, "schema_version", set->schema_version) ||
        !cJSON_AddStringToObject(item, "id", set->id.value) ||
        !cJSON_AddNumberToObject(item, "revision", set->revision) ||
        !cJSON_AddStringToObject(item, "name", set->name)) {
        cJSON_Delete(item);
        return NULL;
    }
    return item;
}

app_error_code_t web_api_handler_package_list_json(const storage_package_list_t *list,
                                                   char **out_json) {
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

static cJSON *macro_summary(const macro_t *macro) {
    double source_bytes = 0.0;
    if (!size_to_json_number(macro->source_length, &source_bytes)) {
        return NULL;
    }
    cJSON *item = cJSON_CreateObject();
    if (item == NULL || !cJSON_AddNumberToObject(item, "schema_version", macro->schema_version) ||
        !cJSON_AddStringToObject(item, "id", macro->id.value) ||
        !cJSON_AddNumberToObject(item, "revision", macro->revision) ||
        !cJSON_AddStringToObject(item, "name", macro->name) ||
        !cJSON_AddNumberToObject(item, "key_press_ms", macro->key_press_ms) ||
        !cJSON_AddNumberToObject(item, "inter_key_ms", macro->inter_key_ms) ||
        !cJSON_AddNumberToObject(item, "source_bytes", source_bytes)) {
        cJSON_Delete(item);
        return NULL;
    }
    if (!cJSON_AddStringToObject(item, "package_id", macro->set_id.value)) {
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
        !cJSON_AddBoolToObject(root, "alwaysSelectPackage", settings->always_select_package)) {
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    /* activeSetId is read from the set index, not from settings: the active set
     * is repository state (SPEC 12.3), and NVS no longer holds a copy of it. It
     * stays in this response because clients need it in one round trip, but it
     * is read-only here -- selection goes through POST /sets/{setId}/select. */
    bool has_active_package = false;
    app_uuid_t active_package_id = {0};
    const app_error_code_t active =
        storage_active_package_read(&has_active_package, &active_package_id);
    if (active != APP_ERROR_NONE) {
        cJSON_Delete(root);
        return active;
    }
    const bool added = has_active_package ? cJSON_AddStringToObject(root, "activePackageId",
                                                                    active_package_id.value) != NULL
                                          : cJSON_AddNullToObject(root, "activePackageId") != NULL;
    if (!added) {
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    return finish_json(root, out_json);
}

void web_api_handler_json_free(char *json) {
    cJSON_free(json);
}
