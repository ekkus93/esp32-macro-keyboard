#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "app_uuid.h"
#include "storage.h"
#include "storage_atomic_recovery.h"
#include "storage_fs_ops.h"
#include "test_assert.h"
#include "test_temp_dir.h"

/* A canonical lowercase RFC-4122 v4 UUID (version nibble 4, variant nibble 8). */
#define VALID_UUID "0a1b2c3d-0000-4000-8000-000000000001"
#define OP_UUID "0a1b2c3d-0000-4000-8000-0000000000aa"
#define OP_UUID_2 "0a1b2c3d-0000-4000-8000-0000000000ab"
#define VALID_INDEX "{\"schema_version\":1,\"ids\":[]}"

static void test_parse_temporary(void) {
    storage_atomic_artifact_t artifact;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_atomic_recovery_parse(
                                             "/data/sets/set.json.tmp." VALID_UUID, &artifact));
    TEST_CHECK_EQ_STRING("/data/sets/set.json", artifact.destination);
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_ARTIFACT_TEMPORARY, artifact.kind);
    TEST_CHECK_EQ_STRING("/data/sets/set.json.tmp." VALID_UUID, artifact.artifact_path);
    TEST_CHECK_EQ_STRING(VALID_UUID, artifact.operation_id.value);
}

static void test_parse_backup(void) {
    storage_atomic_artifact_t artifact;
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_atomic_recovery_parse("/data/x.bak." VALID_UUID, &artifact));
    TEST_CHECK_EQ_STRING("/data/x", artifact.destination);
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_ARTIFACT_BACKUP, artifact.kind);
}

static void test_non_artifacts_are_skipped(void) {
    storage_atomic_artifact_t artifact;
    /* No suffix, wrong marker, an invalid UUID, and a too-short name are all "not
     * an artifact" rather than malformed -- an enumerator skips them. */
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND,
                         storage_atomic_recovery_parse("/data/set.json", &artifact));
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND,
                         storage_atomic_recovery_parse("/data/x.zip." VALID_UUID, &artifact));
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND,
                         storage_atomic_recovery_parse(
                             "/data/x.tmp.gggggggg-0000-4000-8000-000000000001", &artifact));
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, storage_atomic_recovery_parse("short", &artifact));
}

static void test_malformed_artifacts_are_rejected(void) {
    storage_atomic_artifact_t artifact;
    /* Empty destination. */
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_atomic_recovery_parse(".tmp." VALID_UUID, &artifact));
    /* Destination ends in a separator -- no filename to reconstruct. */
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_atomic_recovery_parse("/data/.tmp." VALID_UUID, &artifact));
    /* Path traversal / cross-directory escape. */
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        storage_atomic_recovery_parse("/data/../etc/passwd.tmp." VALID_UUID, &artifact));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_atomic_recovery_parse("../escape.tmp." VALID_UUID, &artifact));
    /* NULL arguments. */
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_atomic_recovery_parse(NULL, &artifact));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         storage_atomic_recovery_parse("x.tmp." VALID_UUID, NULL));
}

static void test_list_add_dedup_and_skip(void) {
    storage_atomic_artifact_list_t list = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_atomic_recovery_list_add(&list, "/data/a.tmp." VALID_UUID));
    TEST_CHECK_EQ_U64(1U, list.count);
    /* The same artifact path is a duplicate and is not appended. */
    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT,
                         storage_atomic_recovery_list_add(&list, "/data/a.tmp." VALID_UUID));
    TEST_CHECK_EQ_U64(1U, list.count);
    /* A distinct artifact is appended. */
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         storage_atomic_recovery_list_add(&list, "/data/b.bak." VALID_UUID));
    TEST_CHECK_EQ_U64(2U, list.count);
    /* A non-artifact propagates NOT_FOUND and is not appended. */
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND,
                         storage_atomic_recovery_list_add(&list, "/data/regular.json"));
    TEST_CHECK_EQ_U64(2U, list.count);
}

