#ifndef STORAGE_PACKAGE_WRITER_H
#define STORAGE_PACKAGE_WRITER_H

#include <stddef.h>

#include "app_error.h"
#include "macro_model.h"

/* The growing buffer both export paths build a package in.
 *
 * storage_package_export.c and storage_package_backup.c each carried their own
 * copy of this -- identical structs and six identical functions, differing only
 * in local variable names. One set export and one whole-repository backup write
 * the same format; only what they iterate differs. */
typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} package_writer_t;

app_error_code_t package_writer_append_bytes(package_writer_t *writer, const char *data,
                                             size_t length);
app_error_code_t package_writer_append_text(package_writer_t *writer, const char *text);

/* Appends an object someone has just serialized, and frees that JSON on every
 * path including failure -- the callers all had the same leak to avoid. */
app_error_code_t package_writer_append_serialized(package_writer_t *writer,
                                                  app_error_code_t serialization_result, char *json,
                                                  size_t length);
app_error_code_t package_writer_append_set(package_writer_t *writer, const macro_set_t *set);
app_error_code_t package_writer_append_macro(package_writer_t *writer, const macro_t *macro);

#endif
