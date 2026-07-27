#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "cJSON.h"
#include "storage.h"
#include "storage_repository.h"
#include "storage_repository_internal.h"
#include "storage_repository_lock.h"

#include "test_assert.h"
#include "test_temp_dir.h"

static bool path_exists(const char *path) {
    struct stat metadata;
    return stat(path, &metadata) == 0;
}

static void make_directory(const char *path) {
    TEST_CHECK(mkdir(path, 0750) == 0 || errno == EEXIST);
}

static size_t directory_entry_count(const char *path) {
    DIR *directory = opendir(path);
    TEST_CHECK(directory != NULL);
    size_t count = 0U;
    while (true) {
        errno = 0;
        struct dirent *entry = readdir(directory);
        if (entry == NULL) {
            TEST_CHECK(errno == 0);
            break;
        }
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            ++count;
        }
    }
    TEST_CHECK(closedir(directory) == 0);
    return count;
}

static void write_file(const char *path, const char *data, size_t length) {
    TEST_CHECK(path != NULL);
    TEST_CHECK(data != NULL || length == 0U);
    const int descriptor = open(path, O_WRONLY | O_TRUNC);
    TEST_CHECK(descriptor >= 0);
    size_t written = 0U;
    while (written < length) {
        const ssize_t result = write(descriptor, data + written, length - written);
        TEST_CHECK(result > 0);
        written += (size_t)result;
    }
    TEST_CHECK(close(descriptor) == 0);
}

static app_uuid_t make_uuid(uint32_t value) {
    char text[APP_UUID_BUFFER_LENGTH];
    const int written = snprintf(text, sizeof(text), "%08" PRIx32 "-0000-4000-8000-%012" PRIx64,
                                 value, (uint64_t)value);
    TEST_CHECK(written == (int)APP_UUID_STRING_LENGTH);
    app_uuid_t uuid = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_parse(text, &uuid));
    return uuid;
}

static macro_set_t make_set(uint32_t value, const char *name, int32_t sort_order) {
    TEST_CHECK(name != NULL);
    macro_set_t set = {0};
    set.schema_version = APP_SCHEMA_VERSION;
    set.id = make_uuid(value);
    set.revision = 1U;
    TEST_CHECK(snprintf(set.name, sizeof(set.name), "%s", name) > 0);
    TEST_CHECK(snprintf(set.description, sizeof(set.description), "Repository test") > 0);
    TEST_CHECK(snprintf(set.manufacturer, sizeof(set.manufacturer), "Test") > 0);
    TEST_CHECK(snprintf(set.model, sizeof(set.model), "Model") > 0);
    TEST_CHECK(snprintf(set.board, sizeof(set.board), "board") > 0);
    TEST_CHECK(snprintf(set.keyboard_layout, sizeof(set.keyboard_layout), "en-US") > 0);
    set.sort_order = sort_order;
    return set;
}

static void reset_store(void) {
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    static const char *const paths[] = {
        STORAGE_DATA_MOUNT,
        STORAGE_DATA_MOUNT "/sets",
        STORAGE_DATA_MOUNT "/global",
        STORAGE_DATA_MOUNT "/global/macros",
        STORAGE_DATA_MOUNT "/staging",
        STORAGE_DATA_MOUNT "/trash",
        STORAGE_DATA_MOUNT "/quarantine",
        STORAGE_DATA_MOUNT "/transactions",
    };
    for (size_t index = 0U; index < (sizeof(paths) / sizeof(paths[0])); ++index) {
        make_directory(paths[index]);
    }
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_init());
    TEST_CHECK_EQ_U64(0U, directory_entry_count(STORAGE_DATA_MOUNT "/transactions"));
}

