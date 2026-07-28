#!/usr/bin/env python3
"""Apply the Phase 16 request-JSON boundary fix with fail-closed assertions."""

from __future__ import annotations

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
        raise SystemExit(f"{relative}: expected one match, found {count}: {old[:120]!r}")
    write(relative, text.replace(old, new, 1))


def replace_exact_count(relative: str, old: str, new: str, expected: int) -> None:
    text = read(relative)
    count = text.count(old)
    if count != expected:
        raise SystemExit(
            f"{relative}: expected {expected} matches, found {count}: {old[:120]!r}"
        )
    write(relative, text.replace(old, new))


# The HTTP parser must be able to validate the largest resource object (a set has
# ten fields), reject duplicate/unknown fields, and translate persisted-data
# corruption codes into request-domain invalid-argument results.
replace_once(
    "firmware/components/web_server/web_api_json.c",
    '#include "web_execution_submit.h"\n',
    '#include "web_execution_submit.h"\n\n#define WEB_API_JSON_MAX_FIELDS 16U\n',
)
replace_once(
    "firmware/components/web_server/web_api_json.c",
    "    if (!cJSON_IsObject(root) || fields == NULL || field_count == 0U || field_count > 8U) {\n",
    "    if (!cJSON_IsObject(root) || fields == NULL || field_count == 0U ||\n"
    "        field_count > WEB_API_JSON_MAX_FIELDS) {\n",
)
replace_once(
    "firmware/components/web_server/web_api_json.c",
    "    bool seen[8U] = {false};\n",
    "    bool seen[WEB_API_JSON_MAX_FIELDS] = {false};\n",
)

read_uuid_function = '''static bool read_uuid(const cJSON *root, const char *field, app_uuid_t *out_uuid) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, field);
    return cJSON_IsString(item) && item->valuestring != NULL &&
           app_uuid_parse(item->valuestring, out_uuid) == APP_ERROR_NONE;
}
'''

