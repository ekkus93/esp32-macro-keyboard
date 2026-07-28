#!/usr/bin/env python3
"""Register the repository-backed Phase 16 handler acceptance target."""

from pathlib import Path

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
print("Phase 16 repository handler acceptance test registered")
