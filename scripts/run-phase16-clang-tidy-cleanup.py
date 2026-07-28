#!/usr/bin/env python3
"""Run the Phase 16 cleanup with structural source matchers."""

from pathlib import Path
import runpy

ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "scripts/fix-phase16-clang-tidy.py"
text = SCRIPT.read_text(encoding="utf-8")

old_define_lookup = "    first_define = defines.splitlines()[0]\n"
new_define_lookup = "    first_define = next(line for line in defines.splitlines() if line)\n"
if text.count(old_define_lookup) != 1:
    raise SystemExit("Phase 16 define helper changed unexpectedly")
text = text.replace(old_define_lookup, new_define_lookup, 1)

old_declarations = '''replace_once(
    "firmware/components/web_server/web_api_json.h",
    "app_error_code_t web_api_json_parse_resource_mutation(const char *body, size_t body_length,\\n"
    "                                                       size_t maximum_resource_length,\\n"
    "                                                       web_api_resource_mutation_t *out_mutation);\\n",
    "app_error_code_t web_api_json_parse_resource_mutation(\\n"
    "    const char *body, const web_api_resource_parse_limits_t *limits,\\n"
    "    web_api_resource_mutation_t *out_mutation);\\n",
)
replace_once(
    "firmware/components/web_server/web_api_json.h",
    "app_error_code_t web_api_json_parse_uuid_order(const char *body, size_t body_length,\\n"
    "                                                size_t maximum_count,\\n"
    "                                                storage_uuid_order_t *out_order);\\n",
    "app_error_code_t web_api_json_parse_uuid_order(\\n"
    "    const char *body, const web_api_order_parse_limits_t *limits,\\n"
    "    storage_uuid_order_t *out_order);\\n",
)
'''
new_declarations = '''replace_regex_once(
    "firmware/components/web_server/web_api_json.h",
    r"app_error_code_t web_api_json_parse_resource_mutation\\(.*?out_mutation\\);\\n",
    "app_error_code_t web_api_json_parse_resource_mutation(\\n"
    "    const char *body, const web_api_resource_parse_limits_t *limits,\\n"
    "    web_api_resource_mutation_t *out_mutation);\\n",
)
replace_regex_once(
    "firmware/components/web_server/web_api_json.h",
    r"app_error_code_t web_api_json_parse_uuid_order\\(.*?out_order\\);\\n",
    "app_error_code_t web_api_json_parse_uuid_order(\\n"
    "    const char *body, const web_api_order_parse_limits_t *limits,\\n"
    "    storage_uuid_order_t *out_order);\\n",
)
'''
if text.count(old_declarations) != 1:
    raise SystemExit("Phase 16 cleanup declaration matcher block changed unexpectedly")
text = text.replace(old_declarations, new_declarations, 1)

start = text.find("for relative, resource_limit in (")
end = text.find("# Error response options prevent", start)
if start < 0 or end < 0:
    raise SystemExit("Phase 16 cleanup call-site matcher block changed unexpectedly")
new_calls = '''for relative, resource_limit in (
    ("firmware/components/web_server/web_api_sets.c", "STORAGE_SET_FILE_MAX_BYTES"),
    ("firmware/components/web_server/web_api_macros.c", "STORAGE_MACRO_FILE_MAX_BYTES"),
    ("firmware/components/web_server/web_api_procedures.c", "STORAGE_PROCEDURE_FILE_MAX_BYTES"),
):
    replace_regex_once(
        relative,
        rf"web_api_json_parse_resource_mutation\\(\\s*call->body,\\s*call->body_length,\\s*{resource_limit},\\s*&mutation\\)",
        "web_api_json_parse_resource_mutation(\\n"
        "            call->body,\\n"
        "            &(web_api_resource_parse_limits_t){\\n"
        "                .body_length = call->body_length,\\n"
        f"                .maximum_resource_length = {resource_limit},\\n"
        "            },\\n"
        "            &mutation)",
    )

for relative, maximum_count in (
    ("firmware/components/web_server/web_api_macros.c", "APP_MACROS_PER_SET_MAX"),
    ("firmware/components/web_server/web_api_procedures.c", "APP_PROCEDURES_PER_SET_MAX"),
):
    replace_regex_once(
        relative,
        rf"web_api_json_parse_uuid_order\\(\\s*call->body,\\s*call->body_length,\\s*{maximum_count},\\s*&order\\)",
        "web_api_json_parse_uuid_order(\\n"
        "        call->body,\\n"
        "        &(web_api_order_parse_limits_t){\\n"
        "            .body_length = call->body_length,\\n"
        f"            .maximum_count = {maximum_count},\\n"
        "        },\\n"
        "        &order)",
    )

replace_regex_once(
    "tests/host/test_web_api_json.c",
    r"web_api_json_parse_resource_mutation\\(\\s*update,\\s*sizeof\\(update\\) - 1U,\\s*512U,\\s*&mutation\\)",
    "web_api_json_parse_resource_mutation(\\n"
    "            update,\\n"
    "            &(web_api_resource_parse_limits_t){\\n"
    "                .body_length = sizeof(update) - 1U, .maximum_resource_length = 512U},\\n"
    "            &mutation)",
)
for variable in ("order", "duplicate"):
    replace_regex_once(
        "tests/host/test_web_api_json.c",
        rf"web_api_json_parse_uuid_order\\(\\s*{variable},\\s*sizeof\\({variable}\\) - 1U,\\s*4U,\\s*&parsed\\)",
        "web_api_json_parse_uuid_order(\\n"
        f"            {variable},\\n"
        "            &(web_api_order_parse_limits_t){\\n"
        f"                .body_length = sizeof({variable}) - 1U, .maximum_count = 4U}},\\n"
        "            &parsed)",
    )

'''
text = text[:start] + new_calls + text[end:]

