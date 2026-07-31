from pathlib import Path

def replace_once(path: str, old: str, new: str) -> None:
    file = Path(path)
    text = file.read_text()
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{path}: expected one match, found {count}")
    file.write_text(text.replace(old, new, 1))

internal = Path("firmware/components/storage/storage_transaction_internal.h")
text = internal.read_text()
old = '''typedef app_error_code_t (*storage_transaction_set_index_presence_fn)(void *context,
                                                                      const app_uuid_t *set_id,
                                                                      bool should_be_present);
'''
new = old + '''typedef app_error_code_t (*storage_transaction_validate_set_fn)(
    void *context, const char *path, const app_uuid_t *set_id, uint32_t expected_revision);
typedef app_error_code_t (*storage_transaction_remove_tree_fn)(void *context, const char *path);
'''
if text.count(old) != 1:
    raise SystemExit("transaction callback insertion point changed")
text = text.replace(old, new, 1)
old_sig = '''app_error_code_t storage_transaction_recover_all_with_ops(
    const storage_fs_ops_t *operations, storage_uuid_generate_fn generate_uuid, void *uuid_context,
    storage_transaction_set_index_presence_fn set_index_presence, void *index_context);
'''
new_sig = '''app_error_code_t storage_transaction_recover_replace_with_ops(
    storage_transaction_manifest_t *manifest, const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid, void *uuid_context,
    storage_transaction_set_index_presence_fn set_index_presence, void *index_context,
    storage_transaction_validate_set_fn validate_set, void *validation_context,
    storage_transaction_remove_tree_fn remove_tree, void *remove_context);

app_error_code_t storage_transaction_recover_all_with_ops(
    const storage_fs_ops_t *operations, storage_uuid_generate_fn generate_uuid, void *uuid_context,
    storage_transaction_set_index_presence_fn set_index_presence, void *index_context,
    storage_transaction_validate_set_fn validate_set, void *validation_context,
    storage_transaction_remove_tree_fn remove_tree, void *remove_context);
'''
if text.count(old_sig) != 1:
    raise SystemExit("recover signature changed")
internal.write_text(text.replace(old_sig, new_sig, 1))

source = Path("firmware/components/storage/storage_transaction.c")
text = source.read_text()
old_callbacks = '''static app_error_code_t production_set_index_presence(void *context, const app_uuid_t *set_id,
                                                       bool should_be_present) {
    (void)context;
    return storage_repository_set_index_presence(set_id, should_be_present);
}

static bool operations_valid(const storage_fs_ops_t *operations,
                             storage_uuid_generate_fn generate_uuid,
                             storage_transaction_set_index_presence_fn set_index_presence) {
    return storage_fs_ops_has_directory(operations) && generate_uuid != NULL &&
           set_index_presence != NULL;
}
'''
new_callbacks = '''static app_error_code_t production_set_index_presence(void *context, const app_uuid_t *set_id,
                                                       bool should_be_present) {
    (void)context;
    return storage_repository_set_index_presence(set_id, should_be_present);
}

#ifdef ESP_PLATFORM
static app_error_code_t production_validate_set(void *context, const char *path,
                                                const app_uuid_t *set_id,
                                                uint32_t expected_revision) {
    (void)context;
    char set_path[APP_PATH_MAX_BYTES];
    const int written = snprintf(set_path, sizeof(set_path), "%s/set.json", path);
    if (written < 0 || (size_t)written >= sizeof(set_path)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char *data = NULL;
    size_t length = 0U;
    app_error_code_t result =
        storage_repository_read_bounded_file(set_path, STORAGE_SET_FILE_MAX_BYTES, &data, &length);
    macro_set_t set = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_repository_parse_set_json(data, length, &set);
    }
    free(data);
    if (result == APP_ERROR_NONE &&
        (!app_uuid_equal(&set.id, set_id) || set.revision != expected_revision)) {
        result = APP_ERROR_STORAGE_CORRUPT;
    }
    return result;
}

static app_error_code_t production_remove_tree(void *context, const char *path) {
    (void)context;
    return storage_repository_remove_tree(path);
}
#else
static app_error_code_t production_validate_set(void *context, const char *path,
                                                const app_uuid_t *set_id,
                                                uint32_t expected_revision) {
    (void)context;
    (void)path;
    (void)set_id;
    (void)expected_revision;
    return APP_ERROR_STORAGE_UNAVAILABLE;
}

static app_error_code_t production_remove_tree(void *context, const char *path) {
    (void)context;
    (void)path;
    return APP_ERROR_STORAGE_UNAVAILABLE;
}
#endif

static bool operations_valid(const storage_fs_ops_t *operations,
                             storage_uuid_generate_fn generate_uuid,
                             storage_transaction_set_index_presence_fn set_index_presence,
                             storage_transaction_validate_set_fn validate_set,
                             storage_transaction_remove_tree_fn remove_tree) {
    return storage_fs_ops_has_directory(operations) && generate_uuid != NULL &&
           set_index_presence != NULL && validate_set != NULL && remove_tree != NULL;
}
'''
if text.count(old_callbacks) != 1:
    raise SystemExit("production callback block changed")
