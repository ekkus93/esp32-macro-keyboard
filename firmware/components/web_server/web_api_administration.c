#include "web_api_handlers.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "auth.h"
#include "cJSON.h"
#include "macro_limits.h"
#include "macro_model.h"
#include "provisioning.h"
#include "storage.h"
#include "storage_repository.h"
#include "web_api_core.h"
#include "web_api_handler_common.h"
#include "web_api_json.h"
#include "web_api_response.h"
#include "web_http_status.h"
#include "web_server_internal.h"

#define WEB_PASSWORD_CHANGE_BODY_MAX_BYTES 512U
#define WEB_PASSWORD_CHANGE_RESPONSE_BYTES 80U

typedef struct {
    uint32_t expected_revision;
    char current_password[AUTH_PASSWORD_MAX_BYTES + 1U];
    char new_password[AUTH_PASSWORD_MAX_BYTES + 1U];
} password_change_t;

static void secure_zero(void *memory, size_t length) {
    volatile uint8_t *bytes = memory;
    for (size_t index = 0U; index < length; ++index) {
        bytes[index] = 0U;
    }
}

static bool exact_password_fields(const cJSON *root) {
    static const char *const fields[] = {"expectedRevision", "currentPassword", "newPassword"};
    bool seen[3U] = {false};
    size_t count = 0U;
    if (!cJSON_IsObject(root)) {
        return false;
    }
    for (const cJSON *item = root->child; item != NULL; item = item->next) {
        bool matched = false;
        if (item->string == NULL) {
            return false;
        }
        for (size_t index = 0U; index < 3U; ++index) {
            if (strcmp(item->string, fields[index]) == 0 && !seen[index]) {
                seen[index] = true;
                matched = true;
                break;
            }
        }
        if (!matched) {
            return false;
        }
        ++count;
    }
    return count == 3U && seen[0] && seen[1] && seen[2];
}

static bool copy_password(const cJSON *item, char *output, size_t output_size) {
    if (!cJSON_IsString(item) || item->valuestring == NULL || output == NULL || output_size == 0U) {
        return false;
    }
    const size_t length = strlen(item->valuestring);
    if (length < AUTH_PASSWORD_MIN_BYTES || length > AUTH_PASSWORD_MAX_BYTES ||
        length >= output_size) {
        return false;
    }
    memcpy(output, item->valuestring, length + 1U);
    return true;
}

static void wipe_json_strings(cJSON *root) {
    if (root == NULL) {
        return;
    }
    for (cJSON *item = root->child; item != NULL; item = item->next) {
        if (cJSON_IsString(item) && item->valuestring != NULL) {
            secure_zero(item->valuestring, strlen(item->valuestring));
        }
    }
}

