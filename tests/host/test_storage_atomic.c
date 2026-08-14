#include <dirent.h>
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

static ssize_t overreport_read(void *context, int descriptor, void *buffer, size_t length) {
    (void)context;
    (void)descriptor;
    (void)buffer;
    (void)length;
    return 2;
}

static ssize_t overreport_write(void *context, int descriptor, const void *buffer, size_t length) {
    (void)context;
    (void)descriptor;
    (void)buffer;
    (void)length;
    return 2;
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

static void write_file(const char *path, const char *text) {
    int descriptor = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    TEST_CHECK(descriptor >= 0);
    const size_t length = strlen(text);
    TEST_CHECK_EQ_U64(length, (size_t)write(descriptor, text, length));
    TEST_CHECK_EQ_INT(0, close(descriptor));
}

static void read_file(const char *path, char *output, size_t output_size) {
    TEST_CHECK(output_size > 0U);
    int descriptor = open(path, O_RDONLY);
    TEST_CHECK(descriptor >= 0);
    const ssize_t count = read(descriptor, output, output_size - 1U);
    TEST_CHECK(count >= 0);
    output[(size_t)count] = '\0';
    TEST_CHECK_EQ_INT(0, close(descriptor));
}

static void assert_no_temporary_files(const test_temp_dir_t *directory) {
    char command[APP_PATH_MAX_BYTES * 2U];
    const int written = snprintf(command, sizeof(command),
                                 "find '%s' -maxdepth 1 -type f "
                                 "-name '*.tmp' -print",
                                 directory->path);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < sizeof(command));
    FILE *pipe = popen(command, "r");
    TEST_CHECK(pipe != NULL);
    char output[APP_PATH_MAX_BYTES] = {0};
    TEST_CHECK(fgets(output, sizeof(output), pipe) == NULL);
    TEST_CHECK_EQ_INT(0, pclose(pipe));
}

static void test_invalid_arguments(void) {
    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    static const char data[] = "data";

    TEST_CHECK_EQ_INT(
        APP_ERROR_INVALID_ARGUMENT,
        storage_atomic_write_with_ops(NULL, data, sizeof(data) - 1U, true, &operations));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      storage_atomic_write_with_ops("file", NULL, 1U, true, &operations));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      storage_atomic_write_with_ops("file", data, sizeof(data) - 1U, true, NULL));
    operations.read_file = NULL;
    TEST_CHECK_EQ_INT(
        APP_ERROR_INVALID_ARGUMENT,
        storage_atomic_write_with_ops("file", data, sizeof(data) - 1U, true, &operations));
}

static void test_create_and_replace(void) {
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "object.json");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    static const char first[] = "first";
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_atomic_write_with_ops(path, first, sizeof(first) - 1U,
                                                                    true, &operations));
    char output[32U];
    read_file(path, output, sizeof(output));
    TEST_CHECK_EQ_STRING(first, output);
    assert_no_temporary_files(&directory);

    static const char second[] = "second-value";
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_atomic_write_with_ops(
                                          path, second, sizeof(second) - 1U, true, &operations));
    read_file(path, output, sizeof(output));
    TEST_CHECK_EQ_STRING(second, output);
    assert_no_temporary_files(&directory);
    test_temp_dir_remove(&directory);
}

/* SPEC 24.2 item: short writes */
static void test_short_io_is_completed(void) {
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "object.bin");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    fake_fs_backend_set_short_write(&filesystem, 2U);
    fake_fs_backend_set_short_read(&filesystem, 3U);
    storage_fs_ops_t operations = make_operations(&filesystem);
    static const char data[] = "0123456789";
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_atomic_write_with_ops(path, data, sizeof(data) - 1U,
                                                                    true, &operations));
    char output[32U];
    read_file(path, output, sizeof(output));
    TEST_CHECK_EQ_STRING(data, output);
    TEST_CHECK(filesystem.operation_counts[FAKE_FS_WRITE] > 1U);
    TEST_CHECK(filesystem.operation_counts[FAKE_FS_READ] > 1U);
    test_temp_dir_remove(&directory);
}