text = text.replace(old_callbacks, new_callbacks, 1)
old_revisions = '''    case STORAGE_TRANSACTION_REPLACE_SET:
    case STORAGE_TRANSACTION_RESTORE:
    case STORAGE_TRANSACTION_MIGRATE:
        return true;
'''
new_revisions = '''    case STORAGE_TRANSACTION_REPLACE_SET:
        return manifest->expected_revision != 0U && manifest->replacement_revision != 0U;
    case STORAGE_TRANSACTION_RESTORE:
    case STORAGE_TRANSACTION_MIGRATE:
        return true;
'''
if text.count(old_revisions) != 1:
    raise SystemExit("manifest revision switch changed")
text = text.replace(old_revisions, new_revisions, 1)
old_function = '''app_error_code_t storage_transaction_recover_all_with_ops(
    const storage_fs_ops_t *operations, storage_uuid_generate_fn generate_uuid, void *uuid_context,
    storage_transaction_set_index_presence_fn set_index_presence, void *index_context) {
    if (!operations_valid(operations, generate_uuid, set_index_presence)) {
'''
new_function = '''app_error_code_t storage_transaction_recover_all_with_ops(
    const storage_fs_ops_t *operations, storage_uuid_generate_fn generate_uuid, void *uuid_context,
    storage_transaction_set_index_presence_fn set_index_presence, void *index_context,
    storage_transaction_validate_set_fn validate_set, void *validation_context,
    storage_transaction_remove_tree_fn remove_tree, void *remove_context) {
    if (!operations_valid(operations, generate_uuid, set_index_presence, validate_set, remove_tree)) {
'''
if text.count(old_function) != 1:
    raise SystemExit("recover implementation signature changed")
text = text.replace(old_function, new_function, 1)
old_switch = '''            case STORAGE_TRANSACTION_REPLACE_SET:
            case STORAGE_TRANSACTION_RESTORE:
            case STORAGE_TRANSACTION_MIGRATE:
            default:
                result = APP_ERROR_STORAGE_CORRUPT;
                break;
'''
new_switch = '''            case STORAGE_TRANSACTION_REPLACE_SET:
                result = storage_transaction_recover_replace_with_ops(
                    &manifest, operations, generate_uuid, uuid_context, set_index_presence,
                    index_context, validate_set, validation_context, remove_tree, remove_context);
                break;
            case STORAGE_TRANSACTION_RESTORE:
            case STORAGE_TRANSACTION_MIGRATE:
            default:
                result = APP_ERROR_STORAGE_CORRUPT;
                break;
'''
if text.count(old_switch) != 1:
    raise SystemExit("recover switch changed")
text = text.replace(old_switch, new_switch, 1)
old_wrapper = '''        storage_transaction_recover_all_with_ops(storage_fs_ops_posix(), production_uuid_generate,
                                                 NULL, production_set_index_presence, NULL);
'''
new_wrapper = '''        storage_transaction_recover_all_with_ops(
            storage_fs_ops_posix(), production_uuid_generate, NULL, production_set_index_presence,
            NULL, production_validate_set, NULL, production_remove_tree, NULL);
'''
if text.count(old_wrapper) != 1:
    raise SystemExit("production recover wrapper changed")
source.write_text(text.replace(old_wrapper, new_wrapper, 1))

