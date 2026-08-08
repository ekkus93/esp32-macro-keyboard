#include "web_settings.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "cJSON.h"
#include "settings_contract_v2.h"

#define SETTINGS_NUL_ESCAPE "\\u0000"
#define SETTINGS_SEND_MODE_QUICK "quick"
#define SETTINGS_SEND_MODE_PREVIEW "preview"

static void secure_zero_local(void *memory, size_t length) {
    volatile uint8_t *bytes = memory;
    for (size_t index = 0U; index < length; ++index) {
        bytes[index] = 0U;
    }
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
        cJSON_Delete(root);
        return NULL;
    }
    return root;
}

/* -------------------------------------------------------------------------
 * Response JSON (GET, and the "settings" object nested inside PUT's
 * response).
 * ---------------------------------------------------------------------- */

static bool add_optional_string(cJSON *root, const char *key,
                                app_v2_optional_string_view_t value) {
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
    return mode == APP_V2_SEND_MODE_PREVIEW ? SETTINGS_SEND_MODE_PREVIEW
                                            : SETTINGS_SEND_MODE_QUICK;
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
           cJSON_AddBoolToObject(root, "stationConfigured", response->station_configured) !=
               NULL &&
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

web_settings_get_outcome_t web_settings_get_handle(const web_settings_ops_t *ops,
                                                   char **out_json) {
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
    if (app_v2_settings_response_from_settings(&settings, &response) !=
        APP_V2_SETTINGS_UPDATE_OK) {
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
    "deviceName",  "requireSerialConfirmation", "sendMode",  "snapshotRetentionTarget",
    "showMacroSourcePreviews", "lastSelectedPackageId", "accessPoint", "station",
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

/* Populates `*out_request`'s has_-flag/value pairs from `root`. Returns false
 * on any wrong-type field; the caller has already confirmed the field-name
 * shape via exact_settings_put_fields(). Every populated string view points
 * into `root`, which must stay alive (and un-wiped) until the caller is
 * finished reading `*out_request`. */
static bool populate_settings_update_request(const cJSON *root,
                                             app_v2_settings_update_request_t *out_request) {
    const cJSON *device_name = cJSON_GetObjectItemCaseSensitive(root, "deviceName");
    if (device_name != NULL) {
        out_request->has_device_name = true;
        if (!string_view_from_item(device_name, &out_request->device_name)) {
            return false;
        }
    }

    const cJSON *require_confirmation =
        cJSON_GetObjectItemCaseSensitive(root, "requireSerialConfirmation");
    if (require_confirmation != NULL) {
        if (!cJSON_IsBool(require_confirmation)) {
            return false;
        }
        out_request->has_require_serial_confirmation = true;
        out_request->require_serial_confirmation = cJSON_IsTrue(require_confirmation);
    }

    const cJSON *send_mode = cJSON_GetObjectItemCaseSensitive(root, "sendMode");
    if (send_mode != NULL) {
        if (!cJSON_IsString(send_mode) || send_mode->valuestring == NULL) {
            return false;
        }
        out_request->has_send_mode = true;
        if (strcmp(send_mode->valuestring, SETTINGS_SEND_MODE_QUICK) == 0) {
            out_request->send_mode = APP_V2_SEND_MODE_QUICK;
        } else if (strcmp(send_mode->valuestring, SETTINGS_SEND_MODE_PREVIEW) == 0) {
            out_request->send_mode = APP_V2_SEND_MODE_PREVIEW;
        } else {
            /* Structurally a string, semantically not one of the two
             * accepted values -- WEB_SETTINGS_PUT_INVALID_SEND_MODE, not a
             * generic invalid-body result. Signalled by leaving has_send_mode
             * set with an out-of-enum sentinel the caller checks for. */
            out_request->send_mode = (app_v2_send_mode_t)-1;
        }
    }

    const cJSON *retention = cJSON_GetObjectItemCaseSensitive(root, "snapshotRetentionTarget");
    if (retention != NULL) {
        if (!cJSON_IsNumber(retention) || retention->valuedouble < 0.0 ||
            retention->valuedouble > 255.0) {
            return false;
        }
        const uint8_t candidate = (uint8_t)retention->valuedouble;
        if ((double)candidate != retention->valuedouble) {
            return false;
        }
        out_request->has_snapshot_retention_target = true;
        out_request->snapshot_retention_target = candidate;
    }

    const cJSON *show_previews = cJSON_GetObjectItemCaseSensitive(root, "showMacroSourcePreviews");
    if (show_previews != NULL) {
        if (!cJSON_IsBool(show_previews)) {
            return false;
        }
        out_request->has_show_macro_source_previews = true;
        out_request->show_macro_source_previews = cJSON_IsTrue(show_previews);
    }

    const cJSON *package_id = cJSON_GetObjectItemCaseSensitive(root, "lastSelectedPackageId");
    if (package_id != NULL) {
        out_request->has_last_selected_package_id = true;
        if (cJSON_IsNull(package_id)) {
            out_request->last_selected_package_id.present = false;
        } else if (cJSON_IsString(package_id) && package_id->valuestring != NULL) {
            out_request->last_selected_package_id.present = true;
            out_request->last_selected_package_id.value = (app_v2_string_view_t){
                .data = package_id->valuestring,
                .length = strlen(package_id->valuestring),
            };
        } else {
            return false;
        }
    }

    const cJSON *access_point = cJSON_GetObjectItemCaseSensitive(root, "accessPoint");
    if (access_point != NULL) {
        if (!exact_credential_fields(access_point, &out_request->access_point)) {
            return false;
        }
        out_request->has_access_point = true;
    }

    const cJSON *station = cJSON_GetObjectItemCaseSensitive(root, "station");
    if (station != NULL) {
        out_request->has_station = true;
        if (cJSON_IsNull(station)) {
            out_request->remove_station = true;
        } else if (exact_credential_fields(station, &out_request->station)) {
            out_request->remove_station = false;
        } else {
            return false;
        }
    }

    return true;
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

web_settings_put_outcome_t web_settings_put_handle(char *body, size_t body_capacity,
                                                   const web_settings_ops_t *ops, char **out_json) {
    if (out_json != NULL) {
        *out_json = NULL;
    }
    if (!ops_valid_common(ops)) {
        if (body != NULL && body_capacity > 0U) {
            secure_zero_local(body, body_capacity);
        }
        return put_outcome(WEB_SETTINGS_PUT_INTERNAL);
    }
    if (body == NULL || body_capacity == 0U || out_json == NULL) {
        if (body != NULL && body_capacity > 0U) {
            secure_zero_local(body, body_capacity);
        }
        return put_outcome(WEB_SETTINGS_PUT_INTERNAL);
    }

    cJSON *root = parse_exact_body(body, body_capacity);
    if (root == NULL) {
        return put_outcome(WEB_SETTINGS_PUT_INVALID_BODY);
    }
    if (!exact_settings_put_fields(root)) {
        cJSON_Delete(root);
        return put_outcome(WEB_SETTINGS_PUT_INVALID_BODY);
    }

    app_v2_settings_update_request_t request = {0};
    if (!populate_settings_update_request(root, &request)) {
        cJSON_Delete(root);
        return put_outcome(WEB_SETTINGS_PUT_INVALID_BODY);
    }
    if (request.has_send_mode && request.send_mode != APP_V2_SEND_MODE_QUICK &&
        request.send_mode != APP_V2_SEND_MODE_PREVIEW) {
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
    const app_error_code_t replace_result = ops->settings_replace(ops->context, &candidate, &changed);
    if (replace_result != APP_ERROR_NONE) {
        return put_outcome_with_detail(WEB_SETTINGS_PUT_BACKEND_UNAVAILABLE, replace_result);
    }

    app_v2_settings_response_t response = {0};
    if (app_v2_settings_response_from_settings(&candidate, &response) !=
        APP_V2_SETTINGS_UPDATE_OK) {
        return put_outcome(WEB_SETTINGS_PUT_INTERNAL);
    }

    cJSON *response_root = cJSON_CreateObject();
    if (response_root == NULL) {
        return put_outcome(WEB_SETTINGS_PUT_INTERNAL);
    }
    cJSON *settings_object = cJSON_CreateObject();
    if (settings_object == NULL || !settings_response_populate(settings_object, &response)) {
        cJSON_Delete(settings_object);
        cJSON_Delete(response_root);
        return put_outcome(WEB_SETTINGS_PUT_INTERNAL);
    }
    if (!cJSON_AddItemToObject(response_root, "settings", settings_object)) {
        cJSON_Delete(settings_object);
        cJSON_Delete(response_root);
        return put_outcome(WEB_SETTINGS_PUT_INTERNAL);
    }
    if (cJSON_AddBoolToObject(response_root, "restartRequired", restart_required) == NULL ||
        cJSON_AddBoolToObject(response_root, "reconnectRequired", reconnect_required) == NULL) {
        cJSON_Delete(response_root);
        return put_outcome(WEB_SETTINGS_PUT_INTERNAL);
    }
    char *json = finish_json(response_root);
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
change_password_outcome_with_detail(web_change_password_result_t result,
                                    app_error_code_t detail) {
    return (web_change_password_outcome_t){.result = result, .detail = detail};
}

web_change_password_outcome_t web_change_password_handle(char *body, size_t body_capacity,
                                                          const web_settings_ops_t *ops) {
    if (ops == NULL || !ops_valid_common(ops) || ops->password_verify == NULL ||
        ops->password_create == NULL || ops->invalidate_all_sessions == NULL) {
        if (body != NULL && body_capacity > 0U) {
            secure_zero_local(body, body_capacity);
        }
        return change_password_outcome(WEB_CHANGE_PASSWORD_INTERNAL);
    }
    if (body == NULL || body_capacity == 0U) {
        if (body != NULL && body_capacity > 0U) {
            secure_zero_local(body, body_capacity);
        }
        return change_password_outcome(WEB_CHANGE_PASSWORD_INTERNAL);
    }

    cJSON *root = parse_exact_body(body, body_capacity);
    if (root == NULL) {
        return change_password_outcome(WEB_CHANGE_PASSWORD_INVALID_BODY);
    }
    if (!exact_change_password_fields(root)) {
        cJSON_Delete(root);
        return change_password_outcome(WEB_CHANGE_PASSWORD_INVALID_BODY);
    }

    app_v2_string_view_t current_password_view = {0};
    app_v2_string_view_t new_password_view = {0};
    if (!string_view_from_item(cJSON_GetObjectItemCaseSensitive(root, "currentPassword"),
                              &current_password_view) ||
        !string_view_from_item(cJSON_GetObjectItemCaseSensitive(root, "newPassword"),
                              &new_password_view)) {
        cJSON_Delete(root);
        return change_password_outcome(WEB_CHANGE_PASSWORD_INVALID_BODY);
    }

    app_v2_device_settings_t current = {0};
    const app_error_code_t read_result = ops->settings_read(ops->context, &current);
    if (read_result != APP_ERROR_NONE) {
        cJSON_Delete(root);
        return change_password_outcome_with_detail(WEB_CHANGE_PASSWORD_BACKEND_UNAVAILABLE,
                                                   read_result);
    }

    if (app_v2_password_change_validate(&current, new_password_view) !=
        APP_V2_PASSWORD_CHANGE_OK) {
        cJSON_Delete(root);
        return change_password_outcome(WEB_CHANGE_PASSWORD_INVALID_NEW_PASSWORD);
    }

    bool current_matches = false;
    const app_error_code_t verify_result =
        ops->password_verify(ops->context, current_password_view.data,
                             current_password_view.length, &current, &current_matches);
    if (verify_result != APP_ERROR_NONE) {
        cJSON_Delete(root);
        return change_password_outcome_with_detail(WEB_CHANGE_PASSWORD_BACKEND_UNAVAILABLE,
                                                   verify_result);
    }
    if (!current_matches) {
        cJSON_Delete(root);
        return change_password_outcome(WEB_CHANGE_PASSWORD_INCORRECT_CURRENT_PASSWORD);
    }

    app_v2_setup_password_material_t material = {0};
    const app_error_code_t create_result = ops->password_create(
        ops->context, new_password_view.data, new_password_view.length, &material);
    cJSON_Delete(root);
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
    const app_error_code_t replace_result = ops->settings_replace(ops->context, &candidate, &changed);
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
