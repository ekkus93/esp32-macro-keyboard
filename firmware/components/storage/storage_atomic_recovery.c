#include "storage_atomic_recovery.h"

#include <dirent.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "app_error.h"
#include "macro_limits.h"
#include "storage.h"
#include "storage_fs_ops.h"

#define TEMPORARY_SUFFIX ".tmp"

/* The sweep is a worklist rather than a recursive descent: this runs at boot on
 * main_task's stack, and recursion depth driven by on-disk directory nesting is
 * exactly the input an attacker or a corrupt filesystem controls.
 *
 * The bound is the widest the layout can legitimately be: the mount root, its
 * sets/ directory, and for every set a directory plus its macros/ child. A tree
 * that exceeds it is malformed, and is reported rather than silently truncated. */
#define RECOVERY_MAX_PENDING (((size_t)APP_MACRO_SETS_MAX * 2U) + 2U)

typedef struct {
    char paths[RECOVERY_MAX_PENDING][APP_PATH_MAX_BYTES];
    size_t count;
} directory_queue_t;

static app_error_code_t map_error_number(int error_number) {
    return error_number == ENOSPC ? APP_ERROR_STORAGE_FULL : APP_ERROR_IO;
}

static bool has_temporary_suffix(const char *name) {
    const size_t length = strlen(name);
    const size_t suffix_length = sizeof(TEMPORARY_SUFFIX) - 1U;
    /* Strictly greater: a bare ".tmp" names no destination, so it is ordinary
     * data rather than a staged write. */
    return length > suffix_length && strcmp(name + length - suffix_length, TEMPORARY_SUFFIX) == 0;
}

/* Records the first failure without abandoning the rest of the sweep: one
 * unremovable artifact must not leave the others behind. */
static void record_failure(app_error_code_t *outcome, app_error_code_t result) {
    if (*outcome == APP_ERROR_NONE) {
        *outcome = result;
    }
}

static app_error_code_t queue_push(directory_queue_t *queue, const char *path) {
    if (queue->count >= RECOVERY_MAX_PENDING) {
        return APP_ERROR_STORAGE_CORRUPT;
    }
    const int written = snprintf(queue->paths[queue->count], APP_PATH_MAX_BYTES, "%s", path);
    if (written < 0 || (size_t)written >= APP_PATH_MAX_BYTES) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    ++queue->count;
    return APP_ERROR_NONE;
}

/* Handles one directory entry: queue it if it is a directory, unlink it if it is
 * a stray temporary, ignore it otherwise. */
static void visit_entry(const storage_fs_ops_t *operations, const char *root, const char *name,
                        directory_queue_t *queue, app_error_code_t *outcome) {
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        return;
    }
    char path[APP_PATH_MAX_BYTES];
    const int written = snprintf(path, sizeof(path), "%s/%s", root, name);
    if (written < 0 || (size_t)written >= sizeof(path)) {
        record_failure(outcome, APP_ERROR_INVALID_ARGUMENT);
        return;
    }

    struct stat metadata;
    if (operations->stat_path(operations->context, path, &metadata) != 0) {
        const int stat_error = errno;
        if (stat_error != ENOENT) {
            record_failure(outcome, map_error_number(stat_error));
        }
        return;
    }
    if (S_ISDIR(metadata.st_mode)) {
        record_failure(outcome, queue_push(queue, path));
        return;
    }
    if (!has_temporary_suffix(name)) {
        return;
    }
    if (operations->unlink_path(operations->context, path) != 0) {
        const int unlink_error = errno;
        if (unlink_error != ENOENT) {
            record_failure(outcome, map_error_number(unlink_error));
        }
    }
}

static app_error_code_t sweep_directory(const storage_fs_ops_t *operations, const char *root,
                                        directory_queue_t *queue) {
    DIR *directory = opendir(root);
    if (directory == NULL) {
        const int open_error = errno;
        /* A mount that has never been provisioned has no sets/ yet; that is the
         * defined initial state, not a recovery failure. */
        return open_error == ENOENT ? APP_ERROR_NONE : map_error_number(open_error);
    }

    app_error_code_t outcome = APP_ERROR_NONE;
    while (true) {
        errno = 0;
        const struct dirent *entry = readdir(directory);
        if (entry == NULL) {
            const int read_error = errno;
            if (read_error != 0) {
                record_failure(&outcome, map_error_number(read_error));
            }
            break;
        }
        visit_entry(operations, root, entry->d_name, queue, &outcome);
    }

    if (closedir(directory) != 0) {
        record_failure(&outcome, map_error_number(errno));
    }
    return outcome;
}

app_error_code_t storage_atomic_recover_all_with_ops(const storage_fs_ops_t *operations) {
    if (!storage_fs_ops_is_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    /* ~13 KB of path buffers; far too much for main_task's boot-time frame. */
    directory_queue_t *queue = calloc(1U, sizeof(*queue));
    if (queue == NULL) {
        return APP_ERROR_INTERNAL;
    }

    app_error_code_t outcome = queue_push(queue, STORAGE_DATA_MOUNT);
    for (size_t index = 0U; index < queue->count; ++index) {
        /* sweep_directory appends to the queue as it discovers subdirectories,
         * so this loop re-reads count each iteration by design. */
        record_failure(&outcome, sweep_directory(operations, queue->paths[index], queue));
    }
    free(queue);
    return outcome;
}

app_error_code_t storage_atomic_recover_all(void) {
    return storage_atomic_recover_all_with_ops(storage_fs_ops_posix());
}