resource_parser_block = r'''

static app_error_code_t request_resource_result(app_error_code_t result) {
    return result == APP_ERROR_STORAGE_CORRUPT ? APP_ERROR_INVALID_ARGUMENT : result;
}

static app_error_code_t validate_resource_fields(const char *body, size_t body_length,
                                                 const char *const *fields,
                                                 size_t field_count) {
    cJSON *root = parse_exact_document(body, body_length);
    const bool valid = root != NULL && exact_fields(root, fields, field_count);
    cJSON_Delete(root);
    return valid ? APP_ERROR_NONE : APP_ERROR_INVALID_ARGUMENT;
}

static app_error_code_t validate_macro_resource_fields(const char *body, size_t body_length) {
    static const char *const base_fields[] = {
        "schema_version", "id",          "revision",    "scope",       "name",
        "source",         "favorite",    "key_press_ms", "inter_key_ms",
    };
    static const char *const set_fields[] = {
        "schema_version", "id",       "revision",     "scope",        "name",
        "source",         "favorite", "key_press_ms", "inter_key_ms", "set_id",
    };
    cJSON *root = parse_exact_document(body, body_length);
    const bool valid = root != NULL &&
                       (exact_fields(root, base_fields,
                                     sizeof(base_fields) / sizeof(base_fields[0])) ||
                        exact_fields(root, set_fields,
                                     sizeof(set_fields) / sizeof(set_fields[0])));
    cJSON_Delete(root);
    return valid ? APP_ERROR_NONE : APP_ERROR_INVALID_ARGUMENT;
}

app_error_code_t web_api_json_parse_set_resource(const char *body, size_t body_length,
                                                 macro_set_t *out_set) {
    if (out_set != NULL) {
        memset(out_set, 0, sizeof(*out_set));
    }
    if (out_set == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    static const char *const fields[] = {
        "schema_version", "id",           "revision",     "name",  "description",
        "manufacturer",   "model",        "board",        "keyboard_layout",
        "sort_order",
    };
    app_error_code_t result =
        validate_resource_fields(body, body_length, fields, sizeof(fields) / sizeof(fields[0]));
    if (result == APP_ERROR_NONE) {
        result = request_resource_result(storage_repository_parse_set_json(body, body_length,
                                                                           out_set));
    }
    return result;
}

app_error_code_t web_api_json_parse_macro_resource(const char *body, size_t body_length,
                                                   macro_t *out_macro) {
    if (out_macro != NULL) {
        memset(out_macro, 0, sizeof(*out_macro));
    }
    if (out_macro == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    app_error_code_t result = validate_macro_resource_fields(body, body_length);
    if (result == APP_ERROR_NONE) {
        result = request_resource_result(storage_repository_parse_macro_json(body, body_length,
                                                                             out_macro));
    }
    return result;
}

app_error_code_t web_api_json_parse_procedure_resource(const char *body, size_t body_length,
                                                       procedure_t *out_procedure) {
    if (out_procedure != NULL) {
        memset(out_procedure, 0, sizeof(*out_procedure));
    }
    if (out_procedure == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    static const char *const fields[] = {
        "schema_version", "id", "revision", "set_id", "name", "description", "steps",
        "sort_order",
    };
    app_error_code_t result =
        validate_resource_fields(body, body_length, fields, sizeof(fields) / sizeof(fields[0]));
    if (result == APP_ERROR_NONE) {
        result = request_resource_result(storage_repository_parse_procedure_json(
            body, body_length, out_procedure));
    }
    return result;
}

app_error_code_t web_api_json_parse_progress_resource(const char *body, size_t body_length,
                                                      procedure_progress_t *out_progress) {
    if (out_progress != NULL) {
        memset(out_progress, 0, sizeof(*out_progress));
    }
    if (out_progress == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    static const char *const fields[] = {
        "schema_version",     "set_id",          "procedure_id", "procedure_revision",
        "current_step_id",    "completed_step_ids", "skipped_step_ids",
    };
    app_error_code_t result =
        validate_resource_fields(body, body_length, fields, sizeof(fields) / sizeof(fields[0]));
    if (result == APP_ERROR_NONE) {
        result = request_resource_result(storage_repository_parse_progress_json(
            body, body_length, out_progress));
    }
    return result;
}
'''
replace_once(
    "firmware/components/web_server/web_api_json.c",
    read_uuid_function,
    read_uuid_function + resource_parser_block,
)

replace_once(
    "firmware/components/web_server/web_api_json.h",
    "app_error_code_t web_api_json_parse_resource_mutation(const char *body,\n"
    "                                                       const web_api_resource_parse_limits_t *limits,\n"
    "                                                       web_api_resource_mutation_t *out_mutation);\n",
    "app_error_code_t web_api_json_parse_resource_mutation(const char *body,\n"
    "                                                       const web_api_resource_parse_limits_t *limits,\n"
    "                                                       web_api_resource_mutation_t *out_mutation);\n"
    "app_error_code_t web_api_json_parse_set_resource(const char *body, size_t body_length,\n"
    "                                                 macro_set_t *out_set);\n"
    "app_error_code_t web_api_json_parse_macro_resource(const char *body, size_t body_length,\n"
    "                                                   macro_t *out_macro);\n"
    "app_error_code_t web_api_json_parse_procedure_resource(const char *body, size_t body_length,\n"
    "                                                       procedure_t *out_procedure);\n"
    "app_error_code_t web_api_json_parse_progress_resource(const char *body, size_t body_length,\n"
    "                                                      procedure_progress_t *out_progress);\n",
)

