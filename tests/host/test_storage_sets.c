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

static macro_set_t make_set(uint32_t value, const char *name) {
    TEST_CHECK(name != NULL);
    macro_set_t set = {0};
    set.schema_version = APP_SCHEMA_VERSION;
    set.id = make_uuid(value);
    set.revision = 1U;
    TEST_CHECK(snprintf(set.name, sizeof(set.name), "%s", name) > 0);
    return set;
}

static void reset_store(void) {
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    /* SPEC 13.3: /data holds the set index and sets/, and nothing else. */
    static const char *const paths[] = {
        STORAGE_DATA_MOUNT,
        STORAGE_DATA_MOUNT "/sets",
    };
    for (size_t index = 0U; index < (sizeof(paths) / sizeof(paths[0])); ++index) {
        make_directory(paths[index]);
    }
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_init());
}

/* A set is exactly one file (SPEC 13.3): no directory, no set.json, no
 * macro-order.json, no macros/ child. */
static void assert_set_layout(const macro_set_t *set) {
    TEST_CHECK(set != NULL);
    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_make_set_path(&set->id, path, sizeof(path)));
    TEST_CHECK(path_exists(path));

    char directory[APP_PATH_MAX_BYTES];
    const int written =
        snprintf(directory, sizeof(directory), STORAGE_DATA_MOUNT "/sets/%s", set->id.value);
    TEST_CHECK(written > 0 && (size_t)written < sizeof(directory));
    TEST_CHECK(!path_exists(directory));
}

static void rewrite_set_file(const app_uuid_t *path_id, const macro_set_t *contents) {
    TEST_CHECK(path_id != NULL);
    TEST_CHECK(contents != NULL);
    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_make_set_path(path_id, path, sizeof(path)));
    char *json = NULL;
    size_t json_length = 0U;
    const storage_set_document_t document = {.set = *contents};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_set_document_serialize(&document, &json, &json_length));
    TEST_CHECK(json != NULL);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_atomic_write(path, json, json_length, true));
    cJSON_free(json);
}

static void test_argument_validation(void) {
    reset_store();
    storage_set_list_t list = {0};
    macro_set_t output = {0};
    macro_set_t set = make_set(1U, "Arguments");

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

/* SPEC 24.2 item: create/read/update/delete */
static void test_crud_ordering_revisions_and_cleanup(void) {
    reset_store();
    macro_set_t first = make_set(10U, "First");
    macro_set_t second = make_set(20U, "Second");
    macro_set_t third = make_set(30U, "Third");

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&first));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&second));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&third));
    assert_set_layout(&first);
    assert_set_layout(&second);
    assert_set_layout(&third);

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
    /* Permanent: the set's bytes are gone, not moved aside (SPEC 8.6). */
    TEST_CHECK(!path_exists(deleted_path));
    /* removed-feature-ok: asserts the directory is gone */
    TEST_CHECK(!path_exists(STORAGE_DATA_MOUNT "/trash"));

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
    macro_set_t set = make_set(40U, "Maximum revision");
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
        macro_set_t set = make_set(1000U + index, name);
        TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));
    }

    macro_set_t extra = make_set(9000U, "One too many");
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_FULL, storage_set_create(&extra));

    storage_set_list_t list = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_list(&list));
    TEST_CHECK_EQ_U64(APP_MACRO_SETS_MAX, list.count);
    for (uint32_t index = 0U; index < APP_MACRO_SETS_MAX; ++index) {
        const app_uuid_t expected = make_uuid(1000U + index);
        TEST_CHECK_EQ_UUID(&expected, &list.items[index].id);
    }
}

/* SPEC 24.2 item: corrupt JSON, including that the corrupt file is deleted */
static void test_corrupt_set_is_discarded(void) {
    reset_store();
    macro_set_t set = make_set(50U, "Corrupt JSON");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));

    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_make_set_path(&set.id, path, sizeof(path)));
    static const char invalid[] = "{not json";
    write_file(path, invalid, sizeof(invalid) - 1U);

    macro_set_t output = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, storage_set_read(&set.id, &output));
    TEST_CHECK(!path_exists(path));
    TEST_CHECK_EQ_U64(0U, output.revision);
}

static void test_mismatched_object_id_is_discarded(void) {
    reset_store();
    macro_set_t expected = make_set(60U, "Expected");
    macro_set_t wrong = make_set(61U, "Wrong ID");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&expected));
    rewrite_set_file(&expected.id, &wrong);

    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_make_set_path(&expected.id, path, sizeof(path)));
    macro_set_t output;
    memset(&output, 0xa5, sizeof(output));
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, storage_set_read(&expected.id, &output));
    TEST_CHECK_EQ_U64(0U, output.revision);
    TEST_CHECK(output.id.value[0] == '\0');
    TEST_CHECK(!path_exists(path));
}

