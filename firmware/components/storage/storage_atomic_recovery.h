#ifndef STORAGE_ATOMIC_RECOVERY_H
#define STORAGE_ATOMIC_RECOVERY_H

#include <stddef.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_limits.h"

/* Leftover artifacts from an interrupted atomic write. `storage_atomic_write`
 * stages `<destination>.tmp.<uuid>` and, while swapping, `<destination>.bak.<uuid>`;
 * a crash between steps can leave either behind. Recovery parses those names back
 * into the destination they belong to and the operation that created them. */
typedef enum {
    STORAGE_ATOMIC_ARTIFACT_TEMPORARY = 0,
    STORAGE_ATOMIC_ARTIFACT_BACKUP,
} storage_atomic_artifact_kind_t;

typedef struct {
    char destination[APP_PATH_MAX_BYTES];
    app_uuid_t operation_id;
    storage_atomic_artifact_kind_t kind;
    char artifact_path[APP_PATH_MAX_BYTES];
} storage_atomic_artifact_t;

#define STORAGE_ATOMIC_RECOVERY_MAX_ARTIFACTS 64U

typedef struct {
    storage_atomic_artifact_t items[STORAGE_ATOMIC_RECOVERY_MAX_ARTIFACTS];
    size_t count;
} storage_atomic_artifact_list_t;

/* Parse one artifact path of the exact form
 *   <destination>.tmp.<lowercase-rfc4122-v4-uuid>
 *   <destination>.bak.<lowercase-rfc4122-v4-uuid>
 * Returns:
 *   APP_ERROR_NONE             - parsed; out_artifact populated.
 *   APP_ERROR_NOT_FOUND        - the name is not an artifact (skip it).
 *   APP_ERROR_INVALID_ARGUMENT - malformed input, or an artifact whose
 *                                reconstructed destination is empty, ends in a
 *                                separator, or contains a `..` path component
 *                                (path traversal / cross-directory escape).
 */
app_error_code_t storage_atomic_recovery_parse(const char *artifact_path,
                                               storage_atomic_artifact_t *out_artifact);

/* Parse `artifact_path` and append it to `list`. Propagates the parse result, and
 * additionally returns APP_ERROR_CONFLICT for a duplicate artifact_path already in
 * the list, or APP_ERROR_STORAGE_FULL when the list is full. Nothing is appended
 * on any error. */
app_error_code_t storage_atomic_recovery_list_add(storage_atomic_artifact_list_t *list,
                                                  const char *artifact_path);

#endif
