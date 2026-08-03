#ifndef STORAGE_PACKAGE_READER_H
#define STORAGE_PACKAGE_READER_H

#include <stddef.h>

#include "app_error.h"
#include "macro_model.h"

struct cJSON;

/* The parsed form of a package document, shared by the three operations that
 * apply one: import as new, replace, and restore.
 *
 * Each of them carried its own copy of this open/close pair and its own pair of
 * object parsers -- six functions differing only in variable names, plus a
 * seventh that serialized a cJSON node back to text purely so the text parsers
 * could re-parse it. That round trip is what makes the duplication legible as
 * accidental: nobody designs a parser that prints its input first. */
typedef struct {
    struct cJSON *root;
    const struct cJSON *sets;
    const struct cJSON *macros;
} package_tree_t;

/* Parses a package into a tree, requiring the `sets` and `macros` arrays.
 * SPEC 8.7 requires a package to carry the set and its macros in order and says
 * nothing about how; two sibling arrays is this implementation's choice, and
 * storage_package.c is where it is validated. Callers run
 * storage_package_validate first; this is the second pass that keeps the tree
 * instead of discarding it. */
app_error_code_t package_tree_open(const char *data, size_t length, package_tree_t *out_tree);
void package_tree_close(package_tree_t *tree);

/* The repository's node parsers with the one difference a package needs: a
 * package is caller-supplied input, so a node that does not fit the schema is a
 * bad request (SPEC 17), not a corrupt store. */
app_error_code_t package_parse_set_node(const struct cJSON *node, macro_set_t *out_set);
app_error_code_t package_parse_macro_node(const struct cJSON *node, macro_t *out_macro);

#endif
