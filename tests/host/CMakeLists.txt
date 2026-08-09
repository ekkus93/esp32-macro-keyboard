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

find_package(PkgConfig REQUIRED)
pkg_check_modules(CJSON REQUIRED IMPORTED_TARGET libcjson)

add_library(
    test_support STATIC
    support/test_assert.c
    support/test_memory.c
    support/test_temp_dir.c
    support/test_secret_sentinel.c
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
set(SECRET_SENTINEL_SCANNER "${CMAKE_SOURCE_DIR}/../../scripts/check-secret-sentinel.py")
target_compile_definitions(
    test_support PRIVATE TEST_SECRET_SENTINEL_SCANNER_PATH="${SECRET_SENTINEL_SCANNER}"
)
target_compile_options(test_support PRIVATE ${STRICT_WARNINGS})

# Link a host test executable against test_support, register it with CTest, and tag it with its
# labels.
function(register_host_test target test_name labels)
    target_link_libraries(${target} PRIVATE test_support)
    target_compile_options(${target} PRIVATE ${STRICT_WARNINGS})
    add_test(NAME ${test_name} COMMAND ${target})
    set_tests_properties(${test_name} PROPERTIES LABELS "${labels}")
endfunction()

add_executable(test_support_tests test_support.c)
register_host_test(test_support_tests test_support support)

add_executable(
    app_operation_result_tests test_app_operation_result.c
                               ../../firmware/components/support/app_operation_result.c
)
target_include_directories(
    app_operation_result_tests PRIVATE ../../firmware/components/support/include
                                       ../../firmware/components/macro_model/include
)
register_host_test(app_operation_result_tests app_operation_result support)

add_executable(
    subsystem_health_tests test_subsystem_health.c
                           ../../firmware/components/support/subsystem_health.c
)
target_include_directories(
    subsystem_health_tests PRIVATE ../../firmware/components/support/include
                                   ../../firmware/components/macro_model/include
)
register_host_test(subsystem_health_tests subsystem_health support)

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
register_host_test(macro_parser_tests macro_parser parser)

add_executable(
    macro_model_tests test_macro_model.c ../../firmware/components/macro_model/macro_model.c
)
target_include_directories(macro_model_tests PRIVATE ../../firmware/components/macro_model/include)
register_host_test(macro_model_tests macro_model model)

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
            ../../firmware/components/app_contracts_v2/include
)
register_host_test(macro_executor_tests macro_executor executor)

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
register_host_test(auth_tests auth auth)

add_executable(
    web_security_tests
    test_web_security.c ../../firmware/components/web_server/web_cookie.c
    ../../firmware/components/web_server/web_static_path.c
    ../../firmware/components/web_server/web_content.c
)
target_include_directories(
    web_security_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/auth/include
            ../../firmware/components/storage/include ../../firmware/components/web_server
)
register_host_test(web_security_tests web_security web)

add_executable(
    web_server_adapter_tests
    test_web_server_adapter.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/support/subsystem_health.c
    ../../firmware/components/web_server/web_server_adapter_common.c
    ../../firmware/components/web_server/web_server_adapter_body_auth.c
    ../../firmware/components/web_server/web_server_adapter_json.c
    ../../firmware/components/web_server/web_server_adapter_static_stream.c
    ../../firmware/components/web_server/web_server_adapter_lifecycle.c
    ../../firmware/components/web_server/web_cookie.c
    ../../firmware/components/web_server/web_static_path.c
    ../../firmware/components/web_server/web_content.c
)
target_include_directories(
    web_server_adapter_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/auth/include
            ../../firmware/components/web_server ../../firmware/components/web_server/include
            ../../firmware/components/support/include
)
register_host_test(web_server_adapter_tests web_server_adapter web)

add_executable(
    web_server_setup_submit_tests
    test_web_server_setup_submit.c ../../firmware/components/web_server/web_server_setup_submit.c
    ../../firmware/components/app_contracts_v2/setup_contract_v2.c
    ../../firmware/components/app_contracts_v2/device_settings_v2.c
)
target_include_directories(
    web_server_setup_submit_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/app_contracts_v2/include ../../firmware/components/web_server
)
target_link_libraries(web_server_setup_submit_tests PRIVATE PkgConfig::CJSON)
register_host_test(web_server_setup_submit_tests web_server_setup_submit web)

