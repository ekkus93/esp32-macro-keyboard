#include "web_api_response.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "app_error.h"
#include "cJSON.h"
#include "web_http_status.h"

#define WEB_HTTP_SUCCESS_STATUS_UPPER_BOUND 300U
#define WEB_HTTP_ERROR_STATUS_UPPER_BOUND 599U

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
    if (response == NULL || data_json == NULL || status < WEB_HTTP_STATUS_OK ||
        status >= WEB_HTTP_SUCCESS_STATUS_UPPER_BOUND) {
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

app_error_code_t web_api_response_error(web_api_response_t *response,
                                        const web_api_error_spec_t *error_spec) {
    if (response != NULL) {
        memset(response, 0, sizeof(*response));
    }
    if (response == NULL || error_spec == NULL || error_spec->message == NULL ||
        error_spec->status < WEB_HTTP_STATUS_BAD_REQUEST ||
        error_spec->status > WEB_HTTP_ERROR_STATUS_UPPER_BOUND) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = cJSON_CreateObject();
    cJSON *error = cJSON_CreateObject();
    if (root == NULL || error == NULL || !cJSON_AddBoolToObject(root, "ok", false) ||
        !cJSON_AddStringToObject(error, "code", app_error_code_string(error_spec->code)) ||
        !cJSON_AddStringToObject(error, "message", error_spec->message) ||
        !cJSON_AddItemToObject(root, "error", error)) {
        cJSON_Delete(error);
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    if (error_spec->details_json != NULL) {
        cJSON *details = parse_data(error_spec->details_json);
        if (details == NULL || !cJSON_AddItemToObject(error, "details", details)) {
            cJSON_Delete(details);
            cJSON_Delete(root);
            return APP_ERROR_INTERNAL;
        }
    }
    return set_serialized(response, error_spec->status, root);
}

void web_api_response_free(web_api_response_t *response) {
    if (response == NULL) {
        return;
    }
    cJSON_free(response->body);
    memset(response, 0, sizeof(*response));
}
