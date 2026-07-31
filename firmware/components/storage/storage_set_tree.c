#include "storage_set_tree_internal.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_limits.h"
#include "macro_model.h"
#include "macro_parser.h"
#include "storage.h"
#include "storage_object_json.h"

#define SET_TREE_JSON_SUFFIX ".json"
#define SET_TREE_REQUIRED_ROOT_COUNT 6U

typedef struct {
    app_uuid_t ids[APP_MACROS_PER_SET_MAX];
    size_t count;
} set_tree_macro_ids_t;

typedef struct {
    app_uuid_t id;
    uint32_t revision;
} set_tree_procedure_metadata_t;

typedef struct {
    set_tree_macro_ids_t macros;
    set_tree_procedure_metadata_t procedures[APP_PROCEDURES_PER_SET_MAX];
    size_t procedure_count;
    app_uuid_t progress_ids[APP_PROCEDURES_PER_SET_MAX];
    size_t progress_count;
} set_tree_state_t;

typedef enum {
    ROOT_ENTRY_SET = 0,
    ROOT_ENTRY_MACRO_ORDER,
    ROOT_ENTRY_PROCEDURE_ORDER,
    ROOT_ENTRY_MACROS,
    ROOT_ENTRY_PROCEDURES,
    ROOT_ENTRY_PROGRESS,
    ROOT_ENTRY_COUNT,
} root_entry_t;

typedef struct {
    const char *name;
    bool directory;
} root_entry_spec_t;

static const root_entry_spec_t ROOT_ENTRIES[ROOT_ENTRY_COUNT] = {
    [ROOT_ENTRY_SET] = {.name = "set.json", .directory = false},
    [ROOT_ENTRY_MACRO_ORDER] = {.name = "macro-order.json", .directory = false},
    [ROOT_ENTRY_PROCEDURE_ORDER] = {.name = "procedure-order.json", .directory = false},
    [ROOT_ENTRY_MACROS] = {.name = "macros", .directory = true},
    [ROOT_ENTRY_PROCEDURES] = {.name = "procedures", .directory = true},
    [ROOT_ENTRY_PROGRESS] = {.name = "progress", .directory = true},
};

static app_error_code_t map_error_number(int error_number) {
    if (error_number == ENOSPC) {
        return APP_ERROR_STORAGE_FULL;
    }
    if (error_number == ENOENT || error_number == ENOTDIR) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return APP_ERROR_IO;
}

static app_error_code_t normalize_object_error(app_error_code_t result) {
    switch (result) {
    case APP_ERROR_NONE:
    case APP_ERROR_IO:
    case APP_ERROR_STORAGE_FULL:
    case APP_ERROR_INTERNAL:
        return result;
    default:
        return APP_ERROR_STORAGE_CORRUPT;
    }
}

