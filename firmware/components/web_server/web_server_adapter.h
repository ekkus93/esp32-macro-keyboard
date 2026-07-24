#ifndef WEB_SERVER_ADAPTER_H
#define WEB_SERVER_ADAPTER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "macro_limits.h"

#define WEB_ADAPTER_RECEIVE_TIMEOUT (-2)
#define WEB_ADAPTER_TIMEOUT_RETRIES_MAX 4U
#define WEB_ADAPTER_STATIC_CHUNK_BYTES 1024U

typedef int (*web_adapter_receive_fn)(void *context, char *buffer, size_t capacity);
typedef app_error_code_t (*web_adapter_get_header_fn)(void *context,
                                                       const char *name,
                                                       char *buffer,
                                                       size_t buffer_size);
typedef app_error_code_t (*web_adapter_validate_session_fn)(void *context,
                                                             const char *session_token,
                                                             const char *csrf_token);

typedef void *(*web_adapter_open_fn)(void *context, const char *path);
typedef int (*web_adapter_read_fn)(void *context,
                                   void *handle,
                                   uint8_t *buffer,
                                   size_t capacity,
                                   size_t *out_count,
                                   bool *out_eof);
typedef int (*web_adapter_send_chunk_fn)(void *context,
                                         const uint8_t *buffer,
                                         size_t length);
typedef int (*web_adapter_close_fn)(void *context, void *handle);

typedef struct {
    void *context;
    web_adapter_read_fn read;
    web_adapter_send_chunk_fn send_chunk;
    web_adapter_close_fn close;
} web_adapter_stream_ops_t;

typedef struct {
    void *handle;
    bool compressed;
    char path[APP_PATH_MAX_BYTES];
    const char *content_type;
    const char *cache_control;
} web_adapter_static_file_t;

typedef int (*web_adapter_server_start_fn)(void *context, void **out_handle);
typedef int (*web_adapter_route_register_fn)(void *context,
                                              void *handle,
                                              size_t route_index);
typedef int (*web_adapter_server_stop_fn)(void *context, void *handle);

typedef struct {
    void *context;
    web_adapter_server_start_fn start;
    web_adapter_route_register_fn register_route;
    web_adapter_server_stop_fn stop;
} web_adapter_lifecycle_ops_t;

typedef struct {
    void *handle;
    size_t registered_routes;
    app_error_code_t cleanup_error;
} web_adapter_lifecycle_t;

app_error_code_t web_adapter_read_bounded_body(size_t content_length,
                                                char *buffer,
                                                size_t buffer_size,
                                                size_t maximum_length,
                                                web_adapter_receive_fn receive,
                                                void *context);
app_error_code_t web_adapter_authorize_mutation(web_adapter_get_header_fn get_header,
                                                 web_adapter_validate_session_fn validate,
                                                 void *context,
                                                 char *out_session_token,
                                                 size_t token_size);
app_error_code_t web_adapter_build_error_json(app_error_code_t code,
                                               const char *message,
                                               char *output,
                                               size_t output_size);
app_error_code_t web_adapter_build_status_json(const char *version,
                                                const char *idf_version,
                                                const char *usb_state,
                                                const char *wifi_state,
                                                uint32_t wifi_clients,
                                                const char *execution_state,
                                                char *output,
                                                size_t output_size);
app_error_code_t web_adapter_build_limits_json(char *output, size_t output_size);
app_error_code_t web_adapter_open_static_file(const char *uri,
                                               bool accept_gzip,
                                               const char *mount,
                                               web_adapter_open_fn open_file,
                                               void *context,
                                               web_adapter_static_file_t *out_file);
app_error_code_t web_adapter_stream_file(void *handle,
                                         const web_adapter_stream_ops_t *ops);
void web_adapter_lifecycle_reset(web_adapter_lifecycle_t *lifecycle);
app_error_code_t web_adapter_lifecycle_start(web_adapter_lifecycle_t *lifecycle,
                                              const web_adapter_lifecycle_ops_t *ops,
                                              size_t route_count);
app_error_code_t web_adapter_lifecycle_stop(web_adapter_lifecycle_t *lifecycle,
                                             const web_adapter_lifecycle_ops_t *ops);

#endif
