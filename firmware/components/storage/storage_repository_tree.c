#define _POSIX_C_SOURCE 200809L

#include "storage_repository_tree_internal.h"

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
#include "storage.h"
#include "storage_object_json.h"
#include "storage_package.h"
#include "storage_repository_internal.h"

#define TREE_JSON_SUFFIX ".json"
#define TREE_INDEX_MAX_BYTES 8192U

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} tree_writer_t;

typedef struct {
    size_t set_count;
    size_t local_macro_count;
    size_t global_macro_count;
    size_t procedure_count;
    size_t progress_count;
} tree_counts_t;

typedef struct {
    const char *name;
    bool directory;
} tree_entry_t;

static const tree_entry_t LOGICAL_ROOT_ENTRIES[] = {
    {.name = "set-index.json", .directory = false},
    {.name = "sets", .directory = true},
    {.name = "global", .directory = true},
};

static const tree_entry_t SET_ROOT_ENTRIES[] = {
    {.name = "set.json", .directory = false},
    {.name = "macro-order.json", .directory = false},
    {.name = "procedure-order.json", .directory = false},
    {.name = "macros", .directory = true},
    {.name = "procedures", .directory = true},
    {.name = "progress", .directory = true},
};

static const tree_entry_t GLOBAL_ROOT_ENTRIES[] = {
    {.name = "macro-order.json", .directory = false},
    {.name = "macros", .directory = true},
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

static app_error_code_t join_path(char *output, size_t output_size, const char *root,
                                  const char *name) {
    if (output == NULL || output_size == 0U || root == NULL || name == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const int written = snprintf(output, output_size, "%s/%s", root, name);
    if (written < 0 || (size_t)written >= output_size) {
        output[0] = '\0';
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t verify_type(const char *path, bool directory) {
    struct stat metadata;
    if (stat(path, &metadata) != 0) {
        return map_error_number(errno);
    }
    if ((directory && !S_ISDIR(metadata.st_mode)) || (!directory && !S_ISREG(metadata.st_mode))) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return APP_ERROR_NONE;
}

static size_t entry_index(const tree_entry_t *entries, size_t count, const char *name) {
    for (size_t index = 0U; index < count; ++index) {
        if (strcmp(entries[index].name, name) == 0) {
            return index;
        }
    }
    return SIZE_MAX;
}

static app_error_code_t validate_topology(const char *root, const tree_entry_t *entries,
                                          size_t entry_count, bool allow_extra) {
    if (verify_type(root, true) != APP_ERROR_NONE || entry_count > 8U) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    bool seen[8] = {false};
    DIR *directory = opendir(root);
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
        const size_t index = entry_index(entries, entry_count, entry->d_name);
        if (index == SIZE_MAX) {
            if (!allow_extra) {
                result = APP_ERROR_STORAGE_CORRUPT;
            }
            continue;
        }
        if (seen[index]) {
            result = APP_ERROR_STORAGE_CORRUPT;
            break;
        }
        char path[APP_PATH_MAX_BYTES];
        result = join_path(path, sizeof(path), root, entry->d_name);
        if (result == APP_ERROR_NONE) {
            result = verify_type(path, entries[index].directory);
        }
        if (result == APP_ERROR_NONE) {
            seen[index] = true;
        }
    }
    if (closedir(directory) != 0 && result == APP_ERROR_NONE) {
        result = map_error_number(errno);
    }
    for (size_t index = 0U; result == APP_ERROR_NONE && index < entry_count; ++index) {
        if (!seen[index]) {
            result = APP_ERROR_STORAGE_CORRUPT;
        }
    }
    return result;
}

static app_error_code_t read_bounded(const char *path, size_t maximum, char **out_data,
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

static app_error_code_t writer_reserve(tree_writer_t *writer, size_t additional) {
    if (additional > APP_IMPORT_PACKAGE_MAX_BYTES - writer->length) {
        return APP_ERROR_MACRO_LIMIT;
    }
    const size_t required = writer->length + additional + 1U;
    if (required <= writer->capacity) {
        return APP_ERROR_NONE;
    }
    const size_t maximum = APP_IMPORT_PACKAGE_MAX_BYTES + 1U;
    size_t capacity = writer->capacity == 0U ? 1024U : writer->capacity;
    while (capacity < required) {
        if (capacity > maximum / 2U) {
            capacity = maximum;
            break;
        }
        capacity *= 2U;
    }
    if (capacity < required || capacity > maximum) {
        return APP_ERROR_MACRO_LIMIT;
    }
    char *replacement = realloc(writer->data, capacity);
    if (replacement == NULL) {
        return APP_ERROR_INTERNAL;
    }
    writer->data = replacement;
    writer->capacity = capacity;
    return APP_ERROR_NONE;
}

static app_error_code_t writer_append(tree_writer_t *writer, const char *data, size_t length) {
    app_error_code_t result = writer_reserve(writer, length);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    memcpy(writer->data + writer->length, data, length);
    writer->length += length;
    writer->data[writer->length] = '\0';
    return APP_ERROR_NONE;
}

static app_error_code_t writer_text(tree_writer_t *writer, const char *text) {
    return writer_append(writer, text, strlen(text));
}

static app_error_code_t writer_object(tree_writer_t *writer, bool *first, const char *data,
                                      size_t length) {
    app_error_code_t result = APP_ERROR_NONE;
    if (!*first) {
        result = writer_text(writer, ",");
    }
    if (result == APP_ERROR_NONE) {
        result = writer_append(writer, data, length);
    }
    if (result == APP_ERROR_NONE) {
        *first = false;
    }
    return result;
}

static app_error_code_t filename_uuid(const char *name, bool suffix, app_uuid_t *out_id) {
    const size_t expected = APP_UUID_STRING_LENGTH + (suffix ? sizeof(TREE_JSON_SUFFIX) - 1U : 0U);
    if (strlen(name) != expected ||
        (suffix && strcmp(name + APP_UUID_STRING_LENGTH, TREE_JSON_SUFFIX) != 0)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    char value[APP_UUID_BUFFER_LENGTH];
    memcpy(value, name, APP_UUID_STRING_LENGTH);
    value[APP_UUID_STRING_LENGTH] = '\0';
    return app_uuid_parse(value, out_id) == APP_ERROR_NONE ? APP_ERROR_NONE
                                                           : APP_ERROR_STORAGE_CORRUPT;
}

static bool order_contains(const storage_uuid_order_t *order, const app_uuid_t *id) {
    for (size_t index = 0U; index < order->count; ++index) {
        if (app_uuid_equal(&order->ids[index], id)) {
            return true;
        }
    }
    return false;
}

static bool index_contains(const storage_set_index_t *index, const app_uuid_t *id) {
    for (size_t item = 0U; item < index->count; ++item) {
        if (app_uuid_equal(&index->ids[item], id)) {
            return true;
        }
    }
    return false;
}

static app_error_code_t read_order(const char *path, size_t maximum,
                                   storage_uuid_order_t *out_order) {
    char *data = NULL;
    size_t length = 0U;
    app_error_code_t result = read_bounded(path, STORAGE_ORDER_FILE_MAX_BYTES, &data, &length);
    if (result == APP_ERROR_NONE) {
        result = normalize_object_error(
            storage_repository_parse_order_json(data, length, out_order, maximum));
    }
    free(data);
    return result;
}

static app_error_code_t validate_directory_membership(const char *path,
                                                       const storage_uuid_order_t *order,
                                                       bool suffix) {
    DIR *directory = opendir(path);
    if (directory == NULL) {
        return map_error_number(errno);
    }
    size_t count = 0U;
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
        app_uuid_t id = {0};
        result = filename_uuid(entry->d_name, suffix, &id);
        char child[APP_PATH_MAX_BYTES];
        if (result == APP_ERROR_NONE) {
            result = join_path(child, sizeof(child), path, entry->d_name);
        }
        if (result == APP_ERROR_NONE) {
            result = verify_type(child, false);
        }
        if (result == APP_ERROR_NONE && !order_contains(order, &id)) {
            result = APP_ERROR_STORAGE_CORRUPT;
        }
        if (result == APP_ERROR_NONE) {
            ++count;
        }
    }
    if (closedir(directory) != 0 && result == APP_ERROR_NONE) {
        result = map_error_number(errno);
    }
    return result == APP_ERROR_NONE && count != order->count ? APP_ERROR_STORAGE_CORRUPT : result;
}

static app_error_code_t validate_set_directory_membership(const char *path,
                                                           const storage_set_index_t *index) {
    DIR *directory = opendir(path);
    if (directory == NULL) {
        return map_error_number(errno);
    }
    size_t count = 0U;
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
        app_uuid_t id = {0};
        result = filename_uuid(entry->d_name, false, &id);
        char child[APP_PATH_MAX_BYTES];
        if (result == APP_ERROR_NONE) {
            result = join_path(child, sizeof(child), path, entry->d_name);
        }
        if (result == APP_ERROR_NONE) {
            result = verify_type(child, true);
        }
        if (result == APP_ERROR_NONE && !index_contains(index, &id)) {
            result = APP_ERROR_STORAGE_CORRUPT;
        }
        if (result == APP_ERROR_NONE) {
            ++count;
        }
    }
    if (closedir(directory) != 0 && result == APP_ERROR_NONE) {
        result = map_error_number(errno);
    }
    return result == APP_ERROR_NONE && count != index->count ? APP_ERROR_STORAGE_CORRUPT : result;
}

static app_error_code_t append_macro_file(tree_writer_t *writer, bool *first, const char *path,
                                          const app_uuid_t *expected_id,
                                          const app_uuid_t *expected_set_id,
                                          macro_scope_t expected_scope) {
    char *data = NULL;
    size_t length = 0U;
    app_error_code_t result = read_bounded(path, STORAGE_MACRO_FILE_MAX_BYTES, &data, &length);
    macro_t macro = {0};
    if (result == APP_ERROR_NONE) {
        result = normalize_object_error(storage_repository_parse_macro_json(data, length, &macro));
    }
    if (result == APP_ERROR_NONE &&
        (!app_uuid_equal(&macro.id, expected_id) || macro.scope != expected_scope ||
         (expected_scope == MACRO_SCOPE_SET &&
          (!macro.has_set_id || !app_uuid_equal(&macro.set_id, expected_set_id))) ||
         (expected_scope == MACRO_SCOPE_GLOBAL && macro.has_set_id))) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    if (result == APP_ERROR_NONE) {
        result = writer_object(writer, first, data, length);
    }
    macro_model_free_macro(&macro);
    free(data);
    return result;
}

static app_error_code_t append_procedure_file(tree_writer_t *writer, bool *first,
                                              const char *path, const app_uuid_t *expected_id,
                                              const app_uuid_t *set_id) {
    char *data = NULL;
    size_t length = 0U;
    app_error_code_t result = read_bounded(path, STORAGE_PROCEDURE_FILE_MAX_BYTES, &data, &length);
    procedure_t procedure = {0};
    if (result == APP_ERROR_NONE) {
        result = normalize_object_error(
            storage_repository_parse_procedure_json(data, length, &procedure));
    }
    if (result == APP_ERROR_NONE &&
        (!app_uuid_equal(&procedure.id, expected_id) ||
         !app_uuid_equal(&procedure.set_id, set_id))) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    if (result == APP_ERROR_NONE) {
        result = writer_object(writer, first, data, length);
    }
    macro_model_free_procedure(&procedure);
    free(data);
    return result;
}

static app_error_code_t append_progress_file(tree_writer_t *writer, bool *first,
                                             const char *path, const app_uuid_t *procedure_id,
                                             const app_uuid_t *set_id) {
    char *data = NULL;
    size_t length = 0U;
    app_error_code_t result = read_bounded(path, STORAGE_PROGRESS_FILE_MAX_BYTES, &data, &length);
    procedure_progress_t progress = {0};
    if (result == APP_ERROR_NONE) {
        result = normalize_object_error(
            storage_repository_parse_progress_json(data, length, &progress));
    }
    if (result == APP_ERROR_NONE &&
        (!app_uuid_equal(&progress.procedure_id, procedure_id) ||
         !app_uuid_equal(&progress.set_id, set_id))) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    if (result == APP_ERROR_NONE) {
        result = writer_object(writer, first, data, length);
    }
    free(data);
    return result;
}

static app_error_code_t append_set_metadata(tree_writer_t *writer, bool *first,
                                            const char *set_root, const app_uuid_t *set_id) {
    char path[APP_PATH_MAX_BYTES];
    app_error_code_t result = join_path(path, sizeof(path), set_root, "set.json");
    char *data = NULL;
    size_t length = 0U;
    if (result == APP_ERROR_NONE) {
        result = read_bounded(path, STORAGE_SET_FILE_MAX_BYTES, &data, &length);
    }
    macro_set_t set = {0};
    if (result == APP_ERROR_NONE) {
        result = normalize_object_error(storage_repository_parse_set_json(data, length, &set));
    }
    if (result == APP_ERROR_NONE && !app_uuid_equal(&set.id, set_id)) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    if (result == APP_ERROR_NONE) {
        result = writer_object(writer, first, data, length);
    }
    free(data);
    return result;
}

static app_error_code_t append_ordered_macros(tree_writer_t *writer, bool *first,
                                              const char *set_root, const app_uuid_t *set_id,
                                              tree_counts_t *counts) {
    char path[APP_PATH_MAX_BYTES];
    app_error_code_t result = join_path(path, sizeof(path), set_root, "macro-order.json");
    storage_uuid_order_t order = {0};
    if (result == APP_ERROR_NONE) {
        result = read_order(path, APP_MACROS_PER_SET_MAX, &order);
    }
    char directory[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        result = join_path(directory, sizeof(directory), set_root, "macros");
    }
    if (result == APP_ERROR_NONE) {
        result = validate_directory_membership(directory, &order, true);
    }
    for (size_t index = 0U; result == APP_ERROR_NONE && index < order.count; ++index) {
        char name[APP_UUID_STRING_LENGTH + sizeof(TREE_JSON_SUFFIX)];
        const int written = snprintf(name, sizeof(name), "%s.json", order.ids[index].value);
        if (written < 0 || (size_t)written >= sizeof(name)) {
            result = APP_ERROR_STORAGE_CORRUPT;
            break;
        }
        result = join_path(path, sizeof(path), directory, name);
        if (result == APP_ERROR_NONE) {
            result = append_macro_file(writer, first, path, &order.ids[index], set_id,
                                       MACRO_SCOPE_SET);
        }
        if (result == APP_ERROR_NONE) {
            ++counts->local_macro_count;
        }
    }
    return result;
}

static app_error_code_t append_ordered_procedures(tree_writer_t *writer, bool *first,
                                                  const char *set_root, const app_uuid_t *set_id,
                                                  storage_uuid_order_t *out_order,
                                                  tree_counts_t *counts) {
    char path[APP_PATH_MAX_BYTES];
    app_error_code_t result = join_path(path, sizeof(path), set_root, "procedure-order.json");
    if (result == APP_ERROR_NONE) {
        result = read_order(path, APP_PROCEDURES_PER_SET_MAX, out_order);
    }
    char directory[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        result = join_path(directory, sizeof(directory), set_root, "procedures");
    }
    if (result == APP_ERROR_NONE) {
        result = validate_directory_membership(directory, out_order, true);
    }
    for (size_t index = 0U; result == APP_ERROR_NONE && index < out_order->count; ++index) {
        char name[APP_UUID_STRING_LENGTH + sizeof(TREE_JSON_SUFFIX)];
        const int written = snprintf(name, sizeof(name), "%s.json", out_order->ids[index].value);
        if (written < 0 || (size_t)written >= sizeof(name)) {
            result = APP_ERROR_STORAGE_CORRUPT;
            break;
        }
        result = join_path(path, sizeof(path), directory, name);
        if (result == APP_ERROR_NONE) {
            result = append_procedure_file(writer, first, path, &out_order->ids[index], set_id);
        }
        if (result == APP_ERROR_NONE) {
            ++counts->procedure_count;
        }
    }
    return result;
}

static app_error_code_t validate_progress_membership(const char *directory,
                                                     const storage_uuid_order_t *procedures) {
    DIR *handle = opendir(directory);
    if (handle == NULL) {
        return map_error_number(errno);
    }
    app_error_code_t result = APP_ERROR_NONE;
    while (result == APP_ERROR_NONE) {
        errno = 0;
        const struct dirent *entry = readdir(handle);
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
        result = filename_uuid(entry->d_name, true, &id);
        char child[APP_PATH_MAX_BYTES];
        if (result == APP_ERROR_NONE) {
            result = join_path(child, sizeof(child), directory, entry->d_name);
        }
        if (result == APP_ERROR_NONE) {
            result = verify_type(child, false);
        }
        if (result == APP_ERROR_NONE && !order_contains(procedures, &id)) {
            result = APP_ERROR_STORAGE_CORRUPT;
        }
    }
    if (closedir(handle) != 0 && result == APP_ERROR_NONE) {
        result = map_error_number(errno);
    }
    return result;
}

static app_error_code_t append_progress(tree_writer_t *writer, bool *first, const char *set_root,
                                        const app_uuid_t *set_id,
                                        const storage_uuid_order_t *procedures,
                                        tree_counts_t *counts) {
    char directory[APP_PATH_MAX_BYTES];
    app_error_code_t result = join_path(directory, sizeof(directory), set_root, "progress");
    if (result == APP_ERROR_NONE) {
        result = validate_progress_membership(directory, procedures);
    }
    for (size_t index = 0U; result == APP_ERROR_NONE && index < procedures->count; ++index) {
        char name[APP_UUID_STRING_LENGTH + sizeof(TREE_JSON_SUFFIX)];
        const int written = snprintf(name, sizeof(name), "%s.json", procedures->ids[index].value);
        if (written < 0 || (size_t)written >= sizeof(name)) {
            result = APP_ERROR_STORAGE_CORRUPT;
            break;
        }
        char path[APP_PATH_MAX_BYTES];
        result = join_path(path, sizeof(path), directory, name);
        if (result != APP_ERROR_NONE) {
            break;
        }
        struct stat metadata;
        if (stat(path, &metadata) != 0) {
            if (errno == ENOENT) {
                continue;
            }
            result = map_error_number(errno);
            break;
        }
        if (!S_ISREG(metadata.st_mode)) {
            result = APP_ERROR_STORAGE_CORRUPT;
            break;
        }
        result = append_progress_file(writer, first, path, &procedures->ids[index], set_id);
        if (result == APP_ERROR_NONE) {
            ++counts->progress_count;
        }
    }
    return result;
}

static app_error_code_t read_set_index(const char *root, storage_set_index_t *out_index) {
    char path[APP_PATH_MAX_BYTES];
    app_error_code_t result = join_path(path, sizeof(path), root, "set-index.json");
    char *data = NULL;
    size_t length = 0U;
    if (result == APP_ERROR_NONE) {
        result = read_bounded(path, TREE_INDEX_MAX_BYTES, &data, &length);
    }
    if (result == APP_ERROR_NONE) {
        result = normalize_object_error(storage_repository_parse_index(data, length, out_index));
    }
    free(data);
    return result;
}

static app_error_code_t append_sets(tree_writer_t *writer, const char *root,
                                    const storage_set_index_t *index, tree_counts_t *counts) {
    char sets_root[APP_PATH_MAX_BYTES];
    app_error_code_t result = join_path(sets_root, sizeof(sets_root), root, "sets");
    if (result == APP_ERROR_NONE) {
        result = validate_set_directory_membership(sets_root, index);
    }
    bool first = true;
    if (result == APP_ERROR_NONE) {
        result = writer_text(writer, "[");
    }
    for (size_t item = 0U; result == APP_ERROR_NONE && item < index->count; ++item) {
        char set_root[APP_PATH_MAX_BYTES];
        result = join_path(set_root, sizeof(set_root), sets_root, index->ids[item].value);
        if (result == APP_ERROR_NONE) {
            result = validate_topology(set_root, SET_ROOT_ENTRIES,
                                       sizeof(SET_ROOT_ENTRIES) / sizeof(SET_ROOT_ENTRIES[0]),
                                       false);
        }
        if (result == APP_ERROR_NONE) {
            result = append_set_metadata(writer, &first, set_root, &index->ids[item]);
        }
        if (result == APP_ERROR_NONE) {
            ++counts->set_count;
        }
    }
    return result == APP_ERROR_NONE ? writer_text(writer, "]") : result;
}

static app_error_code_t append_all_local_macros(tree_writer_t *writer, const char *root,
                                                const storage_set_index_t *index,
                                                tree_counts_t *counts) {
    app_error_code_t result = writer_text(writer, "[");
    bool first = true;
    for (size_t item = 0U; result == APP_ERROR_NONE && item < index->count; ++item) {
        char set_root[APP_PATH_MAX_BYTES];
        char sets_root[APP_PATH_MAX_BYTES];
        result = join_path(sets_root, sizeof(sets_root), root, "sets");
        if (result == APP_ERROR_NONE) {
            result = join_path(set_root, sizeof(set_root), sets_root, index->ids[item].value);
        }
        if (result == APP_ERROR_NONE) {
            result = append_ordered_macros(writer, &first, set_root, &index->ids[item], counts);
        }
    }
    return result == APP_ERROR_NONE ? writer_text(writer, "]") : result;
}

static app_error_code_t append_global_macros(tree_writer_t *writer, const char *root,
                                             tree_counts_t *counts) {
    char global_root[APP_PATH_MAX_BYTES];
    app_error_code_t result = join_path(global_root, sizeof(global_root), root, "global");
    if (result == APP_ERROR_NONE) {
        result = validate_topology(global_root, GLOBAL_ROOT_ENTRIES,
                                   sizeof(GLOBAL_ROOT_ENTRIES) / sizeof(GLOBAL_ROOT_ENTRIES[0]),
                                   false);
    }
    char path[APP_PATH_MAX_BYTES];
    storage_uuid_order_t order = {0};
    if (result == APP_ERROR_NONE) {
        result = join_path(path, sizeof(path), global_root, "macro-order.json");
    }
    if (result == APP_ERROR_NONE) {
        result = read_order(path, APP_MACROS_PER_SET_MAX, &order);
    }
    char directory[APP_PATH_MAX_BYTES];
    if (result == APP_ERROR_NONE) {
        result = join_path(directory, sizeof(directory), global_root, "macros");
    }
    if (result == APP_ERROR_NONE) {
        result = validate_directory_membership(directory, &order, true);
    }
    if (result == APP_ERROR_NONE) {
        result = writer_text(writer, "[");
    }
    bool first = true;
    for (size_t index = 0U; result == APP_ERROR_NONE && index < order.count; ++index) {
        char name[APP_UUID_STRING_LENGTH + sizeof(TREE_JSON_SUFFIX)];
        const int written = snprintf(name, sizeof(name), "%s.json", order.ids[index].value);
        if (written < 0 || (size_t)written >= sizeof(name)) {
            result = APP_ERROR_STORAGE_CORRUPT;
            break;
        }
        result = join_path(path, sizeof(path), directory, name);
        if (result == APP_ERROR_NONE) {
            result = append_macro_file(writer, &first, path, &order.ids[index], NULL,
                                       MACRO_SCOPE_GLOBAL);
        }
        if (result == APP_ERROR_NONE) {
            ++counts->global_macro_count;
        }
    }
    return result == APP_ERROR_NONE ? writer_text(writer, "]") : result;
}

static app_error_code_t append_all_procedures(tree_writer_t *writer, const char *root,
                                              const storage_set_index_t *index,
                                              storage_uuid_order_t *orders,
                                              tree_counts_t *counts) {
    app_error_code_t result = writer_text(writer, "[");
    bool first = true;
    for (size_t item = 0U; result == APP_ERROR_NONE && item < index->count; ++item) {
        char sets_root[APP_PATH_MAX_BYTES];
        char set_root[APP_PATH_MAX_BYTES];
        result = join_path(sets_root, sizeof(sets_root), root, "sets");
        if (result == APP_ERROR_NONE) {
            result = join_path(set_root, sizeof(set_root), sets_root, index->ids[item].value);
        }
        if (result == APP_ERROR_NONE) {
            result = append_ordered_procedures(writer, &first, set_root, &index->ids[item],
                                               &orders[item], counts);
        }
    }
    return result == APP_ERROR_NONE ? writer_text(writer, "]") : result;
}

static app_error_code_t append_all_progress(tree_writer_t *writer, const char *root,
                                            const storage_set_index_t *index,
                                            const storage_uuid_order_t *orders,
                                            tree_counts_t *counts) {
    app_error_code_t result = writer_text(writer, "[");
    bool first = true;
    for (size_t item = 0U; result == APP_ERROR_NONE && item < index->count; ++item) {
        char sets_root[APP_PATH_MAX_BYTES];
        char set_root[APP_PATH_MAX_BYTES];
        result = join_path(sets_root, sizeof(sets_root), root, "sets");
        if (result == APP_ERROR_NONE) {
            result = join_path(set_root, sizeof(set_root), sets_root, index->ids[item].value);
        }
        if (result == APP_ERROR_NONE) {
            result = append_progress(writer, &first, set_root, &index->ids[item], &orders[item],
                                     counts);
        }
    }
    return result == APP_ERROR_NONE ? writer_text(writer, "]") : result;
}

app_error_code_t storage_repository_tree_validate(const char *root) {
    if (root == NULL || root[0] == '\0') {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const bool live_root = strcmp(root, STORAGE_DATA_MOUNT) == 0;
    app_error_code_t result = validate_topology(
        root, LOGICAL_ROOT_ENTRIES,
        sizeof(LOGICAL_ROOT_ENTRIES) / sizeof(LOGICAL_ROOT_ENTRIES[0]), live_root);
    storage_set_index_t index = {0};
    if (result == APP_ERROR_NONE) {
        result = read_set_index(root, &index);
    }
    storage_uuid_order_t procedure_orders[APP_MACRO_SETS_MAX] = {0};
    tree_counts_t counts = {0};
    tree_writer_t writer = {0};
    if (result == APP_ERROR_NONE) {
        result = writer_text(&writer,
                             "{\"schema_version\":1,\"package_type\":\"backup\",\"sets\":");
    }
    if (result == APP_ERROR_NONE) {
        result = append_sets(&writer, root, &index, &counts);
    }
    if (result == APP_ERROR_NONE) {
        result = writer_text(&writer, ",\"macros\":");
    }
    if (result == APP_ERROR_NONE) {
        result = append_all_local_macros(&writer, root, &index, &counts);
    }
    if (result == APP_ERROR_NONE) {
        result = writer_text(&writer, ",\"global_macros\":");
    }
    if (result == APP_ERROR_NONE) {
        result = append_global_macros(&writer, root, &counts);
    }
    if (result == APP_ERROR_NONE) {
        result = writer_text(&writer, ",\"procedures\":");
    }
    if (result == APP_ERROR_NONE) {
        result = append_all_procedures(&writer, root, &index, procedure_orders, &counts);
    }
    if (result == APP_ERROR_NONE) {
        result = writer_text(&writer, ",\"progress\":");
    }
    if (result == APP_ERROR_NONE) {
        result = append_all_progress(&writer, root, &index, procedure_orders, &counts);
    }
    if (result == APP_ERROR_NONE) {
        result = writer_text(&writer, "}");
    }
    storage_package_summary_t summary = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_package_validate(writer.data, writer.length, STORAGE_PACKAGE_KIND_BACKUP,
                                          &summary);
    }
    if (result == APP_ERROR_NONE &&
        (summary.set_count != counts.set_count ||
         summary.local_macro_count != counts.local_macro_count ||
         summary.global_macro_count != counts.global_macro_count ||
         summary.procedure_count != counts.procedure_count ||
         summary.progress_count != counts.progress_count)) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    free(writer.data);
    return result;
}
