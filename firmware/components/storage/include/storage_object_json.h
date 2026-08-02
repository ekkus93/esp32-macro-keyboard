#ifndef STORAGE_OBJECT_JSON_H
#define STORAGE_OBJECT_JSON_H

#include <stddef.h>

#include "app_error.h"
#include "app_uuid.h"
#include "macro_limits.h"
#include "macro_model.h"

/* A set file holds the set's metadata and its macros inline (SPEC 12.1), so its
 * bound is the same as a single-set package: the largest thing a user can put in
 * one set. The storage budget (SPEC 10.7) is what actually keeps a device from
 * filling up; this is the parse bound that stops a corrupt length field from
 * being trusted. */
#define STORAGE_SET_FILE_MAX_BYTES APP_IMPORT_PACKAGE_MAX_BYTES
#define STORAGE_MACRO_FILE_MAX_BYTES 8192U
#define STORAGE_INDEX_FILE_MAX_BYTES 8192U

/* A bounded list of object identifiers. This is no longer any file's contents --
 * order lives in the set file's macros array (SPEC 12.1) -- it is the shape a
 * reorder REQUEST arrives in, bounded so a hostile body cannot size an
 * allocation. */
#define STORAGE_ORDER_MAX_IDS APP_MACROS_PER_SET_MAX
_Static_assert(STORAGE_ORDER_MAX_IDS >= APP_MACRO_SETS_MAX,
               "STORAGE_ORDER_MAX_IDS must fit a macro-set order list");

typedef struct {
    app_uuid_t ids[STORAGE_ORDER_MAX_IDS];
    size_t count;
} storage_uuid_order_t;

/* A set and its ordered macros, as one unit.
 *
 * This is the shape of a set file and the unit of every read and write: reading
 * a set is parsing one file, and writing one is serializing one file, staging it
 * as `.tmp`, and renaming it (SPEC 13.4). `macros` is heap-allocated because a
 * full set is several hundred KB, which no task stack on this device can hold.
 *
 * Array order IS the user's order (SPEC 12.1). There is no order file and no
 * `sort_order` field to keep in step with it. */
typedef struct {
    macro_set_t set;
    macro_t *macros;
    size_t macro_count;
} storage_set_document_t;

void storage_set_document_free(storage_set_document_t *document);

/* Parse a complete set file. On success the caller owns `out_document` and must
 * release it with storage_set_document_free. */
app_error_code_t storage_set_document_parse(const char *data, size_t length,
                                            storage_set_document_t *out_document);
app_error_code_t storage_set_document_serialize(const storage_set_document_t *document,
                                                char **out_json, size_t *out_length);

/* Standalone macro JSON, used only by the package layer.
 *
 * A stored macro carries no `set_id` -- the file it lives in identifies its set
 * (SPEC 12.2). A package is a different container, so its macro entries carry
 * the owning set ID as the envelope field SPEC 12.2 explicitly permits, and
 * these two functions are where that envelope is written and read. */
app_error_code_t storage_repository_parse_macro_json(const char *data, size_t length,
                                                     macro_t *out_macro);
app_error_code_t storage_repository_serialize_macro_json(const macro_t *macro, char **out_json,
                                                         size_t *out_length);

/* The envelope form of a set: metadata only, no `macros` array. Used by package
 * `sets` entries and by API responses that list sets without their contents. */
app_error_code_t storage_repository_parse_set_json(const char *data, size_t length,
                                                   macro_set_t *out_set);
app_error_code_t storage_repository_serialize_set_json(const macro_set_t *set, char **out_json,
                                                       size_t *out_length);

#endif
