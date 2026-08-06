#include "storage_blob_internal.h"

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "app_error.h"
#include "storage_blob.h"
#include "storage_fs_ops.h"

#define STORAGE_BLOB_DIRECTORY_MODE ((mode_t)(S_IRWXU | S_IRGRP | S_IXGRP))

typedef struct {
    const storage_fs_ops_t *operations;
    const char *directory_path;
    const storage_blob_scan_observer_t *observer;
    storage_blob_entry_t *entries;
    size_t entry_count;
    size_t entry_capacity;
    storage_blob_scan_summary_t *summary;
} storage_blob_scan_context_t;

static bool is_dot_entry(const char *name) {
    return strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
}

bool storage_blob_parse_filename(const char *name, uint64_t *out_id) {
    if (out_id != NULL) {
        *out_id = 0U;
    }
    if (name == NULL || out_id == NULL || strlen(name) != STORAGE_BLOB_FILENAME_LENGTH ||
        strcmp(name + STORAGE_BLOB_ID_DIGITS, ".gz") != 0) {
        return false;
    }

    uint64_t value = 0U;
    for (size_t index = 0U; index < STORAGE_BLOB_ID_DIGITS; ++index) {
        const unsigned char character = (unsigned char)name[index];
        if (character < (unsigned char)'0' || character > (unsigned char)'9') {
            return false;
        }
        const uint64_t digit = (uint64_t)(character - (unsigned char)'0');
        if (value > (UINT64_MAX - digit) / UINT64_C(10)) {
            return false;
        }
        value = (value * UINT64_C(10)) + digit;
    }
    if (value == 0U) {
        return false;
    }
    *out_id = value;
    return true;
}

app_error_code_t storage_blob_format_filename(uint64_t blob_id, char *out_name, size_t name_size) {
    if (blob_id == 0U || out_name == NULL || name_size < STORAGE_BLOB_FILENAME_CAPACITY) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const int written = snprintf(out_name, name_size, "%020" PRIu64 ".gz", blob_id);
    if (written != (int)STORAGE_BLOB_FILENAME_LENGTH) {
        out_name[0] = '\0';
        return APP_ERROR_INTERNAL;
    }
    return APP_ERROR_NONE;
}

