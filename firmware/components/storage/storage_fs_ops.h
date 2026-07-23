#ifndef STORAGE_FS_OPS_H
#define STORAGE_FS_OPS_H

#include <stdbool.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>

typedef struct {
    void *context;
    int (*open_file)(void *context, const char *path, int flags, mode_t mode);
    ssize_t (*read_file)(void *context, int descriptor, void *buffer, size_t length);
    ssize_t (*write_file)(void *context,
                          int descriptor,
                          const void *buffer,
                          size_t length);
    int (*sync_file)(void *context, int descriptor);
    int (*close_file)(void *context, int descriptor);
    int (*stat_path)(void *context, const char *path, struct stat *metadata);
    int (*rename_path)(void *context, const char *source, const char *destination);
    int (*unlink_path)(void *context, const char *path);
} storage_fs_ops_t;

const storage_fs_ops_t *storage_fs_ops_posix(void);
bool storage_fs_ops_is_valid(const storage_fs_ops_t *operations);

#endif
