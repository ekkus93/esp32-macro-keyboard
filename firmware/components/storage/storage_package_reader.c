#include "storage_package_reader.h"

#include <stddef.h>
#include <string.h>

#include "app_error.h"
#include "cJSON.h"
#include "macro_model.h"
#include "storage_object_json.h"

/* A package arrives from a client, so a node that does not match the schema is a
 * malformed request rather than a damaged file. The repository parsers speak the
 * storage vocabulary because that is where they are usually called from. */
static app_error_code_t as_bad_request(app_error_code_t result) {
    return result == APP_ERROR_STORAGE_CORRUPT ? APP_ERROR_INVALID_ARGUMENT : result;
}

app_error_code_t package_parse_metadata_node(const struct cJSON *node,
                                             macro_package_t *out_package) {
    return as_bad_request(storage_repository_parse_package_node(node, out_package));
}

app_error_code_t package_parse_macro_node(const struct cJSON *node, macro_t *out_macro) {
    return as_bad_request(storage_repository_parse_macro_node(node, out_macro));
}

void package_tree_close(package_tree_t *tree) {
    if (tree == NULL) {
        return;
    }
    cJSON_Delete(tree->root);
    memset(tree, 0, sizeof(*tree));
}

app_error_code_t package_tree_open(const char *data, size_t length, package_tree_t *out_tree) {
    if (out_tree == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_tree, 0, sizeof(*out_tree));
    if (data == NULL || length == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts(data, length, &parse_end, false);
    /* Trailing content is a rejection, not something to ignore: the length is
     * the request body's length, and a document that ends early means the rest
     * of the body was never accounted for. */
    if (root == NULL || parse_end != data + length || !cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    out_tree->sets = cJSON_GetObjectItemCaseSensitive(root, "sets");
    out_tree->macros = cJSON_GetObjectItemCaseSensitive(root, "macros");
    if (!cJSON_IsArray(out_tree->sets) || !cJSON_IsArray(out_tree->macros)) {
        cJSON_Delete(root);
        memset(out_tree, 0, sizeof(*out_tree));
        return APP_ERROR_INVALID_ARGUMENT;
    }
    out_tree->root = root;
    return APP_ERROR_NONE;
}
