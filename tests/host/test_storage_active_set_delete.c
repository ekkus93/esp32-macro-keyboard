#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "storage.h"
#include "storage_repository.h"
#include "storage_repository_lock.h"
#include "test_assert.h"
#include "test_temp_dir.h"

/*
 * The active set lives in the index (SPEC 12.3), beside the set order. That is
 * what makes "delete the active set" a single atomic write rather than a
 * storage write plus an NVS write that could disagree, which is what these
 * tests exist to pin.
 *
 * Their predecessor injected a fake settings backend to observe a separate
 * clear-active-set call. There is no such call any more, so the seam and its
 * stub are gone; the observable behaviour is checked through the index itself.
 */

static app_uuid_t make_uuid(uint32_t value) {
    char text[APP_UUID_BUFFER_LENGTH];
    const int written = snprintf(text, sizeof(text), "%08" PRIx32 "-0000-4000-8000-%012" PRIx64,
                                 value, (uint64_t)value);
    TEST_CHECK_EQ_INT((int)APP_UUID_STRING_LENGTH, written);
    app_uuid_t uuid = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_parse(text, &uuid));
    return uuid;
}

static void make_directory(const char *path) {
    TEST_CHECK(mkdir(path, 0750) == 0 || errno == EEXIST);
}

static bool path_exists(const char *path) {
    struct stat metadata;
    return stat(path, &metadata) == 0;
}

static void reset_store(void) {
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    make_directory(STORAGE_DATA_MOUNT);
    make_directory(STORAGE_DATA_MOUNT "/sets");
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_init());
}

static macro_set_t make_set(uint32_t value) {
    macro_set_t set = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = make_uuid(value),
        .revision = 1U,
    };
    TEST_CHECK(snprintf(set.name, sizeof(set.name), "Active set delete") > 0);
    return set;
}

static void assert_active_set(bool expected_present, const app_uuid_t *expected_id) {
    bool present = true;
    app_uuid_t active = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_active_set_read(&present, &active));
    TEST_CHECK_EQ_INT((int)expected_present, (int)present);
    if (expected_present) {
        TEST_CHECK_EQ_UUID(expected_id, &active);
    }
}

/* SPEC 10.1: nothing infers an active set. A fresh repository has none. */
static void test_fresh_repository_has_no_active_set(void) {
    reset_store();
    assert_active_set(false, NULL);
}

static void test_select_requires_an_existing_set(void) {
    reset_store();
    const app_uuid_t absent = make_uuid(99U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NOT_FOUND, storage_set_select(&absent));
    assert_active_set(false, NULL);
}

static void test_select_records_the_active_set(void) {
    reset_store();
    macro_set_t set = make_set(10U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_select(&set.id));
    assert_active_set(true, &set.id);
}

/* Deleting the active set clears it in the same write that removes it from the
 * order: the two cannot disagree, because they are one file. */
static void test_deleting_the_active_set_clears_it(void) {
    reset_store();
    macro_set_t set = make_set(20U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_select(&set.id));

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_delete(&set.id, 1U));
    assert_active_set(false, NULL);
    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_make_set_path(&set.id, path, sizeof(path)));
    TEST_CHECK(!path_exists(path));
}

static void test_deleting_another_set_preserves_the_active_set(void) {
    reset_store();
    macro_set_t deleted = make_set(30U);
    macro_set_t active = make_set(31U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&deleted));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&active));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_select(&active.id));

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_delete(&deleted.id, 1U));
    assert_active_set(true, &active.id);
}

/* A failed delete must leave both halves untouched -- the set still readable and
 * still active -- rather than clearing the selection for a set that is still
 * there. A stale expected revision is the reachable way to fail one. */
static void test_failed_delete_preserves_the_active_set(void) {
    reset_store();
    macro_set_t set = make_set(40U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_select(&set.id));

    TEST_CHECK_APP_ERROR(APP_ERROR_CONFLICT, storage_set_delete(&set.id, 99U));
    assert_active_set(true, &set.id);
    macro_set_t readback = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_read(&set.id, &readback));
}

/* Re-selecting the set that is already active is a no-op that must not burn an
 * index revision. */
static void test_reselecting_the_active_set_is_idempotent(void) {
    reset_store();
    macro_set_t set = make_set(50U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_select(&set.id));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_select(&set.id));
    assert_active_set(true, &set.id);
}

static void test_invalid_arguments_are_rejected(void) {
    reset_store();
    bool present = false;
    app_uuid_t active = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_set_select(NULL));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_active_set_read(NULL, &active));
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, storage_active_set_read(&present, NULL));
}

int main(void) {
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_init());
    test_fresh_repository_has_no_active_set();
    test_select_requires_an_existing_set();
    test_select_records_the_active_set();
    test_deleting_the_active_set_clears_it();
    test_deleting_another_set_preserves_the_active_set();
    test_failed_delete_preserves_the_active_set();
    test_reselecting_the_active_set_is_idempotent();
    test_invalid_arguments_are_rejected();
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_deinit());
    puts("storage active-set tests passed");
    return EXIT_SUCCESS;
}