static void test_list_full_is_rejected(void) {
    storage_atomic_artifact_list_t list = {0};
    char path[APP_PATH_MAX_BYTES];
    for (size_t index = 0U; index < STORAGE_ATOMIC_RECOVERY_MAX_ARTIFACTS; ++index) {
        const int written = snprintf(path, sizeof(path), "/data/f%zu.tmp." VALID_UUID, index);
        TEST_CHECK(written > 0 && (size_t)written < sizeof(path));
        TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_atomic_recovery_list_add(&list, path));
    }
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_FULL,
                         storage_atomic_recovery_list_add(&list, "/data/overflow.tmp." VALID_UUID));
}

static storage_atomic_reconcile_action_t decide(bool canonical, size_t temporary_count,
                                                bool temporary_valid, size_t backup_count,
                                                bool backup_valid, bool roll_forward_proven) {
    const storage_atomic_reconcile_state_t state = {
        .canonical_present = canonical,
        .temporary_count = temporary_count,
        .temporary_valid = temporary_valid,
        .backup_count = backup_count,
        .backup_valid = backup_valid,
        .roll_forward_proven = roll_forward_proven,
    };
    return storage_atomic_reconcile_decide(&state);
}

static void test_reconcile_decision(void) {
    /* No artifacts. */
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_RECONCILE_NOTHING, decide(false, 0U, false, 0U, false, false));
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_RECONCILE_NOTHING, decide(true, 0U, false, 0U, false, false));
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_RECONCILE_NOTHING, storage_atomic_reconcile_decide(NULL));

    /* Conflicting: more than one temporary or backup, even with a canonical. */
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_RECONCILE_QUARANTINE,
                      decide(false, 2U, true, 0U, false, false));
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_RECONCILE_QUARANTINE,
                      decide(false, 0U, false, 2U, true, false));
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_RECONCILE_QUARANTINE, decide(true, 2U, true, 1U, true, false));

    /* Canonical present is authoritative; stragglers are cleaned regardless of
     * their validity. */
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_RECONCILE_KEEP_CANONICAL,
                      decide(true, 1U, true, 0U, false, false));
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_RECONCILE_KEEP_CANONICAL,
                      decide(true, 0U, false, 1U, true, false));
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_RECONCILE_KEEP_CANONICAL,
                      decide(true, 1U, false, 1U, false, false));

    /* Canonical absent with a valid backup: restore it. The backup (old committed
     * state) is preferred over any temporary, even a roll-forward-proven one. */
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_RECONCILE_RESTORE_BACKUP,
                      decide(false, 0U, false, 1U, true, false));
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_RECONCILE_RESTORE_BACKUP,
                      decide(false, 1U, true, 1U, true, true));

    /* Canonical absent with a corrupt backup: nothing safe to restore. */
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_RECONCILE_QUARANTINE,
                      decide(false, 0U, false, 1U, false, false));
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_RECONCILE_QUARANTINE,
                      decide(false, 1U, true, 1U, false, false));

    /* Canonical absent, only a temporary: an interrupted write is rolled back
     * (discarded) whether or not its bytes are valid, unless the owning
     * transaction proves roll-forward. */
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_RECONCILE_DISCARD_TEMPORARY,
                      decide(false, 1U, true, 0U, false, false));
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_RECONCILE_DISCARD_TEMPORARY,
                      decide(false, 1U, false, 0U, false, false));
    /* Roll-forward proven: activate a valid temporary, discard a corrupt one. */
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_RECONCILE_ACTIVATE_TEMPORARY,
                      decide(false, 1U, true, 0U, false, true));
    TEST_CHECK_EQ_INT(STORAGE_ATOMIC_RECONCILE_QUARANTINE,
                      decide(false, 1U, false, 0U, false, true));
}

/* ---- Executor tests (real POSIX filesystem under STORAGE_DATA_MOUNT). ---- */

static bool path_exists(const char *path) {
    struct stat metadata;
    return stat(path, &metadata) == 0;
}

static void make_directory(const char *path) {
    TEST_CHECK(mkdir(path, 0750) == 0 || errno == EEXIST);
}