replace_exact_count(
    "firmware/components/web_server/web_api_sets.c",
    "storage_repository_parse_set_json(",
    "web_api_json_parse_set_resource(",
    2,
)
replace_exact_count(
    "firmware/components/web_server/web_api_macros.c",
    "storage_repository_parse_macro_json(",
    "web_api_json_parse_macro_resource(",
    3,
)
replace_exact_count(
    "firmware/components/web_server/web_api_procedures.c",
    "storage_repository_parse_procedure_json(",
    "web_api_json_parse_procedure_resource(",
    2,
)
replace_exact_count(
    "firmware/components/web_server/web_api_procedures.c",
    "storage_repository_parse_progress_json(",
    "web_api_json_parse_progress_resource(",
    1,
)

# The standalone JSON test target now links the production object parsers used by
# the HTTP-domain wrappers.
replace_once(
    "tests/host/CMakeLists.txt",
    "    test_web_api_json.c ../../firmware/components/macro_model/app_error.c\n"
    "    ../../firmware/components/macro_model/app_uuid.c\n"
    "    ../../firmware/components/web_server/web_api_json.c\n",
    "    test_web_api_json.c ../../firmware/components/macro_model/app_error.c\n"
    "    ../../firmware/components/macro_model/app_uuid.c\n"
    "    ../../firmware/components/macro_model/macro_model.c\n"
    "    ../../firmware/components/storage/storage_json.c\n"
    "    ../../firmware/components/storage/storage_repository_json.c\n"
    "    ../../firmware/components/storage/storage_repository_objects_json.c\n"
    "    ../../firmware/components/web_server/web_api_json.c\n",
)
replace_once(
    "tests/host/CMakeLists.txt",
    "             ../../firmware/components/storage/include\n"
    "             ../../firmware/components/web_server\n",
    "             ../../firmware/components/storage/include\n"
    "             ../../firmware/components/storage\n"
    "             ../../firmware/components/web_server\n",
)