static void test_duplicate_index_is_discarded_and_output_cleared(void) {
    reset_store();
    macro_set_t set = make_set(70U, "Duplicate index");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));

    char index_json[256U];
    const int written =
        snprintf(index_json, sizeof(index_json), "{\"schema_version\":1,\"ids\":[\"%s\",\"%s\"]}",
                 set.id.value, set.id.value);
    TEST_CHECK(written > 0 && (size_t)written < sizeof(index_json));
    write_file(STORAGE_INDEX_FILE_PATH, index_json, (size_t)written);

    storage_set_list_t list;
    memset(&list, 0xa5, sizeof(list));
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, storage_set_list(&list));
    TEST_CHECK_EQ_U64(0U, list.count);
    TEST_CHECK(!path_exists(STORAGE_INDEX_FILE_PATH));
}

/* Creation writes the set at its final path and only then adds it to the index,
 * so there is never a staging directory and never an index entry pointing at a
 * set that is not fully on disk (SPEC 13.3). */
static void test_create_leaves_no_staging_artifacts(void) {
    reset_store();
    macro_set_t set = make_set(80U, "Direct Create");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));

    char destination[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_make_set_path(&set.id, destination, sizeof(destination)));
    TEST_CHECK(path_exists(destination));
    /* removed-feature-ok: asserts the directory is gone */
    TEST_CHECK(!path_exists(STORAGE_DATA_MOUNT "/staging"));
    /* removed-feature-ok: asserts the directory is gone */
    TEST_CHECK(!path_exists(STORAGE_DATA_MOUNT "/transactions"));

    storage_set_list_t list = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_list(&list));
    TEST_CHECK_EQ_U64(1U, list.count);
}

/* Deletion is permanent (SPEC 8.6): the set's bytes are gone, not moved to a
 * trash directory that a 512 KiB partition cannot afford. */
static void test_delete_is_permanent_and_leaves_no_trash(void) {
    reset_store();
    macro_set_t set = make_set(90U, "Permanent Delete");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));

    char source[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_make_set_path(&set.id, source, sizeof(source)));
    TEST_CHECK(path_exists(source));

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_delete(&set.id, 1U));

    TEST_CHECK(!path_exists(source));
    /* removed-feature-ok: asserts the directory is gone */
    TEST_CHECK(!path_exists(STORAGE_DATA_MOUNT "/trash"));
    storage_set_list_t list = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_list(&list));
    TEST_CHECK_EQ_U64(0U, list.count);
}

/* SPEC 12.3: firmware MUST NOT reconstruct the index from a directory listing,
 * because the listing has no set order in it -- rebuilding would silently
 * replace the user's order with whatever the filesystem returns. A repository
 * with set files but no index is corrupt and must say so. */
/* SPEC 24.2 item: an index naming a set file that is absent */
static void test_index_is_not_rebuilt_from_the_sets_directory(void) {
    reset_store();
    macro_set_t set = make_set(120U, "Orphaned by index loss");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));
    TEST_CHECK(unlink(STORAGE_INDEX_FILE_PATH) == 0);

    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, storage_repository_init());
    TEST_CHECK(!path_exists(STORAGE_INDEX_FILE_PATH));
}

/* A genuinely empty device is not corrupt: with no index and no set files, init
 * writes the initial empty index. */
static void test_fresh_device_initializes_an_empty_index(void) {
    reset_store();
    TEST_CHECK(unlink(STORAGE_INDEX_FILE_PATH) == 0);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_init());
    TEST_CHECK(path_exists(STORAGE_INDEX_FILE_PATH));
    storage_set_list_t list = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_list(&list));
    TEST_CHECK_EQ_U64(0U, list.count);
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

static app_error_code_t interloper_delete(void) {
    return storage_set_delete(&g_interloper_id, 1U);
}

/* Two updates with the same expected revision cannot both succeed: the lock makes
 * each read-check-write atomic, so the second sees the bumped revision. */
/* SPEC 24.2 item: stale revisions */
static void test_concurrency_same_revision_updates_conflict(void) {
    reset_store();
    macro_set_t set = make_set(1U, "Serialize");
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
/* A create and a delete cannot race the index: while create holds the lock, an
 * interloping delete is blocked. */
static void test_concurrency_create_excludes_delete(void) {
    reset_store();
    macro_set_t existing = make_set(1U, "Existing");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&existing));
    g_interloper_id = existing.id;

    macro_set_t created = make_set(2U, "Created");
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
    macro_set_t set = make_set(1U, "LockFail");
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

