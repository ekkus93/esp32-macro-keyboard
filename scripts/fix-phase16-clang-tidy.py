#!/usr/bin/env python3
"""Fix the complete Phase 16 clang-tidy batch with fail-closed source assertions."""

from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def read(relative: str) -> str:
    return (ROOT / relative).read_text(encoding="utf-8")


def write(relative: str, text: str) -> None:
    (ROOT / relative).write_text(text, encoding="utf-8")


def replace_once(relative: str, old: str, new: str) -> None:
    text = read(relative)
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{relative}: expected one match, found {count}: {old[:80]!r}")
    write(relative, text.replace(old, new, 1))


def replace_regex_once(relative: str, pattern: str, replacement: str) -> None:
    text = read(relative)
    updated, count = re.subn(pattern, replacement, text, count=1, flags=re.DOTALL)
    if count != 1:
        raise SystemExit(f"{relative}: expected one regex match, found {count}: {pattern[:80]!r}")
    write(relative, updated)


def add_include_after(relative: str, anchor: str, include: str) -> None:
    text = read(relative)
    if include in text:
        return
    if text.count(anchor) != 1:
        raise SystemExit(f"{relative}: include anchor mismatch: {anchor!r}")
    write(relative, text.replace(anchor, anchor + include, 1))


def add_defines_after(relative: str, anchor: str, defines: str) -> None:
    text = read(relative)
    first_define = defines.splitlines()[0]
    if first_define in text:
        return
    if text.count(anchor) != 1:
        raise SystemExit(f"{relative}: define anchor mismatch: {anchor!r}")
    write(relative, text.replace(anchor, anchor + defines, 1))


# Strong option objects remove adjacent, easily-swapped size_t parameters.
replace_once(
    "firmware/components/web_server/web_api_json.h",
    "typedef struct {\n"
    "    uint32_t expected_procedure_revision;\n"
    "    app_uuid_t step_id;\n"
    "    bool confirmed;\n"
    "} web_api_progress_action_t;\n",
    "typedef struct {\n"
    "    uint32_t expected_procedure_revision;\n"
    "    app_uuid_t step_id;\n"
    "    bool confirmed;\n"
    "} web_api_progress_action_t;\n\n"
    "typedef struct {\n"
    "    size_t body_length;\n"
    "    size_t maximum_resource_length;\n"
    "} web_api_resource_parse_limits_t;\n\n"
    "typedef struct {\n"
    "    size_t body_length;\n"
    "    size_t maximum_count;\n"
    "} web_api_order_parse_limits_t;\n",
)
replace_once(
    "firmware/components/web_server/web_api_json.h",
    "app_error_code_t web_api_json_parse_resource_mutation(const char *body, size_t body_length,\n"
    "                                                       size_t maximum_resource_length,\n"
    "                                                       web_api_resource_mutation_t *out_mutation);\n",
    "app_error_code_t web_api_json_parse_resource_mutation(\n"
    "    const char *body, const web_api_resource_parse_limits_t *limits,\n"
    "    web_api_resource_mutation_t *out_mutation);\n",
)
replace_once(
    "firmware/components/web_server/web_api_json.h",
    "app_error_code_t web_api_json_parse_uuid_order(const char *body, size_t body_length,\n"
    "                                                size_t maximum_count,\n"
    "                                                storage_uuid_order_t *out_order);\n",
    "app_error_code_t web_api_json_parse_uuid_order(\n"
    "    const char *body, const web_api_order_parse_limits_t *limits,\n"
    "    storage_uuid_order_t *out_order);\n",
)