resource_tests = r'''

static void test_resource_request_boundary(void) {
    const char valid_set[] =
        "{\"schema_version\":1,\"id\":\"" SET_ID
        "\",\"revision\":1,\"name\":\"Set\",\"description\":\"\","
        "\"manufacturer\":\"\",\"model\":\"\",\"board\":\"\","
        "\"keyboard_layout\":\"en-US\",\"sort_order\":0}";
    macro_set_t set = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_api_json_parse_set_resource(valid_set, sizeof(valid_set) - 1U, &set));
    TEST_CHECK_EQ_STRING("Set", set.name);

    const char valid_macro[] =
        "{\"schema_version\":1,\"id\":\"" MACRO_ID
        "\",\"revision\":1,\"scope\":\"set\",\"name\":\"Macro\","
        "\"source\":\"a\",\"favorite\":false,\"key_press_ms\":8,"
        "\"inter_key_ms\":15,\"set_id\":\"" SET_ID "\"}";
    macro_t macro = {0};
    TEST_CHECK_APP_ERROR(
        APP_ERROR_NONE,
        web_api_json_parse_macro_resource(valid_macro, sizeof(valid_macro) - 1U, &macro));
    TEST_CHECK_EQ_STRING("a", macro.source);
    macro_model_free_macro(&macro);

    const char valid_procedure[] =
        "{\"schema_version\":1,\"id\":\"" PROCEDURE_ID
        "\",\"revision\":1,\"set_id\":\"" SET_ID
        "\",\"name\":\"Procedure\",\"description\":\"\",\"steps\":[{"
        "\"id\":\"" STEP_ID
        "\",\"type\":\"macro\",\"title\":\"Step\",\"macro_id\":\"" MACRO_ID
        "\",\"required\":true,\"auto_complete_on_success\":false}],\"sort_order\":0}";
    procedure_t procedure = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_api_json_parse_procedure_resource(
                             valid_procedure, sizeof(valid_procedure) - 1U, &procedure));
    TEST_CHECK_EQ_U64(1U, procedure.step_count);
    macro_model_free_procedure(&procedure);

    const char valid_progress[] =
        "{\"schema_version\":1,\"set_id\":\"" SET_ID
        "\",\"procedure_id\":\"" PROCEDURE_ID
        "\",\"procedure_revision\":1,\"current_step_id\":\"" STEP_ID
        "\",\"completed_step_ids\":[],\"skipped_step_ids\":[]}";
    procedure_progress_t progress = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         web_api_json_parse_progress_resource(
                             valid_progress, sizeof(valid_progress) - 1U, &progress));
    TEST_CHECK_EQ_U64(1U, progress.procedure_revision);

    static const char *const invalid_sets[] = {
        "{}",
        "{\"schema_version\":1,\"id\":\"" SET_ID
        "\",\"revision\":1,\"name\":\"Set\",\"description\":\"\","
        "\"manufacturer\":\"\",\"model\":\"\",\"board\":\"\","
        "\"keyboard_layout\":\"en-US\",\"sort_order\":0,\"extra\":true}",
        "{\"schema_version\":1,\"schema_version\":1,\"id\":\"" SET_ID
        "\",\"revision\":1,\"name\":\"Set\",\"description\":\"\","
        "\"manufacturer\":\"\",\"model\":\"\",\"board\":\"\","
        "\"keyboard_layout\":\"en-US\",\"sort_order\":0}",
        "{\"schema_version\":1}x",
        "{",
    };
    for (size_t index = 0U; index < sizeof(invalid_sets) / sizeof(invalid_sets[0]); ++index) {
        memset(&set, 0xa5, sizeof(set));
        TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                             web_api_json_parse_set_resource(
                                 invalid_sets[index], strlen(invalid_sets[index]), &set));
        TEST_CHECK_EQ_U64(0U, set.revision);
    }

    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_macro_resource("{}", 2U, &macro));
    TEST_CHECK(macro.source == NULL);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_procedure_resource("{}", 2U, &procedure));
    TEST_CHECK(procedure.steps == NULL);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_progress_resource("{}", 2U, &progress));
    TEST_CHECK_EQ_U64(0U, progress.procedure_revision);
    TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,
                         web_api_json_parse_set_resource(valid_set, sizeof(valid_set) - 1U, NULL));
}
'''
replace_once(
    "tests/host/test_web_api_json.c",
    "\nstatic void test_uuid_order_matrix(void) {\n",
    resource_tests + "\nstatic void test_uuid_order_matrix(void) {\n",
)
replace_once(
    "tests/host/test_web_api_json.c",
    "    test_resource_mutation_matrix();\n"
    "    test_uuid_order_matrix();\n",
    "    test_resource_mutation_matrix();\n"
    "    test_resource_request_boundary();\n"
    "    test_uuid_order_matrix();\n",
)

# Handler-level assertions prove malformed client input no longer masquerades as
# persisted-storage corruption/service unavailability.
replace_once(
    "tests/host/test_web_api_repository_handlers.c",
    "    response = invoke(web_api_handle_sets, WEB_API_ROUTE_SETS, WEB_API_METHOD_POST,\n"
    "                      \"{\\\"unknown\\\":true}\", NULL, NULL, NULL);\n"
    "    expect_status(&response, 503U, \"could not create set\");\n",
    "    static const char *const invalid_set_bodies[] = {\n"
    "        \"{\\\"unknown\\\":true}\",\n"
    "        \"{\\\"schema_version\\\":1}x\",\n"
    "        \"{\",\n"
    "    };\n"
    "    for (size_t index = 0U;\n"
    "         index < sizeof(invalid_set_bodies) / sizeof(invalid_set_bodies[0]); ++index) {\n"
    "        response = invoke(web_api_handle_sets, WEB_API_ROUTE_SETS, WEB_API_METHOD_POST,\n"
    "                          invalid_set_bodies[index], NULL, NULL, NULL);\n"
    "        expect_status(&response, 422U, \"could not create set\");\n"
    "    }\n",
)

print("Phase 16 request JSON hardening transform applied")
