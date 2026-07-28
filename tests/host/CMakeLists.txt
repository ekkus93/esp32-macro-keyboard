cmake_minimum_required(VERSION 3.20)
project(esp32_macro_keyboard_host_tests C)

enable_testing()
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(STRICT_WARNINGS
    -Wall
    -Wextra
    -Werror
    -Wshadow
    -Wconversion
    -Wsign-conversion
    -Wformat=2
    -Wundef
    -Wdouble-promotion
    -Wmissing-declarations
    -Wstrict-prototypes
)

add_library(
    test_support STATIC
    support/test_assert.c
    support/test_memory.c
    support/test_temp_dir.c
    fakes/fake_call_log.c
    fakes/fake_clock.c
    fakes/fake_random.c
    fakes/fake_freertos.c
    fakes/fake_usb_backend.c
    fakes/fake_gpio_backend.c
    fakes/fake_wifi_backend.c
    fakes/fake_http_backend.c
    fakes/fake_fs_backend.c
)
target_include_directories(test_support PUBLIC support fakes)
target_compile_definitions(test_support PUBLIC _POSIX_C_SOURCE=200809L)
target_compile_options(test_support PRIVATE ${STRICT_WARNINGS})

add_executable(test_support_tests test_support.c)
target_link_libraries(test_support_tests PRIVATE test_support)
target_compile_options(test_support_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME test_support COMMAND test_support_tests)
set_tests_properties(test_support PROPERTIES LABELS "support")

add_executable(
    app_operation_result_tests test_app_operation_result.c
                               ../../firmware/components/support/app_operation_result.c
)
target_include_directories(
    app_operation_result_tests PRIVATE ../../firmware/components/support/include
                                       ../../firmware/components/macro_model/include
)
target_link_libraries(app_operation_result_tests PRIVATE test_support)
target_compile_options(app_operation_result_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME app_operation_result COMMAND app_operation_result_tests)
set_tests_properties(app_operation_result PROPERTIES LABELS "support")

add_executable(
    macro_parser_tests
    test_macro_parser.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/macro_model/macro_model.c
    ../../firmware/components/macro_parser/macro_parser.c
    ../../firmware/components/macro_parser/macro_keymap_us.c
)
target_include_directories(
    macro_parser_tests PRIVATE ../../firmware/components/macro_model/include
                               ../../firmware/components/macro_parser/include
)
target_link_libraries(macro_parser_tests PRIVATE test_support)
target_compile_options(macro_parser_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME macro_parser COMMAND macro_parser_tests)
set_tests_properties(macro_parser PROPERTIES LABELS "parser")

add_executable(
    macro_model_tests test_macro_model.c ../../firmware/components/macro_model/macro_model.c
)
target_include_directories(macro_model_tests PRIVATE ../../firmware/components/macro_model/include)
target_link_libraries(macro_model_tests PRIVATE test_support)
target_compile_options(macro_model_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME macro_model COMMAND macro_model_tests)
set_tests_properties(macro_model PROPERTIES LABELS "model")

add_executable(
    macro_executor_tests test_macro_executor.c
                         ../../firmware/components/macro_executor/macro_executor_engine.c
)
target_include_directories(
    macro_executor_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/macro_parser/include
            ../../firmware/components/macro_executor/include
            ../../firmware/components/macro_executor
)
target_link_libraries(macro_executor_tests PRIVATE test_support)
target_compile_options(macro_executor_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME macro_executor COMMAND macro_executor_tests)
set_tests_properties(macro_executor PROPERTIES LABELS "executor")

add_executable(
    auth_tests
    test_auth.c
    ../../firmware/components/auth/auth_core_common.c
    ../../firmware/components/auth/auth_core_password.c
    ../../firmware/components/auth/auth_core_session.c
    ../../firmware/components/auth/auth_core_rate_limit.c
)
target_include_directories(
    auth_tests PRIVATE ../../firmware/components/macro_model/include
                       ../../firmware/components/auth/include ../../firmware/components/auth
)
target_link_libraries(auth_tests PRIVATE test_support)
target_compile_options(auth_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME auth COMMAND auth_tests)
set_tests_properties(auth PROPERTIES LABELS "auth")

