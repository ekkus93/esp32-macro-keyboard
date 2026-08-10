#include "web_settings.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "api_contracts_v2.h"
#include "app_error.h"
#include "cJSON.h"
#include "device_settings_v2.h"
#include "settings_contract_v2.h"
#include "setup_contract_v2.h"

#define SETTINGS_NUL_ESCAPE "\\u0000"
#define SETTINGS_SEND_MODE_QUICK "quick"
#define SETTINGS_SEND_MODE_PREVIEW "preview"
/* Raw wire-shape ceiling for snapshotRetentionTarget: the widest value that
 * still fits the field's uint8_t storage. app_v2_settings_prepare_update()
 * separately enforces the narrower SPEC_V2 0-100 semantic range, mapped to
 * WEB_SETTINGS_PUT_INVALID_SNAPSHOT_RETENTION_TARGET. */
#define SETTINGS_RETENTION_RAW_MAX 255.0

static void secure_zero_local(void *memory, size_t length) {
    volatile uint8_t *bytes = memory;
    for (size_t index = 0U; index < length; ++index) {
        bytes[index] = 0U;
    }
}

/* cJSON owns separate heap copies of parsed strings. Flatten child lists into
 * the sibling chain while wiping every parsed key and string value so no
 * secret survives in allocator-owned memory when cJSON_Delete() frees it. */
static void wipe_json_tree(cJSON *item) {
    for (cJSON *current = item; current != NULL; current = current->next) {
        if (current->string != NULL) {
            secure_zero_local(current->string, strlen(current->string) + 1U);
        }
        if (cJSON_IsString(current) && current->valuestring != NULL) {
            secure_zero_local(current->valuestring, strlen(current->valuestring) + 1U);
        }
        if (current->child != NULL) {
            cJSON *child = current->child;
            cJSON *child_tail = child;
            while (child_tail->next != NULL) {
                child_tail = child_tail->next;
            }
            child_tail->next = current->next;
            if (current->next != NULL) {
                current->next->prev = child_tail;
            }
            current->next = child;
            child->prev = current;
            current->child = NULL;
        }
    }
}

static void wipe_and_delete_json_tree(cJSON *root) {
    if (root != NULL) {
        wipe_json_tree(root);
    }
    cJSON_Delete(root);
}

static bool ops_valid_common(const web_settings_ops_t *ops) {
    return ops != NULL && ops->settings_read != NULL && ops->settings_replace != NULL;
}

static bool bounded_body_length(const char *body, size_t capacity, size_t *out_length) {
    if (body == NULL || out_length == NULL || capacity == 0U) {
        return false;
    }
    for (size_t index = 0U; index < capacity; ++index) {
        if (body[index] == '\0') {
            *out_length = index;
            return index > 0U;
        }
    }
    return false;
}

static bool contains_embedded_nul_escape(const char *body, size_t length) {
    const size_t escape_length = sizeof(SETTINGS_NUL_ESCAPE) - 1U;
    if (length < escape_length) {
        return false;
    }
    for (size_t index = 0U; index <= length - escape_length; ++index) {
        if (memcmp(body + index, SETTINGS_NUL_ESCAPE, escape_length) == 0) {
            return true;
        }
    }
    return false;
}

static cJSON *parse_exact_body(char *body, size_t body_capacity) {
    size_t body_length = 0U;
    if (!bounded_body_length(body, body_capacity, &body_length) ||
        contains_embedded_nul_escape(body, body_length)) {
        secure_zero_local(body, body_capacity);
        return NULL;
    }
    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts(body, body_length + 1U, &parse_end, true);
    secure_zero_local(body, body_capacity);
    if (root == NULL || parse_end != body + body_length || !cJSON_IsObject(root)) {
        wipe_and_delete_json_tree(root);
        return NULL;
    }
    return root;
}

/* -------------------------------------------------------------------------
 * Response JSON (GET, and the "settings" object nested inside PUT's
 * response).
 * ---------------------------------------------------------------------- */

