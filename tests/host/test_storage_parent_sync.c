#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fake_fs_backend.h"
#include "storage.h"
#include "storage_atomic_internal.h"
#include "test_assert.h"
#include "test_temp_dir.h"

static int adapter_open(void *context, const char *path, int flags, mode_t mode) {
    return fake_fs_open(context, path, flags, mode);
}

static ssize_t adapter_read(void *context, int descriptor, void *buffer, size_t length) {
    return fake_fs_read(context, descriptor, buffer, length);
}

static ssize_t adapter_write(void *context, int descriptor, const void *buffer, size_t length) {
    return fake_fs_write(context, descriptor, buffer, length);
}

static int adapter_sync(void *context, int descriptor) {
    return fake_fs_sync(context, descriptor);
}

static int adapter_sync_parent(void *context, const char *path) {
    return fake_fs_sync_parent(context, path);
}

static int adapter_close(void *context, int descriptor) {
    return fake_fs_close(context, descriptor);
}

static int adapter_stat(void *context, const char *path, struct stat *metadata) {
    return fake_fs_stat(context, path, metadata);
}

static int adapter_rename(void *context, const char *source, const char *destination) {
    return fake_fs_rename(context, source, destination);
}

static int adapter_unlink(void *context, const char *path) {
    return fake_fs_unlink(context, path);
}

static storage_fs_ops_t make_operations(fake_fs_backend_t *filesystem) {
    return (storage_fs_ops_t){
        .context = filesystem,
        .open_file = adapter_open,
        .read_file = adapter_read,
        .write_file = adapter_write,
        .sync_file = adapter_sync,
        .close_file = adapter_close,
        .stat_path = adapter_stat,
        .rename_path = adapter_rename,
        .unlink_path = adapter_unlink,
    };
}

static void make_path(char *output, size_t output_size, const test_temp_dir_t *directory,
                      const char *name) {
    const int written = snprintf(output, output_size, "%s/%s", directory->path, name);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < output_size);
}

static void make_temporary_path(char *output, size_t output_size, const char *path) {
    const int written = snprintf(output, output_size, "%s.tmp", path);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < output_size);
}

static void write_text(const char *path, const char *text) {
    const int descriptor = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    TEST_CHECK(descriptor >= 0);
    const size_t length = strlen(text);
    const ssize_t count = write(descriptor, text, length);
    TEST_CHECK(count >= 0);
    TEST_CHECK_EQ_U64(length, (size_t)count);
    TEST_CHECK_EQ_INT(0, close(descriptor));
}

static void read_text(const char *path, char *output, size_t output_size) {
    TEST_CHECK(output_size > 0U);
    const int descriptor = open(path, O_RDONLY);
    TEST_CHECK(descriptor >= 0);
    const ssize_t count = read(descriptor, output, output_size - 1U);
    TEST_CHECK(count >= 0);
    output[(size_t)count] = '\0';
    TEST_CHECK_EQ_INT(0, close(descriptor));
}

static bool path_exists(const char *path) {
    struct stat metadata;
    return stat(path, &metadata) == 0;
}

static app_error_code_t atomic_write(const char *path, const char *text,
                                     storage_fs_ops_t *operations, fake_fs_backend_t *filesystem) {
    return storage_atomic_write_with_ops_and_parent_sync(path, text, strlen(text), true, operations,
                                                         adapter_sync_parent, filesystem);
}

/* There is no .bak file in the design (SPEC 13.4), so the only artifact a write
 * can leave behind is its temporary. */
static void assert_no_operation_files(const char *path) {
    char temporary[APP_PATH_MAX_BYTES];
    make_temporary_path(temporary, sizeof(temporary), path);
    TEST_CHECK(!path_exists(temporary));
}

static void test_missing_parent_sync_is_rejected(void) {
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "object.json");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);

    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      storage_atomic_write_with_ops_and_parent_sync(
                          path, "new", strlen("new"), true, &operations, NULL, &filesystem));
    TEST_CHECK_EQ_U64(0U, filesystem.calls.call_count);
    TEST_CHECK(!path_exists(path));
    test_temp_dir_remove(&directory);
}

static void test_create_and_replace_barrier_counts(void) {
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "object.json");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, atomic_write(path, "first", &operations, &filesystem));
    TEST_CHECK_EQ_U64(1U, filesystem.operation_counts[FAKE_FS_SYNC_PARENT]);
    assert_no_operation_files(path);

    /* Replacing an existing file costs exactly one parent sync, same as
     * creating it: there is no backup rename to barrier separately. */
    fake_fs_backend_reset(&filesystem);
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, atomic_write(path, "second", &operations, &filesystem));
    TEST_CHECK_EQ_U64(1U, filesystem.operation_counts[FAKE_FS_SYNC_PARENT]);
    char output[32U];
    read_text(path, output, sizeof(output));
    TEST_CHECK_EQ_STRING("second", output);
    assert_no_operation_files(path);
    test_temp_dir_remove(&directory);
}