add_executable(
    web_security_tests
    test_web_security.c
    ../../firmware/components/web_server/web_cookie.c
    ../../firmware/components/web_server/web_origin.c
    ../../firmware/components/web_server/web_static_path.c
    ../../firmware/components/web_server/web_content.c
)
target_include_directories(
    web_security_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/auth/include
            ../../firmware/components/storage/include ../../firmware/components/web_server
)
target_link_libraries(web_security_tests PRIVATE test_support)
target_compile_options(web_security_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME web_security COMMAND web_security_tests)
set_tests_properties(web_security PROPERTIES LABELS "web")

add_executable(
    web_server_adapter_tests
    test_web_server_adapter.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/web_server/web_server_adapter_common.c
    ../../firmware/components/web_server/web_server_adapter_body_auth.c
    ../../firmware/components/web_server/web_server_adapter_json.c
    ../../firmware/components/web_server/web_server_adapter_static_stream.c
    ../../firmware/components/web_server/web_server_adapter_lifecycle.c
    ../../firmware/components/web_server/web_cookie.c
    ../../firmware/components/web_server/web_origin.c
    ../../firmware/components/web_server/web_static_path.c
    ../../firmware/components/web_server/web_content.c
)
target_include_directories(
    web_server_adapter_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/auth/include
            ../../firmware/components/web_server
)
target_link_libraries(web_server_adapter_tests PRIVATE test_support)
target_compile_options(web_server_adapter_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME web_server_adapter COMMAND web_server_adapter_tests)
set_tests_properties(web_server_adapter PROPERTIES LABELS "web")

add_executable(
    app_core_tests test_app_core.c ../../firmware/components/app_core/app_core_sequence.c
                   ../../firmware/components/support/app_operation_result.c
)
target_include_directories(
    app_core_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/auth/include
            ../../firmware/components/device_controls/include
            ../../firmware/components/web_server/include
            ../../firmware/components/app_core
            ../../firmware/components/support/include
)
target_link_libraries(app_core_tests PRIVATE test_support)
target_compile_options(app_core_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME app_core COMMAND app_core_tests)
set_tests_properties(app_core PROPERTIES LABELS "startup")

add_executable(
    usb_keyboard_tests test_usb_keyboard.c
                       ../../firmware/components/usb_keyboard/usb_keyboard_state.c
)
target_include_directories(
    usb_keyboard_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/usb_keyboard/include ../../firmware/components/usb_keyboard
)
target_link_libraries(usb_keyboard_tests PRIVATE test_support)
target_compile_options(usb_keyboard_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME usb_keyboard COMMAND usb_keyboard_tests)
set_tests_properties(usb_keyboard PROPERTIES LABELS "usb")

add_executable(
    device_controls_tests test_device_controls.c
                          ../../firmware/components/device_controls/device_controls_logic.c
)
target_include_directories(
    device_controls_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/device_controls/include
            ../../firmware/components/device_controls
)
target_link_libraries(device_controls_tests PRIVATE test_support)
target_compile_options(device_controls_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME device_controls COMMAND device_controls_tests)
set_tests_properties(device_controls PROPERTIES LABELS "controls")

add_executable(wifi_ap_tests test_wifi_ap.c ../../firmware/components/wifi_ap/wifi_ap_state.c)
target_include_directories(
    wifi_ap_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/wifi_ap/include
            ../../firmware/components/wifi_ap
)
target_link_libraries(wifi_ap_tests PRIVATE test_support)
target_compile_options(wifi_ap_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME wifi_ap COMMAND wifi_ap_tests)
set_tests_properties(wifi_ap PROPERTIES LABELS "wifi")

add_executable(
    provisioning_tests test_provisioning.c ../../firmware/components/macro_model/app_uuid.c
                       ../../firmware/components/provisioning/provisioning_core.c
)
target_include_directories(
    provisioning_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/auth/include
            ../../firmware/components/wifi_ap/include
            ../../firmware/components/provisioning/include ../../firmware/components/provisioning
)
target_link_libraries(provisioning_tests PRIVATE test_support)
target_compile_options(provisioning_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME provisioning COMMAND provisioning_tests)
set_tests_properties(provisioning PROPERTIES LABELS "storage")