add_executable(
    settings_contract_v2_tests
    test_settings_contract_v2.c ../../firmware/components/app_contracts_v2/settings_contract_v2.c
    ../../firmware/components/app_contracts_v2/device_settings_v2.c
    ../../firmware/components/app_contracts_v2/setup_contract_v2.c
)
target_include_directories(
    settings_contract_v2_tests PRIVATE ../../firmware/components/macro_model/include
                                       ../../firmware/components/app_contracts_v2/include
)
register_host_test(settings_contract_v2_tests settings_contract_v2 web)

add_executable(
    web_status_limits_tests test_web_status_limits.c
                            ../../firmware/components/web_server/web_status_limits.c
)
target_include_directories(
    web_status_limits_tests PRIVATE ../../firmware/components/macro_model/include
                                    ../../firmware/components/web_server
)
target_link_libraries(web_status_limits_tests PRIVATE PkgConfig::CJSON)
register_host_test(web_status_limits_tests web_status_limits web)

add_executable(
    web_auth_routes_tests test_web_auth_routes.c
                          ../../firmware/components/web_server/web_auth_routes.c
)
target_include_directories(
    web_auth_routes_tests PRIVATE ../../firmware/components/macro_model/include
                                  ../../firmware/components/web_server
)
target_link_libraries(web_auth_routes_tests PRIVATE PkgConfig::CJSON)
register_host_test(web_auth_routes_tests web_auth_routes web)

add_executable(
    web_settings_tests
    test_web_settings.c ../../firmware/components/web_server/web_settings.c
    ../../firmware/components/app_contracts_v2/settings_contract_v2.c
    ../../firmware/components/app_contracts_v2/device_settings_v2.c
)
target_include_directories(
    web_settings_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/app_contracts_v2/include ../../firmware/components/web_server
)
target_link_libraries(web_settings_tests PRIVATE PkgConfig::CJSON)
register_host_test(web_settings_tests web_settings web)

add_executable(
    web_device_actions_tests test_web_device_actions.c
                             ../../firmware/components/web_server/web_device_actions.c
)
target_include_directories(
    web_device_actions_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/app_contracts_v2/include ../../firmware/components/web_server
)
target_link_libraries(web_device_actions_tests PRIVATE PkgConfig::CJSON)
register_host_test(web_device_actions_tests web_device_actions web)

add_executable(
    web_send_tests
    test_web_send.c
    ../../firmware/components/web_server/web_send.c
    ../../firmware/components/macro_parser/macro_parser.c
    ../../firmware/components/macro_parser/macro_parser_v2.c
    ../../firmware/components/macro_parser/macro_plan_v2.c
    ../../firmware/components/macro_parser/macro_keymap_us.c
    ../../firmware/components/macro_parser/macro_keymap_us_v2.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/macro_model.c
)
target_include_directories(
    web_send_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/macro_parser/include
            ../../firmware/components/macro_executor/include
            ../../firmware/components/app_contracts_v2/include
            ../../firmware/components/web_server
            fakes
)
target_link_libraries(web_send_tests PRIVATE PkgConfig::CJSON)
register_host_test(web_send_tests web_send web)

