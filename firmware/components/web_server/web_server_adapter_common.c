#include "web_server_adapter.h"

#include <stdbool.h>
#include <stddef.h>

#include "app_error.h"
#include "web_server_adapter_internal.h"

#define LOW_NIBBLE_MASK 0x0fU

static void writer_append_byte(json_writer_t *writer, char value) {
    if (writer == NULL || writer->failed) {
        return;
    }
    if (writer->length + 1U >= writer->capacity) {
        writer->failed = true;
        return;
    }
    writer->buffer[writer->length++] = value;
    writer->buffer[writer->length] = '\0';
}

void writer_append_text(json_writer_t *writer, const char *text) {
    if (writer == NULL || text == NULL) {
        if (writer != NULL) {
            writer->failed = true;
        }
        return;
    }
    while (*text != '\0') {
        writer_append_byte(writer, *text++);
    }
}

void writer_append_escaped(json_writer_t *writer, const char *text) {
    static const char hexadecimal[] = "0123456789abcdef";
    if (writer == NULL || text == NULL) {
        if (writer != NULL) {
            writer->failed = true;
        }
        return;
    }
    for (const unsigned char *cursor = (const unsigned char *)text; *cursor != '\0'; ++cursor) {
        switch (*cursor) {
        case '"':
            writer_append_text(writer, "\\\"");
            break;
        case '\\':
            writer_append_text(writer, "\\\\");
            break;
        case '\b':
            writer_append_text(writer, "\\b");
            break;
        case '\f':
            writer_append_text(writer, "\\f");
            break;
        case '\n':
            writer_append_text(writer, "\\n");
            break;
        case '\r':
            writer_append_text(writer, "\\r");
            break;
        case '\t':
            writer_append_text(writer, "\\t");
            break;
        default:
            if (*cursor < 0x20U) {
                writer_append_text(writer, "\\u00");
                writer_append_byte(writer, hexadecimal[*cursor >> 4U]);
                writer_append_byte(writer, hexadecimal[*cursor & LOW_NIBBLE_MASK]);
            } else {
                writer_append_byte(writer, (char)*cursor);
            }
            break;
        }
    }
}

app_error_code_t writer_finish(const json_writer_t *writer) {
    return writer != NULL && !writer->failed ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}

bool valid_lifecycle_ops(const web_adapter_lifecycle_ops_t *ops) {
    return ops != NULL && ops->start != NULL && ops->register_route != NULL && ops->stop != NULL;
}