replace_regex_once(
    "firmware/components/web_server/web_api_json.c",
    r"app_error_code_t web_api_json_parse_resource_mutation\(.*?\n\}\n\nvoid web_api_json_free_resource_mutation",
    """app_error_code_t web_api_json_parse_resource_mutation(
    const char *body, const web_api_resource_parse_limits_t *limits,
    web_api_resource_mutation_t *out_mutation) {
    if (out_mutation != NULL) {
        memset(out_mutation, 0, sizeof(*out_mutation));
    }
    if (out_mutation == NULL || limits == NULL || limits->body_length == 0U ||
        limits->maximum_resource_length == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = parse_exact_document(body, limits->body_length);
    static const char *const fields[] = {"expectedRevision", "resource"};
    if (root == NULL || !exact_fields(root, fields, 2U) ||
        !read_revision(root, "expectedRevision", &out_mutation->expected_revision)) {
        cJSON_Delete(root);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const cJSON *resource = cJSON_GetObjectItemCaseSensitive(root, "resource");
    if (!cJSON_IsObject(resource)) {
        cJSON_Delete(root);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    char *serialized = cJSON_PrintUnformatted(resource);
    cJSON_Delete(root);
    if (serialized == NULL) {
        return APP_ERROR_INTERNAL;
    }
    const size_t length = strlen(serialized);
    if (length == 0U || length > limits->maximum_resource_length) {
        cJSON_free(serialized);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    out_mutation->resource_json = serialized;
    out_mutation->resource_length = length;
    return APP_ERROR_NONE;
}

void web_api_json_free_resource_mutation""",
)
replace_regex_once(
    "firmware/components/web_server/web_api_json.c",
    r"app_error_code_t web_api_json_parse_uuid_order\(.*?\n\}\n\napp_error_code_t web_api_json_parse_execution_submit",
    """app_error_code_t web_api_json_parse_uuid_order(
    const char *body, const web_api_order_parse_limits_t *limits,
    storage_uuid_order_t *out_order) {
    if (out_order != NULL) {
        memset(out_order, 0, sizeof(*out_order));
    }
    if (out_order == NULL || limits == NULL || limits->body_length == 0U ||
        limits->maximum_count == 0U || limits->maximum_count > STORAGE_ORDER_MAX_IDS) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = parse_exact_document(body, limits->body_length);
    static const char *const fields[] = {"ids"};
    const cJSON *ids = root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "ids");
    if (root == NULL || !exact_fields(root, fields, 1U) || !cJSON_IsArray(ids)) {
        cJSON_Delete(root);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    const int count = cJSON_GetArraySize(ids);
    if (count < 0 || (size_t)count > limits->maximum_count) {
        cJSON_Delete(root);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    for (int index = 0; index < count; ++index) {
        const cJSON *item = cJSON_GetArrayItem(ids, index);
        if (!cJSON_IsString(item) || item->valuestring == NULL ||
            app_uuid_parse(item->valuestring, &out_order->ids[(size_t)index]) != APP_ERROR_NONE) {
            memset(out_order, 0, sizeof(*out_order));
            cJSON_Delete(root);
            return APP_ERROR_INVALID_ARGUMENT;
        }
        for (int prior = 0; prior < index; ++prior) {
            if (app_uuid_equal(&out_order->ids[(size_t)prior], &out_order->ids[(size_t)index])) {
                memset(out_order, 0, sizeof(*out_order));
                cJSON_Delete(root);
                return APP_ERROR_INVALID_ARGUMENT;
            }
        }
    }
    out_order->count = (size_t)count;
    cJSON_Delete(root);
    return APP_ERROR_NONE;
}

app_error_code_t web_api_json_parse_execution_submit""",
)

for relative, resource_limit in (
    ("firmware/components/web_server/web_api_sets.c", "STORAGE_SET_FILE_MAX_BYTES"),
    ("firmware/components/web_server/web_api_macros.c", "STORAGE_MACRO_FILE_MAX_BYTES"),
    ("firmware/components/web_server/web_api_procedures.c", "STORAGE_PROCEDURE_FILE_MAX_BYTES"),
):
    replace_once(
        relative,
        "web_api_json_parse_resource_mutation(\n"
        f"            call->body, call->body_length, {resource_limit}, &mutation)",
        "web_api_json_parse_resource_mutation(\n"
        "            call->body,\n"
        "            &(web_api_resource_parse_limits_t){\n"
        "                .body_length = call->body_length,\n"
        f"                .maximum_resource_length = {resource_limit},\n"
        "            },\n"
        "            &mutation)",
    )

