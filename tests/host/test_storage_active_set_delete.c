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
#include "storage_repository_sets_internal.h"
#include "test_assert.h"
#include "test_temp_dir.h"

typedef struct {
    app_uuid_t active_set_id;
    bool has_active_set;
    app_error_code_t failure;
    size_t call_count;
} settings_fixture_t;

static app_uuid_t make_uuid(uint32_t value) {
    char text[APP_UUID_BUFFER_LENGTH];
    const int written = snprintf(text, sizeof(text), "%08" PRIx32 "-0000-4000-8000-%012" PRIx64,
                                 value, (uint64_t)value);
    TEST_CHECK_EQ_INT((int)APP_UUID_STRING_LENGTH, written);
    app_uuid_t uuid = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, app_uuid_parse(text, &uuid));
    return uuid;
}

static app_error_code_t clear_active_set(void *context, const app_uuid_t *set_id) {
    settings_fixture_t *fixture = context;
    ++fixture->call_count;
    if (fixture->failure != APP_ERROR_NONE) {
        return fixture->failure;
    }
    if (fixture->has_active_set && app_uuid_equal(&fixture->active_set_id, set_id)) {
        fixture->has_active_set = false;
        memset(&fixture->active_set_id, 0, sizeof(fixture->active_set_id));
    }
    return APP_ERROR_NONE;
}

static void make_directory(const char *path) {
    TEST_CHECK(mkdir(path, 0750) == 0 || errno == EEXIST);
}

static bool path_exists(const char *path) {
    struct stat metadata;
    return stat(path, &metadata) == 0;
}

static void reset_store(settings_fixture_t *fixture) {
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    static const char *const paths[] = {
        STORAGE_DATA_MOUNT,
        STORAGE_DATA_MOUNT "/sets",
        STORAGE_DATA_MOUNT "/staging",
        STORAGE_DATA_MOUNT "/trash",
        STORAGE_DATA_MOUNT "/transactions",
    };
    for (size_t index = 0U; index < (sizeof(paths) / sizeof(paths[0])); ++index) {
        make_directory(paths[index]);
    }
    *fixture = (settings_fixture_t){0};
    const storage_repository_set_settings_ops_t operations = {
        .context = fixture,
        .clear_active_set_if_matches = clear_active_set,
    };
    storage_repository_sets_set_settings_ops_for_test(&operations);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_init());
}

static macro_set_t make_set(uint32_t value) {
    macro_set_t set = {
        .schema_version = APP_SCHEMA_VERSION,
        .id = make_uuid(value),
        .revision = 1U,
    };
    TEST_CHECK(snprintf(set.name, sizeof(set.name), "Active set delete") > 0);
    TEST_CHECK(snprintf(set.description, sizeof(set.description), "Settings bridge") > 0);
    TEST_CHECK(snprintf(set.manufacturer, sizeof(set.manufacturer), "Test") > 0);
    TEST_CHECK(snprintf(set.model, sizeof(set.model), "Model") > 0);
    TEST_CHECK(snprintf(set.board, sizeof(set.board), "board") > 0);
    TEST_CHECK(snprintf(set.keyboard_layout, sizeof(set.keyboard_layout), "en-US") > 0);
    return set;
}

static void test_matching_active_set_is_cleared_before_delete(void) {
    settings_fixture_t fixture;
    reset_store(&fixture);
    macro_set_t set = make_set(10U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));
    fixture.has_active_set = true;
    fixture.active_set_id = set.id;

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_delete(&set.id, 1U));
    TEST_CHECK_EQ_U64(1U, fixture.call_count);
    TEST_CHECK(!fixture.has_active_set);
    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_make_set_path(&set.id, path, sizeof(path)));
    TEST_CHECK(!path_exists(path));
}

static void test_settings_failure_blocks_filesystem_delete(void) {
    settings_fixture_t fixture;
    reset_store(&fixture);
    macro_set_t set = make_set(20U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&set));
    fixture.has_active_set = true;
    fixture.active_set_id = set.id;
    fixture.failure = APP_ERROR_STORAGE_UNAVAILABLE;

    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, storage_set_delete(&set.id, 1U));
    TEST_CHECK_EQ_U64(1U, fixture.call_count);
    TEST_CHECK(fixture.has_active_set);
    char path[APP_PATH_MAX_BYTES];
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_make_set_path(&set.id, path, sizeof(path)));
    TEST_CHECK(path_exists(path));
    macro_set_t readback = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_read(&set.id, &readback));
}

static void test_nonmatching_active_set_is_preserved(void) {
    settings_fixture_t fixture;
    reset_store(&fixture);
    macro_set_t deleted = make_set(30U);
    macro_set_t active = make_set(31U);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&deleted));
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_create(&active));
    fixture.has_active_set = true;
    fixture.active_set_id = active.id;

    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_delete(&deleted.id, 1U));
    TEST_CHECK(fixture.has_active_set);
    TEST_CHECK_EQ_UUID(&active.id, &fixture.active_set_id);
}

int main(void) {
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_init());
    test_matching_active_set_is_cleared_before_delete();
    test_settings_failure_blocks_filesystem_delete();
    test_nonmatching_active_set_is_preserved();
    storage_repository_sets_reset_settings_ops_for_test();
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_repository_lock_deinit());
    puts("storage active-set deletion tests passed");
    return EXIT_SUCCESS;
}
