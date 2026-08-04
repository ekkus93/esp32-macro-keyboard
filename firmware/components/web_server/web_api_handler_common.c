#include "web_api_handler_common.h"

#include <string.h>

#include "app_error.h"
#include "cJSON.h"
#include "provisioning.h"
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
    return finish_json(root, out_json);
}

void web_api_handler_json_free(char *json) {
    cJSON_free(json);
}