static void test_impossible_io_counts_are_rejected(void) {
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "object.bin");
    static const char data[] = "x";

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    operations.write_file = overreport_write;
    TEST_CHECK_EQ_INT(APP_ERROR_IO, storage_atomic_write_with_ops(path, data, sizeof(data) - 1U,
                                                                  true, &operations));
    TEST_CHECK(access(path, F_OK) != 0);
    assert_no_temporary_files(&directory);

    fake_fs_backend_reset(&filesystem);
    operations = make_operations(&filesystem);
    operations.read_file = overreport_read;
    TEST_CHECK_EQ_INT(APP_ERROR_IO, storage_atomic_write_with_ops(path, data, sizeof(data) - 1U,
                                                                  true, &operations));
    TEST_CHECK(access(path, F_OK) != 0);
    assert_no_temporary_files(&directory);
    test_temp_dir_remove(&directory);
}

/* SPEC 24.2 item: interruption between writing `.tmp` and `rename()` */
static void test_failures_preserve_destination(void) {
    static const fake_fs_operation_t operations_to_fail[] = {
        FAKE_FS_WRITE,
        FAKE_FS_SYNC,
        FAKE_FS_READ,
        FAKE_FS_RENAME,
    };
    for (size_t index = 0U; index < (sizeof(operations_to_fail) / sizeof(operations_to_fail[0]));
         ++index) {
        test_temp_dir_t directory = {0};
        test_temp_dir_create(&directory);
        char path[APP_PATH_MAX_BYTES];
        make_path(path, sizeof(path), &directory, "object.json");
        write_file(path, "old");

        fake_fs_backend_t filesystem;
        fake_fs_backend_reset(&filesystem);
        fake_fs_backend_fail_on(&filesystem, operations_to_fail[index], 1U, EIO);
        storage_fs_ops_t operations = make_operations(&filesystem);
        static const char data[] = "new";
        TEST_CHECK_EQ_INT(APP_ERROR_IO, storage_atomic_write_with_ops(path, data, sizeof(data) - 1U,
                                                                      true, &operations));
        char output[16U];
        read_file(path, output, sizeof(output));
        TEST_CHECK_EQ_STRING("old", output);
        assert_no_temporary_files(&directory);
        test_temp_dir_remove(&directory);
    }
}

/* A failed rename leaves the old destination byte-for-byte intact. With a
 * single rename there is nothing to roll back -- the barrier either commits or
 * it does not. */
static void test_activation_failure_leaves_destination_untouched(void) {
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "object.json");
    write_file(path, "old");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    fake_fs_backend_fail_on(&filesystem, FAKE_FS_RENAME, 1U, EIO);
    storage_fs_ops_t operations = make_operations(&filesystem);
    static const char data[] = "new";
    TEST_CHECK_EQ_INT(APP_ERROR_IO, storage_atomic_write_with_ops(path, data, sizeof(data) - 1U,
                                                                  true, &operations));
    char output[16U];
    read_file(path, output, sizeof(output));
    TEST_CHECK_EQ_STRING("old", output);
    assert_no_temporary_files(&directory);
    TEST_CHECK_EQ_U64(1U, filesystem.operation_counts[FAKE_FS_RENAME]);
    test_temp_dir_remove(&directory);
}

static void configure_primary_and_cleanup_failures(fake_fs_backend_t *filesystem,
                                                   fake_fs_operation_t primary_operation) {
    fake_fs_backend_reset(filesystem);
    fake_fs_backend_fail_on(filesystem, primary_operation, 1U, ENOSPC);
    fake_fs_backend_add_failure(filesystem, FAKE_FS_UNLINK, 1U, EIO);
}

