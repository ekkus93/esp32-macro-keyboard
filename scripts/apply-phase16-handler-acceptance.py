#!/usr/bin/env python3
"""Register repository-backed acceptance and harden atomic settings parsing."""

from pathlib import Path
import re

path = Path("tests/host/CMakeLists.txt")
text = path.read_text(encoding="utf-8")
if "web_api_repository_handlers_tests" in text:
    raise SystemExit("web_api_repository_handlers_tests already registered")
marker = 'set_tests_properties(web_api_admin_boundary PROPERTIES LABELS "web")\n'
if text.count(marker) != 1:
    raise SystemExit("administration boundary test registration marker changed")
block = r'''

add_executable(
    web_api_repository_handlers_tests
    test_web_api_repository_handlers.c
    ${STORAGE_OBJECT_REPOSITORY_SOURCES}
    ../../firmware/components/macro_parser/macro_parser.c
    ../../firmware/components/macro_parser/macro_keymap_us.c
    ../../firmware/components/web_server/web_api_core.c
    ../../firmware/components/web_server/web_api_response.c
    ../../firmware/components/web_server/web_api_json.c
    ../../firmware/components/web_server/web_api_handler_common.c
    ../../firmware/components/web_server/web_api_sets.c
    ../../firmware/components/web_server/web_api_macros.c
    ../../firmware/components/web_server/web_api_procedures.c
)
target_include_directories(
    web_api_repository_handlers_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/macro_parser/include
            ../../firmware/components/macro_executor/include
            ../../firmware/components/auth/include
            ../../firmware/components/wifi_ap/include
            ../../firmware/components/provisioning/include
            ../../firmware/components/storage/include
            ../../firmware/components/storage
            ../../firmware/components/web_server
)
target_compile_definitions(
    web_api_repository_handlers_tests
    PRIVATE STORAGE_DATA_MOUNT="${CMAKE_CURRENT_BINARY_DIR}/web-api-repository-data"
)
target_link_libraries(web_api_repository_handlers_tests PRIVATE PkgConfig::CJSON test_support)
target_compile_options(web_api_repository_handlers_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME web_api_repository_handlers COMMAND web_api_repository_handlers_tests)
set_tests_properties(web_api_repository_handlers PROPERTIES LABELS "web")
'''
path.write_text(text.replace(marker, marker + block, 1), encoding="utf-8")

json_path = Path("firmware/components/web_server/web_api_json.c")
json_text = json_path.read_text(encoding="utf-8")
pattern = re.compile(
    r"app_error_code_t web_api_json_parse_settings_update\(.*?\n\}\n$",
    re.DOTALL,
)
replacement = r'''app_error_code_t web_api_json_parse_settings_update(const char *body, size_t body_length,
                                                     provisioning_settings_t *out_settings,
                                                     uint32_t *out_expected_revision) {
    if (out_settings != NULL) {
        memset(out_settings, 0, sizeof(*out_settings));
    }
    if (out_expected_revision != NULL) {
        *out_expected_revision = 0U;
    }
    if (out_settings == NULL || out_expected_revision == NULL) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    cJSON *root = parse_exact_document(body, body_length);
    static const char *const fields[] = {"expectedRevision", "requirePhysicalConfirmation",
                                         "alwaysSelectSet", "activeSetId"};
    const cJSON *require_confirmation =
        root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "requirePhysicalConfirmation");
    const cJSON *always_select =
        root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "alwaysSelectSet");
    const cJSON *active_set =
        root == NULL ? NULL : cJSON_GetObjectItemCaseSensitive(root, "activeSetId");
    uint32_t expected_revision = 0U;
    if (root == NULL || !exact_fields(root, fields, 4U) ||
        !read_revision(root, "expectedRevision", &expected_revision) ||
        !cJSON_IsBool(require_confirmation) || !cJSON_IsBool(always_select) ||
        (!cJSON_IsNull(active_set) && !cJSON_IsString(active_set))) {
        cJSON_Delete(root);
        return APP_ERROR_INVALID_ARGUMENT;
    }
    provisioning_settings_t settings = {
        .schema_version = APP_SCHEMA_VERSION,
        .revision = expected_revision,
        .require_physical_confirmation = cJSON_IsTrue(require_confirmation),
        .always_select_set = cJSON_IsTrue(always_select),
    };
    if (cJSON_IsString(active_set)) {
        if (active_set->valuestring == NULL ||
            app_uuid_parse(active_set->valuestring, &settings.active_set_id) != APP_ERROR_NONE) {
            cJSON_Delete(root);
            return APP_ERROR_INVALID_ARGUMENT;
        }
        settings.has_active_set = true;
    }
    cJSON_Delete(root);
    *out_settings = settings;
    *out_expected_revision = expected_revision;
    return APP_ERROR_NONE;
}
'''
json_text, count = pattern.subn(replacement, json_text, count=1)
if count != 1:
    raise SystemExit(f"web_api_json.c: settings parser replacement matched {count}")
json_path.write_text(json_text, encoding="utf-8")
print("Phase 16 repository handler acceptance and atomic settings parsing applied")