static void write_text_file(const char *path, const char *text) {
    const int descriptor = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0640);
    TEST_CHECK(descriptor >= 0);
    const size_t length = strlen(text);
    size_t written = 0U;
    while (written < length) {
        const ssize_t count = write(descriptor, text + written, length - written);
        TEST_CHECK(count > 0);
        written += (size_t)count;
    }
    TEST_CHECK(close(descriptor) == 0);
}

static void recovery_reset_store(void) {
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    make_directory(STORAGE_DATA_MOUNT);
    make_directory(STORAGE_DATA_MOUNT "/sets");
    make_directory(STORAGE_DATA_MOUNT "/transactions");
    make_directory(STORAGE_DATA_MOUNT "/staging");
}

static app_error_code_t run_recovery(void) {
    return storage_atomic_recover_all_with_ops(storage_fs_ops_posix());
}

static void test_executor_keep_canonical(void) {
    recovery_reset_store();
    const char *canonical = STORAGE_DATA_MOUNT "/set-index.json";
    const char *temporary = STORAGE_DATA_MOUNT "/set-index.json.tmp." OP_UUID;
    write_text_file(canonical, VALID_INDEX);
    write_text_file(temporary, "partial write");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, run_recovery());
    /* Canonical is authoritative; the straggler temporary is removed. */
    TEST_CHECK(path_exists(canonical));
    TEST_CHECK(!path_exists(temporary));
}

static void test_executor_restore_backup(void) {
    recovery_reset_store();
    const char *canonical = STORAGE_DATA_MOUNT "/set-index.json";
    const char *backup = STORAGE_DATA_MOUNT "/set-index.json.bak." OP_UUID;
    const char *temporary = STORAGE_DATA_MOUNT "/set-index.json.tmp." OP_UUID_2;
    write_text_file(backup, VALID_INDEX);
    write_text_file(temporary, "partial write");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, run_recovery());
    /* Canonical absent + valid backup: restore it and discard the temporary. */
    TEST_CHECK(path_exists(canonical));
    TEST_CHECK(!path_exists(backup));
    TEST_CHECK(!path_exists(temporary));
}

static void test_executor_discard_temporary(void) {
    recovery_reset_store();
    const char *canonical = STORAGE_DATA_MOUNT "/set-index.json";
    const char *temporary = STORAGE_DATA_MOUNT "/set-index.json.tmp." OP_UUID;
    write_text_file(temporary, "partial write");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, run_recovery());
    /* Canonical absent, no backup: the interrupted write is rolled back. */
    TEST_CHECK(!path_exists(canonical));
    TEST_CHECK(!path_exists(temporary));
}

static void test_executor_discard_conflict(void) {
    recovery_reset_store();
    const char *first = STORAGE_DATA_MOUNT "/set-index.json.tmp." OP_UUID;
    const char *second = STORAGE_DATA_MOUNT "/set-index.json.tmp." OP_UUID_2;
    write_text_file(first, "one");
    write_text_file(second, "two");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, run_recovery());
    /* Two temporaries for one destination conflict: both are deleted outright
     * and nothing is archived (SPEC §13.6). */
    TEST_CHECK(!path_exists(first));
    TEST_CHECK(!path_exists(second));
}

static void test_executor_discard_corrupt_backup(void) {
    recovery_reset_store();
    const char *canonical = STORAGE_DATA_MOUNT "/set-index.json";
    const char *backup = STORAGE_DATA_MOUNT "/set-index.json.bak." OP_UUID;
    write_text_file(backup, "not a valid index");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, run_recovery());
    /* Canonical absent + corrupt backup: nothing safe to restore, so the
     * corrupt backup is deleted rather than resurrected (SPEC §13.6). */
    TEST_CHECK(!path_exists(canonical));
    TEST_CHECK(!path_exists(backup));
}

/* ---- §7.5 crash-consistency matrix: interrupt after each atomic-write step. ----
 *
 * An atomic write of NEW over OLD proceeds: create+write+sync+close+readback the
 * temporary, rename destination->backup, parent sync, rename temporary->destination,
 * parent sync, unlink backup, parent sync. For each step, the on-disk state a crash
 * would leave is constructed here and reconciled; the destination must always end as
 * OLD-complete or NEW-complete, never a partial/ambiguous active state, with no
 * leftover artifacts. */