replace_source = r'''#include "storage_transaction_internal.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_limits.h"
#include "storage.h"
#include "storage_fs_ops.h"

static app_error_code_t map_error_number(int error_number) {
    if (error_number == ENOENT) {
        return APP_ERROR_NOT_FOUND;
    }
    if (error_number == ENOSPC) {
        return APP_ERROR_STORAGE_FULL;
    }
    return APP_ERROR_IO;
}

static app_error_code_t path_is_directory(const char *path, const storage_fs_ops_t *operations,
                                          bool *out_exists) {
    *out_exists = false;
    struct stat metadata;
    if (operations->stat_path(operations->context, path, &metadata) == 0) {
        if (!S_ISDIR(metadata.st_mode)) {
            return APP_ERROR_STORAGE_CORRUPT;
        }
        *out_exists = true;
        return APP_ERROR_NONE;
    }
    const int stat_error = errno;
    return stat_error == ENOENT ? APP_ERROR_NONE : map_error_number(stat_error);
}

static app_error_code_t sync_rename_parents(const char *source, const char *destination,
                                            const storage_fs_ops_t *operations) {
    if (storage_fs_sync_parent_path(operations->context, source) != 0) {
        return map_error_number(errno);
    }
    if (storage_fs_sync_parent_path(operations->context, destination) != 0) {
        return map_error_number(errno);
    }
    return APP_ERROR_NONE;
}

static app_error_code_t rename_and_sync(const char *source, const char *destination,
                                        const storage_fs_ops_t *operations) {
    if (operations->rename_path(operations->context, source, destination) != 0) {
        return map_error_number(errno);
    }
    return sync_rename_parents(source, destination, operations);
}

static app_error_code_t write_phase(storage_transaction_manifest_t *manifest,
                                    storage_transaction_phase_t phase,
                                    const storage_fs_ops_t *operations,
                                    storage_uuid_generate_fn generate_uuid, void *uuid_context) {
    manifest->phase = phase;
    const app_error_code_t result = storage_transaction_write_manifest_with_ops(
        manifest, operations, generate_uuid, uuid_context);
    return result == APP_ERROR_INVALID_ARGUMENT ? APP_ERROR_STORAGE_CORRUPT : result;
}

static app_error_code_t remove_manifest(const storage_transaction_manifest_t *manifest,
                                        const storage_fs_ops_t *operations) {
    char path[APP_PATH_MAX_BYTES];
    const int written = snprintf(path, sizeof(path), STORAGE_DATA_MOUNT "/transactions/%s.bin",
                                 manifest->id.value);
    if (written < 0 || (size_t)written >= sizeof(path)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    if (operations->unlink_path(operations->context, path) != 0) {
        return map_error_number(errno);
    }
    return storage_fs_sync_parent_path(operations->context, path) == 0
               ? APP_ERROR_NONE
               : map_error_number(errno);
}

static app_error_code_t parse_set_path(const char *path, app_uuid_t *out_set_id) {
    static const char prefix[] = STORAGE_DATA_MOUNT "/sets/";
    const size_t prefix_length = sizeof(prefix) - 1U;
    if (path == NULL || out_set_id == NULL || strncmp(path, prefix, prefix_length) != 0 ||
        strchr(path + prefix_length, '/') != NULL ||
        app_uuid_parse(path + prefix_length, out_set_id) != APP_ERROR_NONE) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return APP_ERROR_NONE;
}

static bool staging_path_matches(const char *actual, const app_uuid_t *transaction_id) {
    char expected[APP_PATH_MAX_BYTES];
    const int written = snprintf(expected, sizeof(expected), STORAGE_DATA_MOUNT "/staging/%s",
                                 transaction_id->value);
    return written >= 0 && (size_t)written < sizeof(expected) && strcmp(actual, expected) == 0;
}

static bool backup_path_matches(const char *actual, const app_uuid_t *set_id,
                                const app_uuid_t *transaction_id) {
    char expected[APP_PATH_MAX_BYTES];
    const int written = snprintf(expected, sizeof(expected), STORAGE_DATA_MOUNT "/trash/%s-%s",
                                 set_id->value, transaction_id->value);
    return written >= 0 && (size_t)written < sizeof(expected) && strcmp(actual, expected) == 0;
}

static app_error_code_t validate_manifest_paths(const storage_transaction_manifest_t *manifest,
                                                app_uuid_t *out_set_id) {
    app_error_code_t result = parse_set_path(manifest->destination, out_set_id);
    if (result != APP_ERROR_NONE || strcmp(manifest->source, manifest->destination) != 0) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    if (!staging_path_matches(manifest->staging, &manifest->id) ||
        !backup_path_matches(manifest->backup, out_set_id, &manifest->id)) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t validate_tree(storage_transaction_validate_set_fn validate_set,
                                      void *validation_context, const char *path,
                                      const app_uuid_t *set_id, uint32_t revision) {
    const app_error_code_t result = validate_set(validation_context, path, set_id, revision);
    return result == APP_ERROR_INVALID_ARGUMENT ? APP_ERROR_STORAGE_CORRUPT : result;
}

static app_error_code_t recover_staged(
    storage_transaction_manifest_t *manifest, const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid, void *uuid_context,
    storage_transaction_validate_set_fn validate_set, void *validation_context,
    const app_uuid_t *set_id) {
    bool staging_exists = false;
    bool destination_exists = false;
    bool backup_exists = false;
    app_error_code_t result = path_is_directory(manifest->staging, operations, &staging_exists);
    if (result == APP_ERROR_NONE) {
        result = path_is_directory(manifest->destination, operations, &destination_exists);
    }
    if (result == APP_ERROR_NONE) {
        result = path_is_directory(manifest->backup, operations, &backup_exists);
    }
    if (result != APP_ERROR_NONE || !staging_exists || destination_exists == backup_exists) {
        return result == APP_ERROR_NONE ? APP_ERROR_STORAGE_CORRUPT : result;
    }
    result = validate_tree(validate_set, validation_context, manifest->staging, set_id,
                           manifest->replacement_revision);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (destination_exists) {
        result = validate_tree(validate_set, validation_context, manifest->destination, set_id,
                               manifest->expected_revision);
        if (result == APP_ERROR_NONE) {
            result = rename_and_sync(manifest->destination, manifest->backup, operations);
        }
    } else {
        result = validate_tree(validate_set, validation_context, manifest->backup, set_id,
                               manifest->expected_revision);
        if (result == APP_ERROR_NONE) {
            result = sync_rename_parents(manifest->destination, manifest->backup, operations);
        }
    }
    return result == APP_ERROR_NONE
               ? write_phase(manifest, STORAGE_TRANSACTION_BACKED_UP, operations, generate_uuid,
                             uuid_context)
               : result;
}

static app_error_code_t recover_backed_up(
    storage_transaction_manifest_t *manifest, const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid, void *uuid_context,
    storage_transaction_validate_set_fn validate_set, void *validation_context,
    const app_uuid_t *set_id) {
    bool staging_exists = false;
    bool destination_exists = false;
    bool backup_exists = false;
    app_error_code_t result = path_is_directory(manifest->staging, operations, &staging_exists);
    if (result == APP_ERROR_NONE) {
        result = path_is_directory(manifest->destination, operations, &destination_exists);
    }
    if (result == APP_ERROR_NONE) {
        result = path_is_directory(manifest->backup, operations, &backup_exists);
    }
    if (result != APP_ERROR_NONE || !backup_exists || staging_exists == destination_exists) {
        return result == APP_ERROR_NONE ? APP_ERROR_STORAGE_CORRUPT : result;
    }
    result = validate_tree(validate_set, validation_context, manifest->backup, set_id,
                           manifest->expected_revision);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (staging_exists) {
        result = validate_tree(validate_set, validation_context, manifest->staging, set_id,
                               manifest->replacement_revision);
        if (result == APP_ERROR_NONE) {
            result = rename_and_sync(manifest->staging, manifest->destination, operations);
        }
    } else {
        result = validate_tree(validate_set, validation_context, manifest->destination, set_id,
                               manifest->replacement_revision);
        if (result == APP_ERROR_NONE) {
            result = sync_rename_parents(manifest->staging, manifest->destination, operations);
        }
    }
    return result == APP_ERROR_NONE
               ? write_phase(manifest, STORAGE_TRANSACTION_ACTIVATED, operations, generate_uuid,
                             uuid_context)
               : result;
}

static app_error_code_t validate_activated_paths(
    const storage_transaction_manifest_t *manifest, const storage_fs_ops_t *operations,
    storage_transaction_validate_set_fn validate_set, void *validation_context,
    const app_uuid_t *set_id) {
    bool staging_exists = false;
    bool destination_exists = false;
    bool backup_exists = false;
    app_error_code_t result = path_is_directory(manifest->staging, operations, &staging_exists);
    if (result == APP_ERROR_NONE) {
        result = path_is_directory(manifest->destination, operations, &destination_exists);
    }
    if (result == APP_ERROR_NONE) {
        result = path_is_directory(manifest->backup, operations, &backup_exists);
    }
    if (result != APP_ERROR_NONE || staging_exists || !destination_exists || !backup_exists) {
        return result == APP_ERROR_NONE ? APP_ERROR_STORAGE_CORRUPT : result;
    }
    result = validate_tree(validate_set, validation_context, manifest->destination, set_id,
                           manifest->replacement_revision);
    if (result == APP_ERROR_NONE) {
        result = validate_tree(validate_set, validation_context, manifest->backup, set_id,
                               manifest->expected_revision);
    }
    return result;
}

static app_error_code_t recover_activated(
    storage_transaction_manifest_t *manifest, const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid, void *uuid_context,
    storage_transaction_set_index_presence_fn set_index_presence, void *index_context,
    storage_transaction_validate_set_fn validate_set, void *validation_context,
    const app_uuid_t *set_id) {
    app_error_code_t result = validate_activated_paths(manifest, operations, validate_set,
                                                       validation_context, set_id);
    if (result == APP_ERROR_NONE) {
        result = set_index_presence(index_context, set_id, true);
    }
    return result == APP_ERROR_NONE
               ? write_phase(manifest, STORAGE_TRANSACTION_INDEXED, operations, generate_uuid,
                             uuid_context)
               : result;
}

static app_error_code_t recover_indexed(
    storage_transaction_manifest_t *manifest, const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid, void *uuid_context,
    storage_transaction_set_index_presence_fn set_index_presence, void *index_context,
    storage_transaction_validate_set_fn validate_set, void *validation_context,
    const app_uuid_t *set_id) {
    app_error_code_t result = validate_activated_paths(manifest, operations, validate_set,
                                                       validation_context, set_id);
    if (result == APP_ERROR_NONE) {
        result = set_index_presence(index_context, set_id, true);
    }
    return result == APP_ERROR_NONE
               ? write_phase(manifest, STORAGE_TRANSACTION_COMPLETE, operations, generate_uuid,
                             uuid_context)
               : result;
}

static app_error_code_t recover_complete(
    const storage_transaction_manifest_t *manifest, const storage_fs_ops_t *operations,
    storage_transaction_set_index_presence_fn set_index_presence, void *index_context,
    storage_transaction_validate_set_fn validate_set, void *validation_context,
    storage_transaction_remove_tree_fn remove_tree, void *remove_context,
    const app_uuid_t *set_id) {
    bool staging_exists = false;
    bool destination_exists = false;
    bool backup_exists = false;
    app_error_code_t result = path_is_directory(manifest->staging, operations, &staging_exists);
    if (result == APP_ERROR_NONE) {
        result = path_is_directory(manifest->destination, operations, &destination_exists);
    }
    if (result == APP_ERROR_NONE) {
        result = path_is_directory(manifest->backup, operations, &backup_exists);
    }
    if (result != APP_ERROR_NONE || staging_exists || !destination_exists) {
        return result == APP_ERROR_NONE ? APP_ERROR_STORAGE_CORRUPT : result;
    }
    result = validate_tree(validate_set, validation_context, manifest->destination, set_id,
                           manifest->replacement_revision);
    if (result == APP_ERROR_NONE) {
        result = set_index_presence(index_context, set_id, true);
    }
    if (result == APP_ERROR_NONE && backup_exists) {
        result = validate_tree(validate_set, validation_context, manifest->backup, set_id,
                               manifest->expected_revision);
    }
    if (result == APP_ERROR_NONE && backup_exists) {
        result = remove_tree(remove_context, manifest->backup);
        if (result == APP_ERROR_NONE &&
            storage_fs_sync_parent_path(operations->context, manifest->backup) != 0) {
            result = map_error_number(errno);
        }
    }
    return result == APP_ERROR_NONE ? remove_manifest(manifest, operations) : result;
}

app_error_code_t storage_transaction_recover_replace_with_ops(
    storage_transaction_manifest_t *manifest, const storage_fs_ops_t *operations,
    storage_uuid_generate_fn generate_uuid, void *uuid_context,
    storage_transaction_set_index_presence_fn set_index_presence, void *index_context,
    storage_transaction_validate_set_fn validate_set, void *validation_context,
    storage_transaction_remove_tree_fn remove_tree, void *remove_context) {
    if (manifest == NULL || !storage_fs_ops_has_directory(operations) || generate_uuid == NULL ||
        set_index_presence == NULL || validate_set == NULL || remove_tree == NULL ||
        manifest->type != STORAGE_TRANSACTION_REPLACE_SET || manifest->expected_revision == 0U ||
        manifest->replacement_revision == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_uuid_t set_id = {0};
    app_error_code_t result = validate_manifest_paths(manifest, &set_id);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (manifest->phase == STORAGE_TRANSACTION_STAGED) {
        result = recover_staged(manifest, operations, generate_uuid, uuid_context, validate_set,
                                validation_context, &set_id);
    }
    if (result == APP_ERROR_NONE && manifest->phase == STORAGE_TRANSACTION_BACKED_UP) {
        result = recover_backed_up(manifest, operations, generate_uuid, uuid_context, validate_set,
                                   validation_context, &set_id);
    }
    if (result == APP_ERROR_NONE && manifest->phase == STORAGE_TRANSACTION_ACTIVATED) {
        result = recover_activated(manifest, operations, generate_uuid, uuid_context,
                                   set_index_presence, index_context, validate_set,
                                   validation_context, &set_id);
    }
    if (result == APP_ERROR_NONE && manifest->phase == STORAGE_TRANSACTION_INDEXED) {
        result = recover_indexed(manifest, operations, generate_uuid, uuid_context,
                                 set_index_presence, index_context, validate_set,
                                 validation_context, &set_id);
    }
    if (result == APP_ERROR_NONE && manifest->phase == STORAGE_TRANSACTION_COMPLETE) {
        result = recover_complete(manifest, operations, set_index_presence, index_context,
                                  validate_set, validation_context, remove_tree, remove_context,
                                  &set_id);
    }
    if (result == APP_ERROR_NONE && manifest->phase < STORAGE_TRANSACTION_STAGED) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    return result;
}
'''
Path("firmware/components/storage/storage_transaction_replace.c").write_text(replace_source)

