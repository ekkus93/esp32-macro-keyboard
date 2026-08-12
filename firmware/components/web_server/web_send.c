#include "web_send.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "app_error.h"
#include "app_uuid.h"
#include "cJSON.h"
#include "macro_executor.h"
#include "macro_parser.h"

#define SEND_NUL_ESCAPE "\\u0000"
#define SEND_CURRENT_ACTION_NONE "none"

static void secure_zero_local(void *memory, size_t length) {
    volatile uint8_t *bytes = memory;
    for (size_t index = 0U; index < length; ++index) {
        bytes[index] = 0U;
    }
}

static bool ops_valid(const web_send_ops_t *ops) {
    return ops != NULL && ops->submit != NULL && ops->get_require_confirmation != NULL &&
           ops->get_status != NULL && ops->cancel != NULL;
}

static bool bounded_body_length(const char *body, size_t capacity, size_t *out_length) {
    if (body == NULL || out_length == NULL || capacity == 0U) {
        return false;
    }
    for (size_t index = 0U; index < capacity; ++index) {
        if (body[index] == '\0') {
            *out_length = index;
            return index > 0U;
        }
    }
    return false;
}

static bool contains_embedded_nul_escape(const char *body, size_t length) {
    const size_t escape_length = sizeof(SEND_NUL_ESCAPE) - 1U;
    if (length < escape_length) {
        return false;
    }
    for (size_t index = 0U; index <= length - escape_length; ++index) {
        if (memcmp(body + index, SEND_NUL_ESCAPE, escape_length) == 0) {
            return true;
        }
    }
    return false;
}

static bool exact_send_fields(const cJSON *root) {
    if (!cJSON_IsObject(root)) {
        return false;
    }
    bool seen_source = false;
    bool seen_key_press = false;
    bool seen_inter_key = false;
    for (const cJSON *item = root->child; item != NULL; item = item->next) {
        if (item->string == NULL) {
            return false;
        }
        if (strcmp(item->string, "source") == 0 && !seen_source) {
            seen_source = true;
        } else if (strcmp(item->string, "keyPressMs") == 0 && !seen_key_press) {
            seen_key_press = true;
        } else if (strcmp(item->string, "interKeyMs") == 0 && !seen_inter_key) {
            seen_inter_key = true;
        } else {
            return false;
        }
    }
    return seen_source && seen_key_press && seen_inter_key;
}

static bool number_to_bounded_ms(const cJSON *item, uint32_t *out_value) {
    if (!cJSON_IsNumber(item) || item->valuedouble < 0.0 ||
        item->valuedouble > (double)UINT32_MAX) {
        return false;
    }
    const uint32_t candidate = (uint32_t)item->valuedouble;
    if ((double)candidate != item->valuedouble) {
        return false;
    }
    *out_value = candidate;
    return true;
}

typedef struct {
    const char *source;
    size_t source_length;
    uint32_t key_press_ms;
    uint32_t inter_key_ms;
} send_request_t;

static bool populate_request(const cJSON *root, send_request_t *out_request) {
    const cJSON *source = cJSON_GetObjectItemCaseSensitive(root, "source");
    const cJSON *key_press = cJSON_GetObjectItemCaseSensitive(root, "keyPressMs");
    const cJSON *inter_key = cJSON_GetObjectItemCaseSensitive(root, "interKeyMs");
    if (!cJSON_IsString(source) || source->valuestring == NULL) {
        return false;
    }
    if (!number_to_bounded_ms(key_press, &out_request->key_press_ms) ||
        !number_to_bounded_ms(inter_key, &out_request->inter_key_ms)) {
        return false;
    }
    out_request->source = source->valuestring;
    out_request->source_length = strlen(source->valuestring);
    return true;
}

static web_send_create_outcome_t outcome_of(web_send_create_result_t result) {
    return (web_send_create_outcome_t){.result = result, .detail = APP_ERROR_NONE};
}

static web_send_create_outcome_t outcome_with_detail(web_send_create_result_t result,
                                                     app_error_code_t detail) {
    return (web_send_create_outcome_t){.result = result, .detail = detail};
}

static web_send_create_outcome_t map_submit_error(app_error_code_t error) {
    switch (error) {
    case APP_ERROR_EXECUTOR_BUSY:
        return outcome_of(WEB_SEND_CREATE_BUSY);
    case APP_ERROR_USB_NOT_READY:
        return outcome_with_detail(WEB_SEND_CREATE_BACKEND_UNAVAILABLE, error);
    case APP_ERROR_INVALID_ARGUMENT:
    default:
        /* macro_compile_v2() has already validated source/action count/
         * estimated duration and (per its doc comment) accepts the complete
         * 0-10000ms range for both timing fields, so the only known way to
         * reach this branch today is macro_executor_engine.c's
         * validate_request() rejecting key_press_ms == 0 -- a v1-shaped
         * executor constraint the v2 compiler does not share. Reconciling
         * that is macro_executor's Phase 6 scope, not this HTTP layer's;
         * reported to the caller as an internal error rather than silently
         * reimplementing the executor's bound here. */
        return outcome_with_detail(WEB_SEND_CREATE_INTERNAL, error);
    }
}

