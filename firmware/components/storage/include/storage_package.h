#ifndef STORAGE_PACKAGE_H
#define STORAGE_PACKAGE_H

#include <stdbool.h>
#include <stddef.h>

#include "app_error.h"
#include "app_uuid.h"
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
    size_t procedure_count;
    size_t progress_count;
} storage_package_summary_t;

app_error_code_t storage_package_validate(const char *data, size_t length,
                                          storage_package_kind_t expected_kind,
                                          storage_package_summary_t *out_summary);
app_error_code_t storage_package_export_set(const app_uuid_t *set_id, bool include_progress,
                                            char **out_data, size_t *out_length);
/* Which object a failed export stopped on. A backup aborts on the first
 * unreadable object; without this the caller can only report that the backup
 * failed, leaving the user no way to find or repair the offending object. */
typedef enum {
    STORAGE_PACKAGE_OBJECT_NONE = 0,
    STORAGE_PACKAGE_OBJECT_SET,
    STORAGE_PACKAGE_OBJECT_MACRO,
    STORAGE_PACKAGE_OBJECT_PROCEDURE,
    STORAGE_PACKAGE_OBJECT_PROGRESS,
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

app_error_code_t storage_package_export_backup(bool include_progress, char **out_data,
                                               size_t *out_length);
/* As storage_package_export_backup, but tolerant: objects that are individually
 * unusable are omitted and reported in out_skipped rather than failing the
 * whole backup, so one bad object cannot make the repository unbackupable.
 * Device-level errors still fail the export, and out_failure then names the
 * object it stopped on. Both out params may be NULL; passing NULL for
 * out_skipped does NOT restore the old abort-on-bad-object behaviour, it only
 * discards the report. */
app_error_code_t storage_package_export_backup_detail(bool include_progress, char **out_data,
                                                      size_t *out_length,
                                                      storage_package_failure_t *out_failure,
                                                      storage_package_skip_report_t *out_skipped);
app_error_code_t storage_package_restore_backup(const char *data, size_t length);
app_error_code_t storage_package_replace_set(const app_uuid_t *target_set_id,
                                             uint32_t expected_revision, const char *data,
                                             size_t length, macro_set_t *out_set);
app_error_code_t storage_package_import_set(const app_uuid_t *new_set_id, const char *data,
                                            size_t length, macro_set_t *out_set);
void storage_package_free(char *data);

#endif
