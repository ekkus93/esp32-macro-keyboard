#ifndef WEB_SEND_H
#define WEB_SEND_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_error.h"
#include "macro_executor.h"
#include "macro_limits.h"
#include "macro_parser.h"

/* Business logic for POST/GET/DELETE /api/v1/send (SPEC_V2 13.10). No
 * esp_http_server dependency, so this module is host-testable; the httpd
 * adapter (web_server_send.c) supplies `ops`, bridging to the real
 * macro_executor, and owns the small estimatedDurationMs cache described
 * below (macro_execution_status_t, unlike the accepted response, does not
 * carry it). */
typedef struct {
    void *context;
    /* Bridges to macro_executor_submit(). On APP_ERROR_NONE, ownership of
     * request->plan's heap allocation has transferred to the executor; on
     * any other return, the caller (web_send_create_handle) still owns it
     * and frees it with macro_plan_v2_free(). */
    app_error_code_t (*submit)(void *context, macro_execution_request_t *request);
    macro_execution_status_t (*get_status)(void *context);
    /* Bridges to macro_executor_cancel(). */
    app_error_code_t (*cancel)(void *context);
} web_send_ops_t;

typedef enum {
    WEB_SEND_CREATE_OK = 0,
    WEB_SEND_CREATE_INVALID_BODY,
    WEB_SEND_CREATE_PARSE_ERROR,
    WEB_SEND_CREATE_BUSY,
    WEB_SEND_CREATE_BACKEND_UNAVAILABLE,
    WEB_SEND_CREATE_INTERNAL,
} web_send_create_result_t;

typedef struct {
    char id[APP_UUID_BUFFER_LENGTH];
    uint32_t action_count;
    uint32_t estimated_duration_ms;
} web_send_accepted_t;

typedef struct {
    web_send_create_result_t result;
    /* Underlying backend error for BACKEND_UNAVAILABLE/INTERNAL; APP_ERROR_NONE
     * otherwise. */
    app_error_code_t detail;
    /* Valid iff result == WEB_SEND_CREATE_PARSE_ERROR. */
    macro_parse_error_t parse_error;
    /* Valid iff result == WEB_SEND_CREATE_OK. */
    web_send_accepted_t accepted;
} web_send_create_outcome_t;

/* Parses `body` (NUL-terminated, real length `body_length`) as the exact
 * {"source","keyPressMs","interKeyMs"} schema -- rejecting unknown, missing,
 * duplicate, and wrong-type fields, trailing content, and malformed JSON --
 * compiles the complete source with macro_compile_v2() before accepting
 * anything, and only on a successful compile calls ops->submit(). `body` is
 * wiped (every byte set to 0) before this function returns on every path. */
web_send_create_outcome_t web_send_create_handle(char *body, size_t body_length,
                                                 const web_send_ops_t *ops);

typedef enum {
    WEB_SEND_GET_OK = 0,
    WEB_SEND_GET_NEVER_SENT,
    WEB_SEND_GET_INTERNAL,
} web_send_get_result_t;

typedef struct {
    char id[APP_UUID_BUFFER_LENGTH];
    const char *state;
    uint32_t action_index;
    uint32_t action_count;
    uint32_t estimated_duration_ms;
    bool cancellation_requested;
    const char *error;         /* "" when none */
    const char *release_error; /* "" when none */
} web_send_status_view_t;

typedef struct {
    web_send_get_result_t result;
    web_send_status_view_t status;
} web_send_get_outcome_t;

/* `cached_estimated_duration_ms` is the value returned in the accepted
 * response for the send currently reported by ops->get_status() (0 if no
 * send has ever been accepted); see the module comment above. */
web_send_get_outcome_t web_send_get_handle(const web_send_ops_t *ops,
                                           uint32_t cached_estimated_duration_ms);

typedef enum {
    /* 202: includes both a freshly-requested cancellation and the idempotent
     * no-op cases (already cancelling, or the send already reached a
     * terminal state) SPEC_V2 13.10 requires. */
    WEB_SEND_CANCEL_ACCEPTED = 0,
    WEB_SEND_CANCEL_NEVER_SENT, /* 404: no send has existed since boot */
    WEB_SEND_CANCEL_INTERNAL,
} web_send_cancel_result_t;

web_send_cancel_result_t web_send_cancel_handle(const web_send_ops_t *ops);

/* Builds the exact SPEC_V2 13.10 accepted-response JSON:
 * {"id","state":"running","actionCount","estimatedDurationMs"}. *out_json is
 * heap-allocated (cJSON_free()/web_api_handler_json_free()). */
app_error_code_t web_send_accepted_json(const web_send_accepted_t *accepted, char **out_json);
/* Builds the exact SPEC_V2 13.10 status-response JSON. */
app_error_code_t web_send_status_json(const web_send_status_view_t *status, char **out_json);

#endif