web_send_create_outcome_t web_send_create_handle(char *body, size_t body_length,
                                                 const web_send_ops_t *ops) {
    if (!ops_valid(ops)) {
        if (body != NULL && body_length > 0U) {
            secure_zero_local(body, body_length);
        }
        return outcome_of(WEB_SEND_CREATE_INTERNAL);
    }
    if (body == NULL || body_length == 0U) {
        return outcome_of(WEB_SEND_CREATE_INVALID_BODY);
    }

    size_t exact_length = 0U;
    if (!bounded_body_length(body, body_length, &exact_length) ||
        contains_embedded_nul_escape(body, exact_length)) {
        secure_zero_local(body, body_length);
        return outcome_of(WEB_SEND_CREATE_INVALID_BODY);
    }

    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts(body, exact_length + 1U, &parse_end, true);
    secure_zero_local(body, body_length);
    const bool exact_document = root != NULL && parse_end == body + exact_length;
    if (!exact_document || !exact_send_fields(root)) {
        cJSON_Delete(root);
        return outcome_of(WEB_SEND_CREATE_INVALID_BODY);
    }

    send_request_t request = {0};
    if (!populate_request(root, &request)) {
        cJSON_Delete(root);
        return outcome_of(WEB_SEND_CREATE_INVALID_BODY);
    }

    macro_compile_options_t options = {
        .key_press_ms = request.key_press_ms,
        .inter_key_ms = request.inter_key_ms,
    };
    macro_plan_t plan = {0};
    macro_parse_error_t parse_error = {0};
    const app_error_code_t compile_result =
        macro_compile_v2(request.source, request.source_length, &options, &plan, &parse_error);
    cJSON_Delete(root);
    if (compile_result != APP_ERROR_NONE) {
        web_send_create_outcome_t outcome = outcome_of(WEB_SEND_CREATE_PARSE_ERROR);
        outcome.parse_error = parse_error;
        return outcome;
    }

    bool require_confirmation = false;
    const app_error_code_t policy_result =
        ops->get_require_confirmation(ops->context, &require_confirmation);
    if (policy_result != APP_ERROR_NONE) {
        macro_plan_v2_free(&plan);
        return outcome_with_detail(WEB_SEND_CREATE_BACKEND_UNAVAILABLE, policy_result);
    }

    app_uuid_t send_id = {0};
    if (app_uuid_generate(&send_id) != APP_ERROR_NONE) {
        macro_plan_v2_free(&plan);
        return outcome_of(WEB_SEND_CREATE_INTERNAL);
    }

    macro_execution_request_t execution_request = {
        .execution_id = send_id,
        .set_id = send_id,
        .macro_id = send_id,
        .macro_revision = 1U,
        .key_press_ms = request.key_press_ms,
        .inter_key_ms = request.inter_key_ms,
        .plan = plan,
        .require_confirmation = require_confirmation,
    };
    const uint32_t action_count = (uint32_t)plan.action_count;
    const uint32_t estimated_duration_ms = plan.estimated_duration_ms;

    const app_error_code_t submit_result = ops->submit(ops->context, &execution_request);
    if (submit_result != APP_ERROR_NONE) {
        macro_plan_v2_free(&plan);
        return map_submit_error(submit_result);
    }

    web_send_create_outcome_t outcome = outcome_of(WEB_SEND_CREATE_OK);
    memcpy(outcome.accepted.id, send_id.value, sizeof(outcome.accepted.id));
    outcome.accepted.action_count = action_count;
    outcome.accepted.estimated_duration_ms = estimated_duration_ms;
    outcome.accepted.require_confirmation = require_confirmation;
    return outcome;
}

static const char *send_state_string(execution_state_t state) {
    switch (state) {
    case EXECUTION_AWAITING_CONFIRMATION:
        return "awaiting_confirmation";
    case EXECUTION_RUNNING:
        return "running";
    case EXECUTION_COMPLETED:
        return "completed";
    case EXECUTION_CANCELLED:
        return "cancelled";
    case EXECUTION_FAILED:
        return "failed";
    case EXECUTION_TIMED_OUT:
        return "timed_out";
    case EXECUTION_IDLE:
    default:
        return "";
    }
}

