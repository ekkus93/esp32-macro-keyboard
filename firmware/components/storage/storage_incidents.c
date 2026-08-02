#include "storage_incidents.h"

#include <stdio.h>
#include <string.h>

#include "app_error.h"

static storage_incident_report_t incidents;

void storage_incident_record_discard(const char *path, app_error_code_t error) {
    ++incidents.total;
    if (path == NULL || incidents.count >= STORAGE_INCIDENT_MAX) {
        return;
    }
    storage_incident_t *entry = &incidents.items[incidents.count];
    const int written = snprintf(entry->path, sizeof(entry->path), "%s", path);
    if (written < 0) {
        return;
    }
    entry->error = error;
    ++incidents.count;
}

void storage_incident_record_temporary_removed(void) {
    ++incidents.temporaries_removed;
}

void storage_incidents_snapshot(storage_incident_report_t *out_report) {
    if (out_report == NULL) {
        return;
    }
    *out_report = incidents;
}

void storage_incidents_reset(void) {
    memset(&incidents, 0, sizeof(incidents));
}