static bool add_optional_string(cJSON *root, const char *key, app_v2_optional_string_view_t value) {
    if (!value.present) {
        return cJSON_AddNullToObject(root, key) != NULL;
    }
    char buffer[APP_V2_UUID_TEXT_BYTES + 1U > APP_V2_WIFI_SSID_MAX_BYTES + 1U
                    ? APP_V2_UUID_TEXT_BYTES + 1U
                    : APP_V2_WIFI_SSID_MAX_BYTES + 1U];
    if (value.value.length >= sizeof(buffer)) {
        return false;
    }
    memcpy(buffer, value.value.data, value.value.length);
    buffer[value.value.length] = '\0';
    return cJSON_AddStringToObject(root, key, buffer) != NULL;
}

static bool add_view_string(cJSON *root, const char *key, app_v2_string_view_t value) {
    char buffer[APP_V2_DEVICE_NAME_MAX_BYTES + 1U];
    if (value.data == NULL || value.length >= sizeof(buffer)) {
        return false;
    }
    memcpy(buffer, value.data, value.length);
    buffer[value.length] = '\0';
    return cJSON_AddStringToObject(root, key, buffer) != NULL;
}

static const char *send_mode_string(app_v2_send_mode_t mode) {
    return mode == APP_V2_SEND_MODE_PREVIEW ? SETTINGS_SEND_MODE_PREVIEW : SETTINGS_SEND_MODE_QUICK;
}

static bool settings_response_populate(cJSON *root, const app_v2_settings_response_t *response) {
    return add_view_string(root, "deviceName", response->device_name) &&
           cJSON_AddBoolToObject(root, "requireSerialConfirmation",
                                 response->require_serial_confirmation) != NULL &&
           cJSON_AddStringToObject(root, "sendMode", send_mode_string(response->send_mode)) !=
               NULL &&
           cJSON_AddNumberToObject(root, "snapshotRetentionTarget",
                                   (double)response->snapshot_retention_target) != NULL &&
           cJSON_AddBoolToObject(root, "showMacroSourcePreviews",
                                 response->show_macro_source_previews) != NULL &&
           add_optional_string(root, "lastSelectedPackageId", response->last_selected_package_id) &&
           add_view_string(root, "apSsid", response->ap_ssid) &&
           cJSON_AddBoolToObject(root, "stationConfigured", response->station_configured) != NULL &&
           add_optional_string(root, "stationSsid", response->station_ssid);
}

