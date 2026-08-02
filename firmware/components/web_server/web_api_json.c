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
#include "macro_model.h"
#include "provisioning.h"
#include "storage_object_json.h"
#include "web_execution_submit.h"

#define WEB_API_JSON_MAX_FIELDS 16U

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

static bool read_uuid(const cJSON *root, const char *field, app_uuid_t *out_uuid) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, field);
    return cJSON_IsString(item) && item->valuestring != NULL &&
           app_uuid_parse(item->valuestring, out_uuid) == APP_ERROR_NONE;
}

static bool read_execution_source_context(const cJSON *root,
                                          web_execution_submit_request_t *out_request) {
    static const char *const fields[] = {"procedureId", "stepId"};
    const cJSON *context = cJSON_GetObjectItemCaseSensitive(root, "sourceContext");
    if (!cJSON_IsObject(context) || !exact_fields(context, fields, 2U) ||
        !read_uuid(context, "procedureId", &out_request->procedure_id) ||
        !read_uuid(context, "stepId", &out_request->step_id)) {
        return false;
    }
    out_request->has_procedure_context = true;
    return true;
}

static app_error_code_t request_resource_result(app_error_code_t result) {
    return result == APP_ERROR_STORAGE_CORRUPT ? APP_ERROR_INVALID_ARGUMENT : result;
}

static app_error_code_t validate_resource_fields(const char *body, size_t body_length,
                                                 const char *const *fields, size_t field_count) {
    cJSON *root = parse_exact_document(body, body_length);
    const bool valid = root != NULL && exact_fields(root, fields, field_count);
    cJSON_Delete(root);
    return valid ? APP_ERROR_NONE : APP_ERROR_INVALID_ARGUMENT;
}

/* Every macro belongs to a set (SPEC §7.2), so `set_id` is required and there
 * is no scope discriminator. */
static app_error_code_t validate_macro_resource_fields(const char *body, size_t body_length) {
    static const char *const fields[] = {
        "schema_version", "id",     "revision",     "set_id",
        "name",           "source", "key_press_ms", "inter_key_ms",
    };
    return validate_resource_fields(body, body_length, fields, sizeof(fields) / sizeof(fields[0]));
}