static void test_stage_failure_retains_primary_and_cleanup_errors(void) {
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "object.json");
    char temporary[APP_PATH_MAX_BYTES];
    const int temporary_length = snprintf(temporary, sizeof(temporary), "%s.tmp", path);
    TEST_CHECK(temporary_length > 0);
    TEST_CHECK((size_t)temporary_length < sizeof(temporary));

    fake_fs_backend_t filesystem;
    configure_primary_and_cleanup_failures(&filesystem, FAKE_FS_WRITE);
    storage_fs_ops_t operations = make_operations(&filesystem);
    static const char data[] = "new";

    const app_operation_result_t detailed = storage_atomic_write_with_ops_and_parent_sync_result(
        path, data, sizeof(data) - 1U, true, &operations, storage_fs_sync_parent_path, NULL);
    TEST_CHECK_EQ_INT(APP_ERROR_STORAGE_FULL, detailed.primary_error);
    TEST_CHECK_EQ_INT(APP_ERROR_IO, detailed.cleanup_error);
    TEST_CHECK(detailed.cleanup_incomplete);
    TEST_CHECK_EQ_INT(APP_OPERATION_NOT_COMMITTED, detailed.commit_state);
    TEST_CHECK_EQ_U64(1U, filesystem.operation_counts[FAKE_FS_WRITE]);
    TEST_CHECK_EQ_U64(1U, filesystem.operation_counts[FAKE_FS_UNLINK]);
    TEST_CHECK(access(path, F_OK) != 0);
    TEST_CHECK_EQ_INT(0, access(temporary, F_OK));

    configure_primary_and_cleanup_failures(&filesystem, FAKE_FS_WRITE);
    operations = make_operations(&filesystem);
    TEST_CHECK_EQ_INT(
        APP_ERROR_STORAGE_FULL,
        storage_atomic_write_with_ops(path, data, sizeof(data) - 1U, true, &operations));
    TEST_CHECK_EQ_U64(1U, filesystem.operation_counts[FAKE_FS_UNLINK]);
    test_temp_dir_remove(&directory);
}

static void test_verify_failure_retains_primary_and_cleanup_errors(void) {
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "object.json");
    char temporary[APP_PATH_MAX_BYTES];
    const int temporary_length = snprintf(temporary, sizeof(temporary), "%s.tmp", path);
    TEST_CHECK(temporary_length > 0);
    TEST_CHECK((size_t)temporary_length < sizeof(temporary));

    fake_fs_backend_t filesystem;
    configure_primary_and_cleanup_failures(&filesystem, FAKE_FS_READ);
    storage_fs_ops_t operations = make_operations(&filesystem);
    static const char data[] = "new";

    const app_operation_result_t detailed = storage_atomic_write_with_ops_and_parent_sync_result(
        path, data, sizeof(data) - 1U, true, &operations, storage_fs_sync_parent_path, NULL);
    TEST_CHECK_EQ_INT(APP_ERROR_STORAGE_FULL, detailed.primary_error);
    TEST_CHECK_EQ_INT(APP_ERROR_IO, detailed.cleanup_error);
    TEST_CHECK(detailed.cleanup_incomplete);
    TEST_CHECK_EQ_INT(APP_OPERATION_NOT_COMMITTED, detailed.commit_state);
    TEST_CHECK_EQ_U64(1U, filesystem.operation_counts[FAKE_FS_READ]);
    TEST_CHECK_EQ_U64(1U, filesystem.operation_counts[FAKE_FS_UNLINK]);
    TEST_CHECK(access(path, F_OK) != 0);
    TEST_CHECK_EQ_INT(0, access(temporary, F_OK));

    configure_primary_and_cleanup_failures(&filesystem, FAKE_FS_READ);
    operations = make_operations(&filesystem);
    TEST_CHECK_EQ_INT(
        APP_ERROR_STORAGE_FULL,
        storage_atomic_write_with_ops(path, data, sizeof(data) - 1U, true, &operations));
    TEST_CHECK_EQ_U64(1U, filesystem.operation_counts[FAKE_FS_UNLINK]);
    test_temp_dir_remove(&directory);
}

