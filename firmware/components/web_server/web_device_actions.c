#include "web_device_actions.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "cJSON.h"
#include "device_controls.h"
#include "device_settings_v2.h"

#define DEVICE_ACTIONS_NUL_ESCAPE "\\u0000"
#define RESET_SETTINGS_CONFIRMATION "RESET SETTINGS"
#define FACTORY_RESET_CONFIRMATION "FACTORY RESET"

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
    const size_t escape_length = sizeof(DEVICE_ACTIONS_NUL_ESCAPE) - 1U;
    if (length < escape_length) {
        return false;
    }
    for (size_t index = 0U; index <= length - escape_length; ++index) {
        if (memcmp(body + index, DEVICE_ACTIONS_NUL_ESCAPE, escape_length) == 0) {
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

static bool string_field(const cJSON *root, const char *key, const char **out_value,
                         size_t *out_length) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (item == NULL || !cJSON_IsString(item) || item->valuestring == NULL) {
        return false;
    }
    *out_value = item->valuestring;
    *out_length = strlen(item->valuestring);
    return true;
}

static bool confirmation_matches(const char *supplied, size_t supplied_length,
                                 const char *expected) {
    const size_t expected_length = strlen(expected);
    return supplied_length == expected_length && memcmp(supplied, expected, expected_length) == 0;
}

/* -------------------------------------------------------------------------
 * POST /api/v1/device/restart
 * ---------------------------------------------------------------------- */

web_device_restart_outcome_t web_device_restart_handle(const web_device_actions_ops_t *ops) {
    if (ops == NULL || ops->restart == NULL) {
        return (web_device_restart_outcome_t){.result = WEB_DEVICE_RESTART_INTERNAL};
    }
    const app_error_code_t result = ops->restart(ops->context);
    if (result != APP_ERROR_NONE) {
        return (web_device_restart_outcome_t){
            .result = WEB_DEVICE_RESTART_BACKEND_UNAVAILABLE,
            .detail = result,
        };
    }
    return (web_device_restart_outcome_t){.result = WEB_DEVICE_RESTART_OK};
}

/* -------------------------------------------------------------------------
 * POST /api/v1/device/reset-settings
 * ---------------------------------------------------------------------- */

#define RESET_SETTINGS_FIELD_COUNT 1U

static bool exact_reset_settings_fields(const cJSON *root) {
    size_t field_count = 0U;
    for (const cJSON *item = root->child; item != NULL; item = item->next) {
        if (item->string == NULL || strcmp(item->string, "confirmation") != 0) {
            return false;
        }
        ++field_count;
    }
    return field_count == RESET_SETTINGS_FIELD_COUNT;
}

static web_device_reset_settings_outcome_t reset_settings_backend_outcome(
    device_controls_reset_settings_outcome_t reset) {
    if (!reset.settings_applied) {
        if (reset.sessions_invalidated || reset.restart_owned ||
            reset.primary_error == APP_ERROR_NONE || reset.restart_error != APP_ERROR_NONE) {
            return (web_device_reset_settings_outcome_t){
                .result = WEB_DEVICE_RESET_SETTINGS_INTERNAL,
                .detail = APP_ERROR_INTERNAL,
            };
        }
        return (web_device_reset_settings_outcome_t){
            .result = WEB_DEVICE_RESET_SETTINGS_BACKEND_UNAVAILABLE,
            .detail = reset.primary_error,
        };
    }

    if (!reset.restart_owned) {
        if (reset.restart_error == APP_ERROR_NONE ||
            (reset.sessions_invalidated && reset.primary_error != APP_ERROR_NONE) ||
            (!reset.sessions_invalidated && reset.primary_error == APP_ERROR_NONE)) {
            return (web_device_reset_settings_outcome_t){
                .result = WEB_DEVICE_RESET_SETTINGS_INTERNAL,
                .detail = APP_ERROR_INTERNAL,
            };
        }
        return (web_device_reset_settings_outcome_t){
            .result = WEB_DEVICE_RESET_SETTINGS_COMMITTED_RESTART_INCOMPLETE,
            .detail = reset.primary_error,
            .restart_detail = reset.restart_error,
        };
    }
    if (reset.restart_error != APP_ERROR_NONE) {
        return (web_device_reset_settings_outcome_t){
            .result = WEB_DEVICE_RESET_SETTINGS_INTERNAL,
            .detail = APP_ERROR_INTERNAL,
        };
    }

    if (!reset.sessions_invalidated) {
        if (reset.primary_error == APP_ERROR_NONE) {
            return (web_device_reset_settings_outcome_t){
                .result = WEB_DEVICE_RESET_SETTINGS_INTERNAL,
                .detail = APP_ERROR_INTERNAL,
            };
        }
        return (web_device_reset_settings_outcome_t){
            .result = WEB_DEVICE_RESET_SETTINGS_REBOOT_RECOVERY_REQUIRED,
            .detail = reset.primary_error,
        };
    }
    if (reset.primary_error != APP_ERROR_NONE) {
        return (web_device_reset_settings_outcome_t){
            .result = WEB_DEVICE_RESET_SETTINGS_INTERNAL,
            .detail = APP_ERROR_INTERNAL,
        };
    }
    return (web_device_reset_settings_outcome_t){.result = WEB_DEVICE_RESET_SETTINGS_OK};
}

web_device_reset_settings_outcome_t
web_device_reset_settings_handle(char *body, size_t body_capacity,
                                 const web_device_actions_ops_t *ops) {
    if (ops == NULL || ops->reset_settings == NULL || body == NULL || body_capacity == 0U) {
        if (body != NULL && body_capacity > 0U) {
            secure_zero_local(body, body_capacity);
        }
        return (web_device_reset_settings_outcome_t){.result = WEB_DEVICE_RESET_SETTINGS_INTERNAL};
    }

    cJSON *root = parse_exact_body(body, body_capacity);
    if (root == NULL) {
        return (web_device_reset_settings_outcome_t){.result =
                                                         WEB_DEVICE_RESET_SETTINGS_INVALID_BODY};
    }
    if (!exact_reset_settings_fields(root)) {
        cJSON_Delete(root);
        return (web_device_reset_settings_outcome_t){.result =
                                                         WEB_DEVICE_RESET_SETTINGS_INVALID_BODY};
    }

    const char *confirmation = NULL;
    size_t confirmation_length = 0U;
    if (!string_field(root, "confirmation", &confirmation, &confirmation_length)) {
        cJSON_Delete(root);
        return (web_device_reset_settings_outcome_t){.result =
                                                         WEB_DEVICE_RESET_SETTINGS_INVALID_BODY};
    }
    const bool matches =
        confirmation_matches(confirmation, confirmation_length, RESET_SETTINGS_CONFIRMATION);
    cJSON_Delete(root);
    if (!matches) {
        return (web_device_reset_settings_outcome_t){
            .result = WEB_DEVICE_RESET_SETTINGS_INVALID_CONFIRMATION};
    }

    const device_controls_reset_settings_outcome_t reset = ops->reset_settings(ops->context);
    return reset_settings_backend_outcome(reset);
}

/* -------------------------------------------------------------------------
 * POST /api/v1/device/factory-reset
 * ---------------------------------------------------------------------- */

#define FACTORY_RESET_FIELD_COUNT 2U

static bool exact_factory_reset_fields(const cJSON *root) {
    bool seen_password = false;
    bool seen_confirmation = false;
    size_t field_count = 0U;
    for (const cJSON *item = root->child; item != NULL; item = item->next) {
        if (item->string == NULL) {
            return false;
        }
        if (strcmp(item->string, "adminPassword") == 0 && !seen_password) {
            seen_password = true;
        } else if (strcmp(item->string, "confirmation") == 0 && !seen_confirmation) {
            seen_confirmation = true;
        } else {
            return false;
        }
        ++field_count;
    }
    return field_count == FACTORY_RESET_FIELD_COUNT && seen_password && seen_confirmation;
}

static web_device_factory_reset_outcome_t factory_reset_backend_outcome(
    device_controls_factory_reset_outcome_t reset) {
    if (!reset.durably_accepted) {
        if (reset.recovery_required || reset.primary_error == APP_ERROR_NONE) {
            return (web_device_factory_reset_outcome_t){
                .result = WEB_DEVICE_FACTORY_RESET_INTERNAL,
                .detail = APP_ERROR_INTERNAL,
            };
        }
        return (web_device_factory_reset_outcome_t){
            .result = WEB_DEVICE_FACTORY_RESET_BACKEND_UNAVAILABLE,
            .detail = reset.primary_error,
        };
    }
    if (reset.recovery_required) {
        if (reset.primary_error == APP_ERROR_NONE) {
            return (web_device_factory_reset_outcome_t){
                .result = WEB_DEVICE_FACTORY_RESET_INTERNAL,
                .detail = APP_ERROR_INTERNAL,
            };
        }
        return (web_device_factory_reset_outcome_t){
            .result = WEB_DEVICE_FACTORY_RESET_RECOVERY_REQUIRED,
            .detail = reset.primary_error,
        };
    }
    if (reset.primary_error != APP_ERROR_NONE) {
        return (web_device_factory_reset_outcome_t){
            .result = WEB_DEVICE_FACTORY_RESET_INTERNAL,
            .detail = APP_ERROR_INTERNAL,
        };
    }
    return (web_device_factory_reset_outcome_t){.result = WEB_DEVICE_FACTORY_RESET_OK};
}

web_device_factory_reset_outcome_t
web_device_factory_reset_handle(char *body, size_t body_capacity,
                                const web_device_actions_ops_t *ops) {
    if (ops == NULL || ops->settings_read == NULL || ops->password_verify == NULL ||
        ops->factory_reset == NULL || body == NULL || body_capacity == 0U) {
        if (body != NULL && body_capacity > 0U) {
            secure_zero_local(body, body_capacity);
        }
        return (web_device_factory_reset_outcome_t){.result = WEB_DEVICE_FACTORY_RESET_INTERNAL};
    }

    cJSON *root = parse_exact_body(body, body_capacity);
    if (root == NULL) {
        return (web_device_factory_reset_outcome_t){.result =
                                                        WEB_DEVICE_FACTORY_RESET_INVALID_BODY};
    }
    if (!exact_factory_reset_fields(root)) {
        wipe_and_delete_json_tree(root);
        return (web_device_factory_reset_outcome_t){.result =
                                                        WEB_DEVICE_FACTORY_RESET_INVALID_BODY};
    }

    const char *confirmation = NULL;
    size_t confirmation_length = 0U;
    const char *password = NULL;
    size_t password_length = 0U;
    if (!string_field(root, "confirmation", &confirmation, &confirmation_length) ||
        !string_field(root, "adminPassword", &password, &password_length)) {
        wipe_and_delete_json_tree(root);
        return (web_device_factory_reset_outcome_t){.result =
                                                        WEB_DEVICE_FACTORY_RESET_INVALID_BODY};
    }

    if (!confirmation_matches(confirmation, confirmation_length, FACTORY_RESET_CONFIRMATION)) {
        wipe_and_delete_json_tree(root);
        return (web_device_factory_reset_outcome_t){
            .result = WEB_DEVICE_FACTORY_RESET_INVALID_CONFIRMATION};
    }

    app_v2_device_settings_t current = {0};
    const app_error_code_t read_result = ops->settings_read(ops->context, &current);
    if (read_result != APP_ERROR_NONE) {
        wipe_and_delete_json_tree(root);
        return (web_device_factory_reset_outcome_t){
            .result = WEB_DEVICE_FACTORY_RESET_BACKEND_UNAVAILABLE,
            .detail = read_result,
        };
    }

    bool password_matches = false;
    const app_error_code_t verify_result =
        ops->password_verify(ops->context, password, password_length, &current, &password_matches);
    wipe_and_delete_json_tree(root);
    if (verify_result != APP_ERROR_NONE) {
        return (web_device_factory_reset_outcome_t){
            .result = WEB_DEVICE_FACTORY_RESET_BACKEND_UNAVAILABLE,
            .detail = verify_result,
        };
    }
    if (!password_matches) {
        return (web_device_factory_reset_outcome_t){
            .result = WEB_DEVICE_FACTORY_RESET_INCORRECT_PASSWORD};
    }

    const device_controls_factory_reset_outcome_t reset = ops->factory_reset(ops->context);
    return factory_reset_backend_outcome(reset);
}

/* -------------------------------------------------------------------------
 * Response JSON
 * ---------------------------------------------------------------------- */

static char *finish_json(cJSON *root) {
    if (root == NULL) {
        return NULL;
    }
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json;
}

app_error_code_t web_device_restart_accepted_json(char **out_json) {
    if (out_json != NULL) {
        *out_json = NULL;
    }
    if (out_json == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = cJSON_CreateObject();
    if (root == NULL || cJSON_AddBoolToObject(root, "accepted", true) == NULL ||
        cJSON_AddBoolToObject(root, "connectionWillClose", true) == NULL ||
        cJSON_AddBoolToObject(root, "reprovisioningRequired", false) == NULL) {
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    char *json = finish_json(root);
    if (json == NULL) {
        return APP_ERROR_INTERNAL;
    }
    *out_json = json;
    return APP_ERROR_NONE;
}

app_error_code_t web_device_reset_accepted_json(bool reprovisioning_required,
                                                bool repository_blobs_preserved, char **out_json) {
    if (out_json != NULL) {
        *out_json = NULL;
    }
    if (out_json == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = cJSON_CreateObject();
    if (root == NULL || cJSON_AddBoolToObject(root, "accepted", true) == NULL ||
        cJSON_AddBoolToObject(root, "connectionWillClose", true) == NULL ||
        cJSON_AddBoolToObject(root, "reprovisioningRequired", reprovisioning_required) == NULL ||
        cJSON_AddBoolToObject(root, "repositoryBlobsPreserved", repository_blobs_preserved) ==
            NULL) {
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    char *json = finish_json(root);
    if (json == NULL) {
        return APP_ERROR_INTERNAL;
    }
    *out_json = json;
    return APP_ERROR_NONE;
}
