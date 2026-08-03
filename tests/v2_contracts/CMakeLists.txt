cmake_minimum_required(VERSION 3.22)
project(esp32_macro_keyboard_v2_contracts C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CJSON REQUIRED IMPORTED_TARGET libcjson)

set(V2_CONTRACT_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/components/app_contracts_v2/include
)

add_executable(
    v2_macro_conformance_tests
    test_macro_conformance.c
    ../../firmware/components/macro_parser/macro_parser_v2.c
    ../../firmware/components/macro_parser/macro_plan_v2.c
    ../../firmware/components/macro_parser/macro_keymap_us.c
)

target_include_directories(
    v2_macro_conformance_tests
    PRIVATE
        ${V2_CONTRACT_INCLUDE_DIR}
        ../../firmware/components/macro_parser/include
        ../../firmware/components/macro_model/include
)

target_compile_definitions(
    v2_macro_conformance_tests
    PRIVATE
        macro_compile=macro_compile_v2
        macro_plan_free=macro_plan_v2_free
)

target_link_libraries(v2_macro_conformance_tests PRIVATE PkgConfig::CJSON)

add_executable(
    v2_device_settings_tests
    test_device_settings.c
    ../../firmware/components/app_contracts_v2/device_settings_v2.c
)

target_include_directories(v2_device_settings_tests PRIVATE ${V2_CONTRACT_INCLUDE_DIR})

foreach(target v2_macro_conformance_tests v2_device_settings_tests)
    target_compile_options(
        ${target}
        PRIVATE
            -Wall
            -Wextra
            -Werror
            -Wshadow
            -Wconversion
            -Wsign-conversion
            -Wformat=2
            -Wdouble-promotion
            -Wstrict-prototypes
    )
endforeach()

enable_testing()
add_test(
    NAME v2_macro_conformance
    COMMAND
        v2_macro_conformance_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/../../contracts/v2/macro-conformance.json
)
add_test(NAME v2_device_settings COMMAND v2_device_settings_tests)
