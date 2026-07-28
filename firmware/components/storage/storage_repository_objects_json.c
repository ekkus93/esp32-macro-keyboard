#include "storage_repository_objects_json.h"

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
#include "storage_json.h"

#define MACRO_FIELD_COUNT 10U
#define MACRO_REQUIRED_FIELD_COUNT 9U
#define PROCEDURE_FIELD_COUNT 8U
#define PROGRESS_FIELD_COUNT 7U
#define ORDER_FIELD_COUNT 2U
#define MACRO_STEP_FIELD_COUNT 6U
#define MANUAL_STEP_FIELD_COUNT 5U
#define KEY_PRESS_STORAGE_MAX_MS 1000U
#define MACRO_SCOPE_BUFFER_BYTES sizeof("global")
#define PROCEDURE_STEP_TYPE_BUFFER_BYTES sizeof("instruction")

static const char *const MACRO_FIELDS[MACRO_FIELD_COUNT] = {
    "schema_version", "id",       "revision",     "scope",        "name",
    "source",         "favorite", "key_press_ms", "inter_key_ms", "set_id",
};
static const char *const PROCEDURE_FIELDS[PROCEDURE_FIELD_COUNT] = {
    "schema_version", "id", "revision", "set_id", "name", "description", "steps", "sort_order",
};
static const char *const PROGRESS_FIELDS[PROGRESS_FIELD_COUNT] = {
    "schema_version",     "set_id",          "procedure_id",
    "procedure_revision", "current_step_id", "completed_step_ids",
    "skipped_step_ids",
};
static const char *const ORDER_FIELDS[ORDER_FIELD_COUNT] = {"schema_version", "ids"};
static const char *const MACRO_STEP_FIELDS[MACRO_STEP_FIELD_COUNT] = {
    "id", "type", "title", "macro_id", "required", "auto_complete_on_success",
};
static const char *const MANUAL_STEP_FIELDS[MANUAL_STEP_FIELD_COUNT] = {
    "id", "type", "title", "body", "required",
};

static bool uuid_zero(const app_uuid_t *value) {
    static const app_uuid_t zero = {0};
    return memcmp(value, &zero, sizeof(*value)) == 0;
}

static bool canonical_buffer(const char *value, size_t capacity, size_t *out_length) {
    if (value == NULL || capacity == 0U || out_length == NULL) {
        return false;
    }
    const void *terminator = memchr(value, '\0', capacity);
    if (terminator == NULL) {
        return false;
    }
    const size_t length = (size_t)((const char *)terminator - value);
    for (size_t index = length + 1U; index < capacity; ++index) {
        if (value[index] != '\0') {
            return false;
        }
    }
    *out_length = length;
    return true;
}

static bool macro_shape_valid(const macro_t *macro) {
    if (macro == NULL || macro->schema_version != APP_SCHEMA_VERSION || macro->revision == 0U ||
        !app_uuid_is_valid_string(macro->id.value) || macro->source == NULL ||
        macro->source_length > APP_MACRO_SOURCE_MAX_BYTES ||
        macro->source[macro->source_length] != '\0' ||
        strlen(macro->source) != macro->source_length || macro->key_press_ms == 0U ||
        macro->key_press_ms > KEY_PRESS_STORAGE_MAX_MS || macro->inter_key_ms > APP_DELAY_MAX_MS) {
        return false;
    }
    size_t name_length = 0U;
    if (!canonical_buffer(macro->name, sizeof(macro->name), &name_length) || name_length == 0U) {
        return false;
    }
    if (macro->scope == MACRO_SCOPE_SET) {
        return macro->has_set_id && app_uuid_is_valid_string(macro->set_id.value);
    }
    return macro->scope == MACRO_SCOPE_GLOBAL && !macro->has_set_id && uuid_zero(&macro->set_id);
}

static app_error_code_t parse_scope(const cJSON *root, macro_t *out_macro) {
    char scope[MACRO_SCOPE_BUFFER_BYTES];
    app_error_code_t result = storage_json_get_string(root, "scope", scope, sizeof(scope), true);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (strcmp(scope, "set") == 0) {
        if (!storage_json_has_field(root, "set_id")) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
        out_macro->scope = MACRO_SCOPE_SET;
        out_macro->has_set_id = true;
        return storage_json_get_uuid(root, "set_id", &out_macro->set_id);
    }
    if (strcmp(scope, "global") != 0 || storage_json_has_field(root, "set_id")) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    out_macro->scope = MACRO_SCOPE_GLOBAL;
    return APP_ERROR_NONE;
}