cmake = Path("firmware/components/storage/CMakeLists.txt")
text = cmake.read_text()
needle = '    "storage_transaction.c"\n'
if text.count(needle) != 1 or "storage_transaction_replace.c" in text:
    raise SystemExit("firmware CMake transaction source point changed")
cmake.write_text(text.replace(needle, needle + '    "storage_transaction_replace.c"\n', 1))

host_cmake = Path("tests/host/CMakeLists.txt")
text = host_cmake.read_text()
needle = "    ../../firmware/components/storage/storage_transaction.c\n"
count = text.count(needle)
if count < 4 or "storage_transaction_replace.c" in text:
    raise SystemExit(f"host CMake transaction source count changed: {count}")
host_cmake.write_text(
    text.replace(needle, needle + "    ../../firmware/components/storage/storage_transaction_replace.c\n")
)

tests = Path("tests/host/test_storage_transactions.c")
text = tests.read_text()
old_globals = '''typedef struct {
    app_uuid_t ids[INDEX_CALL_CAPACITY];
    bool presence[INDEX_CALL_CAPACITY];
    size_t count;
    size_t fail_on_call;
    app_error_code_t failure;
} index_fixture_t;
'''
new_globals = old_globals + '''
typedef struct {
    size_t calls;
    size_t fail_on_call;
    app_error_code_t failure;
} validation_fixture_t;

typedef struct {
    size_t calls;
    size_t fail_on_call;
    app_error_code_t failure;
} remove_fixture_t;

static validation_fixture_t validation_fixture;
static remove_fixture_t remove_fixture;
'''
if text.count(old_globals) != 1:
    raise SystemExit("test fixture insertion point changed")