static char *finish_json(cJSON *root) {
    if (root == NULL) {
        return NULL;
    }
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

/* -------------------------------------------------------------------------
 * GET /api/v1/settings
 * ---------------------------------------------------------------------- */

web_settings_get_outcome_t web_settings_get_handle(const web_settings_ops_t *ops, char **out_json) {
    if (out_json != NULL) {
        *out_json = NULL;
    }
    if (!ops_valid_common(ops) || out_json == NULL) {
        return (web_settings_get_outcome_t){.result = WEB_SETTINGS_GET_INTERNAL};
    }

    app_v2_device_settings_t settings = {0};
    const app_error_code_t read_result = ops->settings_read(ops->context, &settings);
    if (read_result != APP_ERROR_NONE) {
        return (web_settings_get_outcome_t){
            .result = WEB_SETTINGS_GET_BACKEND_UNAVAILABLE,
            .detail = read_result,
        };
    }

    app_v2_settings_response_t response = {0};
    if (app_v2_settings_response_from_settings(&settings, &response) != APP_V2_SETTINGS_UPDATE_OK) {
        return (web_settings_get_outcome_t){.result = WEB_SETTINGS_GET_INTERNAL};
    }

    cJSON *root = cJSON_CreateObject();
    if (root == NULL || !settings_response_populate(root, &response)) {
        cJSON_Delete(root);
        return (web_settings_get_outcome_t){.result = WEB_SETTINGS_GET_INTERNAL};
    }
    char *json = finish_json(root);
    if (json == NULL) {
        return (web_settings_get_outcome_t){.result = WEB_SETTINGS_GET_INTERNAL};
    }
    *out_json = json;
    return (web_settings_get_outcome_t){.result = WEB_SETTINGS_GET_OK};
}

/* -------------------------------------------------------------------------
 * PUT /api/v1/settings
 * ---------------------------------------------------------------------- */

#define SETTINGS_PUT_FIELD_COUNT 8U

static const char *const SETTINGS_PUT_FIELDS[SETTINGS_PUT_FIELD_COUNT] = {
    "deviceName",
    "requireSerialConfirmation",
    "sendMode",
    "snapshotRetentionTarget",
    "showMacroSourcePreviews",
    "lastSelectedPackageId",
    "accessPoint",
    "station",
};

static bool exact_settings_put_fields(const cJSON *root) {
    bool seen[SETTINGS_PUT_FIELD_COUNT] = {false};
    for (const cJSON *item = root->child; item != NULL; item = item->next) {
        if (item->string == NULL) {
            return false;
        }
        bool matched = false;
        for (size_t index = 0U; index < SETTINGS_PUT_FIELD_COUNT; ++index) {
            if (strcmp(item->string, SETTINGS_PUT_FIELDS[index]) == 0) {
                if (seen[index]) {
                    return false;
                }
                seen[index] = true;
                matched = true;
                break;
            }
        }
        if (!matched) {
            return false;
        }
    }
    return true;
}

static bool string_view_from_item(const cJSON *item, app_v2_string_view_t *out_view) {
    if (item == NULL || !cJSON_IsString(item) || item->valuestring == NULL) {
        return false;
    }
    out_view->data = item->valuestring;
    out_view->length = strlen(item->valuestring);
    return true;
}

static bool exact_credential_fields(const cJSON *object, app_v2_network_credentials_t *out) {
    if (!cJSON_IsObject(object)) {
        return false;
    }
    const cJSON *ssid = NULL;
    const cJSON *passphrase = NULL;
    size_t field_count = 0U;
    for (const cJSON *item = object->child; item != NULL; item = item->next) {
        if (item->string == NULL) {
            return false;
        }
        if (strcmp(item->string, "ssid") == 0 && ssid == NULL) {
            ssid = item;
        } else if (strcmp(item->string, "passphrase") == 0 && passphrase == NULL) {
            passphrase = item;
        } else {
            return false;
        }
        ++field_count;
    }
    return field_count == 2U && string_view_from_item(ssid, &out->ssid) &&
           string_view_from_item(passphrase, &out->passphrase);
}

static bool populate_device_name(const cJSON *root, app_v2_settings_update_request_t *out_request) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, "deviceName");
    if (item == NULL) {
        return true;
    }
    out_request->has_device_name = true;
    return string_view_from_item(item, &out_request->device_name);
}

static bool populate_require_confirmation(const cJSON *root,
                                          app_v2_settings_update_request_t *out_request) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, "requireSerialConfirmation");
    if (item == NULL) {
        return true;
    }
    if (!cJSON_IsBool(item)) {
        return false;
    }
    out_request->has_require_serial_confirmation = true;
    out_request->require_serial_confirmation = cJSON_IsTrue(item);
    return true;
}

/* Structurally-a-string-but-unrecognized ("sendMode":"eventually") is not a
 * malformed-body condition: it leaves has_send_mode false and reports itself
 * through *out_invalid_enum instead of a false return, so the caller can
 * answer the dedicated WEB_SETTINGS_PUT_INVALID_SEND_MODE result (matching
 * accessPoint/station's field-specific errors) rather than a generic
 * invalid-body one -- and so no out-of-range app_v2_send_mode_t value is
 * ever constructed. */
