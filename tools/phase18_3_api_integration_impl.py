from pathlib import Path


def replace_once(path: str, old: str, new: str) -> None:
    target = Path(path)
    text = target.read_text()
    if text.count(old) != 1:
        raise SystemExit(f"expected one anchor in {path}: {old!r}; found {text.count(old)}")
    target.write_text(text.replace(old, new, 1))


sets_path = "firmware/components/web_server/web_api_sets.c"
import_code = r'''
typedef struct {
    app_uuid_t target_set_id;
    uint32_t expected_revision;
    char *package_json;
    size_t package_length;
} web_set_import_request_t;

static void free_set_import_request(web_set_import_request_t *request) {
    if (request == NULL) {
        return;
    }
    cJSON_free(request->package_json);
    memset(request, 0, sizeof(*request));
}

static app_error_code_t parse_set_import(const web_api_call_t *call,
                                         web_set_import_request_t *out_request) {
    if (call == NULL || out_request == NULL || call->body == NULL || call->body_length == 0U) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    memset(out_request, 0, sizeof(*out_request));
    const char *parse_end = NULL;
    cJSON *root = cJSON_ParseWithLengthOpts(call->body, call->body_length, &parse_end, false);
    bool target_seen = false;
    bool revision_seen = false;
    bool package_seen = false;
    app_error_code_t result =
        root != NULL && parse_end == call->body + call->body_length && cJSON_IsObject(root)
            ? APP_ERROR_NONE
            : APP_ERROR_INVALID_ARGUMENT;
    for (const cJSON *item = result == APP_ERROR_NONE ? root->child : NULL; item != NULL;
         item = item->next) {
        if (item->string != NULL && strcmp(item->string, "targetSetId") == 0 && !target_seen &&
            cJSON_IsString(item) && item->valuestring != NULL &&
            app_uuid_parse(item->valuestring, &out_request->target_set_id) == APP_ERROR_NONE) {
            target_seen = true;
        } else if (item->string != NULL && strcmp(item->string, "expectedRevision") == 0 &&
                   !revision_seen && cJSON_IsNumber(item) && item->valuedouble >= 1.0 &&
                   item->valuedouble <= (double)UINT32_MAX) {
            const uint32_t revision = (uint32_t)item->valuedouble;
            if ((double)revision != item->valuedouble) {
                result = APP_ERROR_INVALID_ARGUMENT;
                break;
            }
            out_request->expected_revision = revision;
            revision_seen = true;
        } else if (item->string != NULL && strcmp(item->string, "package") == 0 &&
                   !package_seen && cJSON_IsObject(item)) {
            out_request->package_json = cJSON_PrintUnformatted(item);
            if (out_request->package_json == NULL) {
                result = APP_ERROR_INTERNAL;
                break;
            }
            out_request->package_length = strlen(out_request->package_json);
            if (out_request->package_length == 0U ||
                out_request->package_length > APP_IMPORT_PACKAGE_MAX_BYTES) {
                result = APP_ERROR_INVALID_ARGUMENT;
                break;
            }
            package_seen = true;
        } else {
            result = APP_ERROR_INVALID_ARGUMENT;
            break;
        }
    }
    cJSON_Delete(root);
    if (result == APP_ERROR_NONE && (!target_seen || !revision_seen || !package_seen)) {
        result = APP_ERROR_INVALID_ARGUMENT;
    }
    if (result != APP_ERROR_NONE) {
        free_set_import_request(out_request);
    }
    return result;
}

static app_error_code_t handle_import(const web_api_call_t *call, web_api_response_t *response) {
    web_set_import_request_t request = {0};
    app_error_code_t result = parse_set_import(call, &request);
    macro_set_t committed = {0};
    if (result == APP_ERROR_NONE) {
        result = storage_package_replace_set(&request.target_set_id, request.expected_revision,
                                             request.package_json, request.package_length,
                                             &committed);
    }
    free_set_import_request(&request);
    return result == APP_ERROR_NONE ? send_set(response, WEB_HTTP_STATUS_OK, &committed)
                                    : respond_result(response, result, "could not replace set");
}

'''
replace_once(sets_path, "static app_error_code_t handle_export", import_code + "static app_error_code_t handle_export")
replace_once(
    sets_path,
    '''static app_error_code_t unavailable(web_api_response_t *response, const char *operation) {
    return web_api_handler_error(response, APP_ERROR_STORAGE_UNAVAILABLE, operation, NULL);
}

''',
    "",
)
replace_once(
    sets_path,
    '''    case WEB_API_ROUTE_SET_IMPORT:
        return unavailable(response, "set import requires the Phase 18 package service");''',
    '''    case WEB_API_ROUTE_SET_IMPORT:
        return handle_import(call, response);''',
)

