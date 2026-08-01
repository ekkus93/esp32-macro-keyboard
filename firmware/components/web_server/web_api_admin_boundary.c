#include "web_api_admin_boundary.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "app_error.h"
#include "storage.h"
#include "storage_package.h"
#include "web_api_core.h"
#include "web_api_handlers.h"
#include "web_api_response.h"
#include "web_http_status.h"

#define WEB_ADMIN_STORAGE_HEALTH_RESPONSE_BYTES 256U

static app_error_code_t respond_error(web_api_response_t *response, unsigned int status,
                                      app_error_code_t error, const char *message) {
    return web_api_response_error(response, &(web_api_error_spec_t){
                                                .status = status,
                                                .code = error,
                                                .message = message,
                                            });
}

static app_error_code_t respond_operation_error(web_api_response_t *response,
                                                app_error_code_t result, const char *message) {
    return respond_error(response, web_api_http_status_for_error(result), result, message);
}

static app_error_code_t send_storage_snapshot(web_api_response_t *response) {
    const storage_mount_state_t mounts = storage_mount_state();
    /* Heap-allocated, not a stack local: storage_quarantine_list_t is far
     * larger than the httpd task stack (see web_server_lifecycle.c), so a
     * stack local here overflows it and panics the device. */
    storage_quarantine_list_t *quarantine = calloc(1U, sizeof(*quarantine));
    if (quarantine == NULL) {
        return respond_error(response, web_api_http_status_for_error(APP_ERROR_INTERNAL),
                             APP_ERROR_INTERNAL, "storage health unavailable");
    }
    const app_error_code_t result = storage_quarantine_list(quarantine);
    if (result != APP_ERROR_NONE) {
        free(quarantine);
        return respond_error(response, web_api_http_status_for_error(result), result,
                             "storage health unavailable");
    }
    /* Copy the two scalars out and release the large allocation before
     * building the response, so no path can leak it. */
    const unsigned long quarantine_count = (unsigned long)quarantine->count;
    const unsigned long damaged_count = (unsigned long)quarantine->damaged_count;
    free(quarantine);

    char data[WEB_ADMIN_STORAGE_HEALTH_RESPONSE_BYTES];
    const int length =
        snprintf(data, sizeof(data),
                 "{\"verified\":false,\"webMounted\":%s,\"dataMounted\":%s,"
                 "\"quarantineCount\":%lu,\"damagedQuarantineCount\":%lu}",
                 mounts.web_mounted ? "true" : "false", mounts.data_mounted ? "true" : "false",
                 quarantine_count, damaged_count);
    return length < 0 || (size_t)length >= sizeof(data)
               ? APP_ERROR_INTERNAL
               : web_api_response_success(response, WEB_HTTP_STATUS_OK, data);
}

static app_error_code_t send_backup(web_api_response_t *response) {
    char *package_json = NULL;
    size_t package_length = 0U;
    app_error_code_t result = storage_package_export_backup(true, &package_json, &package_length);
    if (result != APP_ERROR_NONE) {
        return respond_operation_error(response, result, "backup unavailable");
    }
    result = web_api_response_take_json(response, WEB_HTTP_STATUS_OK, package_json, package_length);
    if (result != APP_ERROR_NONE) {
        storage_package_free(package_json);
    }
    return result;
}

static app_error_code_t restore_backup(const web_api_call_t *call, web_api_response_t *response) {
    const app_error_code_t result = storage_package_restore_backup(call->body, call->body_length);
    return result == APP_ERROR_NONE
               ? web_api_response_success(response, WEB_HTTP_STATUS_OK,
                                          "{\"restored\":true,\"reloadRequired\":true}")
               : respond_operation_error(response, result, "restore failed");
}

static app_error_code_t unavailable(web_api_response_t *response, const char *message) {
    return respond_error(response, WEB_HTTP_STATUS_SERVICE_UNAVAILABLE,
                         APP_ERROR_STORAGE_UNAVAILABLE, message);
}

app_error_code_t web_api_admin_boundary_handle(const web_api_call_t *call,
                                               web_api_response_t *response) {
    if (call == NULL || response == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    switch (call->path.route) {
    case WEB_API_ROUTE_DIAGNOSTICS_STORAGE:
        return send_storage_snapshot(response);
    case WEB_API_ROUTE_DIAGNOSTICS_STORAGE_CHECK:
        return unavailable(response,
                           "storage verification requires the Phase 19 diagnostics service");
    case WEB_API_ROUTE_BACKUP:
        return send_backup(response);
    case WEB_API_ROUTE_RESTORE:
        return restore_backup(call, response);
    case WEB_API_ROUTE_SET_EXPORT:
    case WEB_API_ROUTE_SET_IMPORT:
        return unavailable(response, "package operation requires the Phase 18 transaction service");
    default:
        return APP_ERROR_NOT_FOUND;
    }
}