text = text.replace(old_globals, new_globals, 1)
old_index_end = '''    return APP_ERROR_NONE;
}

app_error_code_t storage_repository_set_index_presence(
'''
new_index_end = '''    return APP_ERROR_NONE;
}

static app_error_code_t validate_set(void *context,
                                     const char *path,
                                     const app_uuid_t *set_id,
                                     uint32_t expected_revision)
{
    (void)context;
    TEST_CHECK(path != NULL);
    TEST_CHECK(set_id != NULL);
    TEST_CHECK(expected_revision != 0U);
    struct stat metadata;
    TEST_CHECK(stat(path, &metadata) == 0);
    TEST_CHECK(S_ISDIR(metadata.st_mode));
    ++validation_fixture.calls;
    if (validation_fixture.fail_on_call != 0U &&
        validation_fixture.calls == validation_fixture.fail_on_call) {
        return validation_fixture.failure;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t remove_tree(void *context, const char *path)
{
    (void)context;
    TEST_CHECK(path != NULL);
    ++remove_fixture.calls;
    if (remove_fixture.fail_on_call != 0U &&
        remove_fixture.calls == remove_fixture.fail_on_call) {
        return remove_fixture.failure;
    }
    return rmdir(path) == 0 ? APP_ERROR_NONE : APP_ERROR_IO;
}

app_error_code_t storage_repository_set_index_presence(
'''
if text.count(old_index_end) != 1:
    raise SystemExit("test callback insertion point changed")
