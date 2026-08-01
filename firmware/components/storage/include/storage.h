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

#define STORAGE_QUARANTINE_REASON_MAX_BYTES 96U
#define STORAGE_QUARANTINE_MAX_ENTRIES 64U

typedef struct {
    app_uuid_t id;
    char source_path[APP_PATH_MAX_BYTES];
    char evidence_path[APP_PATH_MAX_BYTES];
    char reason[STORAGE_QUARANTINE_REASON_MAX_BYTES];
} storage_quarantine_entry_t;

typedef struct {
    storage_quarantine_entry_t items[STORAGE_QUARANTINE_MAX_ENTRIES];
    size_t count;
    /* Entries skipped because their committed directory was damaged (unparseable
     * name, corrupt/missing record, missing evidence, or over the entry limit).
     * A single damaged entry never makes the whole list unreadable (FIX1 §8.3):
     * `count` valid entries are still returned alongside this health signal. */
    size_t damaged_count;
} storage_quarantine_list_t;

typedef enum {
    STORAGE_TRANSACTION_IMPORT_SET = 0,
    STORAGE_TRANSACTION_REPLACE_SET,
    STORAGE_TRANSACTION_DUPLICATE_SET,
    STORAGE_TRANSACTION_DELETE_SET,
    STORAGE_TRANSACTION_RESTORE,
    STORAGE_TRANSACTION_MIGRATE,
    STORAGE_TRANSACTION_IMPORT_PACKAGE_SET
} storage_transaction_type_t;

typedef enum {
    STORAGE_TRANSACTION_PREPARED = 0,
    STORAGE_TRANSACTION_STAGED,
    STORAGE_TRANSACTION_BACKED_UP,
    STORAGE_TRANSACTION_ACTIVATED,
    STORAGE_TRANSACTION_INDEXED,
    STORAGE_TRANSACTION_COMPLETE
} storage_transaction_phase_t;

typedef struct {
    uint32_t schema_version;
    app_uuid_t id;
    storage_transaction_type_t type;
    storage_transaction_phase_t phase;
    char source[APP_PATH_MAX_BYTES];
    char staging[APP_PATH_MAX_BYTES];
    char destination[APP_PATH_MAX_BYTES];
    char backup[APP_PATH_MAX_BYTES];
    uint32_t expected_revision;
    uint32_t replacement_revision;
} storage_transaction_manifest_t;

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
app_error_code_t storage_make_macro_path(const app_uuid_t *set_id, const app_uuid_t *macro_id,
                                         char *buffer, size_t buffer_size);
app_error_code_t storage_make_global_macro_path(const app_uuid_t *macro_id, char *buffer,
                                                size_t buffer_size);
app_error_code_t storage_make_set_macro_order_path(const app_uuid_t *set_id, char *buffer,
                                                   size_t buffer_size);
app_error_code_t storage_make_procedure_path(const app_uuid_t *set_id,
                                             const app_uuid_t *procedure_id, char *buffer,
                                             size_t buffer_size);
app_error_code_t storage_make_procedure_order_path(const app_uuid_t *set_id, char *buffer,
                                                   size_t buffer_size);
app_error_code_t storage_make_progress_path(const app_uuid_t *set_id,
                                            const app_uuid_t *procedure_id, char *buffer,
                                            size_t buffer_size);
app_error_code_t storage_atomic_write(const char *path, const void *data, size_t data_length,
                                      bool sync_required);
app_error_code_t storage_atomic_recover_all(void);
app_error_code_t storage_transaction_recover_restores(void);
app_error_code_t storage_transaction_recover_all(void);
app_error_code_t storage_quarantine_file(const char *source_path, const char *reason,
                                         storage_quarantine_entry_t *out_entry);
app_error_code_t storage_quarantine_list(storage_quarantine_list_t *out_list);
app_error_code_t storage_quarantine_recover_all(void);

/* Total/used bytes for a mounted LittleFS partition (STORAGE_WEB_PARTITION or
 * STORAGE_DATA_PARTITION), for Phase 19 diagnostics (FIX1 handoff §7.1). Not
 * host-testable (queries the mounted LittleFS partition directly); the
 * diagnostics aggregator reaches it through an injected ops seam. */
app_error_code_t storage_partition_capacity(const char *partition_label, size_t *out_total_bytes,
                                            size_t *out_used_bytes);
app_error_code_t storage_transaction_write_manifest(const storage_transaction_manifest_t *manifest);

#endif
