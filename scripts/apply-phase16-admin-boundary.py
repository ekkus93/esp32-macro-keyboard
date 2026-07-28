#!/usr/bin/env python3
"""Wire the validated administration boundary and its host test."""

from pathlib import Path


def replace_once(path_text: str, old: str, new: str) -> None:
    path = Path(path_text)
    text = path.read_text(encoding="utf-8")
    if text.count(old) != 1:
        raise SystemExit(f"{path_text}: expected one match, found {text.count(old)}")
    path.write_text(text.replace(old, new, 1), encoding="utf-8")


replace_once(
    "firmware/components/web_server/web_api_administration.c",
    '#include "web_api_core.h"\n',
    '#include "web_api_admin_boundary.h"\n#include "web_api_core.h"\n',
)

path = Path("firmware/components/web_server/web_api_administration.c")
text = path.read_text(encoding="utf-8")
start = text.find("static app_error_code_t handle_storage_health")
end = text.find("static app_error_code_t handle_quarantine", start)
if start < 0 or end < 0:
    raise SystemExit("web_api_administration.c: storage health block changed")
text = text[:start] + text[end:]
unavailable = '''static app_error_code_t unavailable(web_api_response_t *response, const char *message) {
    return web_api_handler_error(response, APP_ERROR_STORAGE_UNAVAILABLE, message, NULL);
}

'''
if text.count(unavailable) != 1:
    raise SystemExit("web_api_administration.c: unavailable helper changed")
text = text.replace(unavailable, "", 1)
old_cases = '''    case WEB_API_ROUTE_DIAGNOSTICS_STORAGE:
        return handle_storage_health(response, false);
    case WEB_API_ROUTE_DIAGNOSTICS_STORAGE_CHECK:
        return handle_storage_health(response, true);
    case WEB_API_ROUTE_DIAGNOSTICS_QUARANTINE:
        return handle_quarantine(response);
    case WEB_API_ROUTE_SET_EXPORT:
    case WEB_API_ROUTE_SET_IMPORT:
    case WEB_API_ROUTE_BACKUP:
    case WEB_API_ROUTE_RESTORE:
        return unavailable(response, "package operation requires the Phase 18 transaction service");
'''
new_cases = '''    case WEB_API_ROUTE_DIAGNOSTICS_STORAGE:
    case WEB_API_ROUTE_DIAGNOSTICS_STORAGE_CHECK:
    case WEB_API_ROUTE_SET_EXPORT:
    case WEB_API_ROUTE_SET_IMPORT:
    case WEB_API_ROUTE_BACKUP:
    case WEB_API_ROUTE_RESTORE:
        return web_api_admin_boundary_handle(call->path.route, response);
    case WEB_API_ROUTE_DIAGNOSTICS_QUARANTINE:
        return handle_quarantine(response);
'''
if text.count(old_cases) != 1:
    raise SystemExit("web_api_administration.c: administration cases changed")
path.write_text(text.replace(old_cases, new_cases, 1), encoding="utf-8")

replace_once(
    "firmware/components/web_server/CMakeLists.txt",
    '    "web_api_administration.c"\n',
    '    "web_api_administration.c"\n    "web_api_admin_boundary.c"\n',
)

cmake = Path("tests/host/CMakeLists.txt")
text = cmake.read_text(encoding="utf-8")
if "web_api_admin_boundary_tests" in text:
    raise SystemExit("web_api_admin_boundary_tests already registered")
marker = 'set_tests_properties(web_api_dispatch PROPERTIES LABELS "web")\n'
if text.count(marker) != 1:
    raise SystemExit("host CMake dispatcher marker changed")
block = r'''

add_executable(
    web_api_admin_boundary_tests
    test_web_api_admin_boundary.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/web_server/web_api_core.c
    ../../firmware/components/web_server/web_api_response.c
    ../../firmware/components/web_server/web_api_admin_boundary.c
)
target_include_directories(
    web_api_admin_boundary_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/macro_parser/include
            ../../firmware/components/macro_executor/include
            ../../firmware/components/storage/include
            ../../firmware/components/web_server
)
target_link_libraries(web_api_admin_boundary_tests PRIVATE PkgConfig::CJSON test_support)
target_compile_options(web_api_admin_boundary_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME web_api_admin_boundary COMMAND web_api_admin_boundary_tests)
set_tests_properties(web_api_admin_boundary PROPERTIES LABELS "web")
'''
cmake.write_text(text.replace(marker, marker + block, 1), encoding="utf-8")
print("Phase 16 administration boundary wired")