replace_once(
    "firmware/components/web_server/web_api_macros.c",
    "web_api_json_parse_uuid_order(call->body, call->body_length,\n"
    "                                                             APP_MACROS_PER_SET_MAX, &order)",
    "web_api_json_parse_uuid_order(\n"
    "        call->body,\n"
    "        &(web_api_order_parse_limits_t){\n"
    "            .body_length = call->body_length,\n"
    "            .maximum_count = APP_MACROS_PER_SET_MAX,\n"
    "        },\n"
    "        &order)",
)
replace_once(
    "firmware/components/web_server/web_api_procedures.c",
    "web_api_json_parse_uuid_order(call->body, call->body_length,\n"
    "                                                             APP_PROCEDURES_PER_SET_MAX, &order)",
    "web_api_json_parse_uuid_order(\n"
    "        call->body,\n"
    "        &(web_api_order_parse_limits_t){\n"
    "            .body_length = call->body_length,\n"
    "            .maximum_count = APP_PROCEDURES_PER_SET_MAX,\n"
    "        },\n"
    "        &order)",
)
replace_once(
    "tests/host/test_web_api_json.c",
    "web_api_json_parse_resource_mutation(\n"
    "                                             update, sizeof(update) - 1U, 512U, &mutation)",
    "web_api_json_parse_resource_mutation(\n"
    "            update,\n"
    "            &(web_api_resource_parse_limits_t){\n"
    "                .body_length = sizeof(update) - 1U, .maximum_resource_length = 512U},\n"
    "            &mutation)",
)
replace_once(
    "tests/host/test_web_api_json.c",
    "web_api_json_parse_uuid_order(order, sizeof(order) - 1U, 4U, &parsed)",
    "web_api_json_parse_uuid_order(\n"
    "                              order,\n"
    "                              &(web_api_order_parse_limits_t){\n"
    "                                  .body_length = sizeof(order) - 1U, .maximum_count = 4U},\n"
    "                              &parsed)",
)
replace_once(
    "tests/host/test_web_api_json.c",
    "web_api_json_parse_uuid_order(duplicate, sizeof(duplicate) - 1U, 4U, &parsed)",
    "web_api_json_parse_uuid_order(\n"
    "            duplicate,\n"
    "            &(web_api_order_parse_limits_t){\n"
    "                .body_length = sizeof(duplicate) - 1U, .maximum_count = 4U},\n"
    "            &parsed)",
)

