#ifndef STORAGE_INCIDENTS_H
#define STORAGE_INCIDENTS_H

#include <stddef.h>

#include "app_error.h"
#include "macro_limits.h"

/*
 * What the device lost, and why.
 *
 * SPEC 13.6 requires a deleted corrupt object to be reported with its path AND
 * its error; SPEC 20.3 additionally requires diagnostics to show stray
 * temporary files removed at boot. Neither could be satisfied by logging: these
 * files compile for host tests, where ESP_LOG does not exist, and a log line is
 * not reachable through the API anyway.
 *
 * So the record is a small bounded table in RAM. It is deliberately NOT
 * persisted: writing a record of a storage failure back to the same storage is
 * how the quarantine mechanism this replaced grew to a thousand lines, and on a
 * 512 KiB partition the evidence competes with the user's own data (SPEC 13.6).
 * The device reports what was lost; it does not hoard it.
 */
#define STORAGE_INCIDENT_MAX 8U

typedef struct {
    char path[APP_PATH_MAX_BYTES];
    app_error_code_t error;
} storage_incident_t;

typedef struct {
    storage_incident_t items[STORAGE_INCIDENT_MAX];
    /* Entries enumerated in `items`. */
    size_t count;
    /* Objects discarded overall since boot; may exceed `count`, so a caller can
     * always tell that more were lost than it can enumerate. */
    size_t total;
    /* Stray `*.tmp` files unlinked by boot recovery (SPEC 13.4). */
    size_t temporaries_removed;
} storage_incident_report_t;

/* Records an object deleted because it could not be parsed or validated.
 * Keeps the FIRST STORAGE_INCIDENT_MAX incidents rather than the most recent:
 * the earliest failure after boot is the one that explains the rest. */
void storage_incident_record_discard(const char *path, app_error_code_t error);
void storage_incident_record_temporary_removed(void);
void storage_incidents_snapshot(storage_incident_report_t *out_report);
void storage_incidents_reset(void);

#endif