add_executable(
    storage_mount_tests
    test_storage_mount.c ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/storage/storage_mount_core.c
    ../../firmware/components/storage/storage_mount_topology.c
)
target_include_directories(
    storage_mount_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/storage/include
            ../../firmware/components/storage
)
target_compile_definitions(
    storage_mount_tests PRIVATE STORAGE_DATA_MOUNT="${CMAKE_CURRENT_BINARY_DIR}/storage-mount-data"
)
target_link_libraries(storage_mount_tests PRIVATE test_support)
target_compile_options(storage_mount_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME storage_mount COMMAND storage_mount_tests)
set_tests_properties(storage_mount PROPERTIES LABELS "storage")

add_executable(
    storage_atomic_recovery_tests
    test_storage_atomic_recovery.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/macro_model/macro_model.c
    ../../firmware/components/storage/storage_atomic.c
    ../../firmware/components/storage/storage_atomic_recovery.c
    ../../firmware/components/storage/storage_atomic_validators.c
    ../../firmware/components/storage/storage_fs_ops.c
    ../../firmware/components/storage/storage_paths.c
    ../../firmware/components/storage/storage_transaction.c
    ../../firmware/components/storage/storage_repository_io.c
    ../../firmware/components/storage/storage_repository_json.c
    ../../firmware/components/storage/storage_json.c
    ../../firmware/components/storage/storage_repository_objects_json.c
    ../../firmware/components/storage/storage_repository_index.c
    ../../firmware/components/storage/storage_repository_sets.c
    ../../firmware/components/storage/storage_quarantine.c
    ../../firmware/components/storage/storage_repository_lock.c
)
target_include_directories(
    storage_atomic_recovery_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/storage/include
            ../../firmware/components/storage
)
target_compile_definitions(
    storage_atomic_recovery_tests
    PRIVATE STORAGE_DATA_MOUNT="${CMAKE_CURRENT_BINARY_DIR}/storage-recovery-data"
)
target_link_libraries(storage_atomic_recovery_tests PRIVATE PkgConfig::CJSON test_support)
target_compile_options(storage_atomic_recovery_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME storage_atomic_recovery COMMAND storage_atomic_recovery_tests)
set_tests_properties(storage_atomic_recovery PROPERTIES LABELS "storage")

add_executable(
    storage_atomic_validators_tests
    test_storage_atomic_validators.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/macro_model/macro_model.c
    ../../firmware/components/storage/storage_atomic.c
    ../../firmware/components/storage/storage_atomic_validators.c
    ../../firmware/components/storage/storage_fs_ops.c
    ../../firmware/components/storage/storage_paths.c
    ../../firmware/components/storage/storage_transaction.c
    ../../firmware/components/storage/storage_repository_io.c
    ../../firmware/components/storage/storage_repository_json.c
    ../../firmware/components/storage/storage_json.c
    ../../firmware/components/storage/storage_repository_objects_json.c
    ../../firmware/components/storage/storage_repository_index.c
    ../../firmware/components/storage/storage_repository_sets.c
    ../../firmware/components/storage/storage_quarantine.c
    ../../firmware/components/storage/storage_repository_lock.c
)
target_include_directories(
    storage_atomic_validators_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/storage/include
            ../../firmware/components/storage
)
target_compile_definitions(
    storage_atomic_validators_tests
    PRIVATE STORAGE_DATA_MOUNT="${CMAKE_CURRENT_BINARY_DIR}/storage-validators-data"
)
target_compile_options(storage_atomic_validators_tests PRIVATE ${STRICT_WARNINGS})
target_link_libraries(storage_atomic_validators_tests PRIVATE PkgConfig::CJSON test_support)
add_test(NAME storage_atomic_validators COMMAND storage_atomic_validators_tests)
set_tests_properties(storage_atomic_validators PROPERTIES LABELS "storage")

