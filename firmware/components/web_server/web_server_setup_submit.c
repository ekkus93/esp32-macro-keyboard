#include "web_server_setup_submit.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "api_contracts_v2.h"
#include "app_error.h"
#include "cJSON.h"
#include "device_settings_v2.h"
#include "setup_contract_v2.h"

#define SETUP_SUBMIT_STRING_FIELD_COUNT 5U
#define SETUP_SUBMIT_FIELD_COUNT 6U
#define SETUP_SUBMIT_NUL_ESCAPE "\\u0000"

static const char *const SETUP_SUBMIT_STRING_FIELDS[SETUP_SUBMIT_STRING_FIELD_COUNT] = {
    "setupCode", "deviceName", "apSsid", "apPassphrase", "adminPassword",
};
static const char *const SETUP_SUBMIT_BOOL_FIELD = "requireSerialConfirmation";

static bool operations_valid(const web_setup_submit_ops_t *operations) {
    return operations != NULL && operations->settings_read != NULL &&
           operations->password_create != NULL && operations->settings_replace != NULL &&
           operations->secure_zero != NULL;
}

static web_setup_submit_outcome_t outcome_of(web_setup_submit_result_t result) {
    return (web_setup_submit_outcome_t){.result = result, .detail = APP_ERROR_NONE};
}