# HTTP-adapter-level tests for the blob route handlers (web_server_blob.c), exercised against a fake
# esp_http_server.h (fakes/esp_http_server_stub) and a fake storage_blob backend
# (fakes/fake_storage_blob.c) -- see test_web_server_blob.c and fakes/fake_storage_blob.h for why
# real esp_http_server / storage_blob*.c cannot be linked on a host build.
add_executable(
    web_server_blob_tests
    test_web_server_blob.c
    ../../firmware/components/web_server/web_server_blob.c
    ../../firmware/components/web_server/web_server_common.c
    ../../firmware/components/web_server/web_server_adapter_common.c
    ../../firmware/components/web_server/web_server_adapter_body_auth.c
    ../../firmware/components/web_server/web_server_adapter_static_stream.c
    ../../firmware/components/web_server/web_server_adapter_json.c
    ../../firmware/components/web_server/web_cookie.c
    ../../firmware/components/web_server/web_content.c
    ../../firmware/components/web_server/web_static_path.c
    ../../firmware/components/web_server/web_api_core.c
    ../../firmware/components/web_server/web_api_response.c
    ../../firmware/components/support/subsystem_health.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/macro_model/app_error.c
    fakes/fake_httpd.c
    fakes/fake_storage_blob.c
)
target_include_directories(
    web_server_blob_tests
    PRIVATE fakes/esp_http_server_stub
            fakes
            ../../firmware/components/web_server
            ../../firmware/components/web_server/include
            ../../firmware/components/macro_model/include
            ../../firmware/components/macro_parser/include
            ../../firmware/components/macro_executor/include
            ../../firmware/components/app_contracts_v2/include
            ../../firmware/components/auth/include
            ../../firmware/components/storage/include
            ../../firmware/components/wifi_ap/include
            ../../firmware/components/usb_keyboard/include
            ../../firmware/components/support/include
)
target_link_libraries(web_server_blob_tests PRIVATE PkgConfig::CJSON)
register_host_test(web_server_blob_tests web_server_blob web)

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
register_host_test(app_core_tests app_core startup)

add_executable(
    app_lifecycle_health_tests test_app_lifecycle_health.c
                               ../../firmware/components/support/app_lifecycle_health.c
)
target_include_directories(
    app_lifecycle_health_tests PRIVATE ../../firmware/components/support/include
                                       ../../firmware/components/macro_model/include
)
register_host_test(app_lifecycle_health_tests app_lifecycle_health startup)

add_executable(
    storage_health_tests test_storage_health.c ../../firmware/components/storage/storage_health.c
)
target_include_directories(
    storage_health_tests
    PRIVATE ../../firmware/components/storage/include ../../firmware/components/support/include
            ../../firmware/components/macro_model/include
)
register_host_test(storage_health_tests storage_health storage)

add_executable(auth_health_tests test_auth_health.c ../../firmware/components/auth/auth_health.c)
target_include_directories(
    auth_health_tests
    PRIVATE ../../firmware/components/auth/include ../../firmware/components/support/include
            ../../firmware/components/macro_model/include
)
register_host_test(auth_health_tests auth_health auth)

add_executable(
    usb_health_tests test_usb_health.c ../../firmware/components/usb_keyboard/usb_health.c
)
target_include_directories(
    usb_health_tests
    PRIVATE ../../firmware/components/usb_keyboard/include
            ../../firmware/components/support/include ../../firmware/components/macro_model/include
)
register_host_test(usb_health_tests usb_health usb)

add_executable(
    executor_health_tests test_executor_health.c
                          ../../firmware/components/macro_executor/executor_health.c
)
target_include_directories(
    executor_health_tests
    PRIVATE ../../firmware/components/macro_executor/include
            ../../firmware/components/support/include ../../firmware/components/macro_model/include
)
register_host_test(executor_health_tests executor_health executor)

add_executable(
    http_health_tests test_http_health.c ../../firmware/components/web_server/http_health.c
)
target_include_directories(
    http_health_tests
    PRIVATE ../../firmware/components/web_server/include ../../firmware/components/support/include
            ../../firmware/components/macro_model/include
)
register_host_test(http_health_tests http_health web)

add_executable(
    usb_keyboard_tests test_usb_keyboard.c
                       ../../firmware/components/usb_keyboard/usb_keyboard_state.c
)
target_include_directories(
    usb_keyboard_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/usb_keyboard/include ../../firmware/components/usb_keyboard
)
register_host_test(usb_keyboard_tests usb_keyboard usb)

add_executable(
    device_controls_tests test_device_controls.c
                          ../../firmware/components/device_controls/device_controls_logic.c
)
target_include_directories(
    device_controls_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/device_controls/include
            ../../firmware/components/device_controls ../../firmware/components/support/include
)
register_host_test(device_controls_tests device_controls controls)