app_error_code_t storage_blob_derive_next_id(uint64_t persisted_next_id, bool has_max_id,
                                             uint64_t max_id, uint64_t *out_next_id) {
    if (out_next_id == NULL || (has_max_id && max_id == 0U)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    uint64_t next_id = persisted_next_id == 0U ? UINT64_C(1) : persisted_next_id;
    if (has_max_id) {
        if (max_id == UINT64_MAX) {
            return APP_ERROR_STORAGE_FULL;
        }
        const uint64_t scanned_next_id = max_id + UINT64_C(1);
        if (next_id < scanned_next_id) {
            next_id = scanned_next_id;
        }
    }
    *out_next_id = next_id;
    return APP_ERROR_NONE;
}

void storage_blob_sort_newest_first(storage_blob_entry_t *entries, size_t entry_count) {
    if (entries == NULL || entry_count < 2U) {
        return;
    }
    for (size_t index = 1U; index < entry_count; ++index) {
        const storage_blob_entry_t candidate = entries[index];
        size_t destination = index;
        while (destination > 0U && entries[destination - 1U].id < candidate.id) {
            entries[destination] = entries[destination - 1U];
            --destination;
        }
        entries[destination] = candidate;
    }
}

app_error_code_t storage_blob_prepare_directory_with_ops(const storage_fs_ops_t *operations,
                                                         const char *directory_path) {
    if (!storage_fs_ops_has_directory(operations) || directory_path == NULL ||
        directory_path[0] == '\0') {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    struct stat metadata;
    if (operations->stat_path(operations->context, directory_path, &metadata) == 0) {
        return S_ISDIR(metadata.st_mode) ? APP_ERROR_NONE : APP_ERROR_STORAGE_CORRUPT;
    }
    if (errno != ENOENT) {
        return APP_ERROR_IO;
    }
    if (operations->make_directory(operations->context, directory_path,
                                   STORAGE_BLOB_DIRECTORY_MODE) == 0) {
        return APP_ERROR_NONE;
    }
    if (errno != EEXIST ||
        operations->stat_path(operations->context, directory_path, &metadata) != 0) {
        return APP_ERROR_IO;
    }
    return S_ISDIR(metadata.st_mode) ? APP_ERROR_NONE : APP_ERROR_STORAGE_CORRUPT;
}

static app_error_code_t notify_invalid_name(const storage_blob_scan_observer_t *observer,
                                            const char *name) {
    if (observer == NULL || observer->visit_invalid_name == NULL) {
        return APP_ERROR_NONE;
    }
    return observer->visit_invalid_name(observer->context, name);
}

static app_error_code_t notify_temporary_file(const storage_blob_scan_observer_t *observer,
                                              const char *name) {
    if (observer == NULL || observer->visit_temporary_file == NULL) {
        return APP_ERROR_NONE;
    }
    return observer->visit_temporary_file(observer->context, name);
}

static app_error_code_t notify_entry(const storage_blob_scan_observer_t *observer,
                                     const storage_blob_entry_t *entry) {
    if (observer == NULL || observer->visit_entry == NULL) {
        return APP_ERROR_NONE;
    }
    return observer->visit_entry(observer->context, entry);
}

static app_error_code_t join_path(const char *directory_path, const char *name, char *out_path,
                                  size_t path_size) {
    const int written = snprintf(out_path, path_size, "%s/%s", directory_path, name);
    if (written < 0 || (size_t)written >= path_size) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t append_entry(storage_blob_entry_t **entries, size_t *entry_count,
                                     size_t *entry_capacity, const storage_blob_entry_t *entry) {
    if (entries == NULL || entry_count == NULL || entry_capacity == NULL || entry == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (*entry_count == *entry_capacity) {
        size_t new_capacity = 8U;
        if (*entry_capacity != 0U) {
            if (*entry_capacity > SIZE_MAX / 2U) {
                return APP_ERROR_INTERNAL;
            }
            new_capacity = *entry_capacity * 2U;
        }
        if (new_capacity > SIZE_MAX / sizeof(**entries)) {
            return APP_ERROR_INTERNAL;
        }
        storage_blob_entry_t *resized = realloc(*entries, new_capacity * sizeof(**entries));
        if (resized == NULL) {
            return APP_ERROR_INTERNAL;
        }
        *entries = resized;
        *entry_capacity = new_capacity;
    }
    (*entries)[*entry_count] = *entry;
    ++(*entry_count);
    return APP_ERROR_NONE;
}

static app_error_code_t process_directory_entry(storage_blob_scan_context_t *context,
                                                const char *name) {
    uint64_t blob_id = 0U;
    if (!storage_blob_parse_filename(name, &blob_id)) {
        uint64_t temporary_id = 0U;
        if (storage_blob_parse_temporary_filename(name, &temporary_id)) {
            char temporary_path[STORAGE_FS_ENTRY_NAME_MAX * 2U] = {0};
            app_error_code_t temporary_result =
                join_path(context->directory_path, name, temporary_path, sizeof(temporary_path));
            if (temporary_result != APP_ERROR_NONE) {
                return temporary_result;
            }
            struct stat temporary_metadata;
            if (context->operations->stat_path(context->operations->context, temporary_path,
                                               &temporary_metadata) != 0) {
                return APP_ERROR_IO;
            }
            if (S_ISREG(temporary_metadata.st_mode)) {
                ++context->summary->temporary_file_count;
                return notify_temporary_file(context->observer, name);
            }
        }
        ++context->summary->invalid_name_count;
        return notify_invalid_name(context->observer, name);
    }

    char path[STORAGE_FS_ENTRY_NAME_MAX * 2U] = {0};
    app_error_code_t result = join_path(context->directory_path, name, path, sizeof(path));
    if (result != APP_ERROR_NONE) {
        return result;
    }

    struct stat metadata;
    if (context->operations->stat_path(context->operations->context, path, &metadata) != 0) {
        return APP_ERROR_IO;
    }
    if (!S_ISREG(metadata.st_mode) || metadata.st_size < 0 ||
        (uintmax_t)metadata.st_size > (uintmax_t)SIZE_MAX) {
        ++context->summary->invalid_name_count;
        return notify_invalid_name(context->observer, name);
    }

    const storage_blob_entry_t entry = {
        .id = blob_id,
        .stored_bytes = (size_t)metadata.st_size,
    };
    result =
        append_entry(&context->entries, &context->entry_count, &context->entry_capacity, &entry);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (!context->summary->has_max_id || blob_id > context->summary->max_id) {
        context->summary->has_max_id = true;
        context->summary->max_id = blob_id;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t scan_directory_entries(storage_blob_scan_context_t *context,
                                               void *directory) {
    while (true) {
        char name[STORAGE_FS_ENTRY_NAME_MAX] = {0};
        bool end = false;
        if (context->operations->read_directory(context->operations->context, directory, name,
                                                sizeof(name), &end) != 0) {
            return APP_ERROR_IO;
        }
        if (end) {
            return APP_ERROR_NONE;
        }
        if (is_dot_entry(name)) {
            continue;
        }

        const app_error_code_t result = process_directory_entry(context, name);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
}

static app_error_code_t notify_entries_newest_first(storage_blob_scan_context_t *context) {
    storage_blob_sort_newest_first(context->entries, context->entry_count);
    for (size_t index = 0U; index < context->entry_count; ++index) {
        const app_error_code_t result = notify_entry(context->observer, &context->entries[index]);
        if (result != APP_ERROR_NONE) {
            return result;
        }
    }
    return APP_ERROR_NONE;
}

app_error_code_t storage_blob_scan_with_ops(const storage_fs_ops_t *operations,
                                            const char *directory_path, uint64_t persisted_next_id,
                                            const storage_blob_scan_observer_t *observer,
                                            storage_blob_scan_summary_t *out_summary) {
    if (!storage_fs_ops_has_directory(operations) || directory_path == NULL ||
        directory_path[0] == '\0' || out_summary == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    *out_summary = (storage_blob_scan_summary_t){0};
    void *directory = operations->open_directory(operations->context, directory_path);
    if (directory == NULL) {
        return APP_ERROR_IO;
    }

    storage_blob_scan_context_t context = {
        .operations = operations,
        .directory_path = directory_path,
        .observer = observer,
        .entries = NULL,
        .entry_count = 0U,
        .entry_capacity = 0U,
        .summary = out_summary,
    };
    app_error_code_t result = scan_directory_entries(&context, directory);

    if (operations->close_directory(operations->context, directory) != 0 &&
        result == APP_ERROR_NONE) {
        result = APP_ERROR_IO;
    }

    out_summary->valid_count = context.entry_count;
    if (result == APP_ERROR_NONE) {
        result = storage_blob_derive_next_id(persisted_next_id, out_summary->has_max_id,
                                             out_summary->max_id, &out_summary->next_id);
    }
    if (result == APP_ERROR_NONE) {
        result = notify_entries_newest_first(&context);
    }

    free(context.entries);
    return result;
}