replace_once(
    "firmware/components/web_server/web_api_core.c",
    '''           route == WEB_API_ROUTE_DEVICE_FACTORY_RESET || route == WEB_API_ROUTE_RESTORE;''',
    '''           route == WEB_API_ROUTE_DEVICE_FACTORY_RESET || route == WEB_API_ROUTE_SET_IMPORT ||
           route == WEB_API_ROUTE_RESTORE;''',
)
replace_once(
    "firmware/components/web_server/web_server_api.c",
    '''    case WEB_API_ROUTE_SET_IMPORT:
    case WEB_API_ROUTE_RESTORE:
        return APP_IMPORT_PACKAGE_MAX_BYTES;''',
    '''    case WEB_API_ROUTE_SET_IMPORT:
        return APP_IMPORT_PACKAGE_MAX_BYTES + WEB_API_WRAPPER_OVERHEAD_BYTES;
    case WEB_API_ROUTE_RESTORE:
        return APP_IMPORT_PACKAGE_MAX_BYTES;''',
)
replace_once(
    "tests/host/test_web_api_core.c",
    '''    TEST_CHECK(web_api_route_requires_physical_confirmation(WEB_API_ROUTE_DEVICE_FACTORY_RESET));''',
    '''    TEST_CHECK(web_api_route_requires_physical_confirmation(WEB_API_ROUTE_DEVICE_FACTORY_RESET));
    TEST_CHECK(web_api_route_requires_physical_confirmation(WEB_API_ROUTE_SET_IMPORT));''',
)

api_test = "tests/host/test_web_api_set_export.c"
replace_once(api_test, '#include "app_uuid.h"\n', '#include "app_uuid.h"\n#include "cJSON.h"\n')
import_test_code = r'''
static web_api_response_t invoke_import(const char *body) {
    const web_api_call_t call = {
        .method = WEB_API_METHOD_POST,
        .path = {.route = WEB_API_ROUTE_SET_IMPORT},
        .body = body,
        .body_length = strlen(body),
    };
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_handle_sets(&call, &response));
    TEST_CHECK(response.body != NULL);
    return response;
}

static char *make_replacement_request(uint32_t expected_revision) {
    web_api_response_t exported = invoke_export(SET_ID);
    TEST_CHECK_EQ_U64(200U, exported.status);
    const char *parse_end = NULL;
    cJSON *package =
        cJSON_ParseWithLengthOpts(exported.body, exported.body_length, &parse_end, false);
    TEST_CHECK(package != NULL);
    TEST_CHECK(parse_end == exported.body + exported.body_length);
    cJSON *sets = cJSON_GetObjectItemCaseSensitive(package, "sets");
    cJSON *set = cJSON_GetArrayItem(sets, 0);
    TEST_CHECK(cJSON_IsObject(set));
    cJSON *revision = cJSON_CreateNumber(2.0);
    cJSON *name = cJSON_CreateString("Imported Replacement");
    TEST_CHECK(revision != NULL);
    TEST_CHECK(name != NULL);
    TEST_CHECK(cJSON_ReplaceItemInObjectCaseSensitive(set, "revision", revision));
    TEST_CHECK(cJSON_ReplaceItemInObjectCaseSensitive(set, "name", name));

    cJSON *wrapper = cJSON_CreateObject();
    TEST_CHECK(wrapper != NULL);
    TEST_CHECK(cJSON_AddStringToObject(wrapper, "targetSetId", SET_ID) != NULL);
    TEST_CHECK(cJSON_AddNumberToObject(wrapper, "expectedRevision", (double)expected_revision) !=
               NULL);
    TEST_CHECK(cJSON_AddItemToObject(wrapper, "package", package));
    package = NULL;
    char *request = cJSON_PrintUnformatted(wrapper);
    TEST_CHECK(request != NULL);
    cJSON_Delete(wrapper);
    cJSON_Delete(package);
    web_api_response_free(&exported);
    return request;
}

static void test_import_route(void) {
    char *request = make_replacement_request(1U);
    web_api_response_t response = invoke_import(request);
    TEST_CHECK_EQ_U64(200U, response.status);
    TEST_CHECK(strstr(response.body, "\"ok\":true") != NULL);
    TEST_CHECK(strstr(response.body, "Imported Replacement") != NULL);
    TEST_CHECK(strstr(response.body, "\"revision\":2") != NULL);
    macro_set_t committed = {0};
    const app_uuid_t set_id = uuid(SET_ID);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, storage_set_read(&set_id, &committed));
    TEST_CHECK_EQ_U64(2U, committed.revision);
    TEST_CHECK_EQ_STRING("Imported Replacement", committed.name);
    web_api_response_free(&response);
    cJSON_free(request);

    request = make_replacement_request(1U);
    response = invoke_import(request);
    TEST_CHECK_EQ_U64(409U, response.status);
    TEST_CHECK(strstr(response.body, "\"ok\":false") != NULL);
    TEST_CHECK(strstr(response.body, "could not replace set") != NULL);
    web_api_response_free(&response);
    cJSON_free(request);

    response = invoke_import("{\"targetSetId\":\"" SET_ID
                             "\",\"expectedRevision\":2,\"package\":{},\"extra\":true}");
    TEST_CHECK_EQ_U64(422U, response.status);
    web_api_response_free(&response);
}

'''
replace_once(api_test, "static void test_export_route", import_test_code + "static void test_export_route")
replace_once(
    api_test,
    '''    test_export_route();
    test_missing_set_error_envelope();''',
    '''    test_export_route();
    test_import_route();
    test_missing_set_error_envelope();''',
)

extra = "tests/host/cmake/extra_tests.cmake"
replace_once(
    extra,
    '''    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package_export.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server/web_api_core.c"''',
    '''    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package_export.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package_replace.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_set_tree.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server/web_api_core.c"''',
)
replace_once(
    extra,
    '''    web_api_set_export_tests
    PRIVATE STORAGE_DATA_MOUNT="${CMAKE_CURRENT_BINARY_DIR}/web-api-set-export-data"''',
    '''    web_api_set_export_tests
    PRIVATE STORAGE_DATA_MOUNT="/tmp/esp32-macro-keyboard-web-package"''',
)