add_executable(
    device_controls_reset_tests test_device_controls_reset.c
                                ../../firmware/components/device_controls/device_controls_reset.c
)
target_include_directories(
    device_controls_reset_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/device_controls/include
            ../../firmware/components/device_controls
)
register_host_test(device_controls_reset_tests device_controls_reset controls)

add_executable(wifi_ap_tests test_wifi_ap.c ../../firmware/components/wifi_ap/wifi_ap_state.c)
target_include_directories(
    wifi_ap_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/wifi_ap/include
            ../../firmware/components/wifi_ap ../../firmware/components/support/include
)
register_host_test(wifi_ap_tests wifi_ap wifi)

add_executable(
    wifi_ap_station_tests test_wifi_ap_station.c
                          ../../firmware/components/wifi_ap/wifi_ap_station.c
)
target_include_directories(
    wifi_ap_station_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/wifi_ap/include
            ../../firmware/components/wifi_ap ../../firmware/components/support/include
)
register_host_test(wifi_ap_station_tests wifi_ap_station wifi)

add_executable(
    provisioning_tests test_provisioning.c ../../firmware/components/macro_model/app_uuid.c
                       ../../firmware/components/provisioning/provisioning_core.c
)
target_include_directories(
    provisioning_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/auth/include
            ../../firmware/components/wifi_ap/include
            ../../firmware/components/provisioning/include
            ../../firmware/components/provisioning
            ../../firmware/components/support/include
)
register_host_test(provisioning_tests provisioning storage)

add_executable(
    storage_mount_tests
    test_storage_mount.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/storage/storage_mount_core.c
    ../../firmware/components/storage/storage_fs_ops.c
    ../../firmware/components/storage/storage_blob_core.c
    ../../firmware/components/storage/storage_blob_recovery.c
    ../../firmware/components/storage/storage_blob_access_core.c
    ../../firmware/components/storage/storage_blob.c
    ../../firmware/components/storage/storage_mount_topology.c
)
target_include_directories(
    storage_mount_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/storage/include
            ../../firmware/components/storage
)
register_host_test(storage_mount_tests storage_mount storage)

add_test(
    NAME storage_no_format_policy
    COMMAND
        ${CMAKE_COMMAND}
        -DSTORAGE_MOUNT_SOURCE=${CMAKE_SOURCE_DIR}/../../firmware/components/storage/storage_mount.c
        -P ${CMAKE_SOURCE_DIR}/cmake/storage_no_format_policy.cmake
)
set_tests_properties(storage_no_format_policy PROPERTIES LABELS "storage")

add_executable(
    storage_blob_tests
    test_storage_blob.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/storage/storage_fs_ops.c
    ../../firmware/components/storage/storage_blob_core.c
    ../../firmware/components/storage/storage_blob_recovery.c
    ../../firmware/components/storage/storage_blob_access_core.c
    ../../firmware/components/storage/storage_blob.c
)
target_include_directories(
    storage_blob_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/storage/include
            ../../firmware/components/storage
)
register_host_test(storage_blob_tests storage_blob storage)

add_executable(
    storage_blob_upload_tests
    test_storage_blob_upload.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/storage/storage_blob_core.c
    ../../firmware/components/storage/storage_blob_recovery.c
    ../../firmware/components/storage/storage_blob_upload_core.c
    ../../firmware/components/storage/storage_fs_ops.c
)
target_include_directories(
    storage_blob_upload_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/storage/include
            ../../firmware/components/storage
)
register_host_test(storage_blob_upload_tests storage_blob_upload storage)

add_executable(
    storage_blob_access_tests
    test_storage_blob_access.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/storage/storage_fs_ops.c
    ../../firmware/components/storage/storage_blob_core.c
    ../../firmware/components/storage/storage_blob_recovery.c
    ../../firmware/components/storage/storage_blob_access_core.c
)
target_include_directories(
    storage_blob_access_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/storage/include
            ../../firmware/components/storage
)
register_host_test(storage_blob_access_tests storage_blob_access storage)

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
register_host_test(storage_atomic_tests storage_atomic storage)

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
register_host_test(storage_parent_sync_tests storage_parent_sync storage)

