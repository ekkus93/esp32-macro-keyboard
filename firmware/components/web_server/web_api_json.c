#include "web_api_json.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "app_limits_v2.h"
#include "cJSON.h"
#include "macro_limits.h"
#include "provisioning.h"

#define WEB_API_JSON_MAX_FIELDS 4U

static bool contains_embedded_nul_escape(const char *body, size_t length) {
    static const char escape[] = "\\u0000";
    const size_t escape_length = sizeof(escape) - 1U;
    if (body == NULL || length < escape_length) {
        return false;
    }
    for (size_t index = 0U; index <= length - escape_length; ++index) {
        if (memcmp(body + index, escape, escape_length) == 0) {
            return true;
        }
    }
    return false;
}

static cJSON *parse_exact_document(const char *body, size_t body_length) {
    if (body == NULL || body_length == 0U ||
        body_length > (size_t)APP_V2_JSON_BODY_MAX_BYTES ||
        contains_embedded_nul_escape(body, body_length)) {
        return NULL;
    }
    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts(body, body_length, &parse_end, false);
    if (root == NULL || parse_end != body + body_length) {
        cJSON_Delete(root);
        return NULL;
    }
    return root;
}

static bool exact_fields(const cJSON *root, const char *const *fields, size_t field_count) {
    if (!cJSON_IsObject(root) || fields == NULL || field_count == 0U ||
        field_count > WEB_API_JSON_MAX_FIELDS) {
        return false;
    }
    bool seen[WEB_API_JSON_MAX_FIELDS] = {false};
    size_t actual_count = 0U;
    for (const cJSON *item = root->child; item != NULL; item = item->next) {
        if (item->string == NULL) {
            return false;
        }
        bool matched = false;
        for (size_t index = 0U; index < field_count; ++index) {
            if (strcmp(item->string, fields[index]) == 0) {
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
        ++actual_count;
    }
    if (actual_count != field_count) {
        return false;
    }
    for (size_t index = 0U; index < field_count; ++index) {
        if (!seen[index]) {
            return false;
        }
    }
    return true;
}

static bool read_revision(const cJSON *root, const char *field, uint32_t *out_revision) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, field);
    if (!cJSON_IsNumber(item) || item->valuedouble < 1.0 ||
        item->valuedouble > (double)UINT32_MAX || item->valuedouble != (double)item->valueint) {
        return false;
    }
    *out_revision = (uint32_t)item->valuedouble;
    return true;
}

app_error_code_t web_api_json_parse_expected_revision(const char *body, size_t body_length,
                                                      uint32_t *out_expected_revision) {
    if (out_expected_revision != NULL) {
        *out_expected_revision = 0U;
    }
    if (out_expected_revision == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = parse_exact_document(body, body_length);
    static const char *const fields[] = {"expectedRevision"};
    const bool valid = root != NULL && exact_fields(root, fields, 1U) &&
                       read_revision(root, "expectedRevision", out_expected_revision);
    cJSON_Delete(root);
    if (!valid) {
        *out_expected_revision = 0U;
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return APP_ERROR_NONE;
}

app_error_code_t web_api_json_parse_settings_update(const char *body, size_t body_length,
                                                    provisioning_settings_t *out_settings,
                                                    uint32_t *out_expected_revision) {
    if (out_settings != NULL) {
        memset(out_settings, 0, sizeof(*out_settings));
    }
    if (out_expected_revision != NULL) {
        *out_expected_revision = 0U;
    }
    if (out_settings == NULL || out_expected_revision == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = parse_exact_document(body, body_length);
    static const char *const fields[] = {"expectedRevision", "requirePhysicalConfirmation",
                                         "alwaysSelectPackage"};
    const cJSON *require_confirmation =
        root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "requirePhysicalConfirmation");
    const cJSON *always_select =
        root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "alwaysSelectPackage");
    uint32_t expected_revision = 0U;
    if (root == NULL || !exact_fields(root, fields, 3U) ||
        !read_revision(root, "expectedRevision", &expected_revision) ||
        !cJSON_IsBool(require_confirmation) || !cJSON_IsBool(always_select)) {
        cJSON_Delete(root);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    *out_settings = (provisioning_settings_t){
        .schema_version = APP_SCHEMA_VERSION,
        .revision = expected_revision,
        .require_physical_confirmation = cJSON_IsTrue(require_confirmation),
        .always_select_package = cJSON_IsTrue(always_select),
    };
    *out_expected_revision = expected_revision;
    cJSON_Delete(root);
    return APP_ERROR_NONE;
}