static app_error_code_t join_path(const char *parent, const char *name, char *output,
                                  size_t output_size) {
    if (parent == NULL || name == NULL || output == NULL || output_size == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const int written = snprintf(output, output_size, "%s/%s", parent, name);
    if (written < 0 || (size_t)written >= output_size) {
        output[0] = '\0';
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t read_bounded_file(const char *path, size_t maximum, char **out_data,
                                          size_t *out_length) {
    *out_data = NULL;
    *out_length = 0U;
    struct stat metadata;
    if (stat(path, &metadata) != 0) {
        return map_error_number(errno);
    }
    if (!S_ISREG(metadata.st_mode) || metadata.st_size <= 0 ||
        (uint64_t)metadata.st_size > maximum || (uint64_t)metadata.st_size > SIZE_MAX - 1U) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    const size_t length = (size_t)metadata.st_size;
    char *data = malloc(length + 1U);
    if (data == NULL) {
        return APP_ERROR_INTERNAL;
    }
    const int descriptor = open(path, O_RDONLY);
    if (descriptor < 0) {
        const int open_error = errno;
        free(data);
        return map_error_number(open_error);
    }
    app_error_code_t result = APP_ERROR_NONE;
    size_t offset = 0U;
    while (offset < length) {
        const ssize_t count = read(descriptor, data + offset, length - offset);
        if (count < 0) {
            if (errno == EINTR) {
                continue;
            }
            result = map_error_number(errno);
            break;
        }
        if (count == 0) {
            result = APP_ERROR_IO;
            break;
        }
        offset += (size_t)count;
    }
    if (result == APP_ERROR_NONE) {
        char extra = '\0';
        const ssize_t count = read(descriptor, &extra, 1U);
        if (count < 0) {
            result = map_error_number(errno);
        } else if (count != 0) {
            result = APP_ERROR_IO;
        }
    }
    if (close(descriptor) != 0 && result == APP_ERROR_NONE) {
        result = map_error_number(errno);
    }
    if (result != APP_ERROR_NONE) {
        free(data);
        return result;
    }
    data[length] = '\0';
    *out_data = data;
    *out_length = length;
    return APP_ERROR_NONE;
}

static app_error_code_t verify_path_type(const char *path, bool directory) {
    struct stat metadata;
    if (stat(path, &metadata) != 0) {
        return map_error_number(errno);
    }
    if ((directory && !S_ISDIR(metadata.st_mode)) || (!directory && !S_ISREG(metadata.st_mode))) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return APP_ERROR_NONE;
}

static size_t root_entry_index(const char *name) {
    for (size_t index = 0U; index < ROOT_ENTRY_COUNT; ++index) {
        if (strcmp(name, ROOT_ENTRIES[index].name) == 0) {
            return index;
        }
    }
    return SIZE_MAX;
}

static app_error_code_t validate_root_topology(const char *path) {
    bool seen[ROOT_ENTRY_COUNT] = {false};
    DIR *directory = opendir(path);
    if (directory == NULL) {
        return map_error_number(errno);
    }
    app_error_code_t result = APP_ERROR_NONE;
    while (result == APP_ERROR_NONE) {
        errno = 0;
        const struct dirent *entry = readdir(directory);
        if (entry == NULL) {
            if (errno != 0) {
                result = map_error_number(errno);
            }
            break;
        }
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        const size_t index = root_entry_index(entry->d_name);
        if (index == SIZE_MAX || seen[index]) {
            result = APP_ERROR_STORAGE_CORRUPT;
            break;
        }
        char child[APP_PATH_MAX_BYTES];
        result = join_path(path, entry->d_name, child, sizeof(child));
        if (result == APP_ERROR_NONE) {
            result = verify_path_type(child, ROOT_ENTRIES[index].directory);
        }
        if (result == APP_ERROR_NONE) {
            seen[index] = true;
        }
    }
    if (closedir(directory) != 0 && result == APP_ERROR_NONE) {
        result = map_error_number(errno);
    }
    if (result == APP_ERROR_NONE) {
        size_t count = 0U;
        for (size_t index = 0U; index < ROOT_ENTRY_COUNT; ++index) {
            if (seen[index]) {
                ++count;
            }
        }
        if (count != SET_TREE_REQUIRED_ROOT_COUNT) {
            result = APP_ERROR_STORAGE_CORRUPT;
        }
    }
    return result;
}

static app_error_code_t filename_uuid(const char *name, app_uuid_t *out_id) {
    const size_t suffix_length = sizeof(SET_TREE_JSON_SUFFIX) - 1U;
    const size_t expected_length = APP_UUID_STRING_LENGTH + suffix_length;
    if (name == NULL || out_id == NULL || strlen(name) != expected_length ||
        strcmp(name + APP_UUID_STRING_LENGTH, SET_TREE_JSON_SUFFIX) != 0) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    char value[APP_UUID_BUFFER_LENGTH];
    memcpy(value, name, APP_UUID_STRING_LENGTH);
    value[APP_UUID_STRING_LENGTH] = '\0';
    return app_uuid_parse(value, out_id) == APP_ERROR_NONE ? APP_ERROR_NONE
                                                           : APP_ERROR_STORAGE_CORRUPT;
}

static bool uuid_list_contains(const app_uuid_t *ids, size_t count, const app_uuid_t *id) {
    for (size_t index = 0U; index < count; ++index) {
        if (app_uuid_equal(&ids[index], id)) {
            return true;
        }
    }
    return false;
}

static app_error_code_t add_uuid(app_uuid_t *ids, size_t *count, size_t maximum,
                                 const app_uuid_t *id) {
    if (*count >= maximum || uuid_list_contains(ids, *count, id)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    ids[*count] = *id;
    ++*count;
    return APP_ERROR_NONE;
}

static app_error_code_t compile_macro(const macro_t *macro) {
    const macro_compile_options_t options = {
        .key_press_ms = macro->key_press_ms,
        .inter_key_ms = macro->inter_key_ms,
    };
    macro_plan_t plan = {0};
    macro_parse_error_t error = {0};
    const app_error_code_t result =
        macro_compile(macro->source, macro->source_length, &options, &plan, &error);
    macro_plan_free(&plan);
    return normalize_object_error(result);
}

static app_error_code_t validate_set_metadata(const char *path, const app_uuid_t *set_id,
                                              uint32_t expected_revision) {
    char file_path[APP_PATH_MAX_BYTES];
    app_error_code_t result = join_path(path, "set.json", file_path, sizeof(file_path));
    char *data = NULL;
    size_t length = 0U;
    if (result == APP_ERROR_NONE) {
        result = read_bounded_file(file_path, STORAGE_SET_FILE_MAX_BYTES, &data, &length);
    }
    macro_set_t set = {0};
    if (result == APP_ERROR_NONE) {
        result = normalize_object_error(storage_repository_parse_set_json(data, length, &set));
    }
    free(data);
    if (result == APP_ERROR_NONE &&
        (!app_uuid_equal(&set.id, set_id) || set.revision != expected_revision)) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    return result;
}

static app_error_code_t validate_local_macro_file(const char *path, const app_uuid_t *filename_id,
                                                  const app_uuid_t *set_id,
                                                  set_tree_state_t *state) {
    char *data = NULL;
    size_t length = 0U;
    app_error_code_t result = read_bounded_file(path, STORAGE_MACRO_FILE_MAX_BYTES, &data, &length);
    macro_t macro = {0};
    if (result == APP_ERROR_NONE) {
        result = normalize_object_error(storage_repository_parse_macro_json(data, length, &macro));
    }
    free(data);
    if (result == APP_ERROR_NONE &&
        (!app_uuid_equal(&macro.id, filename_id) || macro.scope != MACRO_SCOPE_SET ||
         !macro.has_set_id || !app_uuid_equal(&macro.set_id, set_id))) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    if (result == APP_ERROR_NONE) {
        result = compile_macro(&macro);
    }
    if (result == APP_ERROR_NONE) {
        result =
            add_uuid(state->macros.ids, &state->macros.count, APP_MACROS_PER_SET_MAX, &macro.id);
    }
    macro_model_free_macro(&macro);
    return result;
}

static app_error_code_t validate_macro_directory(const char *set_path, const app_uuid_t *set_id,
                                                 set_tree_state_t *state) {
    char directory_path[APP_PATH_MAX_BYTES];
    app_error_code_t result = join_path(set_path, "macros", directory_path, sizeof(directory_path));
    DIR *directory = result == APP_ERROR_NONE ? opendir(directory_path) : NULL;
    if (result == APP_ERROR_NONE && directory == NULL) {
        result = map_error_number(errno);
    }
    while (result == APP_ERROR_NONE) {
        errno = 0;
        const struct dirent *entry = readdir(directory);
        if (entry == NULL) {
            if (errno != 0) {
                result = map_error_number(errno);
            }
            break;
        }
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        app_uuid_t id = {0};
        result = filename_uuid(entry->d_name, &id);
        char file_path[APP_PATH_MAX_BYTES];
        if (result == APP_ERROR_NONE) {
            result = join_path(directory_path, entry->d_name, file_path, sizeof(file_path));
        }
        if (result == APP_ERROR_NONE) {
            result = verify_path_type(file_path, false);
        }
        if (result == APP_ERROR_NONE) {
            result = validate_local_macro_file(file_path, &id, set_id, state);
        }
    }
    if (directory != NULL && closedir(directory) != 0 && result == APP_ERROR_NONE) {
        result = map_error_number(errno);
    }
    return result;
}

static app_error_code_t global_macro_path(const app_uuid_t *macro_id, char *path,
                                          size_t path_size) {
    const int written =
        snprintf(path, path_size, STORAGE_DATA_MOUNT "/global/macros/%s.json", macro_id->value);
    return written >= 0 && (size_t)written < path_size ? APP_ERROR_NONE : APP_ERROR_STORAGE_CORRUPT;
}

static app_error_code_t global_macro_exists(const app_uuid_t *macro_id, bool *out_exists) {
    *out_exists = false;
    char path[APP_PATH_MAX_BYTES];
    app_error_code_t result = global_macro_path(macro_id, path, sizeof(path));
    if (result != APP_ERROR_NONE) {
        return result;
    }
    struct stat metadata;
    if (stat(path, &metadata) == 0) {
        if (!S_ISREG(metadata.st_mode)) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
        *out_exists = true;
        return APP_ERROR_NONE;
    }
    return errno == ENOENT ? APP_ERROR_NONE : map_error_number(errno);
}

static app_error_code_t validate_global_macro(const app_uuid_t *macro_id) {
    char path[APP_PATH_MAX_BYTES];
    app_error_code_t result = global_macro_path(macro_id, path, sizeof(path));
    char *data = NULL;
    size_t length = 0U;
    if (result == APP_ERROR_NONE) {
        result = read_bounded_file(path, STORAGE_MACRO_FILE_MAX_BYTES, &data, &length);
    }
    macro_t macro = {0};
    if (result == APP_ERROR_NONE) {
        result = normalize_object_error(storage_repository_parse_macro_json(data, length, &macro));
    }
    free(data);
    if (result == APP_ERROR_NONE && (!app_uuid_equal(&macro.id, macro_id) ||
                                     macro.scope != MACRO_SCOPE_GLOBAL || macro.has_set_id)) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    if (result == APP_ERROR_NONE) {
        result = compile_macro(&macro);
    }
    macro_model_free_macro(&macro);
    return result;
}

static app_error_code_t validate_macro_reference(const app_uuid_t *macro_id,
                                                 const set_tree_state_t *state) {
    const bool local = uuid_list_contains(state->macros.ids, state->macros.count, macro_id);
    bool global = false;
    app_error_code_t result = global_macro_exists(macro_id, &global);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (local == global) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return global ? validate_global_macro(macro_id) : APP_ERROR_NONE;
}

static app_error_code_t add_procedure(set_tree_state_t *state, const procedure_t *procedure) {
    if (state->procedure_count >= APP_PROCEDURES_PER_SET_MAX) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    for (size_t index = 0U; index < state->procedure_count; ++index) {
        if (app_uuid_equal(&state->procedures[index].id, &procedure->id)) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
    }
    state->procedures[state->procedure_count] = (set_tree_procedure_metadata_t){
        .id = procedure->id,
        .revision = procedure->revision,
    };
    ++state->procedure_count;
    return APP_ERROR_NONE;
}

static app_error_code_t validate_procedure_file(const char *path, const app_uuid_t *filename_id,
                                                const app_uuid_t *set_id, set_tree_state_t *state) {
    char *data = NULL;
    size_t length = 0U;
    app_error_code_t result =
        read_bounded_file(path, STORAGE_PROCEDURE_FILE_MAX_BYTES, &data, &length);
    procedure_t procedure = {0};
    if (result == APP_ERROR_NONE) {
        result = normalize_object_error(
            storage_repository_parse_procedure_json(data, length, &procedure));
    }
    free(data);
    if (result == APP_ERROR_NONE && (!app_uuid_equal(&procedure.id, filename_id) ||
                                     !app_uuid_equal(&procedure.set_id, set_id))) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    for (size_t index = 0U; result == APP_ERROR_NONE && index < procedure.step_count; ++index) {
        if (procedure.steps[index].has_macro_id) {
            result = validate_macro_reference(&procedure.steps[index].macro_id, state);
        }
    }
    if (result == APP_ERROR_NONE) {
        result = add_procedure(state, &procedure);
    }
    macro_model_free_procedure(&procedure);
    return result;
}

static app_error_code_t validate_procedure_directory(const char *set_path, const app_uuid_t *set_id,
                                                     set_tree_state_t *state) {
    char directory_path[APP_PATH_MAX_BYTES];
    app_error_code_t result =
        join_path(set_path, "procedures", directory_path, sizeof(directory_path));
    DIR *directory = result == APP_ERROR_NONE ? opendir(directory_path) : NULL;
    if (result == APP_ERROR_NONE && directory == NULL) {
        result = map_error_number(errno);
    }
    while (result == APP_ERROR_NONE) {
        errno = 0;
        const struct dirent *entry = readdir(directory);
        if (entry == NULL) {
            if (errno != 0) {
                result = map_error_number(errno);
            }
            break;
        }
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        app_uuid_t id = {0};
        result = filename_uuid(entry->d_name, &id);
        char file_path[APP_PATH_MAX_BYTES];
        if (result == APP_ERROR_NONE) {
            result = join_path(directory_path, entry->d_name, file_path, sizeof(file_path));
        }
        if (result == APP_ERROR_NONE) {
            result = verify_path_type(file_path, false);
        }
        if (result == APP_ERROR_NONE) {
            result = validate_procedure_file(file_path, &id, set_id, state);
        }
    }
    if (directory != NULL && closedir(directory) != 0 && result == APP_ERROR_NONE) {
        result = map_error_number(errno);
    }
    return result;
}

static const set_tree_procedure_metadata_t *find_procedure(const set_tree_state_t *state,
                                                           const app_uuid_t *procedure_id) {
    for (size_t index = 0U; index < state->procedure_count; ++index) {
        if (app_uuid_equal(&state->procedures[index].id, procedure_id)) {
            return &state->procedures[index];
        }
    }
    return NULL;
}

static bool procedure_has_step(const procedure_t *procedure, const app_uuid_t *step_id) {
    for (size_t index = 0U; index < procedure->step_count; ++index) {
        if (app_uuid_equal(&procedure->steps[index].id, step_id)) {
            return true;
        }
    }
    return false;
}

static bool progress_steps_valid(const procedure_progress_t *progress,
                                 const procedure_t *procedure) {
    if (!procedure_has_step(procedure, &progress->current_step_id)) {
        return false;
    }
    for (size_t index = 0U; index < progress->completed_step_count; ++index) {
        if (!procedure_has_step(procedure, &progress->completed_step_ids[index])) {
            return false;
        }
    }
    for (size_t index = 0U; index < progress->skipped_step_count; ++index) {
        if (!procedure_has_step(procedure, &progress->skipped_step_ids[index])) {
            return false;
        }
    }
    return true;
}

static app_error_code_t load_procedure(const char *set_path, const app_uuid_t *procedure_id,
                                       procedure_t *out_procedure) {
    char name[APP_UUID_STRING_LENGTH + sizeof(SET_TREE_JSON_SUFFIX)];
    const int written = snprintf(name, sizeof(name), "%s.json", procedure_id->value);
    if (written < 0 || (size_t)written >= sizeof(name)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    char directory_path[APP_PATH_MAX_BYTES];
    app_error_code_t result =
        join_path(set_path, "procedures", directory_path, sizeof(directory_path));
    char file_path[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        result = join_path(directory_path, name, file_path, sizeof(file_path));
    }
    char *data = NULL;
    size_t length = 0U;
    if (result == APP_ERROR_NONE) {
        result = read_bounded_file(file_path, STORAGE_PROCEDURE_FILE_MAX_BYTES, &data, &length);
    }
    if (result == APP_ERROR_NONE) {
        result = normalize_object_error(
            storage_repository_parse_procedure_json(data, length, out_procedure));
    }
    free(data);
    return result;
}

static app_error_code_t validate_progress_file(const char *set_path, const char *path,
                                               const app_uuid_t *filename_id,
                                               const app_uuid_t *set_id, set_tree_state_t *state) {
    char *data = NULL;
    size_t length = 0U;
    app_error_code_t result =
        read_bounded_file(path, STORAGE_PROGRESS_FILE_MAX_BYTES, &data, &length);
    procedure_progress_t progress = {0};
    if (result == APP_ERROR_NONE) {
        result =
            normalize_object_error(storage_repository_parse_progress_json(data, length, &progress));
    }
    free(data);
    const set_tree_procedure_metadata_t *procedure_metadata =
        result == APP_ERROR_NONE ? find_procedure(state, &progress.procedure_id) : NULL;
    if (result == APP_ERROR_NONE &&
        (!app_uuid_equal(&progress.procedure_id, filename_id) ||
         !app_uuid_equal(&progress.set_id, set_id) || procedure_metadata == NULL ||
         progress.procedure_revision != procedure_metadata->revision)) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    procedure_t procedure = {0};
    if (result == APP_ERROR_NONE) {
        result = load_procedure(set_path, &progress.procedure_id, &procedure);
    }
    if (result == APP_ERROR_NONE && !progress_steps_valid(&progress, &procedure)) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    macro_model_free_procedure(&procedure);
    if (result == APP_ERROR_NONE) {
        result = add_uuid(state->progress_ids, &state->progress_count, APP_PROCEDURES_PER_SET_MAX,
                          &progress.procedure_id);
    }
    return result;
}

static app_error_code_t validate_progress_directory(const char *set_path, const app_uuid_t *set_id,
                                                    set_tree_state_t *state) {
    char directory_path[APP_PATH_MAX_BYTES];
    app_error_code_t result =
        join_path(set_path, "progress", directory_path, sizeof(directory_path));
    DIR *directory = result == APP_ERROR_NONE ? opendir(directory_path) : NULL;
    if (result == APP_ERROR_NONE && directory == NULL) {
        result = map_error_number(errno);
    }
    while (result == APP_ERROR_NONE) {
        errno = 0;
        const struct dirent *entry = readdir(directory);
        if (entry == NULL) {
            if (errno != 0) {
                result = map_error_number(errno);
            }
            break;
        }
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        app_uuid_t id = {0};
        result = filename_uuid(entry->d_name, &id);
        char file_path[APP_PATH_MAX_BYTES];
        if (result == APP_ERROR_NONE) {
            result = join_path(directory_path, entry->d_name, file_path, sizeof(file_path));
        }
        if (result == APP_ERROR_NONE) {
            result = verify_path_type(file_path, false);
        }
        if (result == APP_ERROR_NONE) {
            result = validate_progress_file(set_path, file_path, &id, set_id, state);
        }
    }
    if (directory != NULL && closedir(directory) != 0 && result == APP_ERROR_NONE) {
        result = map_error_number(errno);
    }
    if (result == APP_ERROR_NONE && state->progress_count > state->procedure_count) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    return result;
}

static app_error_code_t validate_order(const char *set_path, const char *name,
                                       const app_uuid_t *ids, size_t count, size_t maximum) {
    char path[APP_PATH_MAX_BYTES];
    app_error_code_t result = join_path(set_path, name, path, sizeof(path));
    char *data = NULL;
    size_t length = 0U;
    if (result == APP_ERROR_NONE) {
        result = read_bounded_file(path, STORAGE_ORDER_FILE_MAX_BYTES, &data, &length);
    }
    storage_uuid_order_t order = {0};
    if (result == APP_ERROR_NONE) {
        result = normalize_object_error(
            storage_repository_parse_order_json(data, length, &order, maximum));
    }
    free(data);
    if (result == APP_ERROR_NONE && order.count != count) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    for (size_t index = 0U; result == APP_ERROR_NONE && index < order.count; ++index) {
        if (!uuid_list_contains(ids, count, &order.ids[index])) {
            result = APP_ERROR_STORAGE_CORRUPT;
        }
    }
    return result;
}

app_error_code_t storage_set_tree_validate(const char *path, const app_uuid_t *set_id,
                                           uint32_t expected_revision) {
    if (path == NULL || path[0] == '\0' || set_id == NULL || expected_revision == 0U ||
        !app_uuid_is_valid_string(set_id->value)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = validate_root_topology(path);
    if (result == APP_ERROR_NONE) {
        result = validate_set_metadata(path, set_id, expected_revision);
    }
    set_tree_state_t *state = NULL;
    if (result == APP_ERROR_NONE) {
        state = calloc(1U, sizeof(*state));
        if (state == NULL) {
            result = APP_ERROR_INTERNAL;
        }
    }
    if (result == APP_ERROR_NONE) {
        result = validate_macro_directory(path, set_id, state);
    }
    if (result == APP_ERROR_NONE) {
        result = validate_order(path, "macro-order.json", state->macros.ids, state->macros.count,
                                APP_MACROS_PER_SET_MAX);
    }
    if (result == APP_ERROR_NONE) {
        result = validate_procedure_directory(path, set_id, state);
    }
    if (result == APP_ERROR_NONE) {
        app_uuid_t procedure_ids[APP_PROCEDURES_PER_SET_MAX];
        for (size_t index = 0U; index < state->procedure_count; ++index) {
            procedure_ids[index] = state->procedures[index].id;
        }
        result = validate_order(path, "procedure-order.json", procedure_ids, state->procedure_count,
                                APP_PROCEDURES_PER_SET_MAX);
    }
    if (result == APP_ERROR_NONE) {
        result = validate_progress_directory(path, set_id, state);
    }
    free(state);
    return result;
}