# Error response options prevent four easily-swapped scalar/string arguments.
write(
    "firmware/components/web_server/web_api_response.h",
    """#ifndef WEB_API_RESPONSE_H
#define WEB_API_RESPONSE_H

#include <stddef.h>

#include "app_error.h"

#define WEB_API_RESPONSE_MAX_BYTES (512U * 1024U)

typedef struct {
    unsigned int status;
    char *body;
    size_t body_length;
} web_api_response_t;

typedef struct {
    unsigned int status;
    app_error_code_t code;
    const char *message;
    const char *details_json;
} web_api_error_spec_t;

app_error_code_t web_api_response_success(web_api_response_t *response, unsigned int status,
                                          const char *data_json);
app_error_code_t web_api_response_error(web_api_response_t *response,
                                        const web_api_error_spec_t *error);
void web_api_response_free(web_api_response_t *response);

#endif
""",
)
write(
    "firmware/components/web_server/web_api_response.c",
    """#include "web_api_response.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "app_error.h"
#include "cJSON.h"
#include "web_http_status.h"

#define WEB_HTTP_SUCCESS_STATUS_UPPER_BOUND 300U
#define WEB_HTTP_ERROR_STATUS_UPPER_BOUND 599U

static app_error_code_t set_serialized(web_api_response_t *response, unsigned int status,
                                       cJSON *root) {
    char *serialized = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (serialized == NULL) {
        return APP_ERROR_INTERNAL;
    }
    const size_t length = strlen(serialized);
    if (length == 0U || length > WEB_API_RESPONSE_MAX_BYTES) {
        cJSON_free(serialized);
        return APP_ERROR_INTERNAL;
    }
    response->status = status;
    response->body = serialized;
    response->body_length = length;
    return APP_ERROR_NONE;
}

static cJSON *parse_data(const char *json) {
    if (json == NULL) {
        return NULL;
    }
    const size_t length = strlen(json);
    const char *parse_end = NULL;
    cJSON *value = cJSON_ParseWithLengthOpts(json, length, &parse_end, false);
    if (value == NULL || parse_end != json + length) {
        cJSON_Delete(value);
        return NULL;
    }
    return value;
}

app_error_code_t web_api_response_success(web_api_response_t *response, unsigned int status,
                                          const char *data_json) {
    if (response != NULL) {
        memset(response, 0, sizeof(*response));
    }
    if (response == NULL || data_json == NULL || status < WEB_HTTP_STATUS_OK ||
        status >= WEB_HTTP_SUCCESS_STATUS_UPPER_BOUND) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *data = parse_data(data_json);
    cJSON *root = cJSON_CreateObject();
    if (data == NULL || root == NULL || !cJSON_AddBoolToObject(root, "ok", true) ||
        !cJSON_AddItemToObject(root, "data", data)) {
        cJSON_Delete(data);
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    return set_serialized(response, status, root);
}

app_error_code_t web_api_response_error(web_api_response_t *response,
                                        const web_api_error_spec_t *error_spec) {
    if (response != NULL) {
        memset(response, 0, sizeof(*response));
    }
    if (response == NULL || error_spec == NULL || error_spec->message == NULL ||
        error_spec->status < WEB_HTTP_STATUS_BAD_REQUEST ||
        error_spec->status > WEB_HTTP_ERROR_STATUS_UPPER_BOUND) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = cJSON_CreateObject();
    cJSON *error = cJSON_CreateObject();
    if (root == NULL || error == NULL || !cJSON_AddBoolToObject(root, "ok", false) ||
        !cJSON_AddStringToObject(error, "code", app_error_code_string(error_spec->code)) ||
        !cJSON_AddStringToObject(error, "message", error_spec->message) ||
        !cJSON_AddItemToObject(root, "error", error)) {
        cJSON_Delete(error);
        cJSON_Delete(root);
        return APP_ERROR_INTERNAL;
    }
    if (error_spec->details_json != NULL) {
        cJSON *details = parse_data(error_spec->details_json);
        if (details == NULL || !cJSON_AddItemToObject(error, "details", details)) {
            cJSON_Delete(details);
            cJSON_Delete(root);
            return APP_ERROR_INTERNAL;
        }
    }
    return set_serialized(response, error_spec->status, root);
}

void web_api_response_free(web_api_response_t *response) {
    if (response == NULL) {
        return;
    }
    cJSON_free(response->body);
    memset(response, 0, sizeof(*response));
}
""",
)
replace_once(
    "firmware/components/web_server/web_api_handler_common.c",
    "    return web_api_response_error(response, web_api_http_status_for_error(error), error, message,\n"
    "                                  details_json);\n",
    "    return web_api_response_error(\n"
    "        response, &(web_api_error_spec_t){\n"
    "                      .status = web_api_http_status_for_error(error),\n"
    "                      .code = error,\n"
    "                      .message = message,\n"
    "                      .details_json = details_json,\n"
    "                  });\n",
)
replace_once(
    "firmware/components/web_server/web_api_execution.c",
    "    return web_api_response_error(response, status, error, message, NULL);\n",
    "    return web_api_response_error(\n"
    "        response, &(web_api_error_spec_t){\n"
    "                      .status = status,\n"
    "                      .code = error,\n"
    "                      .message = message,\n"
    "                  });\n",
)
replace_once(
    "firmware/components/web_server/web_server_api.c",
    "    return web_api_response_error(response, status, error, message, NULL);\n",
    "    return web_api_response_error(\n"
    "        response, &(web_api_error_spec_t){\n"
    "                      .status = status,\n"
    "                      .code = error,\n"
    "                      .message = message,\n"
    "                  });\n",
)
replace_once(
    "tests/host/test_web_api_response.c",
    "        web_api_response_error(&response, 409U, APP_ERROR_CONFLICT, \"stale revision\",\n"
    "                               \"{\\\"expectedRevision\\\":3,\\\"actualRevision\\\":4}\"));\n",
    "        web_api_response_error(\n"
    "            &response, &(web_api_error_spec_t){\n"
    "                           .status = 409U,\n"
    "                           .code = APP_ERROR_CONFLICT,\n"
    "                           .message = \"stale revision\",\n"
    "                           .details_json =\n"
    "                               \"{\\\"expectedRevision\\\":3,\\\"actualRevision\\\":4}\",\n"
    "                       }));\n",
)
replace_once(
    "tests/host/test_web_api_response.c",
    "                          web_api_response_error(&response, 400U, APP_ERROR_INVALID_ARGUMENT,\n"
    "                                                 \"invalid\", \"not-json\"));\n",
    "                          web_api_response_error(\n"
    "                              &response, &(web_api_error_spec_t){\n"
    "                                             .status = 400U,\n"
    "                                             .code = APP_ERROR_INVALID_ARGUMENT,\n"
    "                                             .message = \"invalid\",\n"
    "                                             .details_json = \"not-json\",\n"
    "                                         }));\n",
)

