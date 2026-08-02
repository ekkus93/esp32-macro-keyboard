#ifndef STORAGE_H
#define STORAGE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_limits.h"

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

/* Explicit mount ownership. app_core consults this during cleanup so a partition
 * that is still mounted after a partial or failed storage_mount_all() is unmounted
 * rather than leaked. */
typedef struct {
    bool web_mounted;
    bool data_mounted;
} storage_mount_state_t;

app_error_code_t storage_mount_all(void);
app_error_code_t storage_unmount_all(void);
storage_mount_state_t storage_mount_state(void);
app_error_code_t storage_prepare_directories(void);
app_error_code_t storage_make_set_path(const app_uuid_t *set_id, char *buffer, size_t buffer_size);
app_error_code_t storage_atomic_write(const char *path, const void *data, size_t data_length,
                                      bool sync_required);
app_error_code_t storage_atomic_recover_all(void);

/* Total/used bytes for a mounted LittleFS partition (STORAGE_WEB_PARTITION or
 * STORAGE_DATA_PARTITION), for Phase 19 diagnostics (FIX1 handoff §7.1). Not
 * host-testable (queries the mounted LittleFS partition directly); the
 * diagnostics aggregator reaches it through an injected ops seam. */
app_error_code_t storage_partition_capacity(const char *partition_label, size_t *out_total_bytes,
                                            size_t *out_used_bytes);
#endif
