#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "storage_atomic_recovery.h"
#include "test_assert.h"

/* A canonical lowercase RFC-4122 v4 UUID (version nibble 4, variant nibble 8). */
#define VALID_UUID "0a1b2c3d-0000-4000-8000-000000000001"

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

int main(void) {
    test_parse_temporary();
    test_parse_backup();
    test_non_artifacts_are_skipped();
    test_malformed_artifacts_are_rejected();
    test_list_add_dedup_and_skip();
    test_list_full_is_rejected();
    puts("storage atomic recovery tests passed");
    return EXIT_SUCCESS;
}
