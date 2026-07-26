#ifndef STORAGE_MOUNT_CORE_H
#define STORAGE_MOUNT_CORE_H

#include "app_error.h"
#include "storage.h"

/* Backend seam for the mount orchestration. The production wrapper supplies
 * littlefs-backed operations; host tests supply fakes. Keeping the orchestration
 * (mount order, rollback, and ownership tracking) here makes it host-testable
 * without the ESP-IDF VFS. */
typedef struct {
    void *context;
    app_error_code_t (*mount_web)(void *context);
    app_error_code_t (*mount_data)(void *context);
    app_error_code_t (*unmount_web)(void *context);
    app_error_code_t (*unmount_data)(void *context);
    app_error_code_t (*prepare_directories)(void *context);
} storage_mount_ops_t;

/* Mount the web partition, then the data partition, then prepare the directory
 * topology, rolling back exactly what was mounted on any failure. The state
 * flags reflect what remains mounted afterward: a flag is set only after that
 * partition mounts and cleared only after it unmounts successfully, so a
 * partition left mounted by a failed rollback stays visible. */
app_error_code_t storage_mount_core_mount(const storage_mount_ops_t *ops,
                                          storage_mount_state_t *state);

/* Unmount every still-mounted partition (data first, then web). Both partitions
 * are attempted even if one unmount fails; the first failure is returned and its
 * flag stays set so the residual mount remains visible for retry. */
app_error_code_t storage_mount_core_unmount(const storage_mount_ops_t *ops,
                                            storage_mount_state_t *state);

#endif