/* A failed rename cannot half-replace the destination: the old bytes are still
 * there, unmodified, and the temporary is cleaned up. This is the property the
 * .bak ladder used to reconstruct after the fact -- a single rename has it by
 * construction. */
static void test_rename_failure_preserves_old_destination(void) {
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "replace.json");
    write_text(path, "old");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    fake_fs_backend_fail_on(&filesystem, FAKE_FS_RENAME, 1U, EIO);
    storage_fs_ops_t operations = make_operations(&filesystem);

    TEST_CHECK_EQ_INT(APP_ERROR_IO, atomic_write(path, "new", &operations, &filesystem));
    char output[32U];
    read_text(path, output, sizeof(output));
    TEST_CHECK_EQ_STRING("old", output);
    /* The parent is never synced, because nothing was durably changed. */
    TEST_CHECK_EQ_U64(0U, filesystem.operation_counts[FAKE_FS_SYNC_PARENT]);
    assert_no_operation_files(path);
    test_temp_dir_remove(&directory);
}

/* A create that fails at rename must leave no destination at all, and no
 * temporary either. */
static void test_rename_failure_on_create_leaves_nothing(void) {
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "create.json");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    fake_fs_backend_fail_on(&filesystem, FAKE_FS_RENAME, 1U, ENOSPC);
    storage_fs_ops_t operations = make_operations(&filesystem);

    TEST_CHECK_EQ_INT(APP_ERROR_STORAGE_FULL, atomic_write(path, "new", &operations, &filesystem));
    TEST_CHECK(!path_exists(path));
    assert_no_operation_files(path);
    test_temp_dir_remove(&directory);
}

/* The rename has already happened when the parent sync fails, so the new
 * content IS the destination. The error is reported and the write is NOT
 * undone: rolling back here would mean renaming a committed object away again,
 * which is exactly the second-copy behaviour SPEC 13.4 removed. */
static void test_parent_sync_failure_after_rename_is_reported_not_undone(void) {
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "replace.json");
    write_text(path, "old");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    fake_fs_backend_fail_on(&filesystem, FAKE_FS_SYNC_PARENT, 1U, EIO);
    storage_fs_ops_t operations = make_operations(&filesystem);

    TEST_CHECK_EQ_INT(APP_ERROR_IO, atomic_write(path, "new", &operations, &filesystem));
    char output[32U];
    read_text(path, output, sizeof(output));
    TEST_CHECK_EQ_STRING("new", output);
    TEST_CHECK_EQ_U64(1U, filesystem.operation_counts[FAKE_FS_RENAME]);
    assert_no_operation_files(path);
    test_temp_dir_remove(&directory);
}

static void test_parent_sync_enforces_operation_sequence(void) {
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "object.json");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);

    /*
     * Strict fake enforcement (UNIT_TESTS1 L915) for the durable atomic-write
     * sequence: stage the temporary, verify it by reading it back, rename it
     * over the destination, then fsync the parent directory. Strict mode aborts on any
     * unexpected, missing, or out-of-order operation, locking the crash-safety
     * barrier that makes the rename durable. If fs_sync_parent were dropped or
     * moved before the rename, this test would abort.
     */
    static const char *const expected[] = {
        "fs_open", "fs_write", "fs_sync",  "fs_close",  "fs_open",
        "fs_read", "fs_read",  "fs_close", "fs_rename", "fs_sync_parent",
    };
    fake_call_log_set_strict(&filesystem.calls, true);
    for (size_t index = 0U; index < (sizeof(expected) / sizeof(expected[0])); ++index) {
        fake_call_log_expect(&filesystem.calls, expected[index]);
    }

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, atomic_write(path, "value", &operations, &filesystem));
    fake_call_log_verify(&filesystem.calls);

    test_temp_dir_remove(&directory);
}

int main(void) {
    test_missing_parent_sync_is_rejected();
    test_create_and_replace_barrier_counts();
    test_parent_sync_enforces_operation_sequence();
    test_rename_failure_preserves_old_destination();
    test_rename_failure_on_create_leaves_nothing();
    test_parent_sync_failure_after_rename_is_reported_not_undone();
    puts("storage parent sync tests passed");
    return EXIT_SUCCESS;
}