static void test_rename_failure_retains_primary_and_cleanup_errors(void) {
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "object.json");
    write_file(path, "old");
    char temporary[APP_PATH_MAX_BYTES];
    const int temporary_length = snprintf(temporary, sizeof(temporary), "%s.tmp", path);
    TEST_CHECK(temporary_length > 0);
    TEST_CHECK((size_t)temporary_length < sizeof(temporary));

    fake_fs_backend_t filesystem;
    configure_primary_and_cleanup_failures(&filesystem, FAKE_FS_RENAME);
    storage_fs_ops_t operations = make_operations(&filesystem);
    static const char data[] = "new";

    const app_operation_result_t detailed = storage_atomic_write_with_ops_and_parent_sync_result(
        path, data, sizeof(data) - 1U, true, &operations, storage_fs_sync_parent_path, NULL);
    TEST_CHECK_EQ_INT(APP_ERROR_STORAGE_FULL, detailed.primary_error);
    TEST_CHECK_EQ_INT(APP_ERROR_IO, detailed.cleanup_error);
    TEST_CHECK(detailed.cleanup_incomplete);
    TEST_CHECK_EQ_INT(APP_OPERATION_NOT_COMMITTED, detailed.commit_state);
    char output[16U];
    read_file(path, output, sizeof(output));
    TEST_CHECK_EQ_STRING("old", output);
    TEST_CHECK_EQ_U64(1U, filesystem.operation_counts[FAKE_FS_RENAME]);
    TEST_CHECK_EQ_U64(1U, filesystem.operation_counts[FAKE_FS_UNLINK]);
    TEST_CHECK_EQ_INT(0, access(temporary, F_OK));

    configure_primary_and_cleanup_failures(&filesystem, FAKE_FS_RENAME);
    operations = make_operations(&filesystem);
    TEST_CHECK_EQ_INT(
        APP_ERROR_STORAGE_FULL,
        storage_atomic_write_with_ops(path, data, sizeof(data) - 1U, true, &operations));
    TEST_CHECK_EQ_U64(1U, filesystem.operation_counts[FAKE_FS_UNLINK]);
    read_file(path, output, sizeof(output));
    TEST_CHECK_EQ_STRING("old", output);
    test_temp_dir_remove(&directory);
}

static void test_parent_sync_failure_is_commit_uncertain(void) {
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "object.json");
    write_file(path, "old");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    fake_fs_backend_fail_on(&filesystem, FAKE_FS_SYNC_PARENT, 1U, EIO);
    storage_fs_ops_t operations = make_operations(&filesystem);
    static const char data[] = "new";

    const app_operation_result_t detailed = storage_atomic_write_with_ops_and_parent_sync_result(
        path, data, sizeof(data) - 1U, true, &operations, adapter_sync_parent, &filesystem);
    TEST_CHECK_EQ_INT(APP_ERROR_IO, detailed.primary_error);
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, detailed.cleanup_error);
    TEST_CHECK(!detailed.cleanup_incomplete);
    TEST_CHECK_EQ_INT(APP_OPERATION_COMMIT_UNCERTAIN, detailed.commit_state);
    char output[16U];
    read_file(path, output, sizeof(output));
    TEST_CHECK_EQ_STRING("new", output);
    TEST_CHECK_EQ_U64(1U, filesystem.operation_counts[FAKE_FS_RENAME]);
    TEST_CHECK_EQ_U64(1U, filesystem.operation_counts[FAKE_FS_SYNC_PARENT]);
    assert_no_temporary_files(&directory);
    test_temp_dir_remove(&directory);
}

static void test_create_enforces_operation_sequence(void) {
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "object.json");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);

    /*
     * Strict fake enforcement (UNIT_TESTS1 L915) for the atomic-write durability
     * sequence: open <target>.tmp, write it, fsync it, close it, verify it by
     * reading it back, then rename it over the destination. There is no stat of
     * the destination and no .bak rename: SPEC 13.4 permits nothing between the
     * verify and the rename. Strict mode aborts
     * on any unexpected, missing, or out-of-order filesystem operation, locking
     * this crash-safety ordering against accidental reordering in refactors.
     */
    static const char *const expected[] = {
        "fs_open", "fs_write", "fs_sync",  "fs_close",  "fs_open",
        "fs_read", "fs_read",  "fs_close", "fs_rename",
    };
    fake_call_log_set_strict(&filesystem.calls, true);
    for (size_t index = 0U; index < (sizeof(expected) / sizeof(expected[0])); ++index) {
        fake_call_log_expect(&filesystem.calls, expected[index]);
    }

    static const char payload[] = "value";
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, storage_atomic_write_with_ops(
                                          path, payload, sizeof(payload) - 1U, true, &operations));
    fake_call_log_verify(&filesystem.calls);

    test_temp_dir_remove(&directory);
}

int main(void) {
    test_invalid_arguments();
    test_create_and_replace();
    test_create_enforces_operation_sequence();
    test_short_io_is_completed();
    test_impossible_io_counts_are_rejected();
    test_failures_preserve_destination();
    test_activation_failure_leaves_destination_untouched();
    test_stage_failure_retains_primary_and_cleanup_errors();
    test_verify_failure_retains_primary_and_cleanup_errors();
    test_rename_failure_retains_primary_and_cleanup_errors();
    test_parent_sync_failure_is_commit_uncertain();
    puts("storage atomic tests passed");
    return EXIT_SUCCESS;
}