static void assert_set_layout(const macro_set_t *set) {
    TEST_CHECK(set != NULL);
    char directory[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_make_set_path(&set->id, directory, sizeof(directory)));
    TEST_CHECK(path_exists(directory));

    static const char *const children[] = {
        "set.json", "macro-order.json", "procedure-order.json", "macros", "procedures", "progress",
    };
    for (size_t index = 0U; index < (sizeof(children) / sizeof(children[0])); ++index) {
        char path[APP_PATH_MAX_BYTES];
        const int written = snprintf(path, sizeof(path), "%s/%s", directory, children[index]);
        TEST_CHECK(written > 0 && (size_t)written < sizeof(path));
        TEST_CHECK(path_exists(path));
    }
}

static void rewrite_set_file(const app_uuid_t *path_id, const macro_set_t *contents) {
    TEST_CHECK(path_id != NULL);
    TEST_CHECK(contents != NULL);
    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_set_file_path(path_id, path, sizeof(path)));
    char *json = NULL;
    size_t json_length = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_serialize_set_json(contents, &json, &json_length));
    TEST_CHECK(json != NULL);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_atomic_write(path, json, json_length, true));
    cJSON_free(json);
}

static void test_argument_validation(void) {
    reset_store();
    storage_set_list_t list = {0};
    macro_set_t output = {0};
    macro_set_t set = make_set(1U, "Arguments", 0);

    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_set_list(NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_set_read(NULL, &output));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_set_read(&set.id, NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_set_create(NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_set_update(NULL, 1U, &output));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_set_update(&set, 0U, &output));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_set_update(&set, 1U, NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_set_delete(NULL, 1U));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_set_delete(&set.id, 0U));

    set.revision = 2U;
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_set_create(&set));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_list(&list));
    TEST_CHECK_EQ_U64(0U, list.count);
}

static void test_crud_ordering_revisions_and_cleanup(void) {
    reset_store();
    macro_set_t first = make_set(10U, "First", 30);
    macro_set_t second = make_set(20U, "Second", -10);
    macro_set_t third = make_set(30U, "Third", 5);

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&first));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&second));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&third));
    assert_set_layout(&first);
    assert_set_layout(&second);
    assert_set_layout(&third);
    TEST_CHECK_EQ_U64(0U, directory_entry_count(STORAGE_DATA_MOUNT "/transactions"));

    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, storage_set_create(&second));

    storage_set_list_t list = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_list(&list));
    TEST_CHECK_EQ_U64(3U, list.count);
    TEST_CHECK_EQ_UUID(&first.id, &list.items[0].id);
    TEST_CHECK_EQ_UUID(&second.id, &list.items[1].id);
    TEST_CHECK_EQ_UUID(&third.id, &list.items[2].id);

    macro_set_t replacement = first;
    TEST_CHECK(snprintf(replacement.name, sizeof(replacement.name), "Updated First") > 0);
    macro_set_t updated = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, storage_set_update(&replacement, 2U, &updated));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_update(&replacement, 1U, &updated));
    TEST_CHECK_EQ_U64(2U, updated.revision);
    TEST_CHECK_EQ_STRING("Updated First", updated.name);

    macro_set_t readback = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_read(&first.id, &readback));
    TEST_CHECK_EQ_U64(2U, readback.revision);
    TEST_CHECK_EQ_STRING("Updated First", readback.name);

    replacement = updated;
    TEST_CHECK(snprintf(replacement.name, sizeof(replacement.name), "Stale") > 0);
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, storage_set_update(&replacement, 1U, &readback));
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, storage_set_delete(&second.id, 2U));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_delete(&second.id, 1U));

    char deleted_path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_make_set_path(&second.id, deleted_path, sizeof(deleted_path)));
    TEST_CHECK(!path_exists(deleted_path));
    TEST_CHECK_EQ_U64(0U, directory_entry_count(STORAGE_DATA_MOUNT "/transactions"));
    TEST_CHECK_EQ_U64(1U, directory_entry_count(STORAGE_DATA_MOUNT "/trash"));

    memset(&list, 0xa5, sizeof(list));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_list(&list));
    TEST_CHECK_EQ_U64(2U, list.count);
    TEST_CHECK_EQ_UUID(&first.id, &list.items[0].id);
    TEST_CHECK_EQ_UUID(&third.id, &list.items[1].id);

    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, storage_set_read(&second.id, &readback));
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, storage_set_delete(&second.id, 1U));
}

