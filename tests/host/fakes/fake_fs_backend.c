#include "fake_fs_backend.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *operation_name(fake_fs_operation_t operation)
{
    static const char *const names[FAKE_FS_OPERATION_COUNT] = {
        "fs_open",
        "fs_read",
        "fs_write",
        "fs_close",
        "fs_sync",
        "fs_rename",
        "fs_unlink",
        "fs_stat",
        "fs_mkdir",
        "fs_open_dir",
        "fs_read_dir",
        "fs_close_dir",
        "fs_rmdir",
    };
    if (operation < 0 || operation >= FAKE_FS_OPERATION_COUNT) {
        abort();
    }
    return names[(size_t)operation];
}

static bool should_fail(fake_fs_backend_t *filesystem,
                        fake_fs_operation_t operation)
{
    if (filesystem == NULL || operation < 0 ||
        operation >= FAKE_FS_OPERATION_COUNT) {
        abort();
    }
    const size_t index = (size_t)operation;
    ++filesystem->operation_counts[index];
    if (fake_call_log_record(&filesystem->calls,
                             operation_name(operation),
                             filesystem->operation_counts[index],
                             0U)) {
        errno = EIO;
        return true;
    }
    for (size_t failure_index = 0U;
         failure_index < filesystem->failure_count;
         ++failure_index) {
        const fake_fs_failure_t *failure =
            &filesystem->failures[failure_index];
        if (failure->active && failure->operation == operation &&
            failure->occurrence == filesystem->operation_counts[index]) {
            errno = failure->error_number;
            return true;
        }
    }
    return false;
}

void fake_fs_backend_reset(fake_fs_backend_t *filesystem)
{
    if (filesystem == NULL) {
        abort();
    }
    memset(filesystem, 0, sizeof(*filesystem));
    fake_call_log_reset(&filesystem->calls);
}

void fake_fs_backend_add_failure(fake_fs_backend_t *filesystem,
                                 fake_fs_operation_t operation,
                                 size_t occurrence,
                                 int error_number)
{
    if (filesystem == NULL || operation < 0 ||
        operation >= FAKE_FS_OPERATION_COUNT || occurrence == 0U ||
        error_number == 0 ||
        filesystem->failure_count >= FAKE_FS_FAILURE_CAPACITY) {
        abort();
    }
    filesystem->failures[filesystem->failure_count++] =
        (fake_fs_failure_t){
            .operation = operation,
            .occurrence = occurrence,
            .error_number = error_number,
            .active = true,
        };
}

void fake_fs_backend_fail_on(fake_fs_backend_t *filesystem,
                             fake_fs_operation_t operation,
                             size_t occurrence,
                             int error_number)
{
    if (filesystem == NULL) {
        abort();
    }
    memset(filesystem->failures, 0, sizeof(filesystem->failures));
    filesystem->failure_count = 0U;
    fake_fs_backend_add_failure(filesystem,
                                operation,
                                occurrence,
                                error_number);
}

void fake_fs_backend_set_short_read(fake_fs_backend_t *filesystem,
                                    size_t maximum)
{
    if (filesystem == NULL) {
        abort();
    }
    filesystem->short_read_limit = maximum;
}

void fake_fs_backend_set_short_write(fake_fs_backend_t *filesystem,
                                     size_t maximum)
{
    if (filesystem == NULL) {
        abort();
    }
    filesystem->short_write_limit = maximum;
}

int fake_fs_open(fake_fs_backend_t *filesystem,
                 const char *path,
                 int flags,
                 mode_t mode)
{
    if (path == NULL || should_fail(filesystem, FAKE_FS_OPEN)) {
        return -1;
    }
    return open(path, flags, mode);
}

ssize_t fake_fs_read(fake_fs_backend_t *filesystem,
                     int descriptor,
                     void *buffer,
                     size_t length)
{
    if (buffer == NULL || should_fail(filesystem, FAKE_FS_READ)) {
        return -1;
    }
    size_t requested = length;
    if (filesystem->short_read_limit != 0U &&
        requested > filesystem->short_read_limit) {
        requested = filesystem->short_read_limit;
    }
    return read(descriptor, buffer, requested);
}

ssize_t fake_fs_write(fake_fs_backend_t *filesystem,
                      int descriptor,
                      const void *buffer,
                      size_t length)
{
    if (buffer == NULL || should_fail(filesystem, FAKE_FS_WRITE)) {
        return -1;
    }
    size_t requested = length;
    if (filesystem->short_write_limit != 0U &&
        requested > filesystem->short_write_limit) {
        requested = filesystem->short_write_limit;
    }
    return write(descriptor, buffer, requested);
}

int fake_fs_close(fake_fs_backend_t *filesystem, int descriptor)
{
    if (should_fail(filesystem, FAKE_FS_CLOSE)) {
        return -1;
    }
    return close(descriptor);
}

int fake_fs_sync(fake_fs_backend_t *filesystem, int descriptor)
{
    if (should_fail(filesystem, FAKE_FS_SYNC)) {
        return -1;
    }
    return fsync(descriptor);
}

int fake_fs_rename(fake_fs_backend_t *filesystem,
                   const char *source,
                   const char *destination)
{
    if (source == NULL || destination == NULL ||
        should_fail(filesystem, FAKE_FS_RENAME)) {
        return -1;
    }
    return rename(source, destination);
}

int fake_fs_unlink(fake_fs_backend_t *filesystem, const char *path)
{
    if (path == NULL || should_fail(filesystem, FAKE_FS_UNLINK)) {
        return -1;
    }
    return unlink(path);
}

int fake_fs_stat(fake_fs_backend_t *filesystem,
                 const char *path,
                 struct stat *metadata)
{
    if (path == NULL || metadata == NULL ||
        should_fail(filesystem, FAKE_FS_STAT)) {
        return -1;
    }
    return stat(path, metadata);
}

int fake_fs_mkdir(fake_fs_backend_t *filesystem,
                  const char *path,
                  mode_t mode)
{
    if (path == NULL || should_fail(filesystem, FAKE_FS_MKDIR)) {
        return -1;
    }
    return mkdir(path, mode);
}

void *fake_fs_open_dir(fake_fs_backend_t *filesystem, const char *path)
{
    if (path == NULL || should_fail(filesystem, FAKE_FS_OPEN_DIR)) {
        return NULL;
    }
    return opendir(path);
}

int fake_fs_read_dir(fake_fs_backend_t *filesystem,
                     void *directory,
                     char *name,
                     size_t name_size,
                     bool *out_end)
{
    if (directory == NULL || name == NULL || name_size == 0U ||
        out_end == NULL || should_fail(filesystem, FAKE_FS_READ_DIR)) {
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

int fake_fs_close_dir(fake_fs_backend_t *filesystem, void *directory)
{
    if (directory == NULL ||
        should_fail(filesystem, FAKE_FS_CLOSE_DIR)) {
        return -1;
    }
    return closedir((DIR *)directory);
}

int fake_fs_rmdir(fake_fs_backend_t *filesystem, const char *path)
{
    if (path == NULL || should_fail(filesystem, FAKE_FS_RMDIR)) {
        return -1;
    }
    return rmdir(path);
}
