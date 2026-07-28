#!/usr/bin/env python3
"""Register the Phase 16 dispatcher acceptance test exactly once."""

from pathlib import Path

path = Path("tests/host/CMakeLists.txt")
text = path.read_text(encoding="utf-8")
if "web_api_dispatch_tests" in text:
    raise SystemExit("web_api_dispatch_tests is already registered")
marker = 'set_tests_properties(web_api_response PROPERTIES LABELS "web")\n'
if text.count(marker) != 1:
    raise SystemExit("web API response test registration marker changed")
block = r'''

add_executable(
    web_api_dispatch_tests
    test_web_api_dispatch.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/web_server/web_api_response.c
    ../../firmware/components/web_server/web_api_dispatch.c
)
target_include_directories(
    web_api_dispatch_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/macro_parser/include
            ../../firmware/components/macro_executor/include
            ../../firmware/components/web_server
)
target_link_libraries(web_api_dispatch_tests PRIVATE PkgConfig::CJSON test_support)
target_compile_options(web_api_dispatch_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME web_api_dispatch COMMAND web_api_dispatch_tests)
set_tests_properties(web_api_dispatch PROPERTIES LABELS "web")
'''
path.write_text(text.replace(marker, marker + block, 1), encoding="utf-8")
print("Phase 16 dispatcher acceptance test registered")
