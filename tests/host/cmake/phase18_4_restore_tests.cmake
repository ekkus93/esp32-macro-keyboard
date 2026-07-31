add_executable(
    storage_package_restore_tests
    "${CMAKE_SOURCE_DIR}/test_storage_package_restore.c"
    ${STORAGE_OBJECT_REPOSITORY_SOURCES}
    "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_parser/macro_parser.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_parser/macro_keymap_us.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package_restore.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_transaction_restore.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_transaction_restore_recovery.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_set_tree.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_repository_tree.c"
)
target_include_directories(
    storage_package_restore_tests
    PRIVATE "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_parser/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage"
)
target_compile_definitions(
    storage_package_restore_tests
    PRIVATE STORAGE_DATA_MOUNT="/tmp/esp32-macro-keyboard-package-restore"
)
target_link_libraries(storage_package_restore_tests PRIVATE PkgConfig::CJSON test_support)
target_link_options(storage_package_restore_tests PRIVATE "-Wl,--wrap=storage_atomic_write")
target_compile_options(storage_package_restore_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME storage_package_restore COMMAND storage_package_restore_tests)
set_tests_properties(storage_package_restore PROPERTIES LABELS "storage")

# The administration package boundary now consumes the complete API call type,
# which includes the fixed-size authenticated-session token.
target_include_directories(
    web_api_admin_boundary_tests
    PRIVATE "${CMAKE_SOURCE_DIR}/../../firmware/components/auth/include"
)