add_executable(
    storage_atomic_tests
    test_storage_atomic.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/storage/storage_fs_ops.c
    ../../firmware/components/storage/storage_atomic.c
)
target_include_directories(
    storage_atomic_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/storage/include
            ../../firmware/components/storage
)
target_link_libraries(storage_atomic_tests PRIVATE test_support)
target_compile_options(storage_atomic_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME storage_atomic COMMAND storage_atomic_tests)
set_tests_properties(storage_atomic PROPERTIES LABELS "storage")

add_executable(
    storage_repository_lock_tests
    test_storage_repository_lock.c ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/storage/storage_repository_lock.c
)
target_include_directories(
    storage_repository_lock_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/storage/include
            ../../firmware/components/storage
)
target_link_libraries(storage_repository_lock_tests PRIVATE test_support)
target_compile_options(storage_repository_lock_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME storage_repository_lock COMMAND storage_repository_lock_tests)
set_tests_properties(storage_repository_lock PROPERTIES LABELS "storage")

add_executable(
    storage_parent_sync_tests
    test_storage_parent_sync.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/storage/storage_fs_ops.c
    ../../firmware/components/storage/storage_atomic.c
)
target_include_directories(
    storage_parent_sync_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/storage/include
            ../../firmware/components/storage
)
target_link_libraries(storage_parent_sync_tests PRIVATE test_support)
target_compile_options(storage_parent_sync_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME storage_parent_sync COMMAND storage_parent_sync_tests)
set_tests_properties(storage_parent_sync PROPERTIES LABELS "storage")

add_executable(
    storage_repository_io_tests
    test_storage_repository_io.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/storage/storage_fs_ops.c
    ../../firmware/components/storage/storage_atomic.c
    ../../firmware/components/storage/storage_paths.c
    ../../firmware/components/storage/storage_repository_io.c
)
target_include_directories(
    storage_repository_io_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/storage/include
            ../../firmware/components/storage
)
target_compile_definitions(
    storage_repository_io_tests
    PRIVATE STORAGE_DATA_MOUNT="${CMAKE_CURRENT_BINARY_DIR}/storage-io-data"
)
target_link_libraries(storage_repository_io_tests PRIVATE test_support)
target_compile_options(storage_repository_io_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME storage_repository_io COMMAND storage_repository_io_tests)
set_tests_properties(storage_repository_io PROPERTIES LABELS "storage")

add_executable(
    storage_transaction_tests
    test_storage_transactions.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/storage/storage_fs_ops.c
    ../../firmware/components/storage/storage_atomic.c
    ../../firmware/components/storage/storage_transaction.c
    ../../firmware/components/storage/storage_repository_lock.c
)
target_include_directories(
    storage_transaction_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/storage/include
            ../../firmware/components/storage
)
target_compile_definitions(
    storage_transaction_tests
    PRIVATE STORAGE_DATA_MOUNT="${CMAKE_CURRENT_BINARY_DIR}/storage-transaction-data"
)
target_link_libraries(storage_transaction_tests PRIVATE test_support)
target_compile_options(storage_transaction_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME storage_transaction COMMAND storage_transaction_tests)
set_tests_properties(storage_transaction PROPERTIES LABELS "storage")

find_package(PkgConfig REQUIRED)
pkg_check_modules(CJSON REQUIRED IMPORTED_TARGET libcjson)

add_executable(
    storage_quarantine_tests
    test_storage_quarantine.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/storage/storage_fs_ops.c
    ../../firmware/components/storage/storage_atomic.c
    ../../firmware/components/storage/storage_quarantine.c
    ../../firmware/components/storage/storage_repository_lock.c
)
target_include_directories(
    storage_quarantine_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/storage/include
            ../../firmware/components/storage
)
target_compile_definitions(
    storage_quarantine_tests
    PRIVATE STORAGE_DATA_MOUNT="${CMAKE_CURRENT_BINARY_DIR}/storage-quarantine-data"
)
target_link_libraries(storage_quarantine_tests PRIVATE PkgConfig::CJSON test_support)
target_compile_options(storage_quarantine_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME storage_quarantine COMMAND storage_quarantine_tests)
set_tests_properties(storage_quarantine PROPERTIES LABELS "storage")

