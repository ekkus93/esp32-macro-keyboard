#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "storage.h"
#include "storage_mount_core.h"
#include "test_assert.h"
#include "test_temp_dir.h"

/* ---- storage_mount_core: rollback + ownership tracking (fake backend) ---- */

typedef struct {
    app_error_code_t mount_web_result;
    app_error_code_t mount_data_result;
    app_error_code_t unmount_web_result;
    app_error_code_t unmount_data_result;
    app_error_code_t prepare_result;
    size_t mount_web_calls;
    size_t mount_data_calls;
    size_t unmount_web_calls;
    size_t unmount_data_calls;
    size_t prepare_calls;
} mount_fake_t;

static app_error_code_t fake_mount_web(void *context) {
    mount_fake_t *fake = context;
    ++fake->mount_web_calls;
    return fake->mount_web_result;
}

static app_error_code_t fake_mount_data(void *context) {
    mount_fake_t *fake = context;
    ++fake->mount_data_calls;
    return fake->mount_data_result;
}

static app_error_code_t fake_unmount_web(void *context) {
    mount_fake_t *fake = context;
    ++fake->unmount_web_calls;
    return fake->unmount_web_result;
}

static app_error_code_t fake_unmount_data(void *context) {
    mount_fake_t *fake = context;
    ++fake->unmount_data_calls;
    return fake->unmount_data_result;
}

static app_error_code_t fake_prepare(void *context) {
    mount_fake_t *fake = context;
    ++fake->prepare_calls;
    return fake->prepare_result;
}

static storage_mount_ops_t make_ops(mount_fake_t *fake) {
    return (storage_mount_ops_t){
        .context = fake,
        .mount_web = fake_mount_web,
        .mount_data = fake_mount_data,
        .unmount_web = fake_unmount_web,
        .unmount_data = fake_unmount_data,
        .prepare_directories = fake_prepare,
    };
}

static void test_clean_mount_tracks_state(void) {
    mount_fake_t fake = {0};
    const storage_mount_ops_t ops = make_ops(&fake);
    storage_mount_state_t state = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_mount_core_mount(&ops, &state));
    TEST_CHECK(state.web_mounted);
    TEST_CHECK(state.data_mounted);
    TEST_CHECK_EQ_U64(1U, fake.prepare_calls);
    TEST_CHECK_EQ_U64(0U, fake.unmount_web_calls);
    TEST_CHECK_EQ_U64(0U, fake.unmount_data_calls);
}

static void test_web_mount_failure(void) {
    mount_fake_t fake = {.mount_web_result = APP_ERROR_STORAGE_UNAVAILABLE};
    const storage_mount_ops_t ops = make_ops(&fake);
    storage_mount_state_t state = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, storage_mount_core_mount(&ops, &state));
    TEST_CHECK(!state.web_mounted);
    TEST_CHECK(!state.data_mounted);
    TEST_CHECK_EQ_U64(0U, fake.mount_data_calls);
    TEST_CHECK_EQ_U64(0U, fake.unmount_web_calls);
}

/* SPEC 24.2 item: no-format mount failure */
static void test_data_mount_failure_web_unmount_ok(void) {
    mount_fake_t fake = {.mount_data_result = APP_ERROR_STORAGE_UNAVAILABLE};
    const storage_mount_ops_t ops = make_ops(&fake);
    storage_mount_state_t state = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, storage_mount_core_mount(&ops, &state));
    /* The web partition was rolled back, so nothing remains mounted. */
    TEST_CHECK(!state.web_mounted);
    TEST_CHECK(!state.data_mounted);
    TEST_CHECK_EQ_U64(1U, fake.unmount_web_calls);
    TEST_CHECK_EQ_U64(0U, fake.prepare_calls);
}

static void test_data_mount_failure_web_unmount_fails(void) {
    mount_fake_t fake = {
        .mount_data_result = APP_ERROR_STORAGE_UNAVAILABLE,
        .unmount_web_result = APP_ERROR_IO,
    };
    const storage_mount_ops_t ops = make_ops(&fake);
    storage_mount_state_t state = {0};
    /* The rollback failed, so the cleanup error is reported and the still-mounted
     * web partition stays visible for a later retry. */
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, storage_mount_core_mount(&ops, &state));
    TEST_CHECK(state.web_mounted);
    TEST_CHECK(!state.data_mounted);
}