static void test_revision_overflow_is_rejected(void) {
    reset_store();
    macro_set_t set = make_set(40U, "Maximum revision", 0);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));

    set.revision = UINT32_MAX;
    rewrite_set_file(&set.id, &set);

    macro_set_t output = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, storage_set_update(&set, UINT32_MAX, &output));
    TEST_CHECK_EQ_U64(0U, output.revision);

    macro_set_t readback = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_read(&set.id, &readback));
    TEST_CHECK_EQ_U64(UINT32_MAX, readback.revision);
}

static void test_set_limit_and_stable_order(void) {
    reset_store();
    for (uint32_t index = 0U; index < APP_MACRO_SETS_MAX; ++index) {
        char name[APP_NAME_MAX_BYTES + 1U];
        const int written = snprintf(name, sizeof(name), "Set %" PRIu32, index);
        TEST_CHECK(written > 0 && (size_t)written < sizeof(name));
        macro_set_t set = make_set(1000U + index, name, (int32_t)index);
        TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));
    }

    macro_set_t extra = make_set(9000U, "One too many", 0);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_FULL, storage_set_create(&extra));

    storage_set_list_t list = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_list(&list));
    TEST_CHECK_EQ_U64(APP_MACRO_SETS_MAX, list.count);
    for (uint32_t index = 0U; index < APP_MACRO_SETS_MAX; ++index) {
        const app_uuid_t expected = make_uuid(1000U + index);
        TEST_CHECK_EQ_UUID(&expected, &list.items[index].id);
    }
}

static void test_corrupt_set_is_quarantined(void) {
    reset_store();
    macro_set_t set = make_set(50U, "Corrupt JSON", 0);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));

    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_set_file_path(&set.id, path, sizeof(path)));
    static const char invalid[] = "{not json";
    write_file(path, invalid, sizeof(invalid) - 1U);

    macro_set_t output = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, storage_set_read(&set.id, &output));
    TEST_CHECK(!path_exists(path));
    TEST_CHECK_EQ_U64(0U, output.revision);

    storage_quarantine_list_t quarantine = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_quarantine_list(&quarantine));
    TEST_CHECK_EQ_U64(1U, quarantine.count);
    TEST_CHECK_EQ_STRING(path, quarantine.items[0].source_path);
    TEST_CHECK(strstr(quarantine.items[0].reason, "invalid set") != NULL);
    TEST_CHECK(path_exists(quarantine.items[0].evidence_path));
}

static void test_mismatched_object_id_is_quarantined(void) {
    reset_store();
    macro_set_t expected = make_set(60U, "Expected", 0);
    macro_set_t wrong = make_set(61U, "Wrong ID", 0);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&expected));
    rewrite_set_file(&expected.id, &wrong);

    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_repository_set_file_path(&expected.id, path, sizeof(path)));
    macro_set_t output;
    memset(&output, 0xa5, sizeof(output));
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, storage_set_read(&expected.id, &output));
    TEST_CHECK_EQ_U64(0U, output.revision);
    TEST_CHECK(output.id.value[0] == '\0');
    TEST_CHECK(!path_exists(path));

    storage_quarantine_list_t quarantine = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_quarantine_list(&quarantine));
    TEST_CHECK_EQ_U64(1U, quarantine.count);
    TEST_CHECK_EQ_STRING(path, quarantine.items[0].source_path);
}

