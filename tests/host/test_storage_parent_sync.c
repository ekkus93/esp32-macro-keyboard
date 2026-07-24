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

typedef struct {
    size_t next_value;
    size_t calls;
} uuid_sequence_t;

static int adapter_open(void *context, const char *path, int flags, mode_t mode)
{
    return fake_fs_open(context, path, flags, mode);
}

static ssize_t adapter_read(void *context,
                            int descriptor,
                            void *buffer,
                            size_t length)
{
    return fake_fs_read(context, descriptor, buffer, length);
}

static ssize_t adapter_write(void *context,
                             int descriptor,
                             const void *buffer,
                             size_t length)
{
    return fake_fs_write(context, descriptor, buffer, length);
}

static int adapter_sync(void *context, int descriptor)
{
    return fake_fs_sync(context, descriptor);
}

static int adapter_sync_parent(void *context, const char *path)
{
    return fake_fs_sync_parent(context, path);
}

static int adapter_close(void *context, int descriptor)
{
    return fake_fs_close(context, descriptor);
}

static int adapter_stat(void *context,
                        const char *path,
                        struct stat *metadata)
{
    return fake_fs_stat(context, path, metadata);
}

static int adapter_rename(void *context,
                          const char *source,
                          const char *destination)
{
    return fake_fs_rename(context, source, destination);
}

static int adapter_unlink(void *context, const char *path)
{
    return fake_fs_unlink(context, path);
}

static storage_fs_ops_t make_operations(fake_fs_backend_t *filesystem)
{
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

static app_error_code_t generate_uuid(void *context, app_uuid_t *out_uuid)
{
    uuid_sequence_t *sequence = context;
    if (sequence == NULL || out_uuid == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    ++sequence->calls;
    ++sequence->next_value;
    const int written = snprintf(out_uuid->value,
                                 sizeof(out_uuid->value),
                                 "00000000-0000-4000-8000-%012zu",
                                 sequence->next_value);
    return written == (int)APP_UUID_STRING_LENGTH ? APP_ERROR_NONE
                                                  : APP_ERROR_INTERNAL;
}

static void make_path(char *output,
                      size_t output_size,
                      const test_temp_dir_t *directory,
                      const char *name)
{
    const int written = snprintf(output,
                                 output_size,
                                 "%s/%s",
                                 directory->path,
                                 name);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < output_size);
}

static void make_operation_path(char *output,
                                size_t output_size,
                                const char *path,
                                const char *kind)
{
    const int written = snprintf(output,
                                 output_size,
                                 "%s.%s.00000000-0000-4000-8000-000000000001",
                                 path,
                                 kind);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < output_size);
}

static void write_text(const char *path, const char *text)
{
    const int descriptor = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    TEST_CHECK(descriptor >= 0);
    const size_t length = strlen(text);
    const ssize_t count = write(descriptor, text, length);
    TEST_CHECK(count >= 0);
    TEST_CHECK_EQ_U64(length, (size_t)count);
    TEST_CHECK_EQ_INT(0, close(descriptor));
}

static void read_text(const char *path, char *output, size_t output_size)
{
    TEST_CHECK(output_size > 0U);
    const int descriptor = open(path, O_RDONLY);
    TEST_CHECK(descriptor >= 0);
    const ssize_t count = read(descriptor, output, output_size - 1U);
    TEST_CHECK(count >= 0);
    output[(size_t)count] = '\0';
    TEST_CHECK_EQ_INT(0, close(descriptor));
}

static bool path_exists(const char *path)
{
    struct stat metadata;
    return stat(path, &metadata) == 0;
}

static app_error_code_t atomic_write(const char *path,
                                     const char *text,
                                     storage_fs_ops_t *operations,
                                     uuid_sequence_t *uuids,
                                     fake_fs_backend_t *filesystem)
{
    return storage_atomic_write_with_ops_and_parent_sync(
        path,
        text,
        strlen(text),
        true,
        operations,
        generate_uuid,
        uuids,
        adapter_sync_parent,
        filesystem);
}

static void assert_no_operation_files(const char *path)
{
    char temporary[APP_PATH_MAX_BYTES];
    char backup[APP_PATH_MAX_BYTES];
    make_operation_path(temporary, sizeof(temporary), path, "tmp");
    make_operation_path(backup, sizeof(backup), path, "bak");
    TEST_CHECK(!path_exists(temporary));
    TEST_CHECK(!path_exists(backup));
}

static void test_missing_parent_sync_is_rejected(void)
{
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "object.json");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {0};

    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      storage_atomic_write_with_ops_and_parent_sync(
                          path,
                          "new",
                          strlen("new"),
                          true,
                          &operations,
                          generate_uuid,
                          &uuids,
                          NULL,
                          &filesystem));
    TEST_CHECK_EQ_U64(0U, filesystem.calls.call_count);
    TEST_CHECK(!path_exists(path));
    test_temp_dir_remove(&directory);
}

