#ifndef STORAGE_H
#define STORAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"

#ifndef STORAGE_WEB_MOUNT
#define STORAGE_WEB_MOUNT "/web"
#endif
#ifndef STORAGE_DATA_MOUNT
#define STORAGE_DATA_MOUNT "/data"
#endif
#ifndef STORAGE_WEB_PARTITION
#define STORAGE_WEB_PARTITION "webfs"
#endif
#ifndef STORAGE_DATA_PARTITION
#define STORAGE_DATA_PARTITION "userdata"
#endif

typedef struct {
    bool web_mounted;
    bool data_mounted;
} storage_mount_state_t;

app_error_code_t storage_mount_all(void);
app_error_code_t storage_unmount_all(void);
storage_mount_state_t storage_mount_state(void);
app_error_code_t storage_prepare_directories(void);
app_error_code_t storage_atomic_write(const char *path, const void *data, size_t data_length,
                                      bool sync_required);
app_error_code_t storage_partition_capacity(const char *partition_label, size_t *out_total_bytes,
                                            size_t *out_used_bytes);

#endif