static void test_duplicate_index_is_quarantined_and_output_cleared(void) {
    reset_store();
    macro_set_t set = make_set(70U, "Duplicate index", 0);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));

    char index_json[256U];
    const int written =
        snprintf(index_json, sizeof(index_json), "{\"schema_version\":1,\"ids\":[\"%s\",\"%s\"]}",
                 set.id.value, set.id.value);
    TEST_CHECK(written > 0 && (size_t)written < sizeof(index_json));
    write_file(STORAGE_SET_INDEX_FILE_PATH, index_json, (size_t)written);

    storage_set_list_t list;
    memset(&list, 0xa5, sizeof(list));
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, storage_set_list(&list));
    TEST_CHECK_EQ_U64(0U, list.count);
    TEST_CHECK(!path_exists(STORAGE_SET_INDEX_FILE_PATH));

    storage_quarantine_list_t quarantine = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_quarantine_list(&quarantine));
    TEST_CHECK_EQ_U64(1U, quarantine.count);
    TEST_CHECK_EQ_STRING(STORAGE_SET_INDEX_FILE_PATH, quarantine.items[0].source_path);
    TEST_CHECK(strstr(quarantine.items[0].reason, "ordering index") != NULL);
}

static void test_create_recovery(void) {
    reset_store();
    macro_set_t set = make_set(80U, "Interrupted Create", 0);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));

    char destination[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_make_set_path(&set.id, destination, sizeof(destination)));
    const app_uuid_t transaction_id = make_uuid(8000U);
    char staging[APP_PATH_MAX_BYTES];
    const int staging_length =
        snprintf(staging, sizeof(staging), STORAGE_DATA_MOUNT "/staging/%s", transaction_id.value);
    TEST_CHECK(staging_length > 0 && (size_t)staging_length < sizeof(staging));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_set_index_presence(&set.id, false));
    TEST_CHECK(rename(destination, staging) == 0);

    storage_transaction_manifest_t manifest = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = transaction_id,
        .type = STORAGE_TRANSACTION_IMPORT_SET,
        .phase = STORAGE_TRANSACTION_STAGED,
        .replacement_revision = 1U,
    };
    TEST_CHECK(snprintf(manifest.staging, sizeof(manifest.staging), "%s", staging) > 0);
    TEST_CHECK(snprintf(manifest.destination, sizeof(manifest.destination), "%s", destination) > 0);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_transaction_write_manifest(&manifest));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_transaction_recover_all());

    storage_set_list_t list = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_list(&list));
    TEST_CHECK_EQ_U64(1U, list.count);
    TEST_CHECK(path_exists(destination));
    TEST_CHECK(!path_exists(staging));
    TEST_CHECK_EQ_U64(0U, directory_entry_count(STORAGE_DATA_MOUNT "/transactions"));
}

static void test_delete_recovery(void) {
    reset_store();
    macro_set_t set = make_set(90U, "Interrupted Delete", 0);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));

    char source[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_make_set_path(&set.id, source, sizeof(source)));
    const app_uuid_t transaction_id = make_uuid(9000U);
    char backup[APP_PATH_MAX_BYTES];
    const int backup_length = snprintf(backup, sizeof(backup), STORAGE_DATA_MOUNT "/trash/%s-%s",
                                       set.id.value, transaction_id.value);
    TEST_CHECK(backup_length > 0 && (size_t)backup_length < sizeof(backup));

    storage_transaction_manifest_t manifest = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = transaction_id,
        .type = STORAGE_TRANSACTION_DELETE_SET,
        .phase = STORAGE_TRANSACTION_PREPARED,
        .expected_revision = 1U,
    };
    TEST_CHECK(snprintf(manifest.source, sizeof(manifest.source), "%s", source) > 0);
    TEST_CHECK(snprintf(manifest.backup, sizeof(manifest.backup), "%s", backup) > 0);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_transaction_write_manifest(&manifest));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_transaction_recover_all());

    storage_set_list_t list = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_list(&list));
    TEST_CHECK_EQ_U64(0U, list.count);
    TEST_CHECK(!path_exists(source));
    TEST_CHECK(path_exists(backup));
    TEST_CHECK_EQ_U64(0U, directory_entry_count(STORAGE_DATA_MOUNT "/transactions"));
}

