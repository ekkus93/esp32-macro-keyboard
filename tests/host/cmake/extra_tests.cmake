target_include_directories(
    app_core_tests
    PRIVATE "${CMAKE_SOURCE_DIR}/../../firmware/components/wifi_ap/include"
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
    web_setup_tests
    "${CMAKE_SOURCE_DIR}/test_web_setup.c"
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
