#include "web_api_admin_boundary.h"

#include <stddef.h>
#include <stdio.h>

#include "app_error.h"
#include "storage.h"
#include "web_api_core.h"
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

static app_error_code_t send_storage_snapshot(web_api_response_t *response) {
    const storage_mount_state_t mounts = storage_mount_state();
    storage_quarantine_list_t quarantine = {0};
    const app_error_code_t result = storage_quarantine_list(&quarantine);
    if (result != APP_ERROR_NONE) {
        return respond_error(response, web_api_http_status_for_error(result), result,
                             "storage health unavailable");
    }
    char data[WEB_ADMIN_STORAGE_HEALTH_RESPONSE_BYTES];
    const int length =
        snprintf(data, sizeof(data),
                 "{\"verified\":false,\"webMounted\":%s,\"dataMounted\":%s,"
                 "\"quarantineCount\":%lu,\"damagedQuarantineCount\":%lu}",
                 mounts.web_mounted ? "true" : "false",
                 mounts.data_mounted ? "true" : "false", (unsigned long)quarantine.count,
                 (unsigned long)quarantine.damaged_count);
    return length < 0 || (size_t)length >= sizeof(data)
               ? APP_ERROR_INTERNAL
               : web_api_response_success(response, WEB_HTTP_STATUS_OK, data);
}

static app_error_code_t unavailable(web_api_response_t *response, const char *message) {
    return respond_error(response, WEB_HTTP_STATUS_SERVICE_UNAVAILABLE,
                         APP_ERROR_STORAGE_UNAVAILABLE, message);
}

app_error_code_t web_api_admin_boundary_handle(web_api_route_t route,
                                               web_api_response_t *response) {
    if (response == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    switch (route) {
    case WEB_API_ROUTE_DIAGNOSTICS_STORAGE:
        return send_storage_snapshot(response);
    case WEB_API_ROUTE_DIAGNOSTICS_STORAGE_CHECK:
        return unavailable(response,
                           "storage verification requires the Phase 19 diagnostics service");
    case WEB_API_ROUTE_SET_EXPORT:
    case WEB_API_ROUTE_SET_IMPORT:
    case WEB_API_ROUTE_BACKUP:
    case WEB_API_ROUTE_RESTORE:
        return unavailable(response, "package operation requires the Phase 18 transaction service");
    default:
        return APP_ERROR_NOT_FOUND;
    }
}
