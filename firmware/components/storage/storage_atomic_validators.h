#ifndef STORAGE_ATOMIC_VALIDATORS_H
#define STORAGE_ATOMIC_VALIDATORS_H

#include "app_error.h"
#include "storage_fs_ops.h"

/* A candidate to validate: the canonical destination it would be activated to, and
 * the artifact path holding the proposed bytes. These are grouped into one struct
 * so a validator never takes two easily-swapped adjacent path parameters. (The
 * FIX1 §7.2 sketch used two separate const char* parameters; they are folded here
 * to satisfy the first-party bugprone-easily-swappable-parameters policy, which
 * forbids exempting parameters we control.) */
typedef struct {
    const char *destination;
    const char *candidate_path;
} storage_atomic_candidate_t;

/* An object-specific validator: confirm that `candidate->candidate_path` holds a
 * well-formed object for `candidate->destination`, using `context` for filesystem
 * access. Returns APP_ERROR_NONE when valid, or a storage error otherwise. */
typedef app_error_code_t (*storage_atomic_validate_fn)(void *context,
                                                       const storage_atomic_candidate_t *candidate);

/* The storage object types an atomic-write destination can name. The
 * MACRO/PROCEDURE/PROGRESS/SETTINGS object repositories are not yet implemented
 * (Phase 15); their destinations classify here but have no validator, so recovery
 * refuses to activate their candidates. */
typedef enum {
    STORAGE_ATOMIC_OBJECT_UNKNOWN = 0,
    STORAGE_ATOMIC_OBJECT_TRANSACTION_MANIFEST,
    STORAGE_ATOMIC_OBJECT_SCHEMA_MARKER,
    STORAGE_ATOMIC_OBJECT_SET_INDEX,
    STORAGE_ATOMIC_OBJECT_GLOBAL_MACRO_INDEX,
    STORAGE_ATOMIC_OBJECT_SET_METADATA,
    STORAGE_ATOMIC_OBJECT_MACRO,
    STORAGE_ATOMIC_OBJECT_PROCEDURE,
    STORAGE_ATOMIC_OBJECT_PROGRESS,
    STORAGE_ATOMIC_OBJECT_SETTINGS,
    STORAGE_ATOMIC_OBJECT_QUARANTINE_RECORD,
} storage_atomic_object_type_t;

/* Classify a canonical destination path into its storage object type
 * (STORAGE_ATOMIC_OBJECT_UNKNOWN if it matches no known layout). */
storage_atomic_object_type_t storage_atomic_classify_destination(const char *destination);

/* Validate that `candidate_path` is a well-formed object for `destination` using
 * the object-specific validator selected from `destination`'s type. Returns:
 *   APP_ERROR_NONE             - the candidate is valid for the destination;
 *   APP_ERROR_NOT_FOUND        - no object-specific validator exists for this
 *                                destination type; recovery must NOT activate it;
 *   other storage errors       - the candidate is malformed or unreadable.
 */
app_error_code_t storage_atomic_validate_candidate(const storage_fs_ops_t *operations,
                                                   const char *destination,
                                                   const char *candidate_path);

#endif