static bool populate_send_mode(const cJSON *root, app_v2_settings_update_request_t *out_request,
                               bool *out_invalid_enum) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, "sendMode");
    if (item == NULL) {
        return true;
    }
    if (!cJSON_IsString(item) || item->valuestring == NULL) {
        return false;
    }
    if (strcmp(item->valuestring, SETTINGS_SEND_MODE_QUICK) == 0) {
        out_request->has_send_mode = true;
        out_request->send_mode = APP_V2_SEND_MODE_QUICK;
    } else if (strcmp(item->valuestring, SETTINGS_SEND_MODE_PREVIEW) == 0) {
        out_request->has_send_mode = true;
        out_request->send_mode = APP_V2_SEND_MODE_PREVIEW;
    } else {
        *out_invalid_enum = true;
    }
    return true;
}

static bool populate_snapshot_retention_target(const cJSON *root,
                                               app_v2_settings_update_request_t *out_request) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, "snapshotRetentionTarget");
    if (item == NULL) {
        return true;
    }
    if (!cJSON_IsNumber(item) || item->valuedouble < 0.0 ||
        item->valuedouble > SETTINGS_RETENTION_RAW_MAX) {
        return false;
    }
    const uint8_t candidate = (uint8_t)item->valuedouble;
    if ((double)candidate != item->valuedouble) {
        return false;
    }
    out_request->has_snapshot_retention_target = true;
    out_request->snapshot_retention_target = candidate;
    return true;
}

static bool populate_show_previews(const cJSON *root,
                                   app_v2_settings_update_request_t *out_request) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, "showMacroSourcePreviews");
    if (item == NULL) {
        return true;
    }
    if (!cJSON_IsBool(item)) {
        return false;
    }
    out_request->has_show_macro_source_previews = true;
    out_request->show_macro_source_previews = cJSON_IsTrue(item);
    return true;
}

static bool populate_last_selected_package_id(const cJSON *root,
                                              app_v2_settings_update_request_t *out_request) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, "lastSelectedPackageId");
    if (item == NULL) {
        return true;
    }
    out_request->has_last_selected_package_id = true;
    if (cJSON_IsNull(item)) {
        out_request->last_selected_package_id.present = false;
        return true;
    }
    if (!cJSON_IsString(item) || item->valuestring == NULL) {
        return false;
    }
    out_request->last_selected_package_id.present = true;
    out_request->last_selected_package_id.value = (app_v2_string_view_t){
        .data = item->valuestring,
        .length = strlen(item->valuestring),
    };
    return true;
}

static bool populate_access_point(const cJSON *root,
                                  app_v2_settings_update_request_t *out_request) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, "accessPoint");
    if (item == NULL) {
        return true;
    }
    if (!exact_credential_fields(item, &out_request->access_point)) {
        return false;
    }
    out_request->has_access_point = true;
    return true;
}

static bool populate_station(const cJSON *root, app_v2_settings_update_request_t *out_request) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, "station");
    if (item == NULL) {
        return true;
    }
    out_request->has_station = true;
    if (cJSON_IsNull(item)) {
        out_request->remove_station = true;
        return true;
    }
    if (!exact_credential_fields(item, &out_request->station)) {
        return false;
    }
    out_request->remove_station = false;
    return true;
}

/* Populates `*out_request`'s has_-flag/value pairs from `root`. Returns false
 * on any wrong-type field; the caller has already confirmed the field-name
 * shape via exact_settings_put_fields(). Every populated string view points
 * into `root`, which must stay alive (and un-wiped) until the caller is
 * finished reading `*out_request`. See populate_send_mode()'s doc comment
 * for *out_invalid_send_mode. */
static bool populate_settings_update_request(const cJSON *root,
                                             app_v2_settings_update_request_t *out_request,
                                             bool *out_invalid_send_mode) {
    return populate_device_name(root, out_request) &&
           populate_require_confirmation(root, out_request) &&
           populate_send_mode(root, out_request, out_invalid_send_mode) &&
           populate_snapshot_retention_target(root, out_request) &&
           populate_show_previews(root, out_request) &&
           populate_last_selected_package_id(root, out_request) &&
           populate_access_point(root, out_request) && populate_station(root, out_request);
}

static web_settings_put_outcome_t put_outcome(web_settings_put_result_t result) {
    return (web_settings_put_outcome_t){.result = result, .detail = APP_ERROR_NONE};
}

