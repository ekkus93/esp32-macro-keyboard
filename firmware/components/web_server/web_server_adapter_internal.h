#ifndef WEB_SERVER_ADAPTER_INTERNAL_H
#define WEB_SERVER_ADAPTER_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>

#include "app_error.h"
#include "web_server_adapter.h"

/* Component-private declarations shared across the web_server_adapter translation
 * units. These were file-scope statics in the former source amalgamation; the
 * split gives them external linkage, kept private to the component via this
 * internal header. */

typedef struct {
    char *buffer;
    size_t capacity;
    size_t length;
    bool failed;
} json_writer_t;

void writer_append_text(json_writer_t *writer, const char *text);
void writer_append_escaped(json_writer_t *writer, const char *text);
app_error_code_t writer_finish(const json_writer_t *writer);
bool valid_lifecycle_ops(const web_adapter_lifecycle_ops_t *ops);

#endif
