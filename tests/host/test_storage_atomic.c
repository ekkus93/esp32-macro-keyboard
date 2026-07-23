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
    size_t fail_on_call;
} uuid_sequence_t;

static int adapter_open(void *context, const char *path, int flags, mode_t mode)
{
    return fake_fs_open(context, path, flags, mode);
}

static ssize_t adapter_read(void *context, int descriptor, void *buffer, size_t length)
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

static int adapter_close(void *context, int descriptor)
{
    return fake_fs_close(context, descriptor);
}

static int adapter_stat(void *context, const char *path, struct stat *metadata)
{
    return fake_fs_stat(context, path, metadata);
}

static int adapter_rename(void *context, const char *source, const char *destination)
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
    if (sequence->fail_on_call != 0U && sequence->calls == sequence->fail_on_call) {
        return APP_ERROR_INTERNAL;
    }
    ++sequence->next_value;
    const int written = snprintf(out_uuid->value, sizeof(out_uuid->value),
                                 "00000000-0000-4000-8000-%012zu",
                                 sequence->next_value);
    return written == (int)APP_UUID_STRING_LENGTH ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

static void make_path(char *output,
                      size_t output_size,
                      const test_temp_dir_t *directory,
                      const char *name)
{
    const int written = snprintf(output, output_size, "%s/%s", directory->path, name);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < output_size);
}

static void write_file(const char *path, const char *text)
{
    int descriptor = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    TEST_CHECK(descriptor >= 0);
    const size_t length = strlen(text);
    TEST_CHECK_EQ_U64(length, (size_t)write(descriptor, text, length));
    TEST_CHECK_EQ_INT(0, close(descriptor));
}

static void read_file(const char *path, char *output, size_t output_size)
{
    TEST_CHECK(output_size > 0U);
    int descriptor = open(path, O_RDONLY);
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

static void assert_no_temporary_files(const test_temp_dir_t *directory)
{
    char command[APP_PATH_MAX_BYTES * 2U];
    const int written = snprintf(command, sizeof(command),
                                 "find '%s' -maxdepth 1 -type f "
                                 "\\( -name '*.tmp.*' -o -name '*.bak.*' \\) -print",
                                 directory->path);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < sizeof(command));
    FILE *pipe = popen(command, "r");
    TEST_CHECK(pipe != NULL);
    char output[APP_PATH_MAX_BYTES] = {0};
    TEST_CHECK(fgets(output, sizeof(output), pipe) == NULL);
    TEST_CHECK_EQ_INT(0, pclose(pipe));
}

static void test_invalid_arguments(void)
{
    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {0};
    static const char data[] = "data";

    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      storage_atomic_write_with_ops(NULL, data, sizeof(data) - 1U,
                                                    true, &operations,
                                                    generate_uuid, &uuids));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      storage_atomic_write_with_ops("file", NULL, 1U, true,
                                                    &operations, generate_uuid,
                                                    &uuids));
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      storage_atomic_write_with_ops("file", data,
                                                    sizeof(data) - 1U, true,
                                                    NULL, generate_uuid, &uuids));
    operations.read_file = NULL;
    TEST_CHECK_EQ_INT(APP_ERROR_INVALID_ARGUMENT,
                      storage_atomic_write_with_ops("file", data,
                                                    sizeof(data) - 1U, true,
                                                    &operations, generate_uuid,
                                                    &uuids));
}

static void test_create_and_replace(void)
{
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "object.json");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {0};
    static const char first[] = "first";
    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      storage_atomic_write_with_ops(path, first, sizeof(first) - 1U,
                                                    true, &operations,
                                                    generate_uuid, &uuids));
    char output[32U];
    read_file(path, output, sizeof(output));
    TEST_CHECK_EQ_STRING(first, output);
    assert_no_temporary_files(&directory);

    static const char second[] = "second-value";
    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      storage_atomic_write_with_ops(path, second, sizeof(second) - 1U,
                                                    true, &operations,
                                                    generate_uuid, &uuids));
    read_file(path, output, sizeof(output));
    TEST_CHECK_EQ_STRING(second, output);
    assert_no_temporary_files(&directory);
    test_temp_dir_remove(&directory);
}

static void test_short_io_is_completed(void)
{
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "object.bin");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    fake_fs_backend_set_short_write(&filesystem, 2U);
    fake_fs_backend_set_short_read(&filesystem, 3U);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {0};
    static const char data[] = "0123456789";
    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      storage_atomic_write_with_ops(path, data, sizeof(data) - 1U,
                                                    true, &operations,
                                                    generate_uuid, &uuids));
    char output[32U];
    read_file(path, output, sizeof(output));
    TEST_CHECK_EQ_STRING(data, output);
    TEST_CHECK(filesystem.operation_counts[FAKE_FS_WRITE] > 1U);
    TEST_CHECK(filesystem.operation_counts[FAKE_FS_READ] > 1U);
    test_temp_dir_remove(&directory);
}

