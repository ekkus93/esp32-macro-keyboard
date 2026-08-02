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
/* Fits the longest message below with two full UUIDs expanded. */
#define WEB_ADMIN_BACKUP_FAILURE_MESSAGE_BYTES 160U

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
    char data[WEB_ADMIN_STORAGE_HEALTH_RESPONSE_BYTES];
    const int length =
        snprintf(data, sizeof(data), "{\"verified\":false,\"webMounted\":%s,\"dataMounted\":%s}",
                 mounts.web_mounted ? "true" : "false", mounts.data_mounted ? "true" : "false");
    return length < 0 || (size_t)length >= sizeof(data)
               ? APP_ERROR_INTERNAL
               : web_api_response_success(response, WEB_HTTP_STATUS_OK, data);
}

static const char *backup_object_kind_name(storage_package_object_kind_t kind) {
    switch (kind) {
    case STORAGE_PACKAGE_OBJECT_SET:
        return "set";
    case STORAGE_PACKAGE_OBJECT_MACRO:
        return "macro";
    case STORAGE_PACKAGE_OBJECT_PROCEDURE:
        return "procedure";
    case STORAGE_PACKAGE_OBJECT_PROGRESS:
        return "procedure progress";
    case STORAGE_PACKAGE_OBJECT_NONE:
    default:
        return "object";
    }
}

/* A backup aborts on the first unreadable object. Reporting only that it failed
 * left no way to find the offending object, so name it whenever the export
 * could identify it. */
static void describe_backup_failure(const storage_package_failure_t *failure, char *buffer,
                                    size_t buffer_size) {
    const char *kind = backup_object_kind_name(failure->kind);
    int written = 0;
    if (failure->kind == STORAGE_PACKAGE_OBJECT_NONE) {
        written = snprintf(buffer, buffer_size, "backup unavailable");
    } else if (failure->has_object_id && failure->has_set_id) {
        written =
            snprintf(buffer, buffer_size, "backup unavailable: %s %s in set %s could not be read",
                     kind, failure->object_id.value, failure->set_id.value);
    } else if (failure->has_object_id) {
        written = snprintf(buffer, buffer_size, "backup unavailable: %s %s could not be read", kind,
                           failure->object_id.value);
    } else if (failure->has_set_id) {
        written =
            snprintf(buffer, buffer_size, "backup unavailable: a %s in set %s could not be read",
                     kind, failure->set_id.value);
    } else {
        written = snprintf(buffer, buffer_size, "backup unavailable: a %s could not be read", kind);
    }
    if (written < 0 || (size_t)written >= buffer_size) {
        /* Never report a truncated identifier: a partial UUID would send the
         * user looking for an object that does not exist. */
        (void)snprintf(buffer, buffer_size, "backup unavailable");
    }
}

static app_error_code_t send_backup(web_api_response_t *response) {
    char *package_json = NULL;
    size_t package_length = 0U;
    storage_package_failure_t failure = {0};
    app_error_code_t result =
        storage_package_export_backup_detail(true, &package_json, &package_length, &failure, NULL);
    if (result != APP_ERROR_NONE) {
        char message[WEB_ADMIN_BACKUP_FAILURE_MESSAGE_BYTES];
        describe_backup_failure(&failure, message, sizeof(message));
        return respond_operation_error(response, result, message);
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
