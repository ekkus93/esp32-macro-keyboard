#include "storage_package_writer.h"

#include <stdlib.h>
#include <string.h>

#include "app_error.h"
#include "cJSON.h"
#include "macro_limits.h"
#include "macro_model.h"
#include "storage_object_json.h"

/* Growth is bounded by APP_IMPORT_PACKAGE_MAX_BYTES: a package that cannot be
 * imported is not worth finishing, and the cap is what keeps a repository that
 * has grown past it from exhausting the heap mid-write. */
static app_error_code_t writer_reserve(package_writer_t *writer, size_t additional) {
    if (writer == NULL || additional > APP_IMPORT_PACKAGE_MAX_BYTES - writer->length) {
        return APP_ERROR_MACRO_LIMIT;
    }
    const size_t required = writer->length + additional + 1U;
    if (required <= writer->capacity) {
        return APP_ERROR_NONE;
    }
    const size_t maximum_capacity = APP_IMPORT_PACKAGE_MAX_BYTES + 1U;
    size_t capacity = writer->capacity == 0U ? 1024U : writer->capacity;
    while (capacity < required) {
        if (capacity > maximum_capacity / 2U) {
            capacity = maximum_capacity;
            break;
        }
        capacity *= 2U;
    }
    if (capacity < required || capacity > maximum_capacity) {
        return APP_ERROR_MACRO_LIMIT;
    }
    char *replacement = realloc(writer->data, capacity);
    if (replacement == NULL) {
        return APP_ERROR_INTERNAL;
    }
    writer->data = replacement;
    writer->capacity = capacity;
    return APP_ERROR_NONE;
}

app_error_code_t package_writer_append_bytes(package_writer_t *writer, const char *data,
                                             size_t length) {
    if (writer == NULL || (data == NULL && length != 0U)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const app_error_code_t result = writer_reserve(writer, length);
    if (result != APP_ERROR_NONE) {
        return result;
    }
    if (length != 0U) {
        memcpy(writer->data + writer->length, data, length);
        writer->length += length;
    }
    writer->data[writer->length] = '\0';
    return APP_ERROR_NONE;
}

app_error_code_t package_writer_append_text(package_writer_t *writer, const char *text) {
    return text == NULL ? APP_ERROR_INVALID_ARGUMENT
                        : package_writer_append_bytes(writer, text, strlen(text));
}

app_error_code_t package_writer_append_serialized(package_writer_t *writer,
                                                  app_error_code_t serialization_result, char *json,
                                                  size_t length) {
    app_error_code_t result = serialization_result;
    if (result == APP_ERROR_NONE) {
        result = package_writer_append_bytes(writer, json, length);
    }
    cJSON_free(json);
    return result;
}

app_error_code_t package_writer_append_metadata(package_writer_t *writer,
                                                const macro_package_t *set) {
    char *json = NULL;
    size_t length = 0U;
    const app_error_code_t result = storage_repository_serialize_package_json(set, &json, &length);
    return package_writer_append_serialized(writer, result, json, length);
}

app_error_code_t package_writer_append_macro(package_writer_t *writer, const macro_t *macro) {
    char *json = NULL;
    size_t length = 0U;
    const app_error_code_t result = storage_repository_serialize_macro_json(macro, &json, &length);
    return package_writer_append_serialized(writer, result, json, length);
}