# Direct include ownership and named constants.
for relative in (
    "firmware/components/web_server/web_api_macros.c",
    "firmware/components/web_server/web_api_procedures.c",
    "firmware/components/web_server/web_api_administration.c",
):
    add_include_after(relative, '#include "storage_repository.h"\n', '#include "web_api_core.h"\n')
    add_include_after(relative, '#include "web_api_response.h"\n', '#include "web_http_status.h"\n')
add_include_after(
    "firmware/components/web_server/web_api_administration.c",
    '#include "macro_limits.h"\n',
    '#include "macro_model.h"\n',
)
add_include_after(
    "firmware/components/web_server/web_api_sets.c",
    '#include "web_api_response.h"\n',
    '#include "web_http_status.h"\n',
)
add_include_after(
    "firmware/components/web_server/web_api_dispatch.c", "#include <stdbool.h>\n", "#include <stddef.h>\n"
)

add_defines_after(
    "firmware/components/web_server/web_api_macros.c",
    '#include "web_http_status.h"\n',
    "\n#define WEB_MACRO_DELETE_RESPONSE_BYTES 80U\n"
    "#define WEB_MACRO_VALIDATION_DETAILS_BYTES 192U\n"
    "#define WEB_MACRO_VALIDATION_RESPONSE_BYTES 160U\n",
)
add_defines_after(
    "firmware/components/web_server/web_api_procedures.c",
    '#include "web_http_status.h"\n',
    "\n#define WEB_PROCEDURE_DELETE_RESPONSE_BYTES 80U\n",
)
add_defines_after(
    "firmware/components/web_server/web_api_sets.c",
    '#include "web_http_status.h"\n',
    "\n#define WEB_SET_DELETE_RESPONSE_BYTES 80U\n",
)
add_defines_after(
    "firmware/components/web_server/web_api_execution.c",
    '#include "web_http_status.h"\n',
    "\n#define WEB_EXECUTION_STATUS_RESPONSE_BYTES 768U\n"
    "#define WEB_EXECUTION_DETAILS_RESPONSE_BYTES 192U\n",
)
add_defines_after(
    "firmware/components/web_server/web_api_administration.c",
    "#define WEB_PASSWORD_CHANGE_BODY_MAX_BYTES 512U\n",
    "#define WEB_PASSWORD_CHANGE_RESPONSE_BYTES 80U\n",
)

for relative in (
    "firmware/components/web_server/web_api_macros.c",
    "firmware/components/web_server/web_api_procedures.c",
    "firmware/components/web_server/web_api_sets.c",
):
    text = read(relative).replace(", 200U,", ", WEB_HTTP_STATUS_OK,")
    text = text.replace(", 201U,", ", WEB_HTTP_STATUS_CREATED,")
    write(relative, text)