static web_settings_put_outcome_t put_outcome_with_detail(web_settings_put_result_t result,
                                                          app_error_code_t detail) {
    return (web_settings_put_outcome_t){.result = result, .detail = detail};
}

static web_settings_put_result_t map_prepare_update_result(app_v2_settings_update_result_t result) {
    switch (result) {
    case APP_V2_SETTINGS_UPDATE_EMPTY:
        return WEB_SETTINGS_PUT_EMPTY;
    case APP_V2_SETTINGS_UPDATE_INVALID_DEVICE_NAME:
        return WEB_SETTINGS_PUT_INVALID_DEVICE_NAME;
    case APP_V2_SETTINGS_UPDATE_INVALID_SNAPSHOT_RETENTION_TARGET:
        return WEB_SETTINGS_PUT_INVALID_SNAPSHOT_RETENTION_TARGET;
    case APP_V2_SETTINGS_UPDATE_INVALID_LAST_SELECTED_PACKAGE_ID:
        return WEB_SETTINGS_PUT_INVALID_LAST_SELECTED_PACKAGE_ID;
    case APP_V2_SETTINGS_UPDATE_INVALID_ACCESS_POINT_SSID:
        return WEB_SETTINGS_PUT_INVALID_ACCESS_POINT_SSID;
    case APP_V2_SETTINGS_UPDATE_INVALID_ACCESS_POINT_PASSPHRASE:
        return WEB_SETTINGS_PUT_INVALID_ACCESS_POINT_PASSPHRASE;
    case APP_V2_SETTINGS_UPDATE_INVALID_STATION_SSID:
        return WEB_SETTINGS_PUT_INVALID_STATION_SSID;
    case APP_V2_SETTINGS_UPDATE_INVALID_STATION_PASSPHRASE:
        return WEB_SETTINGS_PUT_INVALID_STATION_PASSPHRASE;
    case APP_V2_SETTINGS_UPDATE_OK:
    case APP_V2_SETTINGS_UPDATE_INVALID_ARGUMENT:
    case APP_V2_SETTINGS_UPDATE_INVALID_CURRENT_SETTINGS:
    default:
        return WEB_SETTINGS_PUT_INTERNAL;
    }
}

/* Builds the exact SPEC_V2 13.9 PUT response
 * ({"settings":{...},"restartRequired","reconnectRequired"}). Returns NULL on
 * any encoding failure. */
static char *build_settings_put_response_json(const app_v2_device_settings_t *candidate,
                                              bool restart_required, bool reconnect_required) {
    app_v2_settings_response_t response = {0};
    if (app_v2_settings_response_from_settings(candidate, &response) != APP_V2_SETTINGS_UPDATE_OK) {
        return NULL;
    }

    cJSON *response_root = cJSON_CreateObject();
    if (response_root == NULL) {
        return NULL;
    }
    cJSON *settings_fields = cJSON_CreateObject();
    if (settings_fields == NULL || !settings_response_populate(settings_fields, &response)) {
        cJSON_Delete(settings_fields);
        cJSON_Delete(response_root);
        return NULL;
    }
    if (!cJSON_AddItemToObject(response_root, "settings", settings_fields)) {
        cJSON_Delete(settings_fields);
        cJSON_Delete(response_root);
        return NULL;
    }
    if (cJSON_AddBoolToObject(response_root, "restartRequired", restart_required) == NULL ||
        cJSON_AddBoolToObject(response_root, "reconnectRequired", reconnect_required) == NULL) {
        cJSON_Delete(response_root);
        return NULL;
    }
    return finish_json(response_root);
}

