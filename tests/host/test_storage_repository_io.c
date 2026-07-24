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
#include "storage_repository_internal.h"
#include "storage.h"

#define CHECK(expression)                                                              \
    do {                                                                               \
        if (!(expression)) {                                                           \
            fprintf(stderr, "failed: %s:%d: %s\n", __FILE__, __LINE__, #expression); \
            exit(EXIT_FAILURE);                                                        \
        }                                                                              \
    } while (0)

static int ops_open(void *context, const char *path, int flags, mode_t mode)
{
    return fake_fs_open(context, path, flags, mode);
}

static ssize_t ops_read(void *context, int descriptor, void *buffer, size_t length)
{
    return fake_fs_read(context, descriptor, buffer, length);
}

static ssize_t ops_write(void *context,
                         int descriptor,
                         const void *buffer,
                         size_t length)
{
    return fake_fs_write(context, descriptor, buffer, length);
}

static int ops_sync(void *context, int descriptor)
{
    return fake_fs_sync(context, descriptor);
}

static int ops_close(void *context, int descriptor)
{
    return fake_fs_close(context, descriptor);
}

static int ops_stat(void *context, const char *path, struct stat *metadata)
{
    return fake_fs_stat(context, path, metadata);
}

static int ops_rename(void *context, const char *source, const char *destination)
{
    return fake_fs_rename(context, source, destination);
}

static int ops_unlink(void *context, const char *path)
{
    return fake_fs_unlink(context, path);
}

static int ops_mkdir(void *context, const char *path, mode_t mode)
{
    return fake_fs_mkdir(context, path, mode);
}

static void *ops_open_dir(void *context, const char *path)
{
    return fake_fs_open_dir(context, path);
}

static int ops_read_dir(void *context,
                        void *directory,
                        char *name,
                        size_t name_size,
                        bool *out_end)
{
    return fake_fs_read_dir(context, directory, name, name_size, out_end);
}

static int ops_close_dir(void *context, void *directory)
{
    return fake_fs_close_dir(context, directory);
}

static int ops_rmdir(void *context, const char *path)
{
    return fake_fs_rmdir(context, path);
}

static storage_fs_ops_t make_ops(fake_fs_backend_t *fake)
{
    return (storage_fs_ops_t){
        .context = fake,
        .open_file = ops_open,
        .read_file = ops_read,
        .write_file = ops_write,
        .sync_file = ops_sync,
        .close_file = ops_close,
        .stat_path = ops_stat,
        .rename_path = ops_rename,
        .unlink_path = ops_unlink,
        .make_directory = ops_mkdir,
        .open_directory = ops_open_dir,
        .read_directory = ops_read_dir,
        .close_directory = ops_close_dir,
        .remove_directory = ops_rmdir,
    };
}

static void write_file(const char *path, const char *data)
{
    const int descriptor = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    CHECK(descriptor >= 0);
    const size_t length = strlen(data);
    CHECK(write(descriptor, data, length) == (ssize_t)length);
    CHECK(close(descriptor) == 0);
}

static void join_path(char *output,
                      size_t output_size,
                      const char *directory,
                      const char *name)
{
    const int written = snprintf(output, output_size, "%s/%s", directory, name);
    CHECK(written > 0 && (size_t)written < output_size);
}

static void test_read_bounded(const char *root)
{
    char path[512U];
    join_path(path, sizeof(path), root, "value.txt");
    write_file(path, "abcdef");
    fake_fs_backend_t fake;
    fake_fs_backend_reset(&fake);
    storage_fs_ops_t ops = make_ops(&fake);
    char *data = NULL;
    size_t length = 0U;
    CHECK(storage_repository_read_bounded_file_with_ops(
              path, 6U, &data, &length, &ops) == APP_ERROR_NONE);
    CHECK(length == 6U && strcmp(data, "abcdef") == 0);
    free(data);

    fake_fs_backend_reset(&fake);
    fake_fs_backend_set_short_read(&fake, 2U);
    CHECK(storage_repository_read_bounded_file_with_ops(
              path, 6U, &data, &length, &ops) == APP_ERROR_NONE);
    CHECK(length == 6U);
    free(data);

    fake_fs_backend_reset(&fake);
    fake_fs_backend_fail_on(&fake, FAKE_FS_STAT, 1U, EIO);
    CHECK(storage_repository_read_bounded_file_with_ops(
              path, 6U, &data, &length, &ops) == APP_ERROR_IO);
    CHECK(data == NULL && length == 0U);

    fake_fs_backend_reset(&fake);
    fake_fs_backend_fail_on(&fake, FAKE_FS_OPEN, 1U, EIO);
    CHECK(storage_repository_read_bounded_file_with_ops(
              path, 6U, &data, &length, &ops) == APP_ERROR_IO);

    fake_fs_backend_reset(&fake);
    fake_fs_backend_fail_on(&fake, FAKE_FS_READ, 1U, EIO);
    CHECK(storage_repository_read_bounded_file_with_ops(
              path, 6U, &data, &length, &ops) == APP_ERROR_IO);

    fake_fs_backend_reset(&fake);
    fake_fs_backend_fail_on(&fake, FAKE_FS_CLOSE, 1U, EIO);
    CHECK(storage_repository_read_bounded_file_with_ops(
              path, 6U, &data, &length, &ops) == APP_ERROR_IO);

    fake_fs_backend_reset(&fake);
    CHECK(storage_repository_read_bounded_file_with_ops(
              path, 5U, &data, &length, &ops) == APP_ERROR_STORAGE_CORRUPT);
}

static void test_directory_entries(const char *root)
{
    char empty[512U];
    join_path(empty, sizeof(empty), root, "empty");
    CHECK(mkdir(empty, 0700) == 0);
    fake_fs_backend_t fake;
    fake_fs_backend_reset(&fake);
    storage_fs_ops_t ops = make_ops(&fake);
    bool has_entries = true;
    CHECK(storage_repository_directory_has_entries_with_ops(
              empty, &ops, &has_entries) == APP_ERROR_NONE);
    CHECK(!has_entries);
    char path[512U];
    join_path(path, sizeof(path), empty, "value");
    write_file(path, "x");
    fake_fs_backend_reset(&fake);
    CHECK(storage_repository_directory_has_entries_with_ops(
              empty, &ops, &has_entries) == APP_ERROR_NONE);
    CHECK(has_entries);

    fake_fs_backend_reset(&fake);
    fake_fs_backend_fail_on(&fake, FAKE_FS_OPEN_DIR, 1U, EIO);
    CHECK(storage_repository_directory_has_entries_with_ops(
              empty, &ops, &has_entries) == APP_ERROR_IO);
    CHECK(!has_entries);
    fake_fs_backend_reset(&fake);
    fake_fs_backend_fail_on(&fake, FAKE_FS_READ_DIR, 1U, EIO);
    CHECK(storage_repository_directory_has_entries_with_ops(
              empty, &ops, &has_entries) == APP_ERROR_IO);
    fake_fs_backend_reset(&fake);
    fake_fs_backend_fail_on(&fake, FAKE_FS_CLOSE_DIR, 1U, EIO);
    CHECK(storage_repository_directory_has_entries_with_ops(
              empty, &ops, &has_entries) == APP_ERROR_IO);
}

static void test_mkdir_and_remove_tree(const char *root)
{
    fake_fs_backend_t fake;
    fake_fs_backend_reset(&fake);
    storage_fs_ops_t ops = make_ops(&fake);
    char tree[512U];
    join_path(tree, sizeof(tree), root, "tree");
    CHECK(storage_repository_make_directory_with_ops(tree, &ops) == APP_ERROR_NONE);
    CHECK(storage_repository_make_directory_with_ops(tree, &ops) == APP_ERROR_CONFLICT);
    char child[512U];
    join_path(child, sizeof(child), tree, "child");
    CHECK(mkdir(child, 0700) == 0);
    char file[512U];
    join_path(file, sizeof(file), child, "file");
    write_file(file, "x");
    fake_fs_backend_reset(&fake);
    CHECK(storage_repository_remove_tree_with_ops(tree, &ops) == APP_ERROR_NONE);
    struct stat metadata;
    CHECK(stat(tree, &metadata) != 0 && errno == ENOENT);

    join_path(tree, sizeof(tree), root, "tree2");
    CHECK(mkdir(tree, 0700) == 0);
    join_path(file, sizeof(file), tree, "file");
    write_file(file, "x");
    fake_fs_backend_reset(&fake);
    fake_fs_backend_fail_on(&fake, FAKE_FS_STAT, 1U, EIO);
    CHECK(storage_repository_remove_tree_with_ops(tree, &ops) == APP_ERROR_IO);
    fake_fs_backend_reset(&fake);
    fake_fs_backend_fail_on(&fake, FAKE_FS_UNLINK, 1U, EIO);
    CHECK(storage_repository_remove_tree_with_ops(tree, &ops) == APP_ERROR_IO);
    fake_fs_backend_reset(&fake);
    CHECK(storage_repository_remove_tree_with_ops(tree, &ops) == APP_ERROR_NONE);

    join_path(tree, sizeof(tree), root, "tree3");
    CHECK(mkdir(tree, 0700) == 0);
    fake_fs_backend_reset(&fake);
    fake_fs_backend_fail_on(&fake, FAKE_FS_CLOSE_DIR, 1U, EIO);
    CHECK(storage_repository_remove_tree_with_ops(tree, &ops) == APP_ERROR_IO);
    fake_fs_backend_reset(&fake);
    CHECK(storage_repository_remove_tree_with_ops(tree, &ops) == APP_ERROR_NONE);

    join_path(tree, sizeof(tree), root, "tree4");
    CHECK(mkdir(tree, 0700) == 0);
    fake_fs_backend_reset(&fake);
    fake_fs_backend_fail_on(&fake, FAKE_FS_RMDIR, 1U, EIO);
    CHECK(storage_repository_remove_tree_with_ops(tree, &ops) == APP_ERROR_IO);
    fake_fs_backend_reset(&fake);
    CHECK(storage_repository_remove_tree_with_ops(tree, &ops) == APP_ERROR_NONE);
}

int main(void)
{
    char pattern[] = "/tmp/storage-io-test-XXXXXX";
    char *root = mkdtemp(pattern);
    CHECK(root != NULL);
    test_read_bounded(root);
    test_directory_entries(root);
    test_mkdir_and_remove_tree(root);
    CHECK(storage_repository_remove_tree_with_ops(
              root, storage_fs_ops_posix()) == APP_ERROR_NONE);
    puts("storage repository io tests passed");
    return EXIT_SUCCESS;
}
