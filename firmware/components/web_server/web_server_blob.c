#include "web_server_internal.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "app_limits_v2.h"
#include "app_uuid.h"
#include "auth.h"
#include "cJSON.h"
#include "esp_http_server.h"
#include "storage.h"
#include "storage_blob.h"
#include "web_api_core.h"
#include "web_api_response.h"
#include "web_http_status.h"
#include "web_server_adapter.h"

#define BLOB_CONTENT_TYPE_MAX_BYTES 64U
#define BLOB_CREATED_JSON_MAX_BYTES 128U

_Static_assert(WEB_API_BLOB_ID_DIGITS == STORAGE_BLOB_ID_DIGITS,
               "API and storage blob ID widths must match");

typedef struct {
    storage_blob_upload_t *upload;
} blob_stream_context_t;

typedef struct {
    cJSON *entries;
    size_t used_bytes;
} blob_list_context_t;

typedef struct {
    unsigned int code;
    const char *text;
} blob_success_status_t;

static const blob_success_status_t BLOB_STATUS_OK = {
    .code = WEB_HTTP_STATUS_OK,
    .text = "200 OK",
};

static const blob_success_status_t BLOB_STATUS_CREATED = {
    .code = WEB_HTTP_STATUS_CREATED,
    .text = "201 Created",
};

static int receive_blob_body(void *context, char *buffer, size_t capacity) {
    httpd_req_t *request = context;
    const int count = httpd_req_recv(request, buffer, capacity);
    return count == HTTPD_SOCK_ERR_TIMEOUT ? WEB_ADAPTER_RECEIVE_TIMEOUT : count;
}