add_executable(
    storage_repository_tests
    test_storage_repository.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/macro_model/macro_model.c
    ../../firmware/components/storage/storage_atomic.c
    ../../firmware/components/storage/storage_fs_ops.c
    ../../firmware/components/storage/storage_paths.c
    ../../firmware/components/storage/storage_transaction.c
    ../../firmware/components/storage/storage_repository_io.c
    ../../firmware/components/storage/storage_repository_json.c
    ../../firmware/components/storage/storage_json.c
    ../../firmware/components/storage/storage_repository_objects_json.c
    ../../firmware/components/storage/storage_repository_index.c
    ../../firmware/components/storage/storage_repository_sets.c
    ../../firmware/components/storage/storage_repository_lock.c
    ../../firmware/components/storage/storage_quarantine.c
)
target_include_directories(
    storage_repository_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/storage/include
            ../../firmware/components/storage
)
target_compile_definitions(
    storage_repository_tests PRIVATE STORAGE_DATA_MOUNT="${CMAKE_CURRENT_BINARY_DIR}/storage-data"
)
target_compile_options(storage_repository_tests PRIVATE ${STRICT_WARNINGS})
target_link_libraries(storage_repository_tests PRIVATE PkgConfig::CJSON test_support)
add_test(NAME storage_repository COMMAND storage_repository_tests)
set_tests_properties(storage_repository PROPERTIES LABELS "storage")

set(STORAGE_OBJECT_REPOSITORY_SOURCES
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/macro_model/macro_model.c
    ../../firmware/components/storage/storage_atomic.c
    ../../firmware/components/storage/storage_fs_ops.c
    ../../firmware/components/storage/storage_paths.c
    ../../firmware/components/storage/storage_transaction.c
    ../../firmware/components/storage/storage_repository_io.c
    ../../firmware/components/storage/storage_repository_json.c
    ../../firmware/components/storage/storage_json.c
    ../../firmware/components/storage/storage_repository_objects_json.c
    ../../firmware/components/storage/storage_repository_index.c
    ../../firmware/components/storage/storage_repository_sets.c
    ../../firmware/components/storage/storage_repository_set_operations.c
    ../../firmware/components/storage/storage_repository_lock.c
    ../../firmware/components/storage/storage_repository_order.c
    ../../firmware/components/storage/storage_repository_macros.c
    ../../firmware/components/storage/storage_repository_procedures.c
    ../../firmware/components/storage/storage_repository_progress.c
    ../../firmware/components/storage/storage_quarantine.c
)

# Register one isolated storage-object repository host test.
function(add_storage_object_repository_test target source mount_name)
    add_executable(${target} ${source} ${STORAGE_OBJECT_REPOSITORY_SOURCES})
    target_include_directories(
        ${target}
        PRIVATE ../../firmware/components/macro_model/include
                ../../firmware/components/storage/include ../../firmware/components/storage
    )
    target_compile_definitions(
        ${target} PRIVATE STORAGE_DATA_MOUNT="${CMAKE_CURRENT_BINARY_DIR}/${mount_name}"
    )
    target_compile_options(${target} PRIVATE ${STRICT_WARNINGS})
    target_link_libraries(${target} PRIVATE PkgConfig::CJSON test_support)
    add_test(NAME ${target} COMMAND ${target})
    set_tests_properties(${target} PROPERTIES LABELS "storage")
endfunction()

add_storage_object_repository_test(storage_progress_repository_tests test_storage_progress.c p15p)
add_storage_object_repository_test(
    storage_active_set_delete_tests test_storage_active_set_delete.c p15a
)

add_executable(
    provisioning_settings_tests
    test_provisioning_settings.c ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/provisioning/provisioning_core.c
)
target_include_directories(
    provisioning_settings_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/auth/include
            ../../firmware/components/wifi_ap/include
            ../../firmware/components/provisioning/include ../../firmware/components/provisioning
)
target_link_libraries(provisioning_settings_tests PRIVATE test_support)
target_compile_options(provisioning_settings_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME provisioning_settings COMMAND provisioning_settings_tests)
set_tests_properties(provisioning_settings PROPERTIES LABELS "storage")