/* Measured use counts the bytes actually on disk. */
static void test_measured_user_data_tracks_set_files(void) {
    reset_store();
    size_t empty_bytes = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_measure_user_data(NULL, &empty_bytes));
    TEST_CHECK_EQ_U64(0U, empty_bytes);

    macro_set_t set = make_set(210U, "Measured");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));
    size_t with_set = 0U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_measure_user_data(NULL, &with_set));
    TEST_CHECK(with_set > 0U);

    /* Excluding the only set brings the total back to zero, which is what makes
       a rewrite measure its replacement rather than double-counting. */
    size_t excluded = 99U;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_measure_user_data(&set.id, &excluded));
    TEST_CHECK_EQ_U64(0U, excluded);
}

/* SPEC 7.1: "Set order is user-controlled and meaningful... Firmware MUST
 * preserve it exactly and MUST NOT reorder, sort, or renumber on its own."
 *
 * storage_set_reorder had no test at all before this. These are written from
 * the requirement, not from the implementation. */
/* SPEC 24.2 item: macro order preserved exactly across write, reboot */
static void test_reorder_preserves_the_requested_order_exactly(void) {
    reset_store();
    macro_set_t a = make_set(300U, "Alpha");
    macro_set_t b = make_set(301U, "Bravo");
    macro_set_t c = make_set(302U, "Charlie");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&a));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&b));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&c));

    /* Deliberately NOT sorted by id, name, or creation order: the point is that
     * firmware stores what it was given. */
    const app_uuid_t requested[] = {c.id, a.id, b.id};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_reorder(requested, 3U));

    storage_set_list_t list = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_list(&list));
    TEST_CHECK_EQ_U64(3U, list.count);
    for (size_t index = 0U; index < 3U; ++index) {
        TEST_CHECK_EQ_UUID(&requested[index], &list.items[index].id);
    }
}

/* A reorder must be a permutation of exactly the sets that exist. Dropping one,
 * adding an unknown one, or repeating one is a conflict -- not a partial reorder
 * applied as far as it goes, which would renumber on the firmware's own
 * initiative. */
static void test_reorder_rejects_anything_but_a_permutation(void) {
    reset_store();
    macro_set_t a = make_set(310U, "Alpha");
    macro_set_t b = make_set(311U, "Bravo");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&a));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&b));
    const app_uuid_t stranger = make_uuid(999U);

    const app_uuid_t missing_one[] = {a.id};
    const app_uuid_t repeated[] = {a.id, a.id};
    const app_uuid_t unknown[] = {a.id, stranger};
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, storage_set_reorder(missing_one, 1U));
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, storage_set_reorder(repeated, 2U));
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, storage_set_reorder(unknown, 2U));

    /* Every rejection left the original order untouched. */
    storage_set_list_t list = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_list(&list));
    TEST_CHECK_EQ_U64(2U, list.count);
    TEST_CHECK_EQ_UUID(&a.id, &list.items[0].id);
    TEST_CHECK_EQ_UUID(&b.id, &list.items[1].id);
}

/* SPEC 10.1: "The user MUST explicitly select the active set. Firmware MUST NOT
 * infer or automatically switch the active set." Reordering is not selecting,
 * so it must leave the selection exactly where it was. */
static void test_reorder_does_not_disturb_the_active_set(void) {
    reset_store();
    macro_set_t a = make_set(320U, "Alpha");
    macro_set_t b = make_set(321U, "Bravo");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&a));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&b));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_select(&b.id));

    const app_uuid_t swapped[] = {b.id, a.id};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_reorder(swapped, 2U));

    bool present = false;
    app_uuid_t active = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_active_set_read(&present, &active));
    TEST_CHECK(present);
    TEST_CHECK_EQ_UUID(&b.id, &active);
}

int main(void) {
    /* The public set and recovery functions serialize behind the repository
     * mutation lock (FIX1 §9); the default host backend must be initialized before
     * any of them is exercised. */
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_init());
    test_argument_validation();
    test_reorder_preserves_the_requested_order_exactly();
    test_reorder_rejects_anything_but_a_permutation();
    test_reorder_does_not_disturb_the_active_set();
    test_measured_user_data_tracks_set_files();
    test_repository_deinit_is_a_safe_noop();
    test_crud_ordering_revisions_and_cleanup();
    test_revision_overflow_is_rejected();
    test_set_limit_and_stable_order();
    test_corrupt_set_is_discarded();
    test_mismatched_object_id_is_discarded();
    test_duplicate_index_is_discarded_and_output_cleared();
    test_create_leaves_no_staging_artifacts();
    test_delete_is_permanent_and_leaves_no_trash();
    test_index_is_not_rebuilt_from_the_sets_directory();
    test_fresh_device_initializes_an_empty_index();
    test_concurrency_same_revision_updates_conflict();
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