static app_error_code_t consume_blob_chunk(void *context, const char *buffer, size_t length) {
    blob_stream_context_t *stream_context = context;
    if (stream_context == NULL || stream_context->upload == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return storage_blob_upload_write(stream_context->upload, buffer, length);
}

static app_error_code_t establish_request_id(httpd_req_t *request, char *out_request_id,
                                             size_t request_id_size) {
    if (request == NULL || out_request_id == NULL ||
        request_id_size < WEB_API_REQUEST_ID_MAX_BYTES + 1U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    out_request_id[0] = '\0';
    const size_t supplied_length = httpd_req_get_hdr_value_len(request, "X-Request-ID");
    if (supplied_length > WEB_API_REQUEST_ID_MAX_BYTES) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (supplied_length > 0U) {
        if (httpd_req_get_hdr_value_str(request, "X-Request-ID", out_request_id, request_id_size) !=
                ESP_OK ||
            !web_api_request_id_is_valid(out_request_id)) {
            out_request_id[0] = '\0';
            return APP_ERROR_INVALID_ARGUMENT;
        }
        return APP_ERROR_NONE;
    }

    app_uuid_t generated = {0};
    const app_error_code_t result = app_uuid_generate(&generated);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    const size_t generated_length = strlen(generated.value);
    if (generated_length >= request_id_size) {
        return APP_ERROR_INTERNAL;
    }
    memcpy(out_request_id, generated.value, generated_length + 1U);
    return APP_ERROR_NONE;
}

static unsigned int blob_error_status(app_error_code_t error) {
    return error == APP_ERROR_CONFLICT ? WEB_HTTP_STATUS_SERVICE_UNAVAILABLE
                                       : web_api_http_status_for_error(error);
}

static esp_err_t send_blob_error(httpd_req_t *request, const char *request_id, unsigned int status,
                                 app_error_code_t error, const char *message) {
    if (request_id != NULL && request_id[0] != '\0' &&
        httpd_resp_set_hdr(request, "X-Request-ID", request_id) != ESP_OK) {
        return ESP_FAIL;
    }
    return web_api_send_status_error(request, status, error, message);
}

static esp_err_t send_blob_success_json(httpd_req_t *request, const char *request_id,
                                        blob_success_status_t status, const char *data_json) {
    web_api_response_t response = {0};
    const app_error_code_t result = web_api_response_success(&response, status.code, data_json);
    if (result != APP_ERROR_NONE) {
        return send_blob_error(request, request_id, WEB_HTTP_STATUS_INTERNAL_SERVER_ERROR,
                               APP_ERROR_INTERNAL, "blob response encoding failed");
    }
    if (httpd_resp_set_hdr(request, "X-Request-ID", request_id) != ESP_OK) {
        web_api_response_free(&response);
        return ESP_FAIL;
    }
    const esp_err_t send_result = send_json(request, response.body, status.text);
    web_api_response_free(&response);
    return send_result;
}

static esp_err_t send_blob_created(httpd_req_t *request, const char *request_id,
                                   const storage_blob_entry_t *entry) {
    char data_json[BLOB_CREATED_JSON_MAX_BYTES];
    const int written =
        snprintf(data_json, sizeof(data_json), "{\"id\":\"%" PRIu64 "\",\"sizeBytes\":%zu}",
                 entry->id, entry->stored_bytes);
    if (written < 0 || (size_t)written >= sizeof(data_json)) {
        return send_blob_error(request, request_id, WEB_HTTP_STATUS_INTERNAL_SERVER_ERROR,
                               APP_ERROR_INTERNAL, "blob response encoding failed");
    }
    return send_blob_success_json(request, request_id, BLOB_STATUS_CREATED, data_json);
}

static app_error_code_t authenticate_blob_request(httpd_req_t *request) {
    char session_token[AUTH_TOKEN_HEX_BYTES] = {0};
    const app_error_code_t result = authorize_mutation(request, session_token);
    memset(session_token, 0, sizeof(session_token));
    return result;
}

static app_error_code_t add_blob_list_entry(void *context, const storage_blob_entry_t *entry) {
    blob_list_context_t *list = context;
    if (list == NULL || list->entries == NULL || entry == NULL ||
        list->used_bytes > SIZE_MAX - entry->stored_bytes) {
        return APP_ERROR_INTERNAL;
    }
    char blob_id_text[STORAGE_BLOB_ID_DIGITS + 1U];
    const int written = snprintf(blob_id_text, sizeof(blob_id_text), "%" PRIu64, entry->id);
    cJSON *item = cJSON_CreateObject();
    if (written <= 0 || (size_t)written >= sizeof(blob_id_text) || item == NULL ||
        !cJSON_AddStringToObject(item, "id", blob_id_text) ||
        !cJSON_AddNumberToObject(item, "sizeBytes", (double)entry->stored_bytes) ||
        !cJSON_AddItemToArray(list->entries, item)) {
        cJSON_Delete(item);
        return APP_ERROR_INTERNAL;
    }
    list->used_bytes += entry->stored_bytes;
    return APP_ERROR_NONE;
}

static app_error_code_t build_blob_list_json(char **out_json) {
    *out_json = NULL;
    cJSON *root = cJSON_CreateObject();
    cJSON *entries = cJSON_CreateArray();
    if (root == NULL || entries == NULL) {
        cJSON_Delete(root);
        cJSON_Delete(entries);
        return APP_ERROR_INTERNAL;
    }

    blob_list_context_t list = {.entries = entries, .used_bytes = 0U};
    const storage_blob_scan_observer_t observer = {
        .context = &list,
        .visit_entry = add_blob_list_entry,
        .visit_invalid_name = NULL,
    };
    storage_blob_scan_summary_t summary = {0};
    app_error_code_t result = storage_blob_list(&observer, &summary);
    const int entry_count = cJSON_GetArraySize(entries);
    if (result != APP_ERROR_NONE || entry_count < 0 || summary.valid_count != (size_t)entry_count) {
        cJSON_Delete(root);
        cJSON_Delete(entries);
        return result == APP_ERROR_NONE ? APP_ERROR_INTERNAL : result;
    }

    size_t total_bytes = 0U;
    size_t partition_used_bytes = 0U;
    result =
        storage_partition_capacity(STORAGE_DATA_PARTITION, &total_bytes, &partition_used_bytes);
    if (result != APP_ERROR_NONE || partition_used_bytes > total_bytes) {
        cJSON_Delete(root);
        cJSON_Delete(entries);
        return result == APP_ERROR_NONE ? APP_ERROR_STORAGE_CORRUPT : result;
    }
    const size_t remaining_bytes = total_bytes - partition_used_bytes;
    if (!cJSON_AddNumberToObject(root, "usedBytes", (double)list.used_bytes) ||
        !cJSON_AddNumberToObject(root, "remainingBytes", (double)remaining_bytes) ||
        !cJSON_AddItemToObject(root, "blobs", entries)) {
        cJSON_Delete(root);
        cJSON_Delete(entries);
        return APP_ERROR_INTERNAL;
    }
    entries = NULL;
    *out_json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return *out_json == NULL ? APP_ERROR_INTERNAL : APP_ERROR_NONE;
}

static app_error_code_t abort_uncommitted_upload(storage_blob_upload_t *upload,
                                                 app_error_code_t primary_error) {
    if (upload == NULL || upload->committed) {
        return primary_error;
    }
    const app_error_code_t abort_error = storage_blob_upload_abort(upload);
    return abort_error == APP_ERROR_NONE ? primary_error : abort_error;
}

static int read_blob_adapter(void *context, void *handle, uint8_t *buffer, size_t capacity,
                             size_t *out_count, bool *out_eof) {
    (void)context;
    return storage_blob_reader_read(handle, buffer, capacity, out_count, out_eof) == APP_ERROR_NONE
               ? 0
               : -1;
}

static int send_blob_chunk_adapter(void *context, const uint8_t *buffer, size_t length) {
    httpd_req_t *request = context;
    return httpd_resp_send_chunk(request, (const char *)buffer, (ssize_t)length) == ESP_OK ? 0 : -1;
}

static int close_blob_adapter(void *context, void *handle) {
    (void)context;
    return storage_blob_reader_close(handle) == APP_ERROR_NONE ? 0 : -1;
}

esp_err_t blob_list_handler(httpd_req_t *request) {
    if (request == NULL) {
        return ESP_FAIL;
    }
    char request_id[WEB_API_REQUEST_ID_MAX_BYTES + 1U] = {0};
    app_error_code_t result = establish_request_id(request, request_id, sizeof(request_id));
    if (result != APP_ERROR_NONE) {
        const unsigned int status = result == APP_ERROR_INVALID_ARGUMENT
                                        ? WEB_HTTP_STATUS_BAD_REQUEST
                                        : web_api_http_status_for_error(result);
        return web_api_send_status_error(request, status, result, "invalid request ID");
    }
    result = authenticate_blob_request(request);
    if (result != APP_ERROR_NONE) {
        return send_blob_error(request, request_id, WEB_HTTP_STATUS_UNAUTHORIZED, result,
                               "authentication required");
    }

    char *data_json = NULL;
    result = build_blob_list_json(&data_json);
    if (result != APP_ERROR_NONE) {
        return send_blob_error(request, request_id, blob_error_status(result), result,
                               "blob list unavailable");
    }
    const esp_err_t send_result =
        send_blob_success_json(request, request_id, BLOB_STATUS_OK, data_json);
    cJSON_free(data_json);
    return send_result;
}

esp_err_t blob_create_handler(httpd_req_t *request) {
    if (request == NULL) {
        return ESP_FAIL;
    }

    char request_id[WEB_API_REQUEST_ID_MAX_BYTES + 1U] = {0};
    app_error_code_t result = establish_request_id(request, request_id, sizeof(request_id));
    if (result != APP_ERROR_NONE) {
        const unsigned int status = result == APP_ERROR_INVALID_ARGUMENT
                                        ? WEB_HTTP_STATUS_BAD_REQUEST
                                        : web_api_http_status_for_error(result);
        return web_api_send_status_error(request, status, result, "invalid request ID");
    }

    const size_t content_length = request->content_len;
    if (content_length == 0U) {
        return send_blob_error(request, request_id, WEB_HTTP_STATUS_BAD_REQUEST,
                               APP_ERROR_INVALID_ARGUMENT, "blob body is required");
    }
    if (content_length > (size_t)APP_V2_BLOB_MAX_BYTES) {
        return send_blob_error(request, request_id, WEB_HTTP_STATUS_PAYLOAD_TOO_LARGE,
                               APP_ERROR_INVALID_ARGUMENT, "blob exceeds route limit");
    }

    char content_type[BLOB_CONTENT_TYPE_MAX_BYTES] = {0};
    if (web_server_get_header(request, "Content-Type", content_type, sizeof(content_type)) !=
            APP_ERROR_NONE ||
        !web_api_content_type_is_gzip(content_type)) {
        return send_blob_error(request, request_id, WEB_HTTP_STATUS_UNSUPPORTED_MEDIA_TYPE,
                               APP_ERROR_INVALID_ARGUMENT,
                               "application/gzip content type required");
    }

    result = authenticate_blob_request(request);
    if (result != APP_ERROR_NONE) {
        return send_blob_error(request, request_id, WEB_HTTP_STATUS_UNAUTHORIZED, result,
                               "authentication required");
    }

    storage_blob_upload_t upload = {0};
    result = storage_blob_upload_begin(content_length, &upload);
    if (result != APP_ERROR_NONE) {
        return send_blob_error(request, request_id, blob_error_status(result), result,
                               "blob upload could not start");
    }

    char chunk[STORAGE_BLOB_UPLOAD_CHUNK_BYTES];
    blob_stream_context_t stream_context = {.upload = &upload};
    result = web_adapter_stream_bounded_body(content_length, (size_t)APP_V2_BLOB_MAX_BYTES, chunk,
                                             sizeof(chunk), receive_blob_body, request,
                                             consume_blob_chunk, &stream_context);
    if (result != APP_ERROR_NONE) {
        const app_error_code_t final_error = abort_uncommitted_upload(&upload, result);
        return send_blob_error(request, request_id, blob_error_status(final_error), final_error,
                               final_error == result ? "blob upload failed"
                                                     : "blob upload cleanup failed");
    }

    storage_blob_entry_t entry = {0};
    result = storage_blob_upload_commit(&upload, &entry);
    if (result != APP_ERROR_NONE) {
        const app_error_code_t final_error = abort_uncommitted_upload(&upload, result);
        return send_blob_error(request, request_id, blob_error_status(final_error), final_error,
                               final_error == result ? "blob commit failed"
                                                     : "blob upload cleanup failed");
    }
    return send_blob_created(request, request_id, &entry);
}

esp_err_t blob_load_handler(httpd_req_t *request) {
    if (request == NULL) {
        return ESP_FAIL;
    }
    char request_id[WEB_API_REQUEST_ID_MAX_BYTES + 1U] = {0};
    app_error_code_t result = establish_request_id(request, request_id, sizeof(request_id));
    if (result != APP_ERROR_NONE) {
        const unsigned int status = result == APP_ERROR_INVALID_ARGUMENT
                                        ? WEB_HTTP_STATUS_BAD_REQUEST
                                        : web_api_http_status_for_error(result);
        return web_api_send_status_error(request, status, result, "invalid request ID");
    }
    result = authenticate_blob_request(request);
    if (result != APP_ERROR_NONE) {
        return send_blob_error(request, request_id, WEB_HTTP_STATUS_UNAUTHORIZED, result,
                               "authentication required");
    }
    uint64_t blob_id = 0U;
    result = web_api_parse_blob_id(request->uri, &blob_id);
    if (result != APP_ERROR_NONE) {
        return send_blob_error(request, request_id, WEB_HTTP_STATUS_BAD_REQUEST, result,
                               "invalid blob ID");
    }

    storage_blob_reader_t reader = {.descriptor = -1};
    result = storage_blob_reader_open(blob_id, &reader);
    if (result != APP_ERROR_NONE) {
        return send_blob_error(request, request_id, blob_error_status(result), result,
                               result == APP_ERROR_NOT_FOUND ? "blob not found"
                                                             : "blob unavailable");
    }
    if (httpd_resp_set_type(request, "application/gzip") != ESP_OK ||
        httpd_resp_set_hdr(request, "Cache-Control", "no-store") != ESP_OK ||
        httpd_resp_set_hdr(request, "X-Request-ID", request_id) != ESP_OK) {
        (void)storage_blob_reader_close(&reader);
        return ESP_FAIL;
    }
    const web_adapter_stream_ops_t operations = {
        .context = request,
        .read = read_blob_adapter,
        .send_chunk = send_blob_chunk_adapter,
        .close = close_blob_adapter,
    };
    return web_adapter_stream_file(&reader, &operations) == APP_ERROR_NONE ? ESP_OK : ESP_FAIL;
}

esp_err_t blob_delete_handler(httpd_req_t *request) {
    if (request == NULL) {
        return ESP_FAIL;
    }
    char request_id[WEB_API_REQUEST_ID_MAX_BYTES + 1U] = {0};
    app_error_code_t result = establish_request_id(request, request_id, sizeof(request_id));
    if (result != APP_ERROR_NONE) {
        const unsigned int status = result == APP_ERROR_INVALID_ARGUMENT
                                        ? WEB_HTTP_STATUS_BAD_REQUEST
                                        : web_api_http_status_for_error(result);
        return web_api_send_status_error(request, status, result, "invalid request ID");
    }
    result = authenticate_blob_request(request);
    if (result != APP_ERROR_NONE) {
        return send_blob_error(request, request_id, WEB_HTTP_STATUS_UNAUTHORIZED, result,
                               "authentication required");
    }
    uint64_t blob_id = 0U;
    result = web_api_parse_blob_id(request->uri, &blob_id);
    if (result != APP_ERROR_NONE) {
        return send_blob_error(request, request_id, WEB_HTTP_STATUS_BAD_REQUEST, result,
                               "invalid blob ID");
    }
    result = storage_blob_delete(blob_id);
    if (result != APP_ERROR_NONE) {
        return send_blob_error(request, request_id, blob_error_status(result), result,
                               result == APP_ERROR_NOT_FOUND ? "blob not found"
                                                             : "blob delete failed");
    }
    if (httpd_resp_set_status(request, "204 No Content") != ESP_OK ||
        httpd_resp_set_hdr(request, "Cache-Control", "no-store") != ESP_OK ||
        httpd_resp_set_hdr(request, "X-Request-ID", request_id) != ESP_OK) {
        return ESP_FAIL;
    }
    return httpd_resp_send(request, NULL, 0);
}
