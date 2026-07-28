#include "web_api_json.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "macro_limits.h"
#include "provisioning.h"
#include "storage_object_json.h"
#include "web_execution_submit.h"

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
    if (body == NULL || body_length == 0U || contains_embedded_nul_escape(body, body_length)) {
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
    if (!cJSON_IsObject(root) || fields == NULL || field_count == 0U || field_count > 8U) {
        return false;
    }
    bool seen[8U] = {false};
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

static bool read_uuid(const cJSON *root, const char *field, app_uuid_t *out_uuid) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, field);
    return cJSON_IsString(item) && item->valuestring != NULL &&
           app_uuid_parse(item->valuestring, out_uuid) == APP_ERROR_NONE;
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

app_error_code_t web_api_json_parse_resource_mutation(
    const char *body, size_t body_length, size_t maximum_resource_length,
    web_api_resource_mutation_t *out_mutation) {
    if (out_mutation != NULL) {
        memset(out_mutation, 0, sizeof(*out_mutation));
    }
    if (out_mutation == NULL || maximum_resource_length == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = parse_exact_document(body, body_length);
    static const char *const fields[] = {"expectedRevision", "resource"};
    if (root == NULL || !exact_fields(root, fields, 2U) ||
        !read_revision(root, "expectedRevision", &out_mutation->expected_revision)) {
        cJSON_Delete(root);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const cJSON *resource = cJSON_GetObjectItemCaseSensitive(root, "resource");
    if (!cJSON_IsObject(resource)) {
        cJSON_Delete(root);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char *serialized = cJSON_PrintUnformatted(resource);
    cJSON_Delete(root);
    if (serialized == NULL) {
        return APP_ERROR_INTERNAL;
    }
    const size_t length = strlen(serialized);
    if (length == 0U || length > maximum_resource_length) {
        cJSON_free(serialized);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    out_mutation->resource_json = serialized;
    out_mutation->resource_length = length;
    return APP_ERROR_NONE;
}

void web_api_json_free_resource_mutation(web_api_resource_mutation_t *mutation) {
    if (mutation == NULL) {
        return;
    }
    cJSON_free(mutation->resource_json);
    memset(mutation, 0, sizeof(*mutation));
}

app_error_code_t web_api_json_parse_uuid_order(const char *body, size_t body_length,
                                               size_t maximum_count,
                                               storage_uuid_order_t *out_order) {
    if (out_order != NULL) {
        memset(out_order, 0, sizeof(*out_order));
    }
    if (out_order == NULL || maximum_count == 0U || maximum_count > STORAGE_ORDER_MAX_IDS) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = parse_exact_document(body, body_length);
    static const char *const fields[] = {"ids"};
    const cJSON *ids = root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "ids");
    if (root == NULL || !exact_fields(root, fields, 1U) || !cJSON_IsArray(ids)) {
        cJSON_Delete(root);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const int count = cJSON_GetArraySize(ids);
    if (count < 0 || (size_t)count > maximum_count) {
        cJSON_Delete(root);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    for (int index = 0; index < count; ++index) {
        const cJSON *item = cJSON_GetArrayItem(ids, index);
        if (!cJSON_IsString(item) || item->valuestring == NULL ||
            app_uuid_parse(item->valuestring, &out_order->ids[(size_t)index]) != APP_ERROR_NONE) {
            memset(out_order, 0, sizeof(*out_order));
            cJSON_Delete(root);
            return APP_ERROR_INVALID_ARGUMENT;
        }
        for (int prior = 0; prior < index; ++prior) {
            if (app_uuid_equal(&out_order->ids[(size_t)prior],
                               &out_order->ids[(size_t)index])) {
                memset(out_order, 0, sizeof(*out_order));
                cJSON_Delete(root);
                return APP_ERROR_INVALID_ARGUMENT;
            }
        }
    }
    out_order->count = (size_t)count;
    cJSON_Delete(root);
    return APP_ERROR_NONE;
}

app_error_code_t web_api_json_parse_execution_submit(
    const char *body, size_t body_length, web_execution_submit_request_t *out_request) {
    if (out_request != NULL) {
        memset(out_request, 0, sizeof(*out_request));
    }
    if (out_request == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = parse_exact_document(body, body_length);
    static const char *const base_fields[] = {"setId", "macroId", "macroRevision"};
    static const char *const context_fields[] = {"setId", "macroId", "macroRevision",
                                                 "procedureId", "stepId"};
    const bool base = root != NULL && exact_fields(root, base_fields, 3U);
    const bool contextual = root != NULL && exact_fields(root, context_fields, 5U);
    if ((!base && !contextual) || !read_uuid(root, "setId", &out_request->set_id) ||
        !read_uuid(root, "macroId", &out_request->macro_id) ||
        !read_revision(root, "macroRevision", &out_request->macro_revision)) {
        cJSON_Delete(root);
        memset(out_request, 0, sizeof(*out_request));
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (contextual) {
        out_request->has_procedure_context = true;
        if (!read_uuid(root, "procedureId", &out_request->procedure_id) ||
            !read_uuid(root, "stepId", &out_request->step_id)) {
            cJSON_Delete(root);
            memset(out_request, 0, sizeof(*out_request));
            return APP_ERROR_INVALID_ARGUMENT;
        }
    }
    cJSON_Delete(root);
    return APP_ERROR_NONE;
}

app_error_code_t web_api_json_parse_progress_action(const char *body, size_t body_length,
                                                    bool confirmation_required,
                                                    web_api_progress_action_t *out_action) {
    if (out_action != NULL) {
        memset(out_action, 0, sizeof(*out_action));
    }
    if (out_action == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = parse_exact_document(body, body_length);
    static const char *const complete_fields[] = {"expectedProcedureRevision", "stepId"};
    static const char *const skip_fields[] = {"expectedProcedureRevision", "stepId", "confirmed"};
    const bool exact = root != NULL &&
                       exact_fields(root, confirmation_required ? skip_fields : complete_fields,
                                    confirmation_required ? 3U : 2U);
    if (!exact || !read_revision(root, "expectedProcedureRevision",
                                 &out_action->expected_procedure_revision) ||
        !read_uuid(root, "stepId", &out_action->step_id)) {
        cJSON_Delete(root);
        memset(out_action, 0, sizeof(*out_action));
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (confirmation_required) {
        const cJSON *confirmed = cJSON_GetObjectItemCaseSensitive(root, "confirmed");
        if (!cJSON_IsBool(confirmed) || !cJSON_IsTrue(confirmed)) {
            cJSON_Delete(root);
            memset(out_action, 0, sizeof(*out_action));
            return APP_ERROR_INVALID_ARGUMENT;
        }
        out_action->confirmed = true;
    }
    cJSON_Delete(root);
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
                                         "alwaysSelectSet", "activeSetId"};
    const cJSON *require_confirmation =
        root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "requirePhysicalConfirmation");
    const cJSON *always_select =
        root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "alwaysSelectSet");
    const cJSON *active_set =
        root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "activeSetId");
    if (root == NULL || !exact_fields(root, fields, 4U) ||
        !read_revision(root, "expectedRevision", out_expected_revision) ||
        !cJSON_IsBool(require_confirmation) || !cJSON_IsBool(always_select) ||
        (!cJSON_IsNull(active_set) && !cJSON_IsString(active_set))) {
        cJSON_Delete(root);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    out_settings->schema_version = APP_SCHEMA_VERSION;
    out_settings->revision = *out_expected_revision;
    out_settings->require_physical_confirmation = cJSON_IsTrue(require_confirmation);
    out_settings->always_select_set = cJSON_IsTrue(always_select);
    if (cJSON_IsString(active_set)) {
        if (active_set->valuestring == NULL ||
            app_uuid_parse(active_set->valuestring, &out_settings->active_set_id) != APP_ERROR_NONE) {
            cJSON_Delete(root);
            memset(out_settings, 0, sizeof(*out_settings));
            *out_expected_revision = 0U;
            return APP_ERROR_INVALID_ARGUMENT;
        }
        out_settings->has_active_set = true;
    }
    cJSON_Delete(root);
    return APP_ERROR_NONE;
}