replacements = {
    "firmware/components/web_server/web_api_macros.c": {
        "char data[80U];": "char data[WEB_MACRO_DELETE_RESPONSE_BYTES];",
        "char details[192U];": "char details[WEB_MACRO_VALIDATION_DETAILS_BYTES];",
        "char data[160U];": "char data[WEB_MACRO_VALIDATION_RESPONSE_BYTES];",
        "cJSON *id = cJSON_CreateString(references->ids[index].value);":
            "cJSON *identifier = cJSON_CreateString(references->ids[index].value);",
        "if (id == NULL || !cJSON_AddItemToArray(ids, id)) {":
            "if (identifier == NULL || !cJSON_AddItemToArray(ids, identifier)) {",
        "cJSON_Delete(id);": "cJSON_Delete(identifier);",
        "strlen(item->valuestring) <= APP_MACRO_NAME_MAX_BYTES) {":
            "strlen(item->valuestring) <= APP_MACRO_NAME_MAX_BYTES &&\n"
            "                   strlen(item->valuestring) < out_name_size) {",
    },
    "firmware/components/web_server/web_api_procedures.c": {
        "char data[80U];": "char data[WEB_PROCEDURE_DELETE_RESPONSE_BYTES];",
    },
    "firmware/components/web_server/web_api_sets.c": {
        "char data[80U];": "char data[WEB_SET_DELETE_RESPONSE_BYTES];",
    },
    "firmware/components/web_server/web_api_execution.c": {
        "char data[768U];": "char data[WEB_EXECUTION_STATUS_RESPONSE_BYTES];",
        "char details[192U];": "char details[WEB_EXECUTION_DETAILS_RESPONSE_BYTES];",
        "char data[192U];": "char data[WEB_EXECUTION_DETAILS_RESPONSE_BYTES];",
    },
    "firmware/components/web_server/web_api_administration.c": {
        "char data[80U];": "char data[WEB_PASSWORD_CHANGE_RESPONSE_BYTES];",
    },
}
for relative, mapping in replacements.items():
    text = read(relative)
    for old, new in mapping.items():
        if old not in text:
            raise SystemExit(f"{relative}: missing replacement source {old!r}")
        text = text.replace(old, new)
    write(relative, text)