web_settings_put_outcome_t web_settings_put_handle(char *body, size_t body_capacity,
                                                   const web_settings_ops_t *ops, char **out_json) {
    if (out_json != NULL) {
        *out_json = NULL;
    }
    if (!ops_valid_common(ops) || body == NULL || body_capacity == 0U || out_json == NULL) {
        if (body != NULL && body_capacity > 0U) {
            secure_zero_local(body, body_capacity);
        }
        return put_outcome(WEB_SETTINGS_PUT_INTERNAL);
    }

    cJSON *root = parse_exact_body(body, body_capacity);
    if (root == NULL || !exact_settings_put_fields(root)) {
        cJSON_Delete(root);
        return put_outcome(WEB_SETTINGS_PUT_INVALID_BODY);
    }

    app_v2_settings_update_request_t request = {0};
    bool invalid_send_mode = false;
    if (!populate_settings_update_request(root, &request, &invalid_send_mode)) {
        cJSON_Delete(root);
        return put_outcome(WEB_SETTINGS_PUT_INVALID_BODY);
    }
    if (invalid_send_mode) {
        cJSON_Delete(root);
        return put_outcome(WEB_SETTINGS_PUT_INVALID_SEND_MODE);
    }

    app_v2_device_settings_t current = {0};
    const app_error_code_t read_result = ops->settings_read(ops->context, &current);
    if (read_result != APP_ERROR_NONE) {
        cJSON_Delete(root);
        return put_outcome_with_detail(WEB_SETTINGS_PUT_BACKEND_UNAVAILABLE, read_result);
    }

    app_v2_device_settings_t candidate = {0};
    bool restart_required = false;
    bool reconnect_required = false;
    const app_v2_settings_update_result_t prepare_result = app_v2_settings_prepare_update(
        &current, &request, &candidate, &restart_required, &reconnect_required);
    cJSON_Delete(root);
    if (prepare_result != APP_V2_SETTINGS_UPDATE_OK) {
        return put_outcome(map_prepare_update_result(prepare_result));
    }

    bool changed = false;
    const app_error_code_t replace_result =
        ops->settings_replace(ops->context, &candidate, &changed);
    if (replace_result != APP_ERROR_NONE) {
        return put_outcome_with_detail(WEB_SETTINGS_PUT_BACKEND_UNAVAILABLE, replace_result);
    }

    char *json = build_settings_put_response_json(&candidate, restart_required, reconnect_required);
    if (json == NULL) {
        return put_outcome(WEB_SETTINGS_PUT_INTERNAL);
    }
    *out_json = json;
    return put_outcome(WEB_SETTINGS_PUT_OK);
}

/* -------------------------------------------------------------------------
 * POST /api/v1/settings/change-password
 * ---------------------------------------------------------------------- */

#define CHANGE_PASSWORD_FIELD_COUNT 2U

static const char *const CHANGE_PASSWORD_FIELDS[CHANGE_PASSWORD_FIELD_COUNT] = {
    "currentPassword",
    "newPassword",
};

static bool exact_change_password_fields(const cJSON *root) {
    bool seen[CHANGE_PASSWORD_FIELD_COUNT] = {false};
    size_t field_count = 0U;
    for (const cJSON *item = root->child; item != NULL; item = item->next) {
        if (item->string == NULL) {
            return false;
        }
        bool matched = false;
        for (size_t index = 0U; index < CHANGE_PASSWORD_FIELD_COUNT; ++index) {
            if (strcmp(item->string, CHANGE_PASSWORD_FIELDS[index]) == 0) {
                if (seen[index]) {
                    return false;
                }
                seen[index] = true;
                matched = true;
                break;
            }
        }
        if (!matched) {
            return false;
        }
        ++field_count;
    }
    return field_count == CHANGE_PASSWORD_FIELD_COUNT;
}

static web_change_password_outcome_t change_password_outcome(web_change_password_result_t result) {
    return (web_change_password_outcome_t){.result = result, .detail = APP_ERROR_NONE};
}

static web_change_password_outcome_t
change_password_outcome_with_detail(web_change_password_result_t result, app_error_code_t detail) {
    return (web_change_password_outcome_t){.result = result, .detail = detail};
}

static bool ops_valid_change_password(const web_settings_ops_t *ops) {
    return ops_valid_common(ops) && ops->password_verify != NULL && ops->password_create != NULL &&
           ops->invalidate_all_sessions != NULL;
}

