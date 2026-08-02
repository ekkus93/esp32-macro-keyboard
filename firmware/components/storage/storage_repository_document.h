#ifndef STORAGE_REPOSITORY_DOCUMENT_H
#define STORAGE_REPOSITORY_DOCUMENT_H

#include <stddef.h>

#include "app_error.h"
#include "app_uuid.h"
#include "storage_object_json.h"

/* Read one set file into a document. Returns APP_ERROR_NOT_FOUND when the set
 * does not exist, and deletes the file and reports APP_ERROR_STORAGE_CORRUPT
 * when it cannot be parsed (SPEC 13.6). The caller owns `out_document`. */
app_error_code_t storage_repository_load_set_document(const app_uuid_t *set_id,
                                                      storage_set_document_t *out_document);

/* Serialize and atomically replace the set's file (SPEC 13.4). */
app_error_code_t storage_repository_store_set_document(const storage_set_document_t *document);

app_error_code_t storage_repository_remove_set_file(const app_uuid_t *set_id);

/* Index of `macro_id` in the document's ordered macros, or SIZE_MAX. */
size_t storage_repository_find_macro(const storage_set_document_t *document,
                                     const app_uuid_t *macro_id);

#endif
