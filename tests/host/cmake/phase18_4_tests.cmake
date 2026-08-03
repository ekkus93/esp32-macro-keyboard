add_executable(
    storage_package_backup_tests
    "${CMAKE_SOURCE_DIR}/test_storage_package_backup.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/app_error.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/app_uuid.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/macro_model.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_parser/macro_parser.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_parser/macro_keymap_us.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_json.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_repository_json.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_repository_objects_json.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package_export.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package_backup.c"
)
target_include_directories(
    storage_package_backup_tests
    PRIVATE "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_parser/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_parser/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage"
)
target_link_libraries(storage_package_backup_tests PRIVATE PkgConfig::CJSON test_support)
target_compile_options(storage_package_backup_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME storage_package_backup COMMAND storage_package_backup_tests)
set_tests_properties(storage_package_backup PROPERTIES LABELS "storage")