static void test_failures_preserve_destination(void)
{
    static const fake_fs_operation_t operations_to_fail[] = {
        FAKE_FS_WRITE,
        FAKE_FS_SYNC,
        FAKE_FS_READ,
        FAKE_FS_STAT,
        FAKE_FS_RENAME,
    };
    for (size_t index = 0U;
         index < (sizeof(operations_to_fail) / sizeof(operations_to_fail[0]));
         ++index) {
        test_temp_dir_t directory = {0};
        test_temp_dir_create(&directory);
        char path[APP_PATH_MAX_BYTES];
        make_path(path, sizeof(path), &directory, "object.json");
        write_file(path, "old");

        fake_fs_backend_t filesystem;
        fake_fs_backend_reset(&filesystem);
        size_t occurrence = 1U;
        if (operations_to_fail[index] == FAKE_FS_STAT) {
            occurrence = 2U;
        }
        fake_fs_backend_fail_on(&filesystem, operations_to_fail[index], occurrence, EIO);
        storage_fs_ops_t operations = make_operations(&filesystem);
        uuid_sequence_t uuids = {0};
        static const char data[] = "new";
        TEST_CHECK_EQ_INT(APP_ERROR_IO,
                          storage_atomic_write_with_ops(path, data,
                                                        sizeof(data) - 1U, true,
                                                        &operations, generate_uuid,
                                                        &uuids));
        char output[16U];
        read_file(path, output, sizeof(output));
        TEST_CHECK_EQ_STRING("old", output);
        assert_no_temporary_files(&directory);
        test_temp_dir_remove(&directory);
    }
}

static void test_activation_failure_rolls_back(void)
{
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "object.json");
    write_file(path, "old");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    fake_fs_backend_fail_on(&filesystem, FAKE_FS_RENAME, 2U, EIO);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {0};
    static const char data[] = "new";
    TEST_CHECK_EQ_INT(APP_ERROR_IO,
                      storage_atomic_write_with_ops(path, data, sizeof(data) - 1U,
                                                    true, &operations,
                                                    generate_uuid, &uuids));
    char output[16U];
    read_file(path, output, sizeof(output));
    TEST_CHECK_EQ_STRING("old", output);
    assert_no_temporary_files(&directory);
    TEST_CHECK_EQ_U64(3U, filesystem.operation_counts[FAKE_FS_RENAME]);
    test_temp_dir_remove(&directory);
}

static void test_name_collision_retry_and_exhaustion(void)
{
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "object.json");

    char collision[APP_PATH_MAX_BYTES];
    const int written = snprintf(collision, sizeof(collision),
                                 "%s.tmp.00000000-0000-4000-8000-000000000001",
                                 path);
    TEST_CHECK(written > 0);
    TEST_CHECK((size_t)written < sizeof(collision));
    write_file(collision, "occupied");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {0};
    static const char data[] = "new";
    TEST_CHECK_EQ_INT(APP_ERROR_NONE,
                      storage_atomic_write_with_ops(path, data, sizeof(data) - 1U,
                                                    true, &operations,
                                                    generate_uuid, &uuids));
    TEST_CHECK_EQ_U64(2U, uuids.calls);
    TEST_CHECK(unlink(collision) == 0);

    TEST_CHECK(unlink(path) == 0);
    uuids = (uuid_sequence_t){0};
    for (size_t value = 1U; value <= 4U; ++value) {
        const int collision_length = snprintf(
            collision, sizeof(collision),
            "%s.tmp.00000000-0000-4000-8000-%012zu", path, value);
        TEST_CHECK(collision_length > 0);
        TEST_CHECK((size_t)collision_length < sizeof(collision));
        write_file(collision, "occupied");
    }
    fake_fs_backend_reset(&filesystem);
    TEST_CHECK_EQ_INT(APP_ERROR_CONFLICT,
                      storage_atomic_write_with_ops(path, data, sizeof(data) - 1U,
                                                    true, &operations,
                                                    generate_uuid, &uuids));
    TEST_CHECK_EQ_U64(4U, uuids.calls);
    TEST_CHECK(!path_exists(path));
    test_temp_dir_remove(&directory);
}

static void test_uuid_failure_has_no_side_effect(void)
{
    test_temp_dir_t directory = {0};
    test_temp_dir_create(&directory);
    char path[APP_PATH_MAX_BYTES];
    make_path(path, sizeof(path), &directory, "object.json");

    fake_fs_backend_t filesystem;
    fake_fs_backend_reset(&filesystem);
    storage_fs_ops_t operations = make_operations(&filesystem);
    uuid_sequence_t uuids = {.fail_on_call = 1U};
    static const char data[] = "new";
    TEST_CHECK_EQ_INT(APP_ERROR_INTERNAL,
                      storage_atomic_write_with_ops(path, data, sizeof(data) - 1U,
                                                    true, &operations,
                                                    generate_uuid, &uuids));
    TEST_CHECK_EQ_U64(0U, filesystem.calls.call_count);
    TEST_CHECK(!path_exists(path));
    test_temp_dir_remove(&directory);
}

int main(void)
{
    test_invalid_arguments();
    test_create_and_replace();
    test_short_io_is_completed();
    test_failures_preserve_destination();
    test_activation_failure_rolls_back();
    test_name_collision_retry_and_exhaustion();
    test_uuid_failure_has_no_side_effect();
    puts("storage atomic tests passed");
    return EXIT_SUCCESS;
}
