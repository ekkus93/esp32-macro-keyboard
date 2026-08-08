#include "web_status_limits.h"

#include <stdbool.h>
#include <stddef.h>

#include "app_error.h"
#include "cJSON.h"

static bool add_nullable_string(cJSON *object, const char *key, const char *value) {
    if (value == NULL) {
        return cJSON_AddNullToObject(object, key) != NULL;
    }
    return cJSON_AddStringToObject(object, key, value) != NULL;
}

static cJSON *build_usb(const web_status_snapshot_t *snapshot) {
    cJSON *usb = cJSON_CreateObject();
    if (usb == NULL || !cJSON_AddStringToObject(usb, "state", snapshot->usb_state)) {
        cJSON_Delete(usb);
        return NULL;
    }
    return usb;
}

static cJSON *build_access_point(const web_status_snapshot_t *snapshot) {
    cJSON *access_point = cJSON_CreateObject();
    if (access_point == NULL ||
        !cJSON_AddStringToObject(access_point, "state", snapshot->access_point_state) ||
        !cJSON_AddStringToObject(access_point, "ssid", snapshot->access_point_ssid) ||
        !cJSON_AddNumberToObject(access_point, "clientCount",
                                 (double)snapshot->access_point_client_count)) {
        cJSON_Delete(access_point);
        return NULL;
    }
    return access_point;
}

static cJSON *build_station(const web_status_snapshot_t *snapshot) {
    cJSON *station = cJSON_CreateObject();
    if (station == NULL ||
        !cJSON_AddBoolToObject(station, "configured", snapshot->station_configured) ||
        !cJSON_AddStringToObject(station, "state", snapshot->station_state) ||
        !add_nullable_string(station, "ssid", snapshot->station_ssid) ||
        !add_nullable_string(station, "ipv4", snapshot->station_ipv4)) {
        cJSON_Delete(station);
        return NULL;
    }
    return station;
}

static cJSON *build_storage(const web_status_snapshot_t *snapshot) {
    cJSON *storage = cJSON_CreateObject();
    if (storage == NULL || !cJSON_AddStringToObject(storage, "state", snapshot->storage_state) ||
        !cJSON_AddNumberToObject(storage, "totalBytes", (double)snapshot->storage_total_bytes) ||
        !cJSON_AddNumberToObject(storage, "usedBytes", (double)snapshot->storage_used_bytes) ||
        !cJSON_AddNumberToObject(storage, "remainingBytes",
                                 (double)snapshot->storage_remaining_bytes) ||
        !cJSON_AddNumberToObject(storage, "blobCount", (double)snapshot->storage_blob_count)) {
        cJSON_Delete(storage);
        return NULL;
    }
    return storage;
}

static cJSON *build_send(const web_status_snapshot_t *snapshot) {
    cJSON *send = cJSON_CreateObject();
    if (send == NULL || !cJSON_AddBoolToObject(send, "present", snapshot->send_present) ||
        !add_nullable_string(send, "state", snapshot->send_present ? snapshot->send_state : NULL)) {
        cJSON_Delete(send);
        return NULL;
    }
    return send;
}

app_error_code_t web_status_response_json(const web_status_snapshot_t *snapshot, char **out_json) {
    if (out_json != NULL) {
        *out_json = NULL;
    }
    if (snapshot == NULL || out_json == NULL || snapshot->device_name == NULL ||
        snapshot->firmware_version == NULL || snapshot->build_id == NULL ||
        snapshot->usb_state == NULL || snapshot->access_point_state == NULL ||
        snapshot->access_point_ssid == NULL || snapshot->station_state == NULL ||
        snapshot->storage_state == NULL ||
        (snapshot->send_present && snapshot->send_state == NULL)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    /* Each sub-object is created and attached one at a time (rather than all
     * up front) so that on any failure, cJSON_Delete(root) already owns every
     * previously-attached child and the not-yet-attached one is freed
     * separately -- never both, which would double-free. */
    cJSON *root = cJSON_CreateObject();
    if (root == NULL || !cJSON_AddBoolToObject(root, "provisioned", snapshot->provisioned) ||
        !cJSON_AddStringToObject(root, "deviceName", snapshot->device_name) ||
        !cJSON_AddStringToObject(root, "firmwareVersion", snapshot->firmware_version) ||
        !cJSON_AddStringToObject(root, "buildId", snapshot->build_id) ||
        !cJSON_AddNumberToObject(root, "uptimeMs", (double)snapshot->uptime_ms)) {
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    cJSON *usb = build_usb(snapshot);
    if (usb == NULL || !cJSON_AddItemToObject(root, "usb", usb)) {
        cJSON_Delete(usb);
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    cJSON *access_point = build_access_point(snapshot);
    if (access_point == NULL || !cJSON_AddItemToObject(root, "accessPoint", access_point)) {
        cJSON_Delete(access_point);
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    cJSON *station = build_station(snapshot);
    if (station == NULL || !cJSON_AddItemToObject(root, "station", station)) {
        cJSON_Delete(station);
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    cJSON *storage = build_storage(snapshot);
    if (storage == NULL || !cJSON_AddItemToObject(root, "storage", storage)) {
        cJSON_Delete(storage);
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    cJSON *send = build_send(snapshot);
    if (send == NULL || !cJSON_AddItemToObject(root, "send", send)) {
        cJSON_Delete(send);
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }

    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (json == NULL) {
        return APP_ERROR_INTERNAL;
    }
    *out_json = json;
    return APP_ERROR_NONE;
}
