#include "web_setup_json.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "app_error.h"
#include "cJSON.h"
#include "web_setup_core.h"

#define SETUP_FIELD_COUNT 6U

static const char *const SETUP_FIELDS[SETUP_FIELD_COUNT] = {
    "setupCode",
    "apSsid",
    "apPassphrase",
    "administratorPassword",
    "requirePhysicalConfirmation",
    "alwaysSelectSet",
};

static bool operations_valid(const web_setup_json_ops_t *operations) {
    return operations != NULL && operations->secure_zero != NULL;
}

static bool bounded_body_length(const char *body,
                                size_t capacity,
                                size_t *out_length) {
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
    static const char NUL_ESCAPE[] = "\\u0000";
    const size_t escape_length = sizeof(NUL_ESCAPE) - 1U;
    if (length < escape_length) {
        return false;
    }
    for (size_t index = 0U; index <= length - escape_length; ++index) {
        if (memcmp(body + index, NUL_ESCAPE, escape_length) == 0) {
            return true;
        }
    }
    return false;
}

static bool exact_setup_fields(const cJSON *root) {
    if (!cJSON_IsObject(root)) {
        return false;
    }
    bool seen[SETUP_FIELD_COUNT] = {false};
    size_t field_count = 0U;
    for (const cJSON *item = root->child; item != NULL; item = item->next) {
        if (item->string == NULL) {
            return false;
        }
        bool matched = false;
        for (size_t index = 0U; index < SETUP_FIELD_COUNT; ++index) {
            if (strcmp(item->string, SETUP_FIELDS[index]) == 0) {
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
    if (field_count != SETUP_FIELD_COUNT) {
        return false;
    }
    for (size_t index = 0U; index < SETUP_FIELD_COUNT; ++index) {
        if (!seen[index]) {
            return false;
        }
    }
    return true;
}

static bool copy_json_text(const cJSON *item,
                           char *output,
                           size_t output_size) {
    if (!cJSON_IsString(item) || item->valuestring == NULL || output == NULL ||
        output_size == 0U) {
        return false;
    }
    const size_t length = strlen(item->valuestring);
    if (length >= output_size) {
        return false;
    }
    memset(output, 0, output_size);
    memcpy(output, item->valuestring, length + 1U);
    return true;
}

static bool populate_submission(const cJSON *root,
                                web_setup_submission_t *submission) {
    const cJSON *setup_code =
        cJSON_GetObjectItemCaseSensitive(root, "setupCode");
    const cJSON *ap_ssid = cJSON_GetObjectItemCaseSensitive(root, "apSsid");
    const cJSON *ap_passphrase =
        cJSON_GetObjectItemCaseSensitive(root, "apPassphrase");
    const cJSON *administrator_password =
        cJSON_GetObjectItemCaseSensitive(root, "administratorPassword");
    const cJSON *require_confirmation =
        cJSON_GetObjectItemCaseSensitive(root, "requirePhysicalConfirmation");
    const cJSON *always_select =
        cJSON_GetObjectItemCaseSensitive(root, "alwaysSelectSet");
    if (!copy_json_text(setup_code,
                        submission->setup_code,
                        sizeof(submission->setup_code)) ||
        !copy_json_text(ap_ssid,
                        submission->ap_ssid,
                        sizeof(submission->ap_ssid)) ||
        !copy_json_text(ap_passphrase,
                        submission->ap_passphrase,
                        sizeof(submission->ap_passphrase)) ||
        !copy_json_text(administrator_password,
                        submission->administrator_password,
                        sizeof(submission->administrator_password)) ||
        !cJSON_IsBool(require_confirmation) || !cJSON_IsBool(always_select)) {
        return false;
    }
    submission->require_physical_confirmation =
        cJSON_IsTrue(require_confirmation);
    submission->always_select_set = cJSON_IsTrue(always_select);
    return true;
}

static void wipe_json_tree(const web_setup_json_ops_t *operations, cJSON *item) {
    for (cJSON *current = item; current != NULL; current = current->next) {
        if (current->string != NULL) {
            operations->secure_zero(operations->context,
                                    current->string,
                                    strlen(current->string) + 1U);
        }
        if (cJSON_IsString(current) && current->valuestring != NULL) {
            operations->secure_zero(operations->context,
                                    current->valuestring,
                                    strlen(current->valuestring) + 1U);
        }
        if (current->child != NULL) {
            wipe_json_tree(operations, current->child);
        }
    }
}

app_error_code_t web_setup_json_parse(
    char *body,
    size_t body_capacity,
    const web_setup_json_ops_t *operations,
    web_setup_submission_t *out_submission) {
    if (!operations_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (out_submission != NULL) {
        operations->secure_zero(operations->context,
                                out_submission,
                                sizeof(*out_submission));
    }
    if (body == NULL || body_capacity == 0U || out_submission == NULL) {
        if (body != NULL && body_capacity > 0U) {
            operations->secure_zero(operations->context, body, body_capacity);
        }
        return APP_ERROR_INVALID_ARGUMENT;
    }

    size_t body_length = 0U;
    if (!bounded_body_length(body, body_capacity, &body_length) ||
        contains_embedded_nul_escape(body, body_length)) {
        operations->secure_zero(operations->context, body, body_capacity);
        return APP_ERROR_INVALID_ARGUMENT;
    }

    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts(
        body, body_length + 1U, &parse_end, true);
    const bool exact_document = root != NULL && parse_end == body + body_length;
    operations->secure_zero(operations->context, body, body_capacity);
    if (!exact_document) {
        if (root != NULL) {
            wipe_json_tree(operations, root);
            cJSON_Delete(root);
        }
        return APP_ERROR_INVALID_ARGUMENT;
    }

    const bool valid = exact_setup_fields(root) &&
                       populate_submission(root, out_submission);
    wipe_json_tree(operations, root);
    cJSON_Delete(root);
    if (!valid) {
        operations->secure_zero(operations->context,
                                out_submission,
                                sizeof(*out_submission));
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return APP_ERROR_NONE;
}