# Phase 16 HTTP API foundation tests.
add_executable(
    web_api_core_tests
    test_web_api_core.c ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/web_server/web_api_core.c
)
target_include_directories(
    web_api_core_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/macro_parser/include
            ../../firmware/components/macro_executor/include ../../firmware/components/web_server
)
target_link_libraries(web_api_core_tests PRIVATE test_support)
target_compile_options(web_api_core_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME web_api_core COMMAND web_api_core_tests)
set_tests_properties(web_api_core PROPERTIES LABELS "web")

add_executable(
    web_request_policy_tests
    test_web_request_policy.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/web_server/web_api_core.c
    ../../firmware/components/web_server/web_request_policy.c
    ../../firmware/components/web_server/web_cookie.c
    ../../firmware/components/web_server/web_origin.c
)
target_include_directories(
    web_request_policy_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/macro_parser/include
            ../../firmware/components/macro_executor/include
            ../../firmware/components/auth/include
            ../../firmware/components/web_server
)
target_link_libraries(web_request_policy_tests PRIVATE test_support)
target_compile_options(web_request_policy_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME web_request_policy COMMAND web_request_policy_tests)
set_tests_properties(web_request_policy PROPERTIES LABELS "web")

add_executable(
    web_execution_submit_tests
    test_web_execution_submit.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/macro_model/macro_model.c
    ../../firmware/components/web_server/web_execution_submit.c
)
target_include_directories(
    web_execution_submit_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/macro_parser/include
            ../../firmware/components/macro_executor/include
            ../../firmware/components/storage/include
            ../../firmware/components/web_server
)
target_link_libraries(web_execution_submit_tests PRIVATE test_support)
target_compile_options(web_execution_submit_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME web_execution_submit COMMAND web_execution_submit_tests)
set_tests_properties(web_execution_submit PROPERTIES LABELS "web")

add_executable(
    web_execution_route_policy_tests
    test_web_execution_route_policy.c ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/web_server/web_execution_route_policy.c
)
target_include_directories(
    web_execution_route_policy_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/macro_parser/include
            ../../firmware/components/macro_executor/include ../../firmware/components/web_server
)
target_link_libraries(web_execution_route_policy_tests PRIVATE test_support)
target_compile_options(web_execution_route_policy_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME web_execution_route_policy COMMAND web_execution_route_policy_tests)
set_tests_properties(web_execution_route_policy PROPERTIES LABELS "web")

# Phase 16 strict JSON and response envelope tests.
add_executable(
    web_api_json_tests
    test_web_api_json.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/macro_model/macro_model.c
    ../../firmware/components/storage/storage_json.c
    ../../firmware/components/storage/storage_repository_json.c
    ../../firmware/components/storage/storage_repository_objects_json.c
    ../../firmware/components/web_server/web_api_json.c
)
target_include_directories(
    web_api_json_tests
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
target_link_libraries(web_api_json_tests PRIVATE PkgConfig::CJSON test_support)
target_compile_options(web_api_json_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME web_api_json COMMAND web_api_json_tests)
set_tests_properties(web_api_json PROPERTIES LABELS "web")

add_executable(
    web_api_response_tests
    test_web_api_response.c ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/web_server/web_api_response.c
)
target_include_directories(
    web_api_response_tests PRIVATE ../../firmware/components/macro_model/include
                                   ../../firmware/components/web_server
)
target_link_libraries(web_api_response_tests PRIVATE PkgConfig::CJSON test_support)
target_compile_options(web_api_response_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME web_api_response COMMAND web_api_response_tests)
set_tests_properties(web_api_response PROPERTIES LABELS "web")

add_executable(
    web_api_dispatch_tests
    test_web_api_dispatch.c ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/web_server/web_api_response.c
    ../../firmware/components/web_server/web_api_dispatch.c
)
target_include_directories(
    web_api_dispatch_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/macro_parser/include
            ../../firmware/components/macro_executor/include ../../firmware/components/web_server
)
target_link_libraries(web_api_dispatch_tests PRIVATE PkgConfig::CJSON test_support)
target_compile_options(web_api_dispatch_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME web_api_dispatch COMMAND web_api_dispatch_tests)
set_tests_properties(web_api_dispatch PROPERTIES LABELS "web")

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
