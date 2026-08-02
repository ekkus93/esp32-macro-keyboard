#ifndef STORAGE_PACKAGE_H
#define STORAGE_PACKAGE_H

#include <stdbool.h>
#include <stddef.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_limits.h"
#include "macro_model.h"

typedef enum {
    STORAGE_PACKAGE_KIND_SET = 0,
    STORAGE_PACKAGE_KIND_BACKUP,
} storage_package_kind_t;

typedef struct {
    storage_package_kind_t kind;
    size_t package_bytes;
    size_t set_count;
    size_t local_macro_count;
} storage_package_summary_t;

app_error_code_t storage_package_validate(const char *data, size_t length,
                                          storage_package_kind_t expected_kind,
                                          storage_package_summary_t *out_summary);
app_error_code_t storage_package_export_set(const app_uuid_t *set_id, char **out_data,
                                            size_t *out_length);
/* Which object a failed export stopped on. A backup aborts on the first
 * unreadable object; without this the caller can only report that the backup
 * failed, leaving the user no way to find or repair the offending object. */
typedef enum {
    STORAGE_PACKAGE_OBJECT_NONE = 0,
    STORAGE_PACKAGE_OBJECT_SET,
    STORAGE_PACKAGE_OBJECT_MACRO,
} storage_package_object_kind_t;

typedef struct {
    storage_package_object_kind_t kind;
    bool has_set_id;
    app_uuid_t set_id;
    /* The object itself, when the failing read could identify it. */
    bool has_object_id;
    app_uuid_t object_id;
} storage_package_failure_t;

/* How many skipped objects a partial backup enumerates individually. Beyond
 * this the package still reports the true total, so a backup never understates
 * how much it dropped. */
#define STORAGE_PACKAGE_SKIP_REPORT_MAX 16U

typedef struct {
    storage_package_object_kind_t kind;
    bool has_set_id;
    app_uuid_t set_id;
    app_uuid_t object_id;
} storage_package_skipped_object_t;

typedef struct {
    storage_package_skipped_object_t items[STORAGE_PACKAGE_SKIP_REPORT_MAX];
    /* Entries enumerated in items. */
    size_t count;
    /* Objects skipped overall; may exceed count. */
    size_t total;
} storage_package_skip_report_t;

app_error_code_t storage_package_export_backup(char **out_data, size_t *out_length);
/* As storage_package_export_backup, but tolerant: objects that are individually
 * unusable are omitted and reported in out_skipped rather than failing the
 * whole backup, so one bad object cannot make the repository unbackupable.
 * Device-level errors still fail the export, and out_failure then names the
 * object it stopped on. Both out params may be NULL; passing NULL for
 * out_skipped does NOT restore the old abort-on-bad-object behaviour, it only
 * discards the report. */
app_error_code_t storage_package_export_backup_detail(char **out_data, size_t *out_length,
                                                      storage_package_failure_t *out_failure,
                                                      storage_package_skip_report_t *out_skipped);
/* Per-set outcome of a restore. Restore is explicitly NOT atomic across sets
 * (SPEC 13.5), so the caller is told exactly which sets were written and which
 * were not, and a run that failed any of them must not be reported as success
 * (SPEC 17). */
typedef struct {
    app_uuid_t set_id;
    /* APP_ERROR_NONE when this set's file was written in full. */
    app_error_code_t result;
} storage_restore_set_outcome_t;

typedef struct {
    storage_restore_set_outcome_t items[APP_MACRO_SETS_MAX];
    size_t count;
    size_t written;
    size_t failed;
    /* The first failure encountered, so the caller can pick an HTTP status that
     * reflects the actual fault (storage-full vs I/O) rather than flattening
     * every partial restore to one code. */
    app_error_code_t first_failure;
} storage_restore_report_t;

/* out_report may be NULL. Returns APP_ERROR_NONE only when every set in the
 * package was written. */
app_error_code_t storage_package_restore_backup(const char *data, size_t length,
                                                storage_restore_report_t *out_report);
app_error_code_t storage_package_replace_set(const app_uuid_t *target_set_id,
                                             uint32_t expected_revision, const char *data,
                                             size_t length, macro_set_t *out_set);
app_error_code_t storage_package_import_set(const app_uuid_t *new_set_id, const char *data,
                                            size_t length, macro_set_t *out_set);
void storage_package_free(char *data);

#endif
