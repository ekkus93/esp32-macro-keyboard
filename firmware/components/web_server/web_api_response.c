#include "web_api_response.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "cJSON.h"

static app_error_code_t set_serialized(web_api_response_t *response, unsigned int status,
                                       cJSON *root) {
    char *serialized = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (serialized == NULL) {
        return APP_ERROR_INTERNAL;
    }
    const size_t length = strlen(serialized);
    if (length == 0U || length > WEB_API_RESPONSE_MAX_BYTES) {
        cJSON_free(serialized);
        return APP_ERROR_INTERNAL;
    }
    response->status = status;
    response->body = serialized;
    response->body_length = length;
    return APP_ERROR_NONE;
}

static cJSON *parse_data(const char *json) {
    if (json == NULL) {
        return NULL;
    }
    const size_t length = strlen(json);
    const char *parse_end = NULL;
    cJSON *value = cJSON_ParseWithLengthOpts(json, length, &parse_end, false);
    if (value == NULL || parse_end != json + length) {
        cJSON_Delete(value);
        return NULL;
    }
    return value;
}

app_error_code_t web_api_response_success(web_api_response_t *response, unsigned int status,
                                          const char *data_json) {
    if (response != NULL) {
        memset(response, 0, sizeof(*response));
    }
    if (response == NULL || data_json == NULL || status < 200U || status >= 300U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *data = parse_data(data_json);
    cJSON *root = cJSON_CreateObject();
    if (data == NULL || root == NULL || !cJSON_AddBoolToObject(root, "ok", true) ||
        !cJSON_AddItemToObject(root, "data", data)) {
        cJSON_Delete(data);
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    return set_serialized(response, status, root);
}

app_error_code_t web_api_response_error(web_api_response_t *response, unsigned int status,
                                        app_error_code_t code, const char *message,
                                        const char *details_json) {
    if (response != NULL) {
        memset(response, 0, sizeof(*response));
    }
    if (response == NULL || message == NULL || status < 400U || status > 599U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = cJSON_CreateObject();
    cJSON *error = cJSON_CreateObject();
    if (root == NULL || error == NULL || !cJSON_AddBoolToObject(root, "ok", false) ||
        !cJSON_AddStringToObject(error, "code", app_error_code_string(code)) ||
        !cJSON_AddStringToObject(error, "message", message) ||
        !cJSON_AddItemToObject(root, "error", error)) {
        cJSON_Delete(error);
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    if (details_json != NULL) {
        cJSON *details = parse_data(details_json);
        if (details == NULL || !cJSON_AddItemToObject(error, "details", details)) {
            cJSON_Delete(details);
            cJSON_Delete(root);
            return APP_ERROR_INTERNAL;
        }
    }
    return set_serialized(response, status, root);
}

void web_api_response_free(web_api_response_t *response) {
    if (response == NULL) {
        return;
    }
    cJSON_free(response->body);
    memset(response, 0, sizeof(*response));
}