response_test_start = text.find('replace_once(\n    "tests/host/test_web_api_response.c"')
response_test_end = text.find("# Direct include ownership and named constants.", response_test_start)
if response_test_start < 0 or response_test_end < 0:
    raise SystemExit("Phase 16 response-test matcher block changed unexpectedly")
response_test_write = """write(
    "tests/host/test_web_api_response.c",
    r'''#include <stddef.h>
#include <string.h>

#include "app_error.h"
#include "test_assert.h"
#include "web_api_response.h"
#include "web_http_status.h"

static void test_success_envelope(void) {
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        web_api_response_success(&response, WEB_HTTP_STATUS_CREATED, "{\\"id\\":\\"abc\\"}"));
    TEST_CHECK_EQ_U64(WEB_HTTP_STATUS_CREATED, response.status);
    TEST_CHECK(response.body_length == strlen(response.body));
    TEST_CHECK(strstr(response.body, "\\"ok\\":true") != NULL);
    TEST_CHECK(strstr(response.body, "\\"id\\":\\"abc\\"") != NULL);
    web_api_response_free(&response);
    TEST_CHECK(response.body == NULL);
    TEST_CHECK_EQ_U64(0U, response.body_length);
}

static void test_error_envelope(void) {
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        web_api_response_error(
            &response, &(web_api_error_spec_t){
                           .status = WEB_HTTP_STATUS_CONFLICT,
                           .code = APP_ERROR_CONFLICT,
                           .message = "stale revision",
                           .details_json =
                               "{\\"expectedRevision\\":3,\\"actualRevision\\":4}",
                       }));
    TEST_CHECK_EQ_U64(WEB_HTTP_STATUS_CONFLICT, response.status);
    TEST_CHECK(strstr(response.body, "\\"ok\\":false") != NULL);
    TEST_CHECK(strstr(response.body, "\\"code\\":\\"conflict\\"") != NULL);
    TEST_CHECK(strstr(response.body, "\\"actualRevision\\":4") != NULL);
    web_api_response_free(&response);
}

static void test_invalid_payload_rejected(void) {
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INVALID_ARGUMENT,
        web_api_response_success(&response, WEB_HTTP_STATUS_INTERNAL_SERVER_ERROR, "{}"));
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INTERNAL,
        web_api_response_success(&response, WEB_HTTP_STATUS_OK, "not-json"));
    TEST_CHECK_APP_ERROR(
        APP_ERROR_INTERNAL,
        web_api_response_error(
            &response, &(web_api_error_spec_t){
                           .status = WEB_HTTP_STATUS_BAD_REQUEST,
                           .code = APP_ERROR_INVALID_ARGUMENT,
                           .message = "invalid",
                           .details_json = "not-json",
                       }));
}

int main(void) {
    test_success_envelope();
    test_error_envelope();
    test_invalid_payload_rejected();
    return 0;
}
''',
)

"""
text = text[:response_test_start] + response_test_write + text[response_test_end:]

old_length_block = '''# Remove direct ssize_t spelling and split request orchestration into bounded helpers.
text = read("firmware/components/web_server/web_server_api.c")
text = text.replace('#include <sys/types.h>\\n', '')
text = text.replace('app_uuid_t id = {0};', 'app_uuid_t request_id = {0};')
text = text.replace('app_uuid_generate(&id)', 'app_uuid_generate(&request_id)')
text = text.replace('memcpy(output, id.value, sizeof(id.value));',
                    'memcpy(output, request_id.value, sizeof(request_id.value));')
text = text.replace('(ssize_t)response->body_length', 'response->body_length')
'''
new_length_block = '''# Convert the bounded response length explicitly without naming ssize_t.
text = read("firmware/components/web_server/web_server_api.c")
text = text.replace('#include <sys/types.h>\\n', '')
text = text.replace('#include <stdbool.h>\\n', '#include <limits.h>\\n#include <stdbool.h>\\n', 1)
text = text.replace(
    '#include "web_http_status.h"\\n',
    '#include "web_http_status.h"\\n\\n'
    '_Static_assert(WEB_API_RESPONSE_MAX_BYTES <= INT_MAX, '
    '"API response length must fit httpd_resp_send");\\n',
    1,
)
text = text.replace('app_uuid_t id = {0};', 'app_uuid_t request_id = {0};')
text = text.replace('app_uuid_generate(&id)', 'app_uuid_generate(&request_id)')
text = text.replace('memcpy(output, id.value, sizeof(id.value));',
                    'memcpy(output, request_id.value, sizeof(request_id.value));')
text = text.replace('(ssize_t)response->body_length', '(int)response->body_length')
'''
if text.count(old_length_block) != 1:
    raise SystemExit("Phase 16 response-length cleanup block changed unexpectedly")
text = text.replace(old_length_block, new_length_block, 1)

SCRIPT.write_text(text, encoding="utf-8")
runpy.run_path(str(SCRIPT), run_name="__main__")
