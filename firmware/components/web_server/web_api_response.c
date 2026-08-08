#include "web_api_response.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
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
    response->body_free = cJSON_free;
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
    /* SPEC_V2 13 success responses are the flat object itself -- no v1-style
     * {"ok":true,"data":...} envelope. data_json is parsed (not merely
     * copied) so a malformed caller-supplied string is still rejected. */
    cJSON *data = parse_data(data_json);
    if (data == NULL) {
        return APP_ERROR_INTERNAL;
    }
    return set_serialized(response, status, data);
}

app_error_code_t web_api_response_no_content(web_api_response_t *response, unsigned int status) {
    if (response != NULL) {
        memset(response, 0, sizeof(*response));
    }
    if (response == NULL || status < WEB_HTTP_STATUS_OK ||
        status >= WEB_HTTP_SUCCESS_STATUS_UPPER_BOUND) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    response->status = status;
    return APP_ERROR_NONE;
}

app_error_code_t web_api_response_take_json(web_api_response_t *response, unsigned int status,
                                            char *body, size_t body_length) {
    if (response != NULL) {
        memset(response, 0, sizeof(*response));
    }
    if (response == NULL || body == NULL || body_length == 0U ||
        body_length > WEB_API_RESPONSE_MAX_BYTES || memchr(body, '\0', body_length) != NULL ||
        status < WEB_HTTP_STATUS_OK || status >= WEB_HTTP_SUCCESS_STATUS_UPPER_BOUND) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    response->status = status;
    response->body = body;
    response->body_length = body_length;
    response->body_free = free;
    return APP_ERROR_NONE;
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
    /* SPEC_V2 13.2's exact error envelope: {"error":{"code","message",
     * ["field"],["byteOffset","line","column"]}} -- no top-level "ok" key,
     * and no generic "details" sub-object. */
    cJSON *root = cJSON_CreateObject();
    cJSON *error = cJSON_CreateObject();
    const char *code_string = error_spec->has_parser_location
                                  ? "macro_parse_error"
                                  : app_error_code_string(error_spec->code);
    if (root == NULL || error == NULL || !cJSON_AddStringToObject(error, "code", code_string) ||
        !cJSON_AddStringToObject(error, "message", error_spec->message) ||
        !cJSON_AddItemToObject(root, "error", error)) {
        cJSON_Delete(error);
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    if (error_spec->field != NULL && !cJSON_AddStringToObject(error, "field", error_spec->field)) {
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    if (error_spec->has_parser_location &&
        (!cJSON_AddNumberToObject(error, "byteOffset", (double)error_spec->byte_offset) ||
         !cJSON_AddNumberToObject(error, "line", (double)error_spec->line) ||
         !cJSON_AddNumberToObject(error, "column", (double)error_spec->column))) {
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    return set_serialized(response, error_spec->status, root);
}

void web_api_response_free(web_api_response_t *response) {
    if (response == NULL) {
        return;
    }
    if (response->body_free != NULL) {
        response->body_free(response->body);
    }
    memset(response, 0, sizeof(*response));
}