static void test_directory_prepare_failure_rolls_back_both(void) {
    mount_fake_t fake = {.prepare_result = APP_ERROR_STORAGE_CORRUPT};
    const storage_mount_ops_t ops = make_ops(&fake);
    storage_mount_state_t state = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, storage_mount_core_mount(&ops, &state));
    TEST_CHECK(!state.web_mounted);
    TEST_CHECK(!state.data_mounted);
    TEST_CHECK_EQ_U64(1U, fake.unmount_web_calls);
    TEST_CHECK_EQ_U64(1U, fake.unmount_data_calls);
}

static void test_unmount_continues_after_one_failure(void) {
    mount_fake_t fake = {.unmount_data_result = APP_ERROR_IO};
    const storage_mount_ops_t ops = make_ops(&fake);
    storage_mount_state_t state = {.web_mounted = true, .data_mounted = true};
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, storage_mount_core_unmount(&ops, &state));
    /* Both partitions are attempted even though the data unmount failed. */
    TEST_CHECK_EQ_U64(1U, fake.unmount_data_calls);
    TEST_CHECK_EQ_U64(1U, fake.unmount_web_calls);
    TEST_CHECK(state.data_mounted); /* failed unmount stays visible */
    TEST_CHECK(!state.web_mounted); /* succeeded */
}

/* ---- storage_prepare_directories: topology validation (real filesystem) ---- */

/* SPEC 13.3: /data holds the package index and package/, and nothing else. */
static const char *const required_dirs[] = {
    STORAGE_DATA_MOUNT "/package",
};

static bool is_directory(const char *path) {
    struct stat metadata;
    return stat(path, &metadata) == 0 && S_ISDIR(metadata.st_mode);
}

static void reset_data_root(void) {
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    TEST_CHECK(mkdir(STORAGE_DATA_MOUNT, 0750) == 0);
}

static void make_regular_file(const char *path) {
    const int descriptor = open(path, O_WRONLY | O_CREAT | O_EXCL, 0640);
    TEST_CHECK(descriptor >= 0);
    TEST_CHECK(close(descriptor) == 0);
}

static void test_prepare_directories_creates_and_is_idempotent(void) {
    reset_data_root();
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_prepare_directories());
    for (size_t index = 0U; index < (sizeof(required_dirs) / sizeof(required_dirs[0])); ++index) {
        TEST_CHECK(is_directory(required_dirs[index]));
    }
    /* Running again over existing directories is accepted. */
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_prepare_directories());
}

static void test_regular_file_collides_with_each_required_directory(void) {
    for (size_t index = 0U; index < (sizeof(required_dirs) / sizeof(required_dirs[0])); ++index) {
        reset_data_root();
        make_regular_file(required_dirs[index]);
        /* A regular file where a directory is required is a corrupt topology. */
        TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, storage_prepare_directories());
    }
}

static void test_symlink_where_directory_required_is_rejected(void) {
    reset_data_root();
    make_regular_file(STORAGE_DATA_MOUNT "/decoy");
    /* A symlink is not a directory. ensure_directory() resolves it with stat(),
     * so a link to a non-directory is rejected as a corrupt topology (LittleFS has
     * no symlinks, so this can only arise on the host filesystem). */
    TEST_CHECK(symlink(STORAGE_DATA_MOUNT "/decoy", STORAGE_DATA_MOUNT "/package") == 0);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_CORRUPT, storage_prepare_directories());
}

int main(void) {
    test_clean_mount_tracks_state();
    test_web_mount_failure();
    test_data_mount_failure_web_unmount_ok();
    test_data_mount_failure_web_unmount_fails();
    test_directory_prepare_failure_rolls_back_both();
    test_unmount_continues_after_one_failure();
    test_prepare_directories_creates_and_is_idempotent();
    test_regular_file_collides_with_each_required_directory();
    test_symlink_where_directory_required_is_rejected();
    test_temp_dir_remove_path(STORAGE_DATA_MOUNT);
    puts("storage mount tests passed");
    return EXIT_SUCCESS;
}