# Smaller progress helpers keep each unit below the cognitive-complexity limit.
replace_once(
    "firmware/components/web_server/web_api_procedures.c",
    "static bool order_contains(const app_uuid_t *items, size_t count, const app_uuid_t *id) {\n"
    "    for (size_t index = 0U; index < count; ++index) {\n"
    "        if (app_uuid_equal(&items[index], id)) {\n",
    "static bool order_contains(const app_uuid_t *items, size_t count, const app_uuid_t *step_id) {\n"
    "    for (size_t index = 0U; index < count; ++index) {\n"
    "        if (app_uuid_equal(&items[index], step_id)) {\n",
)
replace_once(
    "firmware/components/web_server/web_api_procedures.c",
    "static void order_remove(app_uuid_t *items, size_t *count, const app_uuid_t *id) {\n"
    "    for (size_t index = 0U; index < *count; ++index) {\n"
    "        if (app_uuid_equal(&items[index], id)) {\n",
    "static void order_remove(app_uuid_t *items, size_t *count, const app_uuid_t *step_id) {\n"
    "    for (size_t index = 0U; index < *count; ++index) {\n"
    "        if (app_uuid_equal(&items[index], step_id)) {\n",
)
replace_regex_once(
    "firmware/components/web_server/web_api_procedures.c",
    r"static app_error_code_t handle_progress_action\(.*?\n\}\n\napp_error_code_t web_api_handle_procedures",
    """static app_error_code_t load_progress_action_context(
    const web_api_call_t *call, bool skipped, web_api_progress_action_t *out_action,
    storage_procedure_identity_t *out_identity, storage_progress_snapshot_t *out_current,
    procedure_t *out_procedure, size_t *out_step_index) {
    app_error_code_t result =
        web_api_json_parse_progress_action(call->body, call->body_length, skipped, out_action);
    *out_identity = progress_identity(call);
    if (result == APP_ERROR_NONE) {
        result = storage_progress_read(out_identity, out_current);
    }
    if (result == APP_ERROR_NONE &&
        (out_current->status != STORAGE_PROGRESS_STATUS_CURRENT ||
         out_current->current_procedure_revision != out_action->expected_procedure_revision)) {
        result = APP_ERROR_CONFLICT;
    }
    if (result == APP_ERROR_NONE) {
        result = storage_procedure_read(&out_identity->set_id, &out_identity->procedure_id,
                                        out_procedure);
    }
    if (result == APP_ERROR_NONE &&
        !procedure_has_step(out_procedure, &out_action->step_id, out_step_index)) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    return result;
}

static app_error_code_t apply_progress_action(procedure_progress_t *replacement,
                                              const procedure_t *procedure,
                                              const web_api_progress_action_t *action,
                                              size_t step_index, bool skipped) {
    app_uuid_t *target =
        skipped ? replacement->skipped_step_ids : replacement->completed_step_ids;
    size_t *target_count =
        skipped ? &replacement->skipped_step_count : &replacement->completed_step_count;
    app_uuid_t *opposite =
        skipped ? replacement->completed_step_ids : replacement->skipped_step_ids;
    size_t *opposite_count =
        skipped ? &replacement->completed_step_count : &replacement->skipped_step_count;
    order_remove(opposite, opposite_count, &action->step_id);
    if (!order_contains(target, *target_count, &action->step_id)) {
        if (*target_count >= APP_STEPS_PER_PROCEDURE_MAX) {
            return APP_ERROR_MACRO_LIMIT;
        }
        target[*target_count] = action->step_id;
        ++(*target_count);
    }
    if (step_index + 1U < procedure->step_count) {
        replacement->current_step_id = procedure->steps[step_index + 1U].id;
    }
    return APP_ERROR_NONE;
}

static app_error_code_t handle_progress_action(const web_api_call_t *call,
                                               web_api_response_t *response, bool skipped) {
    web_api_progress_action_t action = {0};
    storage_procedure_identity_t identity = {0};
    storage_progress_snapshot_t current = {0};
    procedure_t procedure = {0};
    size_t step_index = 0U;
    app_error_code_t result = load_progress_action_context(
        call, skipped, &action, &identity, &current, &procedure, &step_index);
    procedure_progress_t replacement = current.progress;
    if (result == APP_ERROR_NONE) {
        result = apply_progress_action(&replacement, &procedure, &action, step_index, skipped);
    }
    storage_progress_snapshot_t committed = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_progress_update(&identity, &replacement, &committed);
    }
    macro_model_free_procedure(&procedure);
    return result == APP_ERROR_NONE ? send_progress(response, &committed)
                                    : respond_error(response, result,
                                                    skipped ? "could not skip procedure step"
                                                            : "could not complete procedure step");
}

app_error_code_t web_api_handle_procedures""",
)

# Remove direct ssize_t spelling and split request orchestration into bounded helpers.
text = read("firmware/components/web_server/web_server_api.c")
text = text.replace('#include <sys/types.h>\n', '')
text = text.replace('app_uuid_t id = {0};', 'app_uuid_t request_id = {0};')
text = text.replace('app_uuid_generate(&id)', 'app_uuid_generate(&request_id)')
text = text.replace('memcpy(output, id.value, sizeof(id.value));',
                    'memcpy(output, request_id.value, sizeof(request_id.value));')
text = text.replace('(ssize_t)response->body_length', 'response->body_length')
text = text.replace('.content_length = (size_t)request->content_len,',
                    '.content_length = request->content_len,')
text = text.replace('const size_t content_length = (size_t)request->content_len;',
                    'const size_t content_length = request->content_len;')
