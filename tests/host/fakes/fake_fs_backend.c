#include "fake_fs_backend.h"

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
    };
    if (operation < 0 || operation >= FAKE_FS_OPERATION_COUNT) {
        abort();
    }
    return names[(size_t)operation];
}

static bool should_fail(fake_fs_backend_t *filesystem, fake_fs_operation_t operation)
{
    if (filesystem == NULL || operation < 0 || operation >= FAKE_FS_OPERATION_COUNT) {
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
    if (filesystem->failure.active && filesystem->failure.operation == operation &&
        filesystem->failure.occurrence == filesystem->operation_counts[index]) {
        errno = filesystem->failure.error_number;
        return true;
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

void fake_fs_backend_fail_on(fake_fs_backend_t *filesystem,
                             fake_fs_operation_t operation,
                             size_t occurrence,
                             int error_number)
{
    if (filesystem == NULL || operation < 0 || operation >= FAKE_FS_OPERATION_COUNT ||
        occurrence == 0U || error_number == 0) {
        abort();
    }
    filesystem->failure = (fake_fs_failure_t){
        .operation = operation,
        .occurrence = occurrence,
        .error_number = error_number,
        .active = true,
    };
}

void fake_fs_backend_set_short_read(fake_fs_backend_t *filesystem, size_t maximum)
{
    if (filesystem == NULL) {
        abort();
    }
    filesystem->short_read_limit = maximum;
}

void fake_fs_backend_set_short_write(fake_fs_backend_t *filesystem, size_t maximum)
{
    if (filesystem == NULL) {
        abort();
    }
    filesystem->short_write_limit = maximum;
}

int fake_fs_open(fake_fs_backend_t *filesystem, const char *path, int flags, mode_t mode)
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
    if (filesystem->short_read_limit != 0U && requested > filesystem->short_read_limit) {
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
    if (filesystem->short_write_limit != 0U && requested > filesystem->short_write_limit) {
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

int fake_fs_rename(fake_fs_backend_t *filesystem, const char *source, const char *destination)
{
    if (source == NULL || destination == NULL || should_fail(filesystem, FAKE_FS_RENAME)) {
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

int fake_fs_stat(fake_fs_backend_t *filesystem, const char *path, void *metadata)
{
    if (path == NULL || metadata == NULL || should_fail(filesystem, FAKE_FS_STAT)) {
        return -1;
    }
    return stat(path, metadata);
}

int fake_fs_mkdir(fake_fs_backend_t *filesystem, const char *path, mode_t mode)
{
    if (path == NULL || should_fail(filesystem, FAKE_FS_MKDIR)) {
        return -1;
    }
    return mkdir(path, mode);
}