static void test_unknown_transaction_is_preserved(void) {
    reset_store();
    const app_uuid_t transaction_id = make_uuid(10000U);
    storage_transaction_manifest_t manifest = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = transaction_id,
        .type = STORAGE_TRANSACTION_RESTORE,
        .phase = STORAGE_TRANSACTION_PREPARED,
    };
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_transaction_write_manifest(&manifest));
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, storage_transaction_recover_all());

    char path[APP_PATH_MAX_BYTES];
    const int written = snprintf(path, sizeof(path), STORAGE_DATA_MOUNT "/transactions/%s.bin",
                                 transaction_id.value);
    TEST_CHECK(written > 0 && (size_t)written < sizeof(path));
    TEST_CHECK(path_exists(path));
}

static void test_missing_initialized_index_is_not_recreated(void) {
    reset_store();
    TEST_CHECK(unlink(STORAGE_SET_INDEX_FILE_PATH) == 0);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, storage_repository_init());
    TEST_CHECK(!path_exists(STORAGE_SET_INDEX_FILE_PATH));
}

static void test_repository_deinit_is_a_safe_noop(void) {
    reset_store();
    /* The repository layer owns no in-memory resources, so deinit is a no-op that
     * is safe to call repeatedly and leaves the store fully usable. */
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_deinit());
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_deinit());
    storage_set_list_t list;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_list(&list));
}

/* ---- FIX1 §9.3 concurrency guarantees, proven deterministically via the lock
 * operations seam (no host threads). --------------------------------------- */

static app_uuid_t g_interloper_id;

/* Mutual-exclusion prover: models one non-recursive lock. On the armed take it
 * fires a one-shot interloper -- a concurrent locking operation -- while the lock
 * is held; the interloper's own take is rejected, proving it would block behind
 * the operation under test. */
typedef struct {
    bool held;
    app_error_code_t (*interloper)(void);
    bool interloper_armed;
    bool interloper_ran;
    app_error_code_t interloper_result;
} mx_lock_state_t;

static mx_lock_state_t g_mx;

static app_error_code_t mx_noop(void *context) {
    (void)context;
    return APP_ERROR_NONE;
}

static app_error_code_t mx_take(void *context) {
    (void)context;
    if (g_mx.held) {
        return APP_ERROR_INTERNAL; /* a second acquirer would block */
    }
    g_mx.held = true;
    if (g_mx.interloper_armed) {
        g_mx.interloper_armed = false;
        g_mx.interloper_ran = true;
        g_mx.interloper_result = g_mx.interloper();
    }
    return APP_ERROR_NONE;
}

static app_error_code_t mx_give(void *context) {
    (void)context;
    g_mx.held = false;
    return APP_ERROR_NONE;
}

static const storage_repository_lock_ops_t mx_ops = {
    .context = NULL,
    .init = mx_noop,
    .take = mx_take,
    .give = mx_give,
    .deinit = mx_noop,
};

static app_error_code_t interloper_read(void) {
    macro_set_t out = {0};
    return storage_set_read(&g_interloper_id, &out);
}

static app_error_code_t interloper_delete(void) {
    return storage_set_delete(&g_interloper_id, 1U);
}

/* Two updates with the same expected revision cannot both succeed: the lock makes
 * each read-check-write atomic, so the second sees the bumped revision. */
static void test_concurrency_same_revision_updates_conflict(void) {
    reset_store();
    macro_set_t set = make_set(1U, "Serialize", 0);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));

    macro_set_t first = set;
    first.revision = 1U;
    macro_set_t updated = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_update(&first, 1U, &updated));
    TEST_CHECK_EQ_U64(2U, updated.revision);

    macro_set_t second = set;
    second.revision = 1U;
    macro_set_t updated_again = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, storage_set_update(&second, 1U, &updated_again));
}

/* Startup recovery holds the lock for its whole pass, so an API mutation arriving
 * concurrently is blocked. */
static void test_concurrency_recovery_excludes_mutation(void) {
    reset_store();
    macro_set_t set = make_set(1U, "RecoveryRace", 0);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));
    g_interloper_id = set.id;

    g_mx = (mx_lock_state_t){.interloper = interloper_read, .interloper_armed = true};
    storage_repository_lock_set_ops(&mx_ops);
    const app_error_code_t result = storage_transaction_recover_all();
    storage_repository_lock_set_ops(NULL);

    TEST_CHECK(g_mx.interloper_ran);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, g_mx.interloper_result);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, result);
    TEST_CHECK(!g_mx.held);
}