app_error_code_t storage_repository_parse_macro_json(const char *data, size_t length,
                                                     macro_t *out_macro) {
    if (out_macro != NULL) {
        memset(out_macro, 0, sizeof(*out_macro));
    }
    if (data == NULL || out_macro == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = NULL;
    app_error_code_t result = storage_json_parse_object_fields(
        data, length, MACRO_FIELDS, MACRO_FIELD_COUNT, MACRO_REQUIRED_FIELD_COUNT, &root);
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(root, "schema_version", APP_SCHEMA_VERSION,
                                      APP_SCHEMA_VERSION, &out_macro->schema_version);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_uuid(root, "id", &out_macro->id);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(root, "revision", 1U, UINT32_MAX, &out_macro->revision);
    }
    if (result == APP_ERROR_NONE) {
        result = parse_scope(root, out_macro);
    }
    if (result == APP_ERROR_NONE) {
        result =
            storage_json_get_string(root, "name", out_macro->name, sizeof(out_macro->name), true);
    }
    if (result == APP_ERROR_NONE) {
        result =
            storage_json_get_allocated_string(root, "source", APP_MACRO_SOURCE_MAX_BYTES, false,
                                              &out_macro->source, &out_macro->source_length);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_bool(root, "favorite", &out_macro->favorite);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(root, "key_press_ms", 1U, KEY_PRESS_STORAGE_MAX_MS,
                                      &out_macro->key_press_ms);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(root, "inter_key_ms", 0U, APP_DELAY_MAX_MS,
                                      &out_macro->inter_key_ms);
    }
    cJSON_Delete(root);
    if (result != APP_ERROR_NONE || !macro_shape_valid(out_macro)) {
        macro_model_free_macro(out_macro);
        memset(out_macro, 0, sizeof(*out_macro));
        return result == APP_ERROR_NONE ? APP_ERROR_STORAGE_CORRUPT : result;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t add_macro_fields(cJSON *root, const macro_t *macro) {
    const char *scope = macro->scope == MACRO_SCOPE_SET ? "set" : "global";
    if (cJSON_AddNumberToObject(root, "schema_version", (double)macro->schema_version) == NULL ||
        cJSON_AddStringToObject(root, "id", macro->id.value) == NULL ||
        cJSON_AddNumberToObject(root, "revision", (double)macro->revision) == NULL ||
        cJSON_AddStringToObject(root, "scope", scope) == NULL) {
        return APP_ERROR_INTERNAL;
    }
    if (macro->scope == MACRO_SCOPE_SET &&
        cJSON_AddStringToObject(root, "set_id", macro->set_id.value) == NULL) {
        return APP_ERROR_INTERNAL;
    }
    return cJSON_AddStringToObject(root, "name", macro->name) != NULL &&
                   cJSON_AddStringToObject(root, "source", macro->source) != NULL &&
                   cJSON_AddBoolToObject(root, "favorite", macro->favorite) != NULL &&
                   cJSON_AddNumberToObject(root, "key_press_ms", (double)macro->key_press_ms) !=
                       NULL &&
                   cJSON_AddNumberToObject(root, "inter_key_ms", (double)macro->inter_key_ms) !=
                       NULL
               ? APP_ERROR_NONE
               : APP_ERROR_INTERNAL;
}

static app_error_code_t print_bounded_json(cJSON *root, size_t maximum, char **out_json,
                                           size_t *out_length) {
    char *json = cJSON_PrintUnformatted(root);
    if (json == NULL) {
        return APP_ERROR_INTERNAL;
    }
    const size_t length = strlen(json);
    if (length == 0U || length > maximum) {
        cJSON_free(json);
        return APP_ERROR_MACRO_LIMIT;
    }
    *out_json = json;
    *out_length = length;
    return APP_ERROR_NONE;
}

app_error_code_t storage_repository_serialize_macro_json(const macro_t *macro, char **out_json,
                                                         size_t *out_length) {
    if (out_json != NULL) {
        *out_json = NULL;
    }
    if (out_length != NULL) {
        *out_length = 0U;
    }
    if (!macro_shape_valid(macro) || out_json == NULL || out_length == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        return APP_ERROR_INTERNAL;
    }
    app_error_code_t result = add_macro_fields(root, macro);
    if (result == APP_ERROR_NONE) {
        result = print_bounded_json(root, STORAGE_MACRO_FILE_MAX_BYTES, out_json, out_length);
    }
    cJSON_Delete(root);
    return result;
}

static app_error_code_t exact_child_fields(const cJSON *object, const char *const *fields,
                                           size_t field_count) {
    uint64_t seen = UINT64_C(0);
    size_t observed = 0U;
    for (const cJSON *item = object->child; item != NULL; item = item->next) {
        bool matched = false;
        for (size_t index = 0U; index < field_count; ++index) {
            if (item->string != NULL && strcmp(item->string, fields[index]) == 0) {
                const uint64_t bit = UINT64_C(1) << index;
                if ((seen & bit) != 0U) {
                    return APP_ERROR_STORAGE_CORRUPT;
                }
                seen |= bit;
                ++observed;
                matched = true;
                break;
            }
        }
        if (!matched) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
    }
    return observed == field_count ? APP_ERROR_NONE : APP_ERROR_STORAGE_CORRUPT;
}

static app_error_code_t parse_step_type(const cJSON *object, procedure_step_t *out_step) {
    char type[PROCEDURE_STEP_TYPE_BUFFER_BYTES];
    app_error_code_t result = storage_json_get_string(object, "type", type, sizeof(type), true);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (strcmp(type, "macro") == 0) {
        result = exact_child_fields(object, MACRO_STEP_FIELDS, MACRO_STEP_FIELD_COUNT);
        if (result == APP_ERROR_NONE) {
            out_step->type = PROCEDURE_STEP_MACRO;
            out_step->has_macro_id = true;
            result = storage_json_get_uuid(object, "macro_id", &out_step->macro_id);
        }
        if (result == APP_ERROR_NONE) {
            result = storage_json_get_bool(object, "auto_complete_on_success",
                                           &out_step->auto_complete_on_success);
        }
        return result;
    }
    result = exact_child_fields(object, MANUAL_STEP_FIELDS, MANUAL_STEP_FIELD_COUNT);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (strcmp(type, "instruction") == 0) {
        out_step->type = PROCEDURE_STEP_INSTRUCTION;
    } else if (strcmp(type, "checkpoint") == 0) {
        out_step->type = PROCEDURE_STEP_CHECKPOINT;
    } else {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return storage_json_get_allocated_string(object, "body", APP_STEP_BODY_MAX_BYTES, false,
                                             &out_step->body, &out_step->body_length);
}

static app_error_code_t parse_step(const cJSON *item, procedure_step_t *out_step) {
    if (!cJSON_IsObject(item)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    app_error_code_t result = storage_json_get_uuid(item, "id", &out_step->id);
    if (result == APP_ERROR_NONE) {
        result =
            storage_json_get_string(item, "title", out_step->title, sizeof(out_step->title), true);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_bool(item, "required", &out_step->required);
    }
    if (result == APP_ERROR_NONE) {
        result = parse_step_type(item, out_step);
    }
    return result;
}

static bool procedure_step_ids_unique(const procedure_t *procedure) {
    for (size_t index = 0U; index < procedure->step_count; ++index) {
        for (size_t prior = 0U; prior < index; ++prior) {
            if (app_uuid_equal(&procedure->steps[prior].id, &procedure->steps[index].id)) {
                return false;
            }
        }
    }
    return true;
}

static app_error_code_t parse_steps(const cJSON *root, procedure_t *out_procedure) {
    const cJSON *steps = cJSON_GetObjectItemCaseSensitive(root, "steps");
    if (!cJSON_IsArray(steps)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    const int count = cJSON_GetArraySize(steps);
    if (count <= 0 || count > (int)APP_STEPS_PER_PROCEDURE_MAX) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    out_procedure->steps = calloc((size_t)count, sizeof(*out_procedure->steps));
    if (out_procedure->steps == NULL) {
        return APP_ERROR_INTERNAL;
    }
    out_procedure->step_count = (size_t)count;
    for (int index = 0; index < count; ++index) {
        app_error_code_t result =
            parse_step(cJSON_GetArrayItem(steps, index), &out_procedure->steps[(size_t)index]);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    return procedure_step_ids_unique(out_procedure) ? APP_ERROR_NONE : APP_ERROR_STORAGE_CORRUPT;
}

static bool procedure_shape_valid(const procedure_t *procedure) {
    if (procedure == NULL || procedure->schema_version != APP_SCHEMA_VERSION ||
        procedure->revision == 0U || !app_uuid_is_valid_string(procedure->id.value) ||
        !app_uuid_is_valid_string(procedure->set_id.value) || procedure->step_count == 0U ||
        procedure->step_count > APP_STEPS_PER_PROCEDURE_MAX || procedure->steps == NULL ||
        !procedure_step_ids_unique(procedure)) {
        return false;
    }
    size_t name_length = 0U;
    size_t description_length = 0U;
    if (!canonical_buffer(procedure->name, sizeof(procedure->name), &name_length) ||
        name_length == 0U ||
        !canonical_buffer(procedure->description, sizeof(procedure->description),
                          &description_length)) {
        return false;
    }
    for (size_t index = 0U; index < procedure->step_count; ++index) {
        const procedure_step_t *step = &procedure->steps[index];
        size_t title_length = 0U;
        if (!app_uuid_is_valid_string(step->id.value) ||
            !canonical_buffer(step->title, sizeof(step->title), &title_length) ||
            title_length == 0U) {
            return false;
        }
        if (step->type == PROCEDURE_STEP_MACRO) {
            if (!step->has_macro_id || !app_uuid_is_valid_string(step->macro_id.value) ||
                step->body != NULL || step->body_length != 0U) {
                return false;
            }
        } else if (step->type == PROCEDURE_STEP_INSTRUCTION ||
                   step->type == PROCEDURE_STEP_CHECKPOINT) {
            if (step->has_macro_id || !uuid_zero(&step->macro_id) || step->body == NULL ||
                step->body_length > APP_STEP_BODY_MAX_BYTES ||
                step->body[step->body_length] != '\0' || strlen(step->body) != step->body_length ||
                step->auto_complete_on_success) {
                return false;
            }
        } else {
            return false;
        }
    }
    return true;
}

app_error_code_t storage_repository_parse_procedure_json(const char *data, size_t length,
                                                         procedure_t *out_procedure) {
    if (out_procedure != NULL) {
        memset(out_procedure, 0, sizeof(*out_procedure));
    }
    if (data == NULL || out_procedure == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = NULL;
    app_error_code_t result = storage_json_parse_exact_object(data, length, PROCEDURE_FIELDS,
                                                              PROCEDURE_FIELD_COUNT, &root);
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(root, "schema_version", APP_SCHEMA_VERSION,
                                      APP_SCHEMA_VERSION, &out_procedure->schema_version);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_uuid(root, "id", &out_procedure->id);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(root, "revision", 1U, UINT32_MAX, &out_procedure->revision);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_uuid(root, "set_id", &out_procedure->set_id);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_string(root, "name", out_procedure->name,
                                         sizeof(out_procedure->name), true);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_string(root, "description", out_procedure->description,
                                         sizeof(out_procedure->description), false);
    }
    if (result == APP_ERROR_NONE) {
        result = parse_steps(root, out_procedure);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_i32(root, "sort_order", &out_procedure->sort_order);
    }
    cJSON_Delete(root);
    if (result != APP_ERROR_NONE || !procedure_shape_valid(out_procedure)) {
        macro_model_free_procedure(out_procedure);
        memset(out_procedure, 0, sizeof(*out_procedure));
        return result == APP_ERROR_NONE ? APP_ERROR_STORAGE_CORRUPT : result;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t add_step(cJSON *steps, const procedure_step_t *step) {
    cJSON *object = cJSON_CreateObject();
    if (object == NULL || cJSON_AddStringToObject(object, "id", step->id.value) == NULL) {
        cJSON_Delete(object);
        return APP_ERROR_INTERNAL;
    }
    const char *type = NULL;
    if (step->type == PROCEDURE_STEP_MACRO) {
        type = "macro";
    } else if (step->type == PROCEDURE_STEP_INSTRUCTION) {
        type = "instruction";
    } else {
        type = "checkpoint";
    }
    bool added = cJSON_AddStringToObject(object, "type", type) != NULL &&
                 cJSON_AddStringToObject(object, "title", step->title) != NULL;
    if (step->type == PROCEDURE_STEP_MACRO) {
        added = added &&
                cJSON_AddStringToObject(object, "macro_id", step->macro_id.value) != NULL &&
                cJSON_AddBoolToObject(object, "required", step->required) != NULL &&
                cJSON_AddBoolToObject(object, "auto_complete_on_success",
                                      step->auto_complete_on_success) != NULL;
    } else {
        added = added && cJSON_AddStringToObject(object, "body", step->body) != NULL &&
                cJSON_AddBoolToObject(object, "required", step->required) != NULL;
    }
    if (!added || !cJSON_AddItemToArray(steps, object)) {
        cJSON_Delete(object);
        return APP_ERROR_INTERNAL;
    }
    return APP_ERROR_NONE;
}

app_error_code_t storage_repository_serialize_procedure_json(const procedure_t *procedure,
                                                             char **out_json, size_t *out_length) {
    if (out_json != NULL) {
        *out_json = NULL;
    }
    if (out_length != NULL) {
        *out_length = 0U;
    }
    if (!procedure_shape_valid(procedure) || out_json == NULL || out_length == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = cJSON_CreateObject();
    cJSON *steps = cJSON_CreateArray();
    if (root == NULL || steps == NULL ||
        cJSON_AddNumberToObject(root, "schema_version", (double)procedure->schema_version) ==
            NULL ||
        cJSON_AddStringToObject(root, "id", procedure->id.value) == NULL ||
        cJSON_AddNumberToObject(root, "revision", (double)procedure->revision) == NULL ||
        cJSON_AddStringToObject(root, "set_id", procedure->set_id.value) == NULL ||
        cJSON_AddStringToObject(root, "name", procedure->name) == NULL ||
        cJSON_AddStringToObject(root, "description", procedure->description) == NULL ||
        !cJSON_AddItemToObject(root, "steps", steps)) {
        cJSON_Delete(steps);
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    app_error_code_t result = APP_ERROR_NONE;
    for (size_t index = 0U; index < procedure->step_count && result == APP_ERROR_NONE; ++index) {
        result = add_step(steps, &procedure->steps[index]);
    }
    if (result == APP_ERROR_NONE &&
        cJSON_AddNumberToObject(root, "sort_order", (double)procedure->sort_order) == NULL) {
        result = APP_ERROR_INTERNAL;
    }
    if (result == APP_ERROR_NONE) {
        result = print_bounded_json(root, STORAGE_PROCEDURE_FILE_MAX_BYTES, out_json, out_length);
    }
    cJSON_Delete(root);
    return result;
}

static app_error_code_t parse_uuid_array(const cJSON *root, const char *name, app_uuid_t *output,
                                         size_t *out_count) {
    const cJSON *array = cJSON_GetObjectItemCaseSensitive(root, name);
    if (!cJSON_IsArray(array)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    const int count = cJSON_GetArraySize(array);
    if (count < 0 || count > (int)APP_STEPS_PER_PROCEDURE_MAX) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    for (int index = 0; index < count; ++index) {
        const cJSON *item = cJSON_GetArrayItem(array, index);
        if (!cJSON_IsString(item) || item->valuestring == NULL ||
            app_uuid_parse(item->valuestring, &output[(size_t)index]) != APP_ERROR_NONE) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
        for (int prior = 0; prior < index; ++prior) {
            if (app_uuid_equal(&output[(size_t)prior], &output[(size_t)index])) {
                return APP_ERROR_STORAGE_CORRUPT;
            }
        }
    }
    *out_count = (size_t)count;
    return APP_ERROR_NONE;
}

static bool progress_shape_valid(const procedure_progress_t *progress) {
    if (progress == NULL || progress->schema_version != APP_SCHEMA_VERSION ||
        progress->procedure_revision == 0U || !app_uuid_is_valid_string(progress->set_id.value) ||
        !app_uuid_is_valid_string(progress->procedure_id.value) ||
        !app_uuid_is_valid_string(progress->current_step_id.value) ||
        progress->completed_step_count > APP_STEPS_PER_PROCEDURE_MAX ||
        progress->skipped_step_count > APP_STEPS_PER_PROCEDURE_MAX) {
        return false;
    }
    for (size_t index = 0U; index < progress->completed_step_count; ++index) {
        if (!app_uuid_is_valid_string(progress->completed_step_ids[index].value)) {
            return false;
        }
        for (size_t prior = 0U; prior < index; ++prior) {
            if (app_uuid_equal(&progress->completed_step_ids[prior],
                               &progress->completed_step_ids[index])) {
                return false;
            }
        }
        for (size_t skipped = 0U; skipped < progress->skipped_step_count; ++skipped) {
            if (app_uuid_equal(&progress->completed_step_ids[index],
                               &progress->skipped_step_ids[skipped])) {
                return false;
            }
        }
    }
    for (size_t index = 0U; index < progress->skipped_step_count; ++index) {
        if (!app_uuid_is_valid_string(progress->skipped_step_ids[index].value)) {
            return false;
        }
        for (size_t prior = 0U; prior < index; ++prior) {
            if (app_uuid_equal(&progress->skipped_step_ids[prior],
                               &progress->skipped_step_ids[index])) {
                return false;
            }
        }
    }
    return true;
}

app_error_code_t storage_repository_parse_progress_json(const char *data, size_t length,
                                                        procedure_progress_t *out_progress) {
    if (out_progress != NULL) {
        memset(out_progress, 0, sizeof(*out_progress));
    }
    if (data == NULL || out_progress == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = NULL;
    app_error_code_t result =
        storage_json_parse_exact_object(data, length, PROGRESS_FIELDS, PROGRESS_FIELD_COUNT, &root);
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(root, "schema_version", APP_SCHEMA_VERSION,
                                      APP_SCHEMA_VERSION, &out_progress->schema_version);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_uuid(root, "set_id", &out_progress->set_id);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_uuid(root, "procedure_id", &out_progress->procedure_id);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(root, "procedure_revision", 1U, UINT32_MAX,
                                      &out_progress->procedure_revision);
    }
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_uuid(root, "current_step_id", &out_progress->current_step_id);
    }
    if (result == APP_ERROR_NONE) {
        result = parse_uuid_array(root, "completed_step_ids", out_progress->completed_step_ids,
                                  &out_progress->completed_step_count);
    }
    if (result == APP_ERROR_NONE) {
        result = parse_uuid_array(root, "skipped_step_ids", out_progress->skipped_step_ids,
                                  &out_progress->skipped_step_count);
    }
    cJSON_Delete(root);
    if (result != APP_ERROR_NONE || !progress_shape_valid(out_progress)) {
        memset(out_progress, 0, sizeof(*out_progress));
        return result == APP_ERROR_NONE ? APP_ERROR_STORAGE_CORRUPT : result;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t add_uuid_array(cJSON *root, const char *name, const app_uuid_t *items,
                                       size_t count) {
    cJSON *array = cJSON_CreateArray();
    if (array == NULL || !cJSON_AddItemToObject(root, name, array)) {
        cJSON_Delete(array);
        return APP_ERROR_INTERNAL;
    }
    for (size_t index = 0U; index < count; ++index) {
        cJSON *item = cJSON_CreateString(items[index].value);
        if (item == NULL || !cJSON_AddItemToArray(array, item)) {
            cJSON_Delete(item);
            return APP_ERROR_INTERNAL;
        }
    }
    return APP_ERROR_NONE;
}

app_error_code_t storage_repository_serialize_progress_json(const procedure_progress_t *progress,
                                                            char **out_json, size_t *out_length) {
    if (out_json != NULL) {
        *out_json = NULL;
    }
    if (out_length != NULL) {
        *out_length = 0U;
    }
    if (!progress_shape_valid(progress) || out_json == NULL || out_length == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = cJSON_CreateObject();
    if (root == NULL ||
        cJSON_AddNumberToObject(root, "schema_version", (double)progress->schema_version) == NULL ||
        cJSON_AddStringToObject(root, "set_id", progress->set_id.value) == NULL ||
        cJSON_AddStringToObject(root, "procedure_id", progress->procedure_id.value) == NULL ||
        cJSON_AddNumberToObject(root, "procedure_revision", (double)progress->procedure_revision) ==
            NULL ||
        cJSON_AddStringToObject(root, "current_step_id", progress->current_step_id.value) == NULL) {
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    app_error_code_t result = add_uuid_array(
        root, "completed_step_ids", progress->completed_step_ids, progress->completed_step_count);
    if (result == APP_ERROR_NONE) {
        result = add_uuid_array(root, "skipped_step_ids", progress->skipped_step_ids,
                                progress->skipped_step_count);
    }
    if (result == APP_ERROR_NONE) {
        result = print_bounded_json(root, STORAGE_PROGRESS_FILE_MAX_BYTES, out_json, out_length);
    }
    cJSON_Delete(root);
    return result;
}

app_error_code_t storage_repository_parse_order_json(const char *data, size_t length,
                                                     storage_uuid_order_t *out_order,
                                                     size_t maximum_count) {
    if (out_order != NULL) {
        memset(out_order, 0, sizeof(*out_order));
    }
    if (data == NULL || out_order == NULL || maximum_count > STORAGE_ORDER_MAX_IDS) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = NULL;
    app_error_code_t result =
        storage_json_parse_exact_object(data, length, ORDER_FIELDS, ORDER_FIELD_COUNT, &root);
    uint32_t schema_version = 0U;
    if (result == APP_ERROR_NONE) {
        result = storage_json_get_u32(root, "schema_version", APP_SCHEMA_VERSION,
                                      APP_SCHEMA_VERSION, &schema_version);
    }
    const cJSON *ids = root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "ids");
    if (result == APP_ERROR_NONE && !cJSON_IsArray(ids)) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    const int count = result == APP_ERROR_NONE ? cJSON_GetArraySize(ids) : -1;
    if (result == APP_ERROR_NONE && (count < 0 || (size_t)count > maximum_count)) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    for (int index = 0; result == APP_ERROR_NONE && index < count; ++index) {
        const cJSON *item = cJSON_GetArrayItem(ids, index);
        if (!cJSON_IsString(item) || item->valuestring == NULL ||
            app_uuid_parse(item->valuestring, &out_order->ids[(size_t)index]) != APP_ERROR_NONE) {
            result = APP_ERROR_STORAGE_CORRUPT;
            break;
        }
        for (int prior = 0; prior < index; ++prior) {
            if (app_uuid_equal(&out_order->ids[(size_t)prior], &out_order->ids[(size_t)index])) {
                result = APP_ERROR_STORAGE_CORRUPT;
                break;
            }
        }
    }
    if (result == APP_ERROR_NONE) {
        out_order->count = (size_t)count;
    }
    cJSON_Delete(root);
    if (result != APP_ERROR_NONE) {
        memset(out_order, 0, sizeof(*out_order));
    }
    return result;
}

app_error_code_t storage_repository_serialize_order_json(const storage_uuid_order_t *order,
                                                         size_t maximum_count, char **out_json,
                                                         size_t *out_length) {
    if (out_json != NULL) {
        *out_json = NULL;
    }
    if (out_length != NULL) {
        *out_length = 0U;
    }
    if (order == NULL || out_json == NULL || out_length == NULL ||
        maximum_count > STORAGE_ORDER_MAX_IDS || order->count > maximum_count) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    for (size_t index = 0U; index < order->count; ++index) {
        if (!app_uuid_is_valid_string(order->ids[index].value)) {
            return APP_ERROR_INVALID_ARGUMENT;
        }
        for (size_t prior = 0U; prior < index; ++prior) {
            if (app_uuid_equal(&order->ids[prior], &order->ids[index])) {
                return APP_ERROR_INVALID_ARGUMENT;
            }
        }
    }
    cJSON *root = cJSON_CreateObject();
    if (root == NULL ||
        cJSON_AddNumberToObject(root, "schema_version", APP_SCHEMA_VERSION) == NULL) {
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    app_error_code_t result = add_uuid_array(root, "ids", order->ids, order->count);
    if (result == APP_ERROR_NONE) {
        result = print_bounded_json(root, STORAGE_ORDER_FILE_MAX_BYTES, out_json, out_length);
    }
    cJSON_Delete(root);
    return result;
}