static web_setup_submit_outcome_t outcome_with_detail(web_setup_submit_result_t result,
                                                      app_error_code_t detail) {
    return (web_setup_submit_outcome_t){.result = result, .detail = detail};
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

/* JSON forbids an unescaped NUL inside a string, but the six-character backslash-u-0-0-0-0 escape
 * is legal JSON that cJSON decodes into a real NUL byte -- which would make a later strlen() on
 * that decoded string silently see a shorter value than what was actually supplied. Rejecting the
 * escape upstream, before parsing, keeps the request-body length check and the field-length checks
 * below looking at the same bytes. */
static bool contains_embedded_nul_escape(const char *body, size_t length) {
    const size_t escape_length = sizeof(SETUP_SUBMIT_NUL_ESCAPE) - 1U;
    if (length < escape_length) {
        return false;
    }
    for (size_t index = 0U; index <= length - escape_length; ++index) {
        if (memcmp(body + index, SETUP_SUBMIT_NUL_ESCAPE, escape_length) == 0) {
            return true;
        }
    }
    return false;
}

/* Returns true and marks the matching slot the first time one of the five
 * fixed string field names is seen; returns false both when `name` names no
 * string field and when that field's slot was already marked (a duplicate
 * key), so the caller does not need to tell those two cases apart. */
static bool mark_string_field_seen(const char *name,
                                   bool seen_string[SETUP_SUBMIT_STRING_FIELD_COUNT]) {
    for (size_t index = 0U; index < SETUP_SUBMIT_STRING_FIELD_COUNT; ++index) {
        if (strcmp(name, SETUP_SUBMIT_STRING_FIELDS[index]) != 0) {
            continue;
        }
        if (seen_string[index]) {
            return false;
        }
        seen_string[index] = true;
        return true;
    }
    return false;
}

static bool exact_setup_fields(const cJSON *root) {
    if (!cJSON_IsObject(root)) {
        return false;
    }
    bool seen_string[SETUP_SUBMIT_STRING_FIELD_COUNT] = {false};
    bool seen_bool = false;
    size_t field_count = 0U;
    for (const cJSON *item = root->child; item != NULL; item = item->next) {
        if (item->string == NULL) {
            return false;
        }
        bool matched = mark_string_field_seen(item->string, seen_string);
        if (!matched && strcmp(item->string, SETUP_SUBMIT_BOOL_FIELD) == 0 && !seen_bool) {
            seen_bool = true;
            matched = true;
        }
        if (!matched) {
            return false;
        }
        ++field_count;
    }
    if (field_count != SETUP_SUBMIT_FIELD_COUNT || !seen_bool) {
        return false;
    }
    for (size_t index = 0U; index < SETUP_SUBMIT_STRING_FIELD_COUNT; ++index) {
        if (!seen_string[index]) {
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

static bool populate_request(const cJSON *root, app_v2_setup_request_t *request) {
    const cJSON *setup_code = cJSON_GetObjectItemCaseSensitive(root, "setupCode");
    const cJSON *device_name = cJSON_GetObjectItemCaseSensitive(root, "deviceName");
    const cJSON *ap_ssid = cJSON_GetObjectItemCaseSensitive(root, "apSsid");
    const cJSON *ap_passphrase = cJSON_GetObjectItemCaseSensitive(root, "apPassphrase");
    const cJSON *admin_password = cJSON_GetObjectItemCaseSensitive(root, "adminPassword");
    const cJSON *require_confirmation =
        cJSON_GetObjectItemCaseSensitive(root, "requireSerialConfirmation");
    if (!string_view_from_item(setup_code, &request->setup_code) ||
        !string_view_from_item(device_name, &request->device_name) ||
        !string_view_from_item(ap_ssid, &request->ap_ssid) ||
        !string_view_from_item(ap_passphrase, &request->ap_passphrase) ||
        !string_view_from_item(admin_password, &request->admin_password) ||
        !cJSON_IsBool(require_confirmation)) {
        return false;
    }
    request->require_serial_confirmation = cJSON_IsTrue(require_confirmation);
    return true;
}

/* cJSON_Delete() frees parsed string buffers without zeroing them first. The
 * request carries the setup code, AP passphrase, and administrator password,
 * so free-without-wipe would leave secrets sitting in the allocator's freed
 * memory. This flattens the tree into a single list and wipes every string
 * and key before deleting it, mirroring the discipline the rest of this
 * component applies to every other secret buffer. */
static void wipe_json_tree(const web_setup_submit_ops_t *operations, cJSON *item) {
    for (cJSON *current = item; current != NULL; current = current->next) {
        if (current->string != NULL) {
            operations->secure_zero(operations->context, current->string,
                                    strlen(current->string) + 1U);
        }
        if (cJSON_IsString(current) && current->valuestring != NULL) {
            operations->secure_zero(operations->context, current->valuestring,
                                    strlen(current->valuestring) + 1U);
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

static web_setup_submit_outcome_t map_prepare_result(app_v2_setup_result_t result) {
    switch (result) {
    case APP_V2_SETUP_MALFORMED_CODE:
        return outcome_of(WEB_SETUP_SUBMIT_MALFORMED_CODE);
    case APP_V2_SETUP_CODE_MISMATCH:
        return outcome_of(WEB_SETUP_SUBMIT_CODE_MISMATCH);
    case APP_V2_SETUP_CODE_CONSUMED:
        return outcome_of(WEB_SETUP_SUBMIT_CODE_CONSUMED);
    case APP_V2_SETUP_ALREADY_PROVISIONED:
        return outcome_of(WEB_SETUP_SUBMIT_ALREADY_PROVISIONED);
    case APP_V2_SETUP_INVALID_DEVICE_NAME:
        return outcome_of(WEB_SETUP_SUBMIT_INVALID_DEVICE_NAME);
    case APP_V2_SETUP_INVALID_AP_SSID:
        return outcome_of(WEB_SETUP_SUBMIT_INVALID_AP_SSID);
    case APP_V2_SETUP_INVALID_AP_PASSPHRASE:
        return outcome_of(WEB_SETUP_SUBMIT_INVALID_AP_PASSPHRASE);
    case APP_V2_SETUP_INVALID_ADMIN_PASSWORD:
        return outcome_of(WEB_SETUP_SUBMIT_INVALID_ADMIN_PASSWORD);
    case APP_V2_SETUP_INVALID_CURRENT_SETTINGS:
        return outcome_with_detail(WEB_SETUP_SUBMIT_INTERNAL, APP_ERROR_STORAGE_CORRUPT);
    case APP_V2_SETUP_OK:
    case APP_V2_SETUP_INVALID_ARGUMENT:
    case APP_V2_SETUP_RANDOM_FAILURE:
    case APP_V2_SETUP_INVALID_PASSWORD_MATERIAL:
    default:
        return outcome_of(WEB_SETUP_SUBMIT_INTERNAL);
    }
}

web_setup_submit_outcome_t web_setup_submit_handle(char *body, size_t body_capacity,
                                                   const web_setup_submit_ops_t *operations,
                                                   app_v2_setup_session_t *session,
                                                   app_v2_setup_accepted_t *out_response) {
    if (out_response != NULL) {
        memset(out_response, 0, sizeof(*out_response));
    }
    if (!operations_valid(operations)) {
        return outcome_of(WEB_SETUP_SUBMIT_INTERNAL);
    }
    if (body == NULL || body_capacity == 0U || session == NULL || out_response == NULL) {
        if (body != NULL && body_capacity > 0U) {
            operations->secure_zero(operations->context, body, body_capacity);
        }
        return outcome_of(WEB_SETUP_SUBMIT_INTERNAL);
    }

    size_t body_length = 0U;
    if (!bounded_body_length(body, body_capacity, &body_length) ||
        contains_embedded_nul_escape(body, body_length)) {
        operations->secure_zero(operations->context, body, body_capacity);
        return outcome_of(WEB_SETUP_SUBMIT_INVALID_BODY);
    }

    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts(body, body_length + 1U, &parse_end, true);
    const bool exact_document = root != NULL && parse_end == body + body_length;
    if (!exact_document) {
        operations->secure_zero(operations->context, body, body_capacity);
        if (root != NULL) {
            wipe_json_tree(operations, root);
            cJSON_Delete(root);
        }
        return outcome_of(WEB_SETUP_SUBMIT_INVALID_BODY);
    }

    app_v2_setup_request_t request = {0};
    if (!exact_setup_fields(root) || !populate_request(root, &request)) {
        operations->secure_zero(operations->context, body, body_capacity);
        wipe_json_tree(operations, root);
        cJSON_Delete(root);
        return outcome_of(WEB_SETUP_SUBMIT_INVALID_BODY);
    }
    operations->secure_zero(operations->context, body, body_capacity);

    app_v2_device_settings_t current = {0};
    const app_error_code_t settings_result =
        operations->settings_read(operations->context, &current);
    if (settings_result != APP_ERROR_NONE) {
        wipe_json_tree(operations, root);
        cJSON_Delete(root);
        return outcome_with_detail(WEB_SETUP_SUBMIT_SETTINGS_UNAVAILABLE, settings_result);
    }

    app_v2_setup_password_material_t material = {0};
    const app_error_code_t password_result = operations->password_create(
        operations->context, request.admin_password.data, request.admin_password.length, &material);
    if (password_result != APP_ERROR_NONE) {
        operations->secure_zero(operations->context, &material, sizeof(material));
        operations->secure_zero(operations->context, &current, sizeof(current));
        wipe_json_tree(operations, root);
        cJSON_Delete(root);
        return password_result == APP_ERROR_INVALID_ARGUMENT
                   ? outcome_of(WEB_SETUP_SUBMIT_INVALID_ADMIN_PASSWORD)
                   : outcome_with_detail(WEB_SETUP_SUBMIT_PASSWORD_DERIVATION_FAILED,
                                         password_result);
    }

    app_v2_device_settings_t candidate = {0};
    const app_v2_setup_result_t prepare_result =
        app_v2_setup_prepare_candidate(session, &request, &current, &material, &candidate);

    operations->secure_zero(operations->context, &material, sizeof(material));
    operations->secure_zero(operations->context, &current, sizeof(current));
    wipe_json_tree(operations, root);
    cJSON_Delete(root);

    if (prepare_result != APP_V2_SETUP_OK) {
        operations->secure_zero(operations->context, &candidate, sizeof(candidate));
        return map_prepare_result(prepare_result);
    }

    bool changed = false;
    const app_error_code_t commit_result =
        operations->settings_replace(operations->context, &candidate, &changed);
    operations->secure_zero(operations->context, &candidate, sizeof(candidate));
    if (commit_result != APP_ERROR_NONE) {
        /* The settings commit failed: the one-time setup code must remain
         * usable for a retry, so `session` is left untouched here. */
        return outcome_with_detail(WEB_SETUP_SUBMIT_COMMIT_FAILED, commit_result);
    }

    if (app_v2_setup_session_consume(session) != APP_V2_SETUP_OK) {
        return outcome_of(WEB_SETUP_SUBMIT_INTERNAL);
    }

    app_v2_setup_accepted_response_init(out_response);
    return outcome_of(WEB_SETUP_SUBMIT_OK);
}
