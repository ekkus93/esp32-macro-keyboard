#include "storage_fs_ops.h"

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

static int posix_open(void *context, const char *path, int flags, mode_t mode)
{
    (void)context;
    return open(path, flags, mode);
}

static ssize_t posix_read(void *context, int descriptor, void *buffer, size_t length)
{
    (void)context;
    return read(descriptor, buffer, length);
}

static ssize_t posix_write(void *context,
                           int descriptor,
                           const void *buffer,
                           size_t length)
{
    (void)context;
    return write(descriptor, buffer, length);
}

static int posix_sync(void *context, int descriptor)
{
    (void)context;
    return fsync(descriptor);
}

static int posix_close(void *context, int descriptor)
{
    (void)context;
    return close(descriptor);
}

static int posix_stat(void *context, const char *path, struct stat *metadata)
{
    (void)context;
    return stat(path, metadata);
}

static int posix_rename(void *context, const char *source, const char *destination)
{
    (void)context;
    return rename(source, destination);
}

static int posix_unlink(void *context, const char *path)
{
    (void)context;
    return unlink(path);
}

const storage_fs_ops_t *storage_fs_ops_posix(void)
{
    static const storage_fs_ops_t operations = {
        .context = NULL,
        .open_file = posix_open,
        .read_file = posix_read,
        .write_file = posix_write,
        .sync_file = posix_sync,
        .close_file = posix_close,
        .stat_path = posix_stat,
        .rename_path = posix_rename,
        .unlink_path = posix_unlink,
    };
    return &operations;
}

bool storage_fs_ops_is_valid(const storage_fs_ops_t *operations)
{
    return operations != NULL && operations->open_file != NULL &&
           operations->read_file != NULL && operations->write_file != NULL &&
           operations->sync_file != NULL && operations->close_file != NULL &&
           operations->stat_path != NULL && operations->rename_path != NULL &&
           operations->unlink_path != NULL;
}