text = text.replace(old_index_end, new_index_end, 1)
old_reset = '''static void reset_storage(void)
{
    char command[APP_PATH_MAX_BYTES + 32U];
'''
new_reset = '''static void reset_storage(void)
{
    memset(&validation_fixture, 0, sizeof(validation_fixture));
    memset(&remove_fixture, 0, sizeof(remove_fixture));
    validation_fixture.failure = APP_ERROR_IO;
    remove_fixture.failure = APP_ERROR_IO;
    char command[APP_PATH_MAX_BYTES + 32U];
'''
if text.count(old_reset) != 1:
    raise SystemExit("reset insertion point changed")
text = text.replace(old_reset, new_reset, 1)
old_recover = '''    return storage_transaction_recover_all_with_ops(operations,
                                                     generate_uuid,
                                                     uuids,
                                                     update_index,
                                                     index);
'''
new_recover = '''    return storage_transaction_recover_all_with_ops(operations,
                                                     generate_uuid,
                                                     uuids,
                                                     update_index,
                                                     index,
                                                     validate_set,
                                                     NULL,
                                                     remove_tree,
                                                     NULL);
'''
if text.count(old_recover) != 1:
    raise SystemExit("test recover helper changed")
text = text.replace(old_recover, new_recover, 1)
old_invalid = '''                          update_index,
                          &index));
'''
new_invalid = '''                          update_index,
                          &index,
                          validate_set,
                          NULL,
                          remove_tree,
                          NULL));
'''
if text.count(old_invalid) != 1:
    raise SystemExit(f"invalid argument call count changed: {text.count(old_invalid)}")