write("firmware/components/web_server/web_server_api.c", text)
replace_regex_once(
    "firmware/components/web_server/web_server_api.c",
    r"esp_err_t api_handler\(httpd_req_t \*request\) \{.*?\n\}\n$",
    """static app_error_code_t prepare_api_call(httpd_req_t *request, web_api_call_t *call,
                                             web_api_response_t *response,
                                             size_t *out_body_limit, bool *out_response_ready) {
    app_error_code_t result = method_from_request(request, &call->method);
    if (result == APP_ERROR_NONE) {
        result = web_api_parse_path(request->uri, &call->path);
    }
    if (result != APP_ERROR_NONE) {
        const unsigned int status = result == APP_ERROR_NOT_FOUND ? WEB_HTTP_STATUS_NOT_FOUND
                                                                  : WEB_HTTP_STATUS_BAD_REQUEST;
        result = set_error_response(response, status, result,
                                    status == WEB_HTTP_STATUS_NOT_FOUND ? "route not found"
                                                                         : "invalid API path");
        *out_response_ready = result == APP_ERROR_NONE;
        return result;
    }
    if (!web_api_route_allows_method(call->path.route, call->method)) {
        result = set_error_response(response, WEB_HTTP_STATUS_METHOD_NOT_ALLOWED,
                                    APP_ERROR_INVALID_ARGUMENT, "method not allowed");
        *out_response_ready = result == APP_ERROR_NONE;
        return result;
    }
    *out_body_limit = route_body_limit(call->path.route);
    if (!web_api_route_requires_body(call->path.route, call->method) && request->content_len != 0U) {
        result = set_error_response(response, WEB_HTTP_STATUS_UNPROCESSABLE_ENTITY,
                                    APP_ERROR_INVALID_ARGUMENT,
                                    "request body is not allowed for this route");
        *out_response_ready = result == APP_ERROR_NONE;
    }
    return result;
}

static app_error_code_t authorize_and_read_api_call(
    httpd_req_t *request, web_api_call_t *call, size_t body_limit,
    web_request_policy_result_t *policy, web_api_response_t *response, char **out_body,
    bool *out_response_ready) {
    web_request_policy_failure_t failure = WEB_REQUEST_POLICY_FAILURE_NONE;
    app_error_code_t result =
        apply_request_policy(request, call, body_limit, policy, &failure);
    if (result != APP_ERROR_NONE) {
        result = set_error_response(response, web_request_policy_http_status(failure, result), result,
                                    policy_failure_message(failure));
        *out_response_ready = result == APP_ERROR_NONE;
        return result;
    }
    result = read_call_body(request, body_limit, out_body, &call->body_length);
    call->body = *out_body == NULL ? "" : *out_body;
    if (result != APP_ERROR_NONE) {
        result = set_error_response(response, web_api_http_status_for_error(result), result,
                                    "could not read request body");
        *out_response_ready = result == APP_ERROR_NONE;
    }
    return result;
}

static app_error_code_t dispatch_api_call(const web_api_call_t *call,
                                          web_api_response_t *response,
                                          bool *out_response_ready) {
    app_error_code_t result = web_api_dispatch(call, response);
    if (result != APP_ERROR_NONE && response->body == NULL) {
        result = set_error_response(response, web_api_http_status_for_error(result), result,
                                    "API operation failed");
    }
    *out_response_ready = result == APP_ERROR_NONE && response->body != NULL;
    return result;
}

esp_err_t api_handler(httpd_req_t *request) {
    web_api_response_t response = {0};
    web_request_policy_result_t policy = {0};
    web_api_call_t call = {0};
    char *body = NULL;
    size_t body_limit = WEB_API_SMALL_BODY_MAX_BYTES;
    bool response_ready = false;

    app_error_code_t result =
        prepare_api_call(request, &call, &response, &body_limit, &response_ready);
    if (!response_ready && result == APP_ERROR_NONE) {
        result = authorize_and_read_api_call(request, &call, body_limit, &policy, &response, &body,
                                             &response_ready);
    }
    if (!response_ready && result == APP_ERROR_NONE) {
        result = dispatch_api_call(&call, &response, &response_ready);
    }
    if (!response_ready) {
        web_api_response_free(&response);
        (void)set_error_response(&response, WEB_HTTP_STATUS_INTERNAL_SERVER_ERROR,
                                 APP_ERROR_INTERNAL, "response encoding failed");
    }

    const bool should_restart = response.body != NULL && restart_after_response(&call, &response);
    const esp_err_t send_result = send_api_response(request, policy.request_id, &response);
    free(body);
    web_api_response_free(&response);
    if (send_result == ESP_OK && should_restart) {
        esp_restart();
    }
    return send_result;
}
""",
)

print("Phase 16 clang-tidy cleanup applied")
