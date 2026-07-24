#include "storage_fs_ops.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#ifndef ESP_PLATFORM
static int copy_parent_path(const char *path, char **out_parent)
{
    if (out_parent != NULL) {
        *out_parent = NULL;
    }
    if (path == NULL || path[0] == '\0' || out_parent == NULL) {
        errno = EINVAL;
        return -1;
    }

    const char *slash = strrchr(path, '/');
    const char *parent_text = ".";
    size_t parent_length = 1U;
    if (slash == path) {
        parent_text = "/";
    } else if (slash != NULL) {
        parent_text = path;
        parent_length = (size_t)(slash - path);
    }

    char *parent = malloc(parent_length + 1U);
    if (parent == NULL) {
        errno = ENOMEM;
        return -1;
    }
    memcpy(parent, parent_text, parent_length);
    parent[parent_length] = '\0';
    *out_parent = parent;
    return 0;
}
#endif

int storage_fs_sync_parent_path(void *context, const char *path)
{
    (void)context;
#ifdef ESP_PLATFORM
    /*
     * LittleFS commits rename metadata atomically and ESP-IDF VFS does not
     * expose a POSIX directory-fsync primitive. File sync/close plus the
     * completed rename is therefore the platform durability boundary.
     */
    if (path == NULL || path[0] == '\0') {
        errno = EINVAL;
        return -1;
    }
    return 0;
#else
    char *parent = NULL;
    if (copy_parent_path(path, &parent) != 0) {
        return -1;
    }

    const int descriptor = open(parent, O_RDONLY);
    const int open_error = errno;
    free(parent);
    if (descriptor < 0) {
        errno = open_error;
        return -1;
    }

    if (fsync(descriptor) != 0) {
        const int sync_error = errno;
        (void)close(descriptor);
        errno = sync_error;
        return -1;
    }
    if (close(descriptor) != 0) {
        return -1;
    }
    return 0;
#endif
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

static int posix_mkdir(void *context, const char *path, mode_t mode)
{
    (void)context;
    return mkdir(path, mode);
}

static void *posix_open_directory(void *context, const char *path)
{
    (void)context;
    return opendir(path);
}

static int posix_read_directory(void *context,
                                void *directory,
                                char *name,
                                size_t name_size,
                                bool *out_end)
{
    (void)context;
    if (directory == NULL || name == NULL || name_size == 0U || out_end == NULL) {
        errno = EINVAL;
        return -1;
    }
    name[0] = '\0';
    *out_end = false;
    errno = 0;
    const struct dirent *entry = readdir((DIR *)directory);
    if (entry == NULL) {
        if (errno != 0) {
            return -1;
        }
        *out_end = true;
        return 0;
    }
    const size_t length = strlen(entry->d_name);
    if (length >= name_size) {
        errno = ENAMETOOLONG;
        return -1;
    }
    memcpy(name, entry->d_name, length + 1U);
    return 0;
}

static int posix_close_directory(void *context, void *directory)
{
    (void)context;
    if (directory == NULL) {
        errno = EINVAL;
        return -1;
    }
    return closedir((DIR *)directory);
}

static int posix_remove_directory(void *context, const char *path)
{
    (void)context;
    return rmdir(path);
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
        .make_directory = posix_mkdir,
        .open_directory = posix_open_directory,
        .read_directory = posix_read_directory,
        .close_directory = posix_close_directory,
        .remove_directory = posix_remove_directory,
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

bool storage_fs_ops_has_directory(const storage_fs_ops_t *operations)
{
    return storage_fs_ops_is_valid(operations) && operations->make_directory != NULL &&
           operations->open_directory != NULL && operations->read_directory != NULL &&
           operations->close_directory != NULL && operations->remove_directory != NULL;
}