web_change_password_outcome_t web_change_password_handle(char *body, size_t body_capacity,
                                                         const web_settings_ops_t *ops) {
    if (!ops_valid_change_password(ops) || body == NULL || body_capacity == 0U) {
        if (body != NULL && body_capacity > 0U) {
            secure_zero_local(body, body_capacity);
        }
        return change_password_outcome(WEB_CHANGE_PASSWORD_INTERNAL);
    }

    cJSON *root = parse_exact_body(body, body_capacity);
    if (root == NULL || !exact_change_password_fields(root)) {
        wipe_and_delete_json_tree(root);
        return change_password_outcome(WEB_CHANGE_PASSWORD_INVALID_BODY);
    }

    app_v2_string_view_t current_password_view = {0};
    app_v2_string_view_t new_password_view = {0};
    if (!string_view_from_item(cJSON_GetObjectItemCaseSensitive(root, "currentPassword"),
                               &current_password_view) ||
        !string_view_from_item(cJSON_GetObjectItemCaseSensitive(root, "newPassword"),
                               &new_password_view)) {
        wipe_and_delete_json_tree(root);
        return change_password_outcome(WEB_CHANGE_PASSWORD_INVALID_BODY);
    }

    app_v2_device_settings_t current = {0};
    const app_error_code_t read_result = ops->settings_read(ops->context, &current);
    if (read_result != APP_ERROR_NONE) {
        wipe_and_delete_json_tree(root);
        return change_password_outcome_with_detail(WEB_CHANGE_PASSWORD_BACKEND_UNAVAILABLE,
                                                   read_result);
    }

    if (app_v2_password_change_validate(&current, new_password_view) != APP_V2_PASSWORD_CHANGE_OK) {
        wipe_and_delete_json_tree(root);
        return change_password_outcome(WEB_CHANGE_PASSWORD_INVALID_NEW_PASSWORD);
    }

    bool current_matches = false;
    const app_error_code_t verify_result =
        ops->password_verify(ops->context, current_password_view.data, current_password_view.length,
                             &current, &current_matches);
    if (verify_result != APP_ERROR_NONE) {
        wipe_and_delete_json_tree(root);
        return change_password_outcome_with_detail(WEB_CHANGE_PASSWORD_BACKEND_UNAVAILABLE,
                                                   verify_result);
    }
    if (!current_matches) {
        wipe_and_delete_json_tree(root);
        return change_password_outcome(WEB_CHANGE_PASSWORD_INCORRECT_CURRENT_PASSWORD);
    }

    app_v2_setup_password_material_t material = {0};
    const app_error_code_t create_result = ops->password_create(
        ops->context, new_password_view.data, new_password_view.length, &material);
    wipe_and_delete_json_tree(root);
    if (create_result != APP_ERROR_NONE) {
        secure_zero_local(&material, sizeof(material));
        return change_password_outcome_with_detail(WEB_CHANGE_PASSWORD_BACKEND_UNAVAILABLE,
                                                   create_result);
    }

    app_v2_device_settings_t candidate = {0};
    const bool prepared = app_v2_password_change_prepare_candidate(&current, &material, &candidate);
    secure_zero_local(&material, sizeof(material));
    if (!prepared) {
        return change_password_outcome(WEB_CHANGE_PASSWORD_INTERNAL);
    }

    bool changed = false;
    const app_error_code_t replace_result =
        ops->settings_replace(ops->context, &candidate, &changed);
    if (replace_result != APP_ERROR_NONE) {
        return change_password_outcome_with_detail(WEB_CHANGE_PASSWORD_BACKEND_UNAVAILABLE,
                                                   replace_result);
    }

    const app_error_code_t invalidate_result = ops->invalidate_all_sessions(ops->context);
    if (invalidate_result != APP_ERROR_NONE) {
        return change_password_outcome_with_detail(WEB_CHANGE_PASSWORD_BACKEND_UNAVAILABLE,
                                                   invalidate_result);
    }

    return change_password_outcome(WEB_CHANGE_PASSWORD_OK);
}