static void test_create_and_replace_barrier_counts(void)
{
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "object.json");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {0};

    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      atomic_write(path,
                                   "first",
                                   &operations,
                                   &uuids,
                                   &filesystem));
    TEST_CHECK_EQ_U64(1U,
                      filesystem.operation_counts[FAKE_FS_SYNC_PARENT]);
    assert_no_operation_files(path);

    fake_fs_backend_reset(&filesystem);
    uuids = (uuid_sequence_t){0};
    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      atomic_write(path,
                                   "second",
                                   &operations,
                                   &uuids,
                                   &filesystem));
    TEST_CHECK_EQ_U64(2U,
                      filesystem.operation_counts[FAKE_FS_SYNC_PARENT]);
    char output[32U];
    read_text(path, output, sizeof(output));
    TEST_CHECK_EQ_STRING("second", output);
    assert_no_operation_files(path);
    test_temp_dir_remove(&directory);
}

static void test_create_activation_barrier_failure_rolls_back(void)
{
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "create.json");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    fake_fs_backend_fail_on(&filesystem, FAKE_FS_SYNC_PARENT, 1U, EIO);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {0};

    TEST_CHECK_EQ_INT(APP_ERROR_IO,
                      atomic_write(path,
                                   "new",
                                   &operations,
                                   &uuids,
                                   &filesystem));
    TEST_CHECK(!path_exists(path));
    TEST_CHECK_EQ_U64(2U,
                      filesystem.operation_counts[FAKE_FS_SYNC_PARENT]);
    assert_no_operation_files(path);
    test_temp_dir_remove(&directory);
}

static void test_backup_barrier_failure_preserves_old_destination(void)
{
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "replace.json");
    write_text(path, "old");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    fake_fs_backend_fail_on(&filesystem, FAKE_FS_SYNC_PARENT, 1U, ENOSPC);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {0};

    TEST_CHECK_EQ_INT(APP_ERROR_STORAGE_FULL,
                      atomic_write(path,
                                   "new",
                                   &operations,
                                   &uuids,
                                   &filesystem));
    char output[32U];
    read_text(path, output, sizeof(output));
    TEST_CHECK_EQ_STRING("old", output);
    TEST_CHECK_EQ_U64(2U,
                      filesystem.operation_counts[FAKE_FS_SYNC_PARENT]);
    assert_no_operation_files(path);
    test_temp_dir_remove(&directory);
}

static void test_activation_barrier_failure_preserves_old_destination(void)
{
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "replace.json");
    write_text(path, "old");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    fake_fs_backend_fail_on(&filesystem, FAKE_FS_SYNC_PARENT, 2U, EIO);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {0};

    TEST_CHECK_EQ_INT(APP_ERROR_IO,
                      atomic_write(path,
                                   "new",
                                   &operations,
                                   &uuids,
                                   &filesystem));
    char output[32U];
    read_text(path, output, sizeof(output));
    TEST_CHECK_EQ_STRING("old", output);
    TEST_CHECK_EQ_U64(3U,
                      filesystem.operation_counts[FAKE_FS_SYNC_PARENT]);
    TEST_CHECK_EQ_U64(4U, filesystem.operation_counts[FAKE_FS_RENAME]);
    assert_no_operation_files(path);
    test_temp_dir_remove(&directory);
}

static void test_restore_failure_compensates_with_new_destination(void)
{
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "replace.json");
    write_text(path, "old");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    fake_fs_backend_add_failure(&filesystem, FAKE_FS_SYNC_PARENT, 2U, EIO);
    fake_fs_backend_add_failure(&filesystem, FAKE_FS_RENAME, 4U, EACCES);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {0};

    TEST_CHECK_EQ_INT(APP_ERROR_IO,
                      atomic_write(path,
                                   "new",
                                   &operations,
                                   &uuids,
                                   &filesystem));
    char output[32U];
    read_text(path, output, sizeof(output));
    TEST_CHECK_EQ_STRING("new", output);

    char temporary[APP_PATH_MAX_BYTES];
    char backup[APP_PATH_MAX_BYTES];
    make_operation_path(temporary, sizeof(temporary), path, "tmp");
    make_operation_path(backup, sizeof(backup), path, "bak");
    TEST_CHECK(!path_exists(temporary));
    TEST_CHECK(path_exists(backup));
    read_text(backup, output, sizeof(output));
    TEST_CHECK_EQ_STRING("old", output);
    TEST_CHECK_EQ_U64(5U, filesystem.operation_counts[FAKE_FS_RENAME]);
    TEST_CHECK_EQ_U64(3U,
                      filesystem.operation_counts[FAKE_FS_SYNC_PARENT]);
    TEST_CHECK_EQ_INT(0, unlink(backup));
    test_temp_dir_remove(&directory);
}

int main(void)
{
    test_missing_parent_sync_is_rejected();
    test_create_and_replace_barrier_counts();
    test_create_activation_barrier_failure_rolls_back();
    test_backup_barrier_failure_preserves_old_destination();
    test_activation_barrier_failure_preserves_old_destination();
    test_restore_failure_compensates_with_new_destination();
    puts("storage parent sync tests passed");
    return EXIT_SUCCESS;
}