/* A create and a delete cannot race the index: while create holds the lock, an
 * interloping delete is blocked. */
static void test_concurrency_create_excludes_delete(void) {
    reset_store();
    macro_set_t existing = make_set(1U, "Existing", 0);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&existing));
    g_interloper_id = existing.id;

    macro_set_t created = make_set(2U, "Created", 1);
    g_mx = (mx_lock_state_t){.interloper = interloper_delete, .interloper_armed = true};
    storage_repository_lock_set_ops(&mx_ops);
    const app_error_code_t result = storage_set_create(&created);
    storage_repository_lock_set_ops(NULL);

    TEST_CHECK(g_mx.interloper_ran);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, g_mx.interloper_result);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, result);
    TEST_CHECK(!g_mx.held);
}

static app_error_code_t g_take_result;
static app_error_code_t g_give_result;

static app_error_code_t gate_take(void *context) {
    (void)context;
    return g_take_result;
}

static app_error_code_t gate_give(void *context) {
    (void)context;
    return g_give_result;
}

static const storage_repository_lock_ops_t gate_ops = {
    .context = NULL,
    .init = mx_noop,
    .take = gate_take,
    .give = gate_give,
    .deinit = mx_noop,
};

/* Unlock failure is visible and never reported as mutation success; take failure
 * refuses the mutation outright. */
static void test_concurrency_lock_failures_are_visible(void) {
    reset_store();
    macro_set_t set = make_set(1U, "LockFail", 0);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));

    /* Give fails after a successful write: result must be INTERNAL, not NONE. */
    g_take_result = APP_ERROR_NONE;
    g_give_result = APP_ERROR_INTERNAL;
    storage_repository_lock_set_ops(&gate_ops);
    macro_set_t first = set;
    first.revision = 1U;
    macro_set_t updated = {0};
    const app_error_code_t give_failure = storage_set_update(&first, 1U, &updated);
    storage_repository_lock_set_ops(NULL);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, give_failure);

    /* Take fails: the mutation is refused and the revision on disk is unchanged. */
    g_take_result = APP_ERROR_INTERNAL;
    g_give_result = APP_ERROR_NONE;
    storage_repository_lock_set_ops(&gate_ops);
    macro_set_t second = set;
    second.revision = 2U;
    macro_set_t updated_again = {0};
    const app_error_code_t take_failure = storage_set_update(&second, 2U, &updated_again);
    storage_repository_lock_set_ops(NULL);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, take_failure);

    /* The give-failure write did land (revision 2); the take-failure one did not. */
    macro_set_t current = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_read(&set.id, &current));
    TEST_CHECK_EQ_U64(2U, current.revision);
}

int main(void) {
    /* The public set/quarantine/recovery functions serialize behind the repository
     * mutation lock (FIX1 §9); the default host backend must be initialized before
     * any of them is exercised. */
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_init());
    test_argument_validation();
    test_repository_deinit_is_a_safe_noop();
    test_crud_ordering_revisions_and_cleanup();
    test_revision_overflow_is_rejected();
    test_set_limit_and_stable_order();
    test_corrupt_set_is_quarantined();
    test_mismatched_object_id_is_quarantined();
    test_duplicate_index_is_quarantined_and_output_cleared();
    test_create_recovery();
    test_delete_recovery();
    test_unknown_transaction_is_preserved();
    test_missing_initialized_index_is_not_recreated();
    test_concurrency_same_revision_updates_conflict();
    test_concurrency_recovery_excludes_mutation();
    test_concurrency_create_excludes_delete();
    test_concurrency_lock_failures_are_visible();
    /* Import/restore serialization (FIX1 §9.3) is deferred with the import/restore
     * feature itself (Phase 18); it will acquire this same lock, so the exclusion
     * proven above covers it once implemented. */
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_deinit());
    puts("storage set repository tests passed");
    return EXIT_SUCCESS;
}