text = text.replace(old_invalid, new_invalid, 1)
helper_anchor = '''static void write_manifest(storage_transaction_manifest_t *manifest,
'''
replace_helper = '''static storage_transaction_manifest_t make_replace_manifest(
    const char *transaction_value,
    const char *set_value,
    storage_transaction_phase_t phase)
{
    storage_transaction_manifest_t manifest = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = parse_uuid(transaction_value),
        .type = STORAGE_TRANSACTION_REPLACE_SET,
        .phase = phase,
        .expected_revision = 3U,
        .replacement_revision = 7U,
    };
    set_path(manifest.source, sizeof(manifest.source), set_value);
    staging_path(manifest.staging, sizeof(manifest.staging), &manifest.id);
    set_path(manifest.destination, sizeof(manifest.destination), set_value);
    trash_path(manifest.backup, sizeof(manifest.backup), set_value, &manifest.id);
    return manifest;
}

'''
if text.count(helper_anchor) != 1:
    raise SystemExit("replace helper anchor changed")
text = text.replace(helper_anchor, replace_helper + helper_anchor, 1)
main_anchor = '''int main(void)
{
'''
replace_tests = '''static void test_replace_recovery_is_idempotent(void)
{
    reset_storage();
    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {0};
    index_fixture_t index = {.failure = APP_ERROR_IO};
    storage_transaction_manifest_t manifest = make_replace_manifest(
        "00000000-0000-4000-8000-000000000100",
        "10000000-0000-4000-8000-000000000100",
        STORAGE_TRANSACTION_STAGED);
    create_directory(manifest.staging);
    create_directory(manifest.destination);
    write_manifest(&manifest, &operations, &uuids);
    fake_fs_backend_reset(&filesystem);

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, recover(&operations, &uuids, &index));
    TEST_CHECK(!path_exists(manifest.staging));
    TEST_CHECK(path_exists(manifest.destination));
    TEST_CHECK(!path_exists(manifest.backup));
    char path[APP_PATH_MAX_BYTES];
    transaction_path(path, sizeof(path), &manifest.id);
    TEST_CHECK(!path_exists(path));
    TEST_CHECK_EQ_U64(3U, index.count);
    TEST_CHECK(index.presence[0]);
    TEST_CHECK(index.presence[1]);
    TEST_CHECK(index.presence[2]);
    TEST_CHECK(validation_fixture.calls >= 7U);
    TEST_CHECK_EQ_U64(1U, remove_fixture.calls);

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, recover(&operations, &uuids, &index));
    TEST_CHECK_EQ_U64(3U, index.count);
}

static void test_replace_recovers_after_unrecorded_renames(void)
{
    static const storage_transaction_phase_t phases[] = {
        STORAGE_TRANSACTION_STAGED,
        STORAGE_TRANSACTION_BACKED_UP,
    };
    for (size_t case_index = 0U;
         case_index < sizeof(phases) / sizeof(phases[0]);
         ++case_index) {
        reset_storage();
        fake_fs_backend_t filesystem;
        fake_fs_backend_reset(&filesystem);
        storage_fs_ops_t operations = make_operations(&filesystem);
        uuid_sequence_t uuids = {0};
        index_fixture_t index = {.failure = APP_ERROR_IO};
        storage_transaction_manifest_t manifest = make_replace_manifest(
            case_index == 0U ? "00000000-0000-4000-8000-000000000101"
                             : "00000000-0000-4000-8000-000000000102",
            case_index == 0U ? "10000000-0000-4000-8000-000000000101"
                             : "10000000-0000-4000-8000-000000000102",
            phases[case_index]);
        create_directory(manifest.backup);
        if (phases[case_index] == STORAGE_TRANSACTION_STAGED) {
            create_directory(manifest.staging);
        } else {
            create_directory(manifest.destination);
        }
        write_manifest(&manifest, &operations, &uuids);
        fake_fs_backend_reset(&filesystem);

        TEST_CHECK_EQ_INT(APP_ERROR_NONE, recover(&operations, &uuids, &index));
        TEST_CHECK(path_exists(manifest.destination));
        TEST_CHECK(!path_exists(manifest.backup));
        char path[APP_PATH_MAX_BYTES];
        transaction_path(path, sizeof(path), &manifest.id);
        TEST_CHECK(!path_exists(path));
    }
}

static void test_replace_failure_is_retryable(void)
{
    reset_storage();
    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {0};
    index_fixture_t index = {.failure = APP_ERROR_IO};
    storage_transaction_manifest_t manifest = make_replace_manifest(
        "00000000-0000-4000-8000-000000000103",
        "10000000-0000-4000-8000-000000000103",
        STORAGE_TRANSACTION_STAGED);
    create_directory(manifest.staging);
    create_directory(manifest.destination);
    write_manifest(&manifest, &operations, &uuids);
    fake_fs_backend_reset(&filesystem);
    fake_fs_backend_fail_on(&filesystem, FAKE_FS_RENAME, 1U, EIO);

    TEST_CHECK_EQ_INT(APP_ERROR_IO, recover(&operations, &uuids, &index));
    TEST_CHECK(path_exists(manifest.staging));
    TEST_CHECK(path_exists(manifest.destination));
    fake_fs_backend_reset(&filesystem);
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, recover(&operations, &uuids, &index));

    reset_storage();
    fake_fs_backend_reset(&filesystem);
    operations = make_operations(&filesystem);
    uuids = (uuid_sequence_t){0};
    index = (index_fixture_t){
        .fail_on_call = 1U,
        .failure = APP_ERROR_IO,
    };
    manifest = make_replace_manifest(
        "00000000-0000-4000-8000-000000000104",
        "10000000-0000-4000-8000-000000000104",
        STORAGE_TRANSACTION_ACTIVATED);
    create_directory(manifest.destination);
    create_directory(manifest.backup);
    write_manifest(&manifest, &operations, &uuids);
    fake_fs_backend_reset(&filesystem);
    TEST_CHECK_EQ_INT(APP_ERROR_IO, recover(&operations, &uuids, &index));
    index.fail_on_call = 0U;
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, recover(&operations, &uuids, &index));

    reset_storage();
    fake_fs_backend_reset(&filesystem);
    operations = make_operations(&filesystem);
    uuids = (uuid_sequence_t){0};
    index = (index_fixture_t){.failure = APP_ERROR_IO};
    manifest = make_replace_manifest(
        "00000000-0000-4000-8000-000000000105",
        "10000000-0000-4000-8000-000000000105",
        STORAGE_TRANSACTION_COMPLETE);
    create_directory(manifest.destination);
    create_directory(manifest.backup);
    write_manifest(&manifest, &operations, &uuids);
    remove_fixture.fail_on_call = 1U;
    fake_fs_backend_reset(&filesystem);
    TEST_CHECK_EQ_INT(APP_ERROR_IO, recover(&operations, &uuids, &index));
    TEST_CHECK(path_exists(manifest.backup));
    remove_fixture.fail_on_call = 0U;
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, recover(&operations, &uuids, &index));
}

'''
if text.count(main_anchor) != 1:
    raise SystemExit("test main anchor changed")
text = text.replace(main_anchor, replace_tests + main_anchor, 1)
old_main_calls = '''    test_delete_recovery_is_idempotent();
    test_conflicting_create_paths_are_preserved();
'''
new_main_calls = '''    test_delete_recovery_is_idempotent();
    test_replace_recovery_is_idempotent();
    test_replace_recovers_after_unrecorded_renames();
    test_replace_failure_is_retryable();
    test_conflicting_create_paths_are_preserved();
'''
if text.count(old_main_calls) != 1:
    raise SystemExit("test main calls changed")
tests.write_text(text.replace(old_main_calls, new_main_calls, 1))
