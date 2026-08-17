target_include_directories(
    app_core_tests
    PRIVATE "${CMAKE_SOURCE_DIR}/../../firmware/components/wifi_ap/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/provisioning/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/app_contracts_v2/include"
)

add_executable(
    provisioning_bootstrap_tests
    "${CMAKE_SOURCE_DIR}/test_provisioning_bootstrap.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/provisioning/provisioning_bootstrap_core.c"
)
target_include_directories(
    provisioning_bootstrap_tests
    PRIVATE "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_parser/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/wifi_ap/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/provisioning/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/provisioning"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/support/include"
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
            "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_parser/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/auth/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/wifi_ap/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/provisioning/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/support/include"
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
            "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_parser/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/auth/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/wifi_ap/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/provisioning/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/support/include"
)
target_link_libraries(web_setup_json_tests PRIVATE PkgConfig::CJSON test_support)
target_compile_options(web_setup_json_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME web_setup_json COMMAND web_setup_json_tests)
set_tests_properties(web_setup_json PROPERTIES LABELS "web")

# Round 2 F-014: route/composition targets keep their existing narrow host
# boundaries and use a direct fake for the new password-record accessor. The
# dedicated target below links the real accessor against a pthread-backed
# portMUX stand-in and is the concurrency proof for the production logic.
set(
    WEB_PASSWORD_RECORD_FAKE
    "${CMAKE_SOURCE_DIR}/fakes/fake_web_server_password_record.c"
)
foreach(
    target
    IN
    ITEMS web_api_administration_tests
          web_server_administration_route_tests
          web_server_async_confirmation_tests
          web_server_lifecycle_tests
)
    target_sources(${target} PRIVATE "${WEB_PASSWORD_RECORD_FAKE}")
endforeach()

find_package(Threads REQUIRED)
add_executable(
    web_server_password_record_tests
    "${CMAKE_SOURCE_DIR}/test_web_server_password_record.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server/web_server_password_record.c"
)
target_include_directories(
    web_server_password_record_tests
    PRIVATE "${CMAKE_SOURCE_DIR}/fakes/freertos_lock_stub"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/auth/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/app_contracts_v2/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server/include"
)
target_link_libraries(web_server_password_record_tests PRIVATE test_support Threads::Threads)
target_compile_options(web_server_password_record_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME web_server_password_record COMMAND web_server_password_record_tests)
set_tests_properties(web_server_password_record PROPERTIES LABELS "auth;web")

add_test(
    NAME web_server_password_record_access_guard
    COMMAND
        ${CMAKE_COMMAND}
        -DWEB_SERVER_SOURCE_DIR=${CMAKE_SOURCE_DIR}/../../firmware/components/web_server
        -P ${CMAKE_SOURCE_DIR}/cmake/web_server_password_record_access_guard.cmake
)
set_tests_properties(web_server_password_record_access_guard PROPERTIES LABELS "auth;web")

# Round 2 F-015: inspect cJSON allocator-owned strings at free-time so the
# regression fails unless both mutation handlers scrub parsed secrets before
# cJSON releases those heap allocations.
add_executable(
    web_parsed_secret_wipe_tests
    "${CMAKE_SOURCE_DIR}/test_web_parsed_secret_wipe.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server/web_settings.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server/web_device_actions.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/app_contracts_v2/settings_contract_v2.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/app_contracts_v2/device_settings_v2.c"
)
target_include_directories(
    web_parsed_secret_wipe_tests
    PRIVATE "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/app_contracts_v2/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/device_controls/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server/include"
)
target_link_libraries(web_parsed_secret_wipe_tests PRIVATE PkgConfig::CJSON test_support)
target_compile_options(web_parsed_secret_wipe_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME web_parsed_secret_wipe COMMAND web_parsed_secret_wipe_tests)
set_tests_properties(web_parsed_secret_wipe PROPERTIES LABELS "web")

# H2-022/H2-023: compose the host-testable password-change transaction with
# the real auth-core password/session implementation. This proves immediate
# old/new password authority, all-session invalidation, normal new-session
# lifetime, partial-commit semantics, and retry behavior.
add_executable(
    web_change_password_transaction_tests
    "${CMAKE_SOURCE_DIR}/test_web_change_password_transaction.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server/web_settings.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/auth/auth_core_common.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/auth/auth_core_password.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/auth/auth_core_session.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/auth/auth_core_rate_limit.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/app_contracts_v2/settings_contract_v2.c"
    "${CMAKE_SOURCE_DIR}/../../firmware/components/app_contracts_v2/device_settings_v2.c"
)
target_include_directories(
    web_change_password_transaction_tests
    PRIVATE "${CMAKE_SOURCE_DIR}"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/macro_model/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/auth/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/auth"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/app_contracts_v2/include"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server"
            "${CMAKE_SOURCE_DIR}/../../firmware/components/web_server/include"
)
target_link_libraries(web_change_password_transaction_tests PRIVATE PkgConfig::CJSON test_support)
target_compile_options(web_change_password_transaction_tests PRIVATE ${STRICT_WARNINGS})
add_test(NAME web_change_password_transaction COMMAND web_change_password_transaction_tests)
set_tests_properties(web_change_password_transaction PROPERTIES LABELS "auth;web")
