from pathlib import Path

path = Path("tests/host/cmake/extra_tests.cmake")
text = path.read_text()
old = '''target_sources(
    web_api_repository_handlers_tests
    PRIVATE "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package.c"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package_export.c"
)'''
new = '''target_sources(
    web_api_repository_handlers_tests
    PRIVATE "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package.c"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package_export.c"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package_replace.c"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_set_tree.c"
)'''
if text.count(old) != 1:
    raise SystemExit("web API repository handler source anchor changed")
path.write_text(text.replace(old, new, 1))