web_send_get_outcome_t web_send_get_handle(const web_send_ops_t *ops,
                                           uint32_t cached_estimated_duration_ms) {
    if (!ops_valid(ops)) {
        return (web_send_get_outcome_t){.result = WEB_SEND_GET_INTERNAL};
    }
    const macro_execution_status_t status = ops->get_status(ops->context);
    if (status.state == EXECUTION_IDLE) {
        return (web_send_get_outcome_t){.result = WEB_SEND_GET_NEVER_SENT};
    }
    /* An unavailable executor caused by an HID release fault still has useful,
     * already-sanitized status to report through the existing releaseError field.
     * Suppressing that status behind a generic 500 would hide the safety fault from
     * the caller. Other unavailable states (for example status publication/locking
     * failure) remain fail-closed because their status cannot be trusted. */
    if (!status.available && status.release_error == APP_ERROR_NONE) {
        return (web_send_get_outcome_t){.result = WEB_SEND_GET_INTERNAL};
    }
    web_send_get_outcome_t outcome = {.result = WEB_SEND_GET_OK};
    memcpy(outcome.status.id, status.execution_id.value, sizeof(outcome.status.id));
    outcome.status.state = send_state_string(status.state);
    outcome.status.action_index = (uint32_t)status.action_index;
    outcome.status.action_count = (uint32_t)status.action_count;
    outcome.status.estimated_duration_ms = cached_estimated_duration_ms;
    outcome.status.cancellation_requested = status.cancellation_requested;
    outcome.status.error =
        status.error == APP_ERROR_NONE ? "" : app_error_code_string(status.error);
    outcome.status.release_error =
        status.release_error == APP_ERROR_NONE ? "" : app_error_code_string(status.release_error);
    return outcome;
}

web_send_cancel_result_t web_send_cancel_handle(const web_send_ops_t *ops) {
    if (!ops_valid(ops)) {
        return WEB_SEND_CANCEL_INTERNAL;
    }
    const macro_execution_status_t status = ops->get_status(ops->context);
    if (status.state == EXECUTION_IDLE) {
        return WEB_SEND_CANCEL_NEVER_SENT;
    }
    const app_error_code_t cancel_result = ops->cancel(ops->context);
    switch (cancel_result) {
    /* APP_ERROR_NONE: freshly recorded cancellation of a non-terminal send.
     * APP_ERROR_CONFLICT: already cancelling a still-non-terminal send --
     * idempotent no-op (SPEC_V2 13.10). APP_ERROR_NOT_FOUND: the send is not
     * currently running (it already reached a terminal state, since
     * state != EXECUTION_IDLE was just confirmed above) -- cancelling
     * something already finished is a harmless no-op, not an error. All
     * three are the same outcome from the caller's point of view. */
    case APP_ERROR_NONE:
    case APP_ERROR_CONFLICT:
    case APP_ERROR_NOT_FOUND:
        return WEB_SEND_CANCEL_ACCEPTED;
    default:
        return WEB_SEND_CANCEL_INTERNAL;
    }
}

app_error_code_t web_send_accepted_json(const web_send_accepted_t *accepted, char **out_json) {
    if (out_json != NULL) {
        *out_json = NULL;
    }
    if (accepted == NULL || out_json == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = cJSON_CreateObject();
    const char *const state = accepted->require_confirmation ? "awaiting_confirmation" : "running";
    if (root == NULL || !cJSON_AddStringToObject(root, "id", accepted->id) ||
        !cJSON_AddStringToObject(root, "state", state) ||
        !cJSON_AddNumberToObject(root, "actionCount", (double)accepted->action_count) ||
        !cJSON_AddNumberToObject(root, "estimatedDurationMs",
                                 (double)accepted->estimated_duration_ms)) {
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (json == NULL) {
        return APP_ERROR_INTERNAL;
    }
    *out_json = json;
    return APP_ERROR_NONE;
}

app_error_code_t web_send_status_json(const web_send_status_view_t *status, char **out_json) {
    if (out_json != NULL) {
        *out_json = NULL;
    }
    if (status == NULL || out_json == NULL || status->state == NULL || status->error == NULL ||
        status->release_error == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = cJSON_CreateObject();
    if (root == NULL || !cJSON_AddStringToObject(root, "id", status->id) ||
        !cJSON_AddStringToObject(root, "state", status->state) ||
        !cJSON_AddNumberToObject(root, "actionIndex", (double)status->action_index) ||
        !cJSON_AddNumberToObject(root, "actionCount", (double)status->action_count) ||
        !cJSON_AddNumberToObject(root, "estimatedDurationMs",
                                 (double)status->estimated_duration_ms) ||
        !cJSON_AddBoolToObject(root, "cancellationRequested", status->cancellation_requested) ||
        !cJSON_AddStringToObject(root, "error", status->error) ||
        !cJSON_AddStringToObject(root, "releaseError", status->release_error)) {
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    char *json = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (json == NULL) {
        return APP_ERROR_INTERNAL;
    }
    *out_json = json;
    return APP_ERROR_NONE;
}
