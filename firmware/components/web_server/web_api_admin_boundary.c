#include "web_api_admin_boundary.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "app_error.h"
#include "cJSON.h"
#include "macro_limits.h"
#include "storage.h"
#include "storage_incidents.h"
#include "storage_package.h"
#include "storage_repository.h"
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

/* SPEC 10.7: the client is told how many bytes remain so it can stop the user
 * before a write is attempted, rather than discovering the budget through a 507.
 * usedBytes is measured from the set files themselves, not inferred from the
 * per-object limits. */
static app_error_code_t send_storage_snapshot(web_api_response_t *response) {
    const storage_mount_state_t mounts = storage_mount_state();
    size_t used_bytes = 0U;
    const app_error_code_t measured = mounts.data_mounted
                                          ? storage_repository_measure_user_data(NULL, &used_bytes)
                                          : APP_ERROR_NONE;
    if (measured != APP_ERROR_NONE) {
        return respond_operation_error(response, measured, "could not measure storage use");
    }
    const size_t remaining =
        used_bytes >= APP_USER_DATA_MAX_BYTES ? 0U : APP_USER_DATA_MAX_BYTES - used_bytes;

    storage_incident_report_t incidents = {0};
    storage_incidents_snapshot(&incidents);

    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return APP_ERROR_INTERNAL;
    }
    app_error_code_t result = APP_ERROR_NONE;
    if (cJSON_AddBoolToObject(root, "verified", false) == NULL ||
        cJSON_AddBoolToObject(root, "webMounted", mounts.web_mounted) == NULL ||
        cJSON_AddBoolToObject(root, "dataMounted", mounts.data_mounted) == NULL ||
        cJSON_AddNumberToObject(root, "usedBytes", (double)used_bytes) == NULL ||
        cJSON_AddNumberToObject(root, "totalBytes", (double)APP_USER_DATA_MAX_BYTES) == NULL ||
        cJSON_AddNumberToObject(root, "remainingBytes", (double)remaining) == NULL ||
        cJSON_AddNumberToObject(root, "setFileMaxBytes", (double)APP_SET_FILE_MAX_BYTES) == NULL ||
        /* SPEC 20.3: stray temporaries removed at boot, and objects discarded as
         * corrupt since boot with their paths and errors. */
        cJSON_AddNumberToObject(root, "temporariesRemovedAtBoot",
                                (double)incidents.temporaries_removed) == NULL ||
        cJSON_AddNumberToObject(root, "discardedObjectCount", (double)incidents.total) == NULL) {
        result = APP_ERROR_INTERNAL;
    }
    cJSON *discarded = cJSON_CreateArray();
    if (result == APP_ERROR_NONE &&
        (discarded == NULL || !cJSON_AddItemToObject(root, "discardedObjects", discarded))) {
        cJSON_Delete(discarded);
        result = APP_ERROR_INTERNAL;
    }
    for (size_t index = 0U; result == APP_ERROR_NONE && index < incidents.count; ++index) {
        cJSON *entry = cJSON_CreateObject();
        if (entry == NULL || !cJSON_AddItemToArray(discarded, entry) ||
            cJSON_AddStringToObject(entry, "path", incidents.items[index].path) == NULL ||
            cJSON_AddStringToObject(entry, "error",
                                    app_error_code_string(incidents.items[index].error)) == NULL) {
            cJSON_Delete(entry);
            result = APP_ERROR_INTERNAL;
        }
    }
    char *json = NULL;
    if (result == APP_ERROR_NONE) {
        json = cJSON_PrintUnformatted(root);
        result = json == NULL ? APP_ERROR_INTERNAL : APP_ERROR_NONE;
    }
    cJSON_Delete(root);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    const app_error_code_t sent = web_api_response_success(response, WEB_HTTP_STATUS_OK, json);
    cJSON_free(json);
    return sent;
}