#define OLD_INDEX VALID_INDEX
#define NEW_INDEX "{\"schema_version\":1,\"ids\":[\"" VALID_UUID "\"]}"

static void read_text_file(const char *path, char *buffer, size_t size) {
    const int descriptor = open(path, O_RDONLY);
    TEST_CHECK(descriptor >= 0);
    ssize_t count = read(descriptor, buffer, size - 1U);
    TEST_CHECK(count >= 0);
    buffer[count] = '\0';
    TEST_CHECK(close(descriptor) == 0);
}

typedef struct {
    const char *label;
    const char *canonical; /* NULL = absent */
    const char *temporary; /* NULL = absent, "" = present-empty */
    const char *backup;    /* NULL = absent */
    const char *expected;  /* expected canonical content, NULL = absent */
} crash_case_t;

static void test_crash_consistency_matrix(void) {
    static const crash_case_t cases[] = {
        {"after temporary open", OLD_INDEX, "", NULL, OLD_INDEX},
        {"after partial write", OLD_INDEX, "partial", NULL, OLD_INDEX},
        {"after file sync", OLD_INDEX, NEW_INDEX, NULL, OLD_INDEX},
        {"after close", OLD_INDEX, NEW_INDEX, NULL, OLD_INDEX},
        {"after readback", OLD_INDEX, NEW_INDEX, NULL, OLD_INDEX},
        {"after destination-to-backup rename", NULL, NEW_INDEX, OLD_INDEX, OLD_INDEX},
        {"after first parent sync", NULL, NEW_INDEX, OLD_INDEX, OLD_INDEX},
        {"after temporary-to-destination rename", NEW_INDEX, NULL, OLD_INDEX, NEW_INDEX},
        {"after second parent sync", NEW_INDEX, NULL, OLD_INDEX, NEW_INDEX},
        {"after backup removal", NEW_INDEX, NULL, NULL, NEW_INDEX},
        {"after final parent sync", NEW_INDEX, NULL, NULL, NEW_INDEX},
    };
    const char *canonical = STORAGE_DATA_MOUNT "/set-index.json";
    const char *temporary = STORAGE_DATA_MOUNT "/set-index.json.tmp." OP_UUID;
    const char *backup = STORAGE_DATA_MOUNT "/set-index.json.bak." OP_UUID_2;

    for (size_t index = 0U; index < (sizeof(cases) / sizeof(cases[0])); ++index) {
        const crash_case_t *scenario = &cases[index];
        recovery_reset_store();
        if (scenario->canonical != NULL) {
            write_text_file(canonical, scenario->canonical);
        }
        if (scenario->temporary != NULL) {
            write_text_file(temporary, scenario->temporary);
        }
        if (scenario->backup != NULL) {
            write_text_file(backup, scenario->backup);
        }

        TEST_CHECK_APP_ERROR(APP_ERROR_NONE, run_recovery());

        /* Old-or-new complete, never a partial active state. */
        if (scenario->expected != NULL) {
            char content[256];
            read_text_file(canonical, content, sizeof(content));
            TEST_CHECK_EQ_STRING(scenario->expected, content);
        } else {
            TEST_CHECK(!path_exists(canonical));
        }
        /* No leftover artifacts. */
        TEST_CHECK(!path_exists(temporary));
        TEST_CHECK(!path_exists(backup));
    }
}

int main(void) {
    test_reconcile_decision();
    test_crash_consistency_matrix();
    test_executor_keep_canonical();
    test_executor_restore_backup();
    test_executor_discard_temporary();
    test_executor_discard_conflict();
    test_executor_discard_corrupt_backup();
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    test_parse_temporary();
    test_parse_backup();
    test_non_artifacts_are_skipped();
    test_malformed_artifacts_are_rejected();
    test_list_add_dedup_and_skip();
    test_list_full_is_rejected();
    puts("storage atomic recovery tests passed");
    return EXIT_SUCCESS;
}
