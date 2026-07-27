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
