#ifndef FAKE_FS_BACKEND_H
#define FAKE_FS_BACKEND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "fake_call_log.h"

#define FAKE_FS_FAILURE_CAPACITY 8U

typedef enum {
    FAKE_FS_OPEN = 0,
    FAKE_FS_READ,
    FAKE_FS_WRITE,
    FAKE_FS_CLOSE,
    FAKE_FS_SYNC,
    FAKE_FS_RENAME,
    FAKE_FS_UNLINK,
    FAKE_FS_STAT,
    FAKE_FS_MKDIR,
    FAKE_FS_OPEN_DIR,
    FAKE_FS_READ_DIR,
    FAKE_FS_CLOSE_DIR,
    FAKE_FS_RMDIR,
    FAKE_FS_OPERATION_COUNT
} fake_fs_operation_t;

typedef struct {
    fake_fs_operation_t operation;
    size_t occurrence;
    int error_number;
    bool active;
} fake_fs_failure_t;

typedef struct {
    fake_fs_failure_t failures[FAKE_FS_FAILURE_CAPACITY];
    size_t failure_count;
    size_t operation_counts[FAKE_FS_OPERATION_COUNT];
    size_t short_read_limit;
    size_t short_write_limit;
    fake_call_log_t calls;
} fake_fs_backend_t;

void fake_fs_backend_reset(fake_fs_backend_t *filesystem);
void fake_fs_backend_fail_on(fake_fs_backend_t *filesystem,
                             fake_fs_operation_t operation,
                             size_t occurrence,
                             int error_number);
void fake_fs_backend_add_failure(fake_fs_backend_t *filesystem,
                                 fake_fs_operation_t operation,
                                 size_t occurrence,
                                 int error_number);
void fake_fs_backend_set_short_read(fake_fs_backend_t *filesystem, size_t maximum);
void fake_fs_backend_set_short_write(fake_fs_backend_t *filesystem, size_t maximum);
int fake_fs_open(fake_fs_backend_t *filesystem, const char *path, int flags, mode_t mode);
ssize_t fake_fs_read(fake_fs_backend_t *filesystem,
                     int descriptor,
                     void *buffer,
                     size_t length);
ssize_t fake_fs_write(fake_fs_backend_t *filesystem,
                      int descriptor,
                      const void *buffer,
                      size_t length);
int fake_fs_close(fake_fs_backend_t *filesystem, int descriptor);
int fake_fs_sync(fake_fs_backend_t *filesystem, int descriptor);
int fake_fs_rename(fake_fs_backend_t *filesystem,
                   const char *source,
                   const char *destination);
int fake_fs_unlink(fake_fs_backend_t *filesystem, const char *path);
int fake_fs_stat(fake_fs_backend_t *filesystem,
                 const char *path,
                 struct stat *metadata);
int fake_fs_mkdir(fake_fs_backend_t *filesystem, const char *path, mode_t mode);
void *fake_fs_open_dir(fake_fs_backend_t *filesystem, const char *path);
int fake_fs_read_dir(fake_fs_backend_t *filesystem,
                     void *directory,
                     char *name,
                     size_t name_size,
                     bool *out_end);
int fake_fs_close_dir(fake_fs_backend_t *filesystem, void *directory);
int fake_fs_rmdir(fake_fs_backend_t *filesystem, const char *path);

#endif