static app_error_code_t parse_password_change(const web_api_call_t *call,
                                              password_change_t *out_change) {
    if (call == NULL || out_change == NULL || call->body == NULL || call->body_length == 0U ||
        call->body_length > WEB_PASSWORD_CHANGE_BODY_MAX_BYTES) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_change, 0, sizeof(*out_change));
    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts(call->body, call->body_length, &parse_end, false);
    const cJSON *expected =
        root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "expectedRevision");
    const cJSON *current =
        root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "currentPassword");
    const cJSON *replacement =
        root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "newPassword");
    const bool valid =
        root != NULL && parse_end == call->body + call->body_length &&
        exact_password_fields(root) && cJSON_IsNumber(expected) && expected->valuedouble >= 1.0 &&
        expected->valuedouble <= (double)UINT32_MAX &&
        expected->valuedouble == (double)expected->valueint &&
        copy_password(current, out_change->current_password,
                      sizeof(out_change->current_password)) &&
        copy_password(replacement, out_change->new_password, sizeof(out_change->new_password));
    if (valid) {
        out_change->expected_revision = (uint32_t)expected->valuedouble;
    }
    wipe_json_strings(root);
    cJSON_Delete(root);
    if (!valid) {
        secure_zero(out_change, sizeof(*out_change));
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t send_settings(web_api_response_t *response,
                                      const provisioning_settings_t *settings) {
    char *json = NULL;
    app_error_code_t result = web_api_handler_settings_json(settings, &json);
    if (result == APP_ERROR_NONE) {
        result = web_api_handler_success_json(response, WEB_HTTP_STATUS_OK, json);
    }
    web_api_handler_json_free(json);
    return result;
}

static app_error_code_t handle_settings(const web_api_call_t *call, web_api_response_t *response) {
    if (call->method == WEB_API_METHOD_GET) {
        provisioning_settings_t settings = {0};
        const app_error_code_t result = provisioning_settings_read(&settings);
        return result == APP_ERROR_NONE
                   ? send_settings(response, &settings)
                   : web_api_handler_error(response, result, "settings unavailable", NULL);
    }
    provisioning_settings_t replacement = {0};
    uint32_t expected_revision = 0U;
    app_error_code_t result = web_api_json_parse_settings_update(call->body, call->body_length,
                                                                 &replacement, &expected_revision);
    if (result == APP_ERROR_NONE && replacement.has_active_set) {
        macro_set_t selected = {0};
        result = storage_set_read(&replacement.active_set_id, &selected);
    }
    provisioning_settings_t committed = {0};
    if (result == APP_ERROR_NONE) {
        result = provisioning_settings_update(&replacement, expected_revision, &committed);
    }
    if (result != APP_ERROR_NONE) {
        return web_api_handler_error(response, result, "could not update settings", NULL);
    }
    server_configuration.require_physical_confirmation = committed.require_physical_confirmation;
    return send_settings(response, &committed);
}

static app_error_code_t handle_change_password(const web_api_call_t *call,
                                               web_api_response_t *response) {
    password_change_t change = {0};
    app_error_code_t result = parse_password_change(call, &change);
    bool matches = false;
    if (result == APP_ERROR_NONE) {
        result = auth_password_verify(change.current_password, strlen(change.current_password),
                                      &server_configuration.password_record, &matches);
    }
    if (result == APP_ERROR_NONE && !matches) {
        result = APP_ERROR_AUTH_FAILED;
    }
    provisioning_config_t configuration = {0};
    if (result == APP_ERROR_NONE) {
        result = provisioning_load(&configuration);
    }
    if (result == APP_ERROR_NONE && configuration.revision != change.expected_revision) {
        result = APP_ERROR_CONFLICT;
    }
    auth_password_record_t replacement_record = {0};
    if (result == APP_ERROR_NONE) {
        result = auth_password_create(change.new_password, strlen(change.new_password),
                                      &replacement_record);
    }
    provisioning_config_t committed = {0};
    if (result == APP_ERROR_NONE) {
        if (configuration.credential_version == UINT32_MAX) {
            result = APP_ERROR_INTERNAL;
        } else {
            configuration.password_record = replacement_record;
            ++configuration.credential_version;
            result = provisioning_commit(&configuration, change.expected_revision, &committed);
        }
    }
    if (result == APP_ERROR_NONE) {
        server_configuration.password_record = committed.password_record;
    }
    const uint32_t credential_version = committed.credential_version;
    secure_zero(&change, sizeof(change));
    secure_zero(&configuration, sizeof(configuration));
    secure_zero(&committed, sizeof(committed));
    secure_zero(&replacement_record, sizeof(replacement_record));
    if (result != APP_ERROR_NONE) {
        return web_api_handler_error(response, result, "could not change password", NULL);
    }
    char data[WEB_PASSWORD_CHANGE_RESPONSE_BYTES];
    const int length = snprintf(data, sizeof(data), "{\"credentialVersion\":%lu}",
                                (unsigned long)credential_version);
    return length < 0 || (size_t)length >= sizeof(data)
               ? APP_ERROR_INTERNAL
               : web_api_handler_success_json(response, WEB_HTTP_STATUS_OK, data);
}

static app_error_code_t handle_reset_settings(const web_api_call_t *call,
                                              web_api_response_t *response) {
    uint32_t expected_revision = 0U;
    app_error_code_t result =
        web_api_json_parse_expected_revision(call->body, call->body_length, &expected_revision);
    const provisioning_settings_t replacement = {
        .schema_version = APP_SCHEMA_VERSION,
        .revision = expected_revision,
        .require_physical_confirmation = true,
        .always_select_set = true,
        .has_active_set = false,
    };
    provisioning_settings_t committed = {0};
    if (result == APP_ERROR_NONE) {
        result = provisioning_settings_update(&replacement, expected_revision, &committed);
    }
    if (result != APP_ERROR_NONE) {
        return web_api_handler_error(response, result, "could not reset settings", NULL);
    }
    server_configuration.require_physical_confirmation = true;
    return send_settings(response, &committed);
}

static app_error_code_t handle_storage_health(web_api_response_t *response, bool checked) {
    const storage_mount_state_t mounts = storage_mount_state();
    storage_quarantine_list_t quarantine = {0};
    const app_error_code_t result = storage_quarantine_list(&quarantine);
    if (result != APP_ERROR_NONE) {
        return web_api_handler_error(response, result, "storage health unavailable", NULL);
    }
    char data[256U];
    const int length =
        snprintf(data, sizeof(data),
                 "{\"checked\":%s,\"webMounted\":%s,\"dataMounted\":%s,"
                 "\"quarantineCount\":%lu,\"damagedQuarantineCount\":%lu}",
                 checked ? "true" : "false", mounts.web_mounted ? "true" : "false",
                 mounts.data_mounted ? "true" : "false", (unsigned long)quarantine.count,
                 (unsigned long)quarantine.damaged_count);
    return length < 0 || (size_t)length >= sizeof(data)
               ? APP_ERROR_INTERNAL
               : web_api_handler_success_json(response, WEB_HTTP_STATUS_OK, data);
}

static app_error_code_t handle_quarantine(web_api_response_t *response) {
    storage_quarantine_list_t list = {0};
    app_error_code_t result = storage_quarantine_list(&list);
    char *json = NULL;
    if (result == APP_ERROR_NONE) {
        result = web_api_handler_quarantine_json(&list, &json);
    }
    if (result == APP_ERROR_NONE) {
        result = web_api_handler_success_json(response, WEB_HTTP_STATUS_OK, json);
    } else {
        const app_error_code_t encoded =
            web_api_handler_error(response, result, "quarantine unavailable", NULL);
        result = encoded == APP_ERROR_NONE ? APP_ERROR_NONE : encoded;
    }
    web_api_handler_json_free(json);
    return result;
}

static app_error_code_t unavailable(web_api_response_t *response, const char *message) {
    return web_api_handler_error(response, APP_ERROR_STORAGE_UNAVAILABLE, message, NULL);
}

app_error_code_t web_api_handle_administration(const web_api_call_t *call,
                                               web_api_response_t *response) {
    if (call == NULL || response == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    switch (call->path.route) {
    case WEB_API_ROUTE_AUTH_SESSION:
        return web_api_handler_success_json(response, WEB_HTTP_STATUS_OK,
                                            "{\"authenticated\":true}");
    case WEB_API_ROUTE_SETTINGS:
        return handle_settings(call, response);
    case WEB_API_ROUTE_SETTINGS_CHANGE_PASSWORD:
        return handle_change_password(call, response);
    case WEB_API_ROUTE_DEVICE_RESTART:
        return web_api_handler_success_json(response, WEB_HTTP_STATUS_ACCEPTED,
                                            "{\"restartScheduled\":true}");
    case WEB_API_ROUTE_DEVICE_RESET_SETTINGS:
        return handle_reset_settings(call, response);
    case WEB_API_ROUTE_DEVICE_FACTORY_RESET: {
        const app_error_code_t result = provisioning_factory_reset();
        return result == APP_ERROR_NONE
                   ? web_api_handler_success_json(response, WEB_HTTP_STATUS_ACCEPTED,
                                                  "{\"factoryReset\":true,"
                                                  "\"restartScheduled\":true}")
                   : web_api_handler_error(response, result, "factory reset failed", NULL);
    }
    case WEB_API_ROUTE_DIAGNOSTICS_STORAGE:
        return handle_storage_health(response, false);
    case WEB_API_ROUTE_DIAGNOSTICS_STORAGE_CHECK:
        return handle_storage_health(response, true);
    case WEB_API_ROUTE_DIAGNOSTICS_QUARANTINE:
        return handle_quarantine(response);
    case WEB_API_ROUTE_SET_EXPORT:
    case WEB_API_ROUTE_SET_IMPORT:
    case WEB_API_ROUTE_BACKUP:
    case WEB_API_ROUTE_RESTORE:
        return unavailable(response, "package operation requires the Phase 18 transaction service");
    default:
        return APP_ERROR_NOT_FOUND;
    }
}