add_executable(
    provisioning_settings_tests
    test_provisioning_settings.c ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/provisioning/provisioning_core.c
)
target_include_directories(
    provisioning_settings_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/auth/include
            ../../firmware/components/wifi_ap/include
            ../../firmware/components/provisioning/include
            ../../firmware/components/provisioning
            ../../firmware/components/support/include
)
register_host_test(provisioning_settings_tests provisioning_settings storage)

add_executable(
    device_settings_core_tests
    test_device_settings_core.c ../../firmware/components/device_settings/device_settings_core.c
    ../../firmware/components/app_contracts_v2/device_settings_v2.c
)
target_include_directories(
    device_settings_core_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/app_contracts_v2/include
            ../../firmware/components/device_settings/include
            ../../firmware/components/device_settings
)
register_host_test(device_settings_core_tests device_settings_core storage)

add_executable(
    device_settings_repository_isolation_tests
    test_device_settings_repository_isolation.c
    ../../firmware/components/device_settings/device_settings_core.c
    ../../firmware/components/app_contracts_v2/device_settings_v2.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/storage/storage_fs_ops.c
    ../../firmware/components/storage/storage_blob_core.c
    ../../firmware/components/storage/storage_blob_recovery.c
    ../../firmware/components/storage/storage_blob_access_core.c
    ../../firmware/components/storage/storage_blob.c
)
target_include_directories(
    device_settings_repository_isolation_tests
    PRIVATE ../../firmware/components/macro_model/include
            ../../firmware/components/app_contracts_v2/include
            ../../firmware/components/device_settings/include
            ../../firmware/components/device_settings
            ../../firmware/components/storage/include
            ../../firmware/components/storage
)
register_host_test(
    device_settings_repository_isolation_tests device_settings_repository_isolation storage
)

add_executable(
    web_api_core_tests test_web_api_core.c ../../firmware/components/macro_model/app_error.c
                       ../../firmware/components/web_server/web_api_core.c
)
target_include_directories(
    web_api_core_tests PRIVATE ../../firmware/components/macro_model/include
                               ../../firmware/components/web_server
)
register_host_test(web_api_core_tests web_api_core web)

add_executable(
    web_request_policy_tests
    test_web_request_policy.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/web_server/web_api_core.c
    ../../firmware/components/web_server/web_request_policy.c
    ../../firmware/components/web_server/web_cookie.c
)
target_include_directories(
    web_request_policy_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/auth/include
            ../../firmware/components/web_server ../../firmware/components/web_server/include
            ../../firmware/components/support/include
)
register_host_test(web_request_policy_tests web_request_policy web)

add_executable(
    web_api_json_tests test_web_api_json.c ../../firmware/components/macro_model/app_error.c
                       ../../firmware/components/web_server/web_api_json.c
)
target_include_directories(
    web_api_json_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/auth/include
            ../../firmware/components/wifi_ap/include
            ../../firmware/components/provisioning/include ../../firmware/components/web_server
)
target_link_libraries(web_api_json_tests PRIVATE PkgConfig::CJSON)
register_host_test(web_api_json_tests web_api_json web)

add_executable(
    web_api_response_tests
    test_web_api_response.c ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/web_server/web_api_response.c
)
target_include_directories(
    web_api_response_tests PRIVATE ../../firmware/components/macro_model/include
                                   ../../firmware/components/web_server
)
target_link_libraries(web_api_response_tests PRIVATE PkgConfig::CJSON)
register_host_test(web_api_response_tests web_api_response web)

add_executable(
    web_api_dispatch_tests
    test_web_api_dispatch.c ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/web_server/web_api_response.c
    ../../firmware/components/web_server/web_api_dispatch.c
)
target_include_directories(
    web_api_dispatch_tests
    PRIVATE ../../firmware/components/macro_model/include ../../firmware/components/auth/include
            ../../firmware/components/web_server
)
target_link_libraries(web_api_dispatch_tests PRIVATE PkgConfig::CJSON)
register_host_test(web_api_dispatch_tests web_api_dispatch web)

add_executable(app_crc32_tests test_app_crc32.c ../../firmware/components/support/app_crc32.c)
target_include_directories(app_crc32_tests PRIVATE ../../firmware/components/support/include)
register_host_test(app_crc32_tests app_crc32 support)