static const char *backup_object_kind_name(storage_package_object_kind_t kind) {
    switch (kind) {
    case STORAGE_PACKAGE_OBJECT_SET:
        return "set";
    case STORAGE_PACKAGE_OBJECT_MACRO:
        return "macro";
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
    } else if (failure->has_object_id && failure->has_package_id) {
        written =
            snprintf(buffer, buffer_size, "backup unavailable: %s %s in set %s could not be read",
                     kind, failure->object_id.value, failure->set_id.value);
    } else if (failure->has_object_id) {
        written = snprintf(buffer, buffer_size, "backup unavailable: %s %s could not be read", kind,
                           failure->object_id.value);
    } else if (failure->has_package_id) {
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
        storage_package_export_backup_detail(&package_json, &package_length, &failure, NULL);
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

/* Renders the per-set outcomes of a restore (SPEC 13.5, 17). Included on both
 * success and failure, because "which sets are on the device now" is exactly
 * what the client needs to know either way. */
static app_error_code_t restore_outcomes_json(const storage_restore_report_t *report,
                                              cJSON *parent) {
    cJSON *sets = cJSON_CreateArray();
    if (sets == NULL || !cJSON_AddItemToObject(parent, "sets", sets)) {
        cJSON_Delete(sets);
        return APP_ERROR_INTERNAL;
    }
    for (size_t index = 0U; index < report->count; ++index) {
        cJSON *entry = cJSON_CreateObject();
        if (entry == NULL || !cJSON_AddItemToArray(sets, entry)) {
            cJSON_Delete(entry);
            return APP_ERROR_INTERNAL;
        }
        const bool restored = report->items[index].result == APP_ERROR_NONE;
        if (cJSON_AddStringToObject(entry, "setId", report->items[index].set_id.value) == NULL ||
            cJSON_AddBoolToObject(entry, "restored", restored) == NULL) {
            return APP_ERROR_INTERNAL;
        }
        if (!restored &&
            cJSON_AddStringToObject(entry, "error",
                                    app_error_code_string(report->items[index].result)) == NULL) {
            return APP_ERROR_INTERNAL;
        }
    }
    return APP_ERROR_NONE;
}

static app_error_code_t restore_response_json(const storage_restore_report_t *report, bool complete,
                                              char **out_json) {
    *out_json = NULL;
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return APP_ERROR_INTERNAL;
    }
    app_error_code_t result = APP_ERROR_NONE;
    if (cJSON_AddBoolToObject(root, "restored", complete) == NULL ||
        cJSON_AddBoolToObject(root, "reloadRequired", true) == NULL ||
        cJSON_AddNumberToObject(root, "setsRestored", (double)report->written) == NULL ||
        cJSON_AddNumberToObject(root, "setsFailed", (double)report->failed) == NULL) {
        result = APP_ERROR_INTERNAL;
    }
    if (result == APP_ERROR_NONE) {
        result = restore_outcomes_json(report, root);
    }
    char *json = NULL;
    if (result == APP_ERROR_NONE) {
        json = cJSON_PrintUnformatted(root);
        result = json == NULL ? APP_ERROR_INTERNAL : APP_ERROR_NONE;
    }
    cJSON_Delete(root);
    *out_json = json;
    return result;
}

static app_error_code_t restore_backup(const web_api_call_t *call, web_api_response_t *response) {
    storage_restore_report_t report = {0};
    const app_error_code_t result =
        storage_package_restore_backup(call->body, call->body_length, &report);
    /* A restore that never reached the write loop -- a malformed package, a lock
     * failure -- has no per-set outcomes to report, so it is an ordinary error. */
    if (result != APP_ERROR_NONE && report.count == 0U) {
        return respond_operation_error(response, result, "restore failed");
    }
    char *json = NULL;
    const app_error_code_t rendered =
        restore_response_json(&report, result == APP_ERROR_NONE, &json);
    if (rendered != APP_ERROR_NONE) {
        return respond_operation_error(response, rendered, "restore failed");
    }
    app_error_code_t sent = APP_ERROR_NONE;
    if (result == APP_ERROR_NONE) {
        sent = web_api_response_success(response, WEB_HTTP_STATUS_OK, json);
    } else {
        /* A partial restore MUST NOT be a 200 (SPEC 17), so it goes out through
         * the error envelope -- but the per-set outcomes travel with it as
         * details, because "which sets are on the device now" is precisely what
         * the client needs after a failure. The status comes from the first
         * per-set failure, so storage exhaustion still reads as 507 rather than
         * being flattened into a generic 500. */
        const web_api_error_spec_t spec = {
            .status = web_api_http_status_for_error(result),
            .code = result,
            .message = "restore did not write every set",
            .details_json = json,
        };
        sent = web_api_response_error(response, &spec);
    }
    cJSON_free(json);
    return sent;
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
