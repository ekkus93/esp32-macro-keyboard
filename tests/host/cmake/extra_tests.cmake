target_include_directories(
    app_core_tests PRIVATE "${CMAKE_SOURCE_DIR}/../../firmware/components/wifi_ap/include"
                           "${CMAKE_SOURCE_DIR}/../../firmware/components/provisioning/include"
)

add_executable(
    provisioning_bootstrap_tests
    "${CMAKE_SOURCE_DIR}/test_provisioning_bootstrap.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/provisioning/provisioning_bootstrap_core.c"
)
target_include_directories(
    provisioning_bootstrap_tests
    PRIVATE "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/wifi_ap/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/provisioning/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/provisioning"
)
target_link_libraries(provisioning_bootstrap_tests PRIVATE test_support)
target_compile_options(provisioning_bootstrap_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME provisioning_bootstrap COMMAND provisioning_bootstrap_tests)
set_tests_properties(provisioning_bootstrap PROPERTIES LABELS "storage")

add_executable(
    web_setup_tests "${CMAKE_SOURCE_DIR}/test_web_setup.c"
                    "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server/web_setup_core.c"
)
target_include_directories(
    web_setup_tests
    PRIVATE "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/auth/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/wifi_ap/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/provisioning/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server"
)
target_link_libraries(web_setup_tests PRIVATE test_support)
target_compile_options(web_setup_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME web_setup COMMAND web_setup_tests)
set_tests_properties(web_setup PROPERTIES LABELS "web")

add_executable(
    web_setup_json_tests
    "${CMAKE_SOURCE_DIR}/test_web_setup_json.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server/web_setup_json.c"
)
target_include_directories(
    web_setup_json_tests
    PRIVATE "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/auth/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/wifi_ap/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/provisioning/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server"
)
target_link_libraries(web_setup_json_tests PRIVATE PkgConfig::CJSON test_support)
target_compile_options(web_setup_json_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME web_setup_json COMMAND web_setup_json_tests)
set_tests_properties(web_setup_json PROPERTIES LABELS "web")

add_executable(
    storage_object_json_tests
    "${CMAKE_SOURCE_DIR}/test_storage_object_json.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/app_uuid.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/macro_model.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_json.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_repository_objects_json.c"
)
target_include_directories(
    storage_object_json_tests
    PRIVATE "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage"
)
target_link_libraries(storage_object_json_tests PRIVATE PkgConfig::CJSON test_support)
target_compile_options(storage_object_json_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME storage_object_json COMMAND storage_object_json_tests)
set_tests_properties(storage_object_json PROPERTIES LABELS "storage")

add_executable(
    storage_macro_repository_tests
    "${CMAKE_SOURCE_DIR}/test_storage_macros.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/app_error.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/app_uuid.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/macro_model.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_atomic.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_fs_ops.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_paths.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_transaction.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_transaction_replace.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_repository_io.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_repository_json.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_json.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_repository_objects_json.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_repository_order.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_repository_index.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_repository_sets.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_repository_macros.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_repository_lock.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_quarantine.c"
)
target_include_directories(
    storage_macro_repository_tests
    PRIVATE "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage"
)
target_compile_definitions(
    storage_macro_repository_tests
    PRIVATE STORAGE_DATA_MOUNT="${CMAKE_CURRENT_BINARY_DIR}/storage-macro-data"
)
target_link_libraries(storage_macro_repository_tests PRIVATE PkgConfig::CJSON test_support)
target_compile_options(storage_macro_repository_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME storage_macro_repository COMMAND storage_macro_repository_tests)
set_tests_properties(storage_macro_repository PROPERTIES LABELS "storage")

add_executable(
    storage_procedure_repository_tests
    "${CMAKE_SOURCE_DIR}/test_storage_procedures.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/app_error.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/app_uuid.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/macro_model.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_atomic.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_fs_ops.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_paths.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_transaction.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_transaction_replace.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_repository_io.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_repository_json.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_json.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_repository_objects_json.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_repository_order.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_repository_index.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_repository_sets.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_repository_macros.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_repository_procedures.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_repository_lock.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_quarantine.c"
)
target_include_directories(
    storage_procedure_repository_tests
    PRIVATE "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage"
)
target_compile_definitions(
    storage_procedure_repository_tests
    PRIVATE STORAGE_DATA_MOUNT="${CMAKE_CURRENT_BINARY_DIR}/storage-procedure-data"
)
target_link_libraries(storage_procedure_repository_tests PRIVATE PkgConfig::CJSON test_support)
target_compile_options(storage_procedure_repository_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME storage_procedure_repository COMMAND storage_procedure_repository_tests)
set_tests_properties(storage_procedure_repository PROPERTIES LABELS "storage")

add_executable(
    storage_package_tests
    "${CMAKE_SOURCE_DIR}/test_storage_package.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/app_error.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/app_uuid.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/macro_model.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_parser/macro_parser.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_parser/macro_keymap_us.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_json.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_repository_json.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_repository_objects_json.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package.c"
)
target_include_directories(
    storage_package_tests
    PRIVATE "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_parser/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage"
)
target_link_libraries(storage_package_tests PRIVATE PkgConfig::CJSON test_support)
target_compile_options(storage_package_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME storage_package COMMAND storage_package_tests)
set_tests_properties(storage_package PROPERTIES LABELS "storage")

add_executable(
    storage_package_export_tests
    "${CMAKE_SOURCE_DIR}/test_storage_package_export.c"
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
)
target_include_directories(
    storage_package_export_tests
    PRIVATE "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_parser/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage"
)
target_link_libraries(storage_package_export_tests PRIVATE PkgConfig::CJSON test_support)
target_compile_options(storage_package_export_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME storage_package_export COMMAND storage_package_export_tests)
set_tests_properties(storage_package_export PROPERTIES LABELS "storage")

target_sources(
    web_api_repository_handlers_tests
    PRIVATE "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package.c"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package_export.c"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package_replace.c"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package_import.c"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_set_tree.c"
)

add_executable(
    web_api_set_export_tests
    "${CMAKE_SOURCE_DIR}/test_web_api_set_export.c"
    ${STORAGE_OBJECT_REPOSITORY_SOURCES}
    "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_parser/macro_parser.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_parser/macro_keymap_us.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package_export.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package_replace.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_package_import.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_set_tree.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server/web_api_core.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server/web_api_response.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server/web_api_json.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server/web_api_handler_common.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server/web_api_sets.c"
)
target_include_directories(
    web_api_set_export_tests
    PRIVATE "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_parser/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_executor/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/auth/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/wifi_ap/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/provisioning/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/storage"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server"
)
target_compile_definitions(
    web_api_set_export_tests
    PRIVATE STORAGE_DATA_MOUNT="/tmp/esp32-macro-keyboard-web-package"
)
target_link_libraries(web_api_set_export_tests PRIVATE PkgConfig::CJSON test_support)
target_compile_options(web_api_set_export_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME web_api_set_export COMMAND web_api_set_export_tests)
set_tests_properties(web_api_set_export PROPERTIES LABELS "web;storage")