app_error_code_t web_api_json_parse_set_resource(const char *body, size_t body_length,
                                                 macro_set_t *out_set) {
    if (out_set != NULL) {
        memset(out_set, 0, sizeof(*out_set));
    }
    if (out_set == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    static const char *const fields[] = {
        "schema_version",
        "id",
        "revision",
        "name",
    };
    app_error_code_t result =
        validate_resource_fields(body, body_length, fields, sizeof(fields) / sizeof(fields[0]));
    if (result == APP_ERROR_NONE) {
        result =
            request_resource_result(storage_repository_parse_set_json(body, body_length, out_set));
    }
    return result;
}

app_error_code_t web_api_json_parse_macro_resource(const char *body, size_t body_length,
                                                   macro_t *out_macro) {
    if (out_macro != NULL) {
        memset(out_macro, 0, sizeof(*out_macro));
    }
    if (out_macro == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = validate_macro_resource_fields(body, body_length);
    if (result == APP_ERROR_NONE) {
        result = request_resource_result(
            storage_repository_parse_macro_json(body, body_length, out_macro));
    }
    return result;
}

app_error_code_t web_api_json_parse_procedure_resource(const char *body, size_t body_length,
                                                       procedure_t *out_procedure) {
    if (out_procedure != NULL) {
        memset(out_procedure, 0, sizeof(*out_procedure));
    }
    if (out_procedure == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    static const char *const fields[] = {
        "schema_version", "id", "revision", "set_id", "name", "description", "steps", "sort_order",
    };
    app_error_code_t result =
        validate_resource_fields(body, body_length, fields, sizeof(fields) / sizeof(fields[0]));
    if (result == APP_ERROR_NONE) {
        result = request_resource_result(
            storage_repository_parse_procedure_json(body, body_length, out_procedure));
    }
    return result;
}

app_error_code_t web_api_json_parse_progress_resource(const char *body, size_t body_length,
                                                      procedure_progress_t *out_progress) {
    if (out_progress != NULL) {
        memset(out_progress, 0, sizeof(*out_progress));
    }
    if (out_progress == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    static const char *const fields[] = {
        "schema_version",     "set_id",          "procedure_id",
        "procedure_revision", "current_step_id", "completed_step_ids",
        "skipped_step_ids",
    };
    app_error_code_t result =
        validate_resource_fields(body, body_length, fields, sizeof(fields) / sizeof(fields[0]));
    if (result == APP_ERROR_NONE) {
        result = request_resource_result(
            storage_repository_parse_progress_json(body, body_length, out_progress));
    }
    return result;
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

app_error_code_t web_api_json_parse_resource_mutation(const char *body,
                                                      const web_api_resource_parse_limits_t *limits,
                                                      web_api_resource_mutation_t *out_mutation) {
    if (out_mutation != NULL) {
        memset(out_mutation, 0, sizeof(*out_mutation));
    }
    if (out_mutation == NULL || limits == NULL || limits->body_length == 0U ||
        limits->maximum_resource_length == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = parse_exact_document(body, limits->body_length);
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
    if (length == 0U || length > limits->maximum_resource_length) {
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

app_error_code_t web_api_json_parse_uuid_order(const char *body,
                                               const web_api_order_parse_limits_t *limits,
                                               storage_uuid_order_t *out_order) {
    if (out_order != NULL) {
        memset(out_order, 0, sizeof(*out_order));
    }
    if (out_order == NULL || limits == NULL || limits->body_length == 0U ||
        limits->maximum_count == 0U || limits->maximum_count > STORAGE_ORDER_MAX_IDS) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = parse_exact_document(body, limits->body_length);
    static const char *const fields[] = {"ids"};
    const cJSON *ids = root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "ids");
    if (root == NULL || !exact_fields(root, fields, 1U) || !cJSON_IsArray(ids)) {
        cJSON_Delete(root);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const int count = cJSON_GetArraySize(ids);
    if (count < 0 || (size_t)count > limits->maximum_count) {
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
            if (app_uuid_equal(&out_order->ids[(size_t)prior], &out_order->ids[(size_t)index])) {
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

app_error_code_t web_api_json_parse_execution_submit(const char *body, size_t body_length,
                                                     web_execution_submit_request_t *out_request) {
    if (out_request != NULL) {
        memset(out_request, 0, sizeof(*out_request));
    }
    if (out_request == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = parse_exact_document(body, body_length);
    static const char *const base_fields[] = {"setId", "macroId", "macroRevision"};
    static const char *const context_fields[] = {"setId", "macroId", "macroRevision",
                                                 "sourceContext"};
    const bool base = root != NULL && exact_fields(root, base_fields, 3U);
    const bool contextual = root != NULL && exact_fields(root, context_fields, 4U);
    if ((!base && !contextual) || !read_uuid(root, "setId", &out_request->set_id) ||
        !read_uuid(root, "macroId", &out_request->macro_id) ||
        !read_revision(root, "macroRevision", &out_request->macro_revision) ||
        (contextual && !read_execution_source_context(root, out_request))) {
        cJSON_Delete(root);
        memset(out_request, 0, sizeof(*out_request));
        return APP_ERROR_INVALID_ARGUMENT;
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
    const bool exact =
        root != NULL && exact_fields(root, confirmation_required ? skip_fields : complete_fields,
                                     confirmation_required ? 3U : 2U);
    if (!exact ||
        !read_revision(root, "expectedProcedureRevision",
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
    uint32_t expected_revision = 0U;
    if (root == NULL || !exact_fields(root, fields, 4U) ||
        !read_revision(root, "expectedRevision", &expected_revision) ||
        !cJSON_IsBool(require_confirmation) || !cJSON_IsBool(always_select) ||
        (!cJSON_IsNull(active_set) && !cJSON_IsString(active_set))) {
        cJSON_Delete(root);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    provisioning_settings_t settings = {
        .schema_version = APP_SCHEMA_VERSION,
        .revision = expected_revision,
        .require_physical_confirmation = cJSON_IsTrue(require_confirmation),
        .always_select_set = cJSON_IsTrue(always_select),
    };
    if (cJSON_IsString(active_set)) {
        if (active_set->valuestring == NULL ||
            app_uuid_parse(active_set->valuestring, &settings.active_set_id) != APP_ERROR_NONE) {
            cJSON_Delete(root);
            return APP_ERROR_INVALID_ARGUMENT;
        }
        settings.has_active_set = true;
    }
    cJSON_Delete(root);
    *out_settings = settings;
    *out_expected_revision = expected_revision;
    return APP_ERROR_NONE;
}
