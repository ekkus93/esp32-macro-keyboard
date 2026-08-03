cmake_minimum_required(VERSION 3.22)
project(esp32_macro_keyboard_v2_contracts C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CJSON REQUIRED IMPORTED_TARGET libcjson)

add_executable(
    v2_macro_conformance_tests
    test_macro_conformance.c
    ../../firmware/components/macro_parser/macro_parser.c
    ../../firmware/components/macro_parser/macro_keymap_us.c
)

target_include_directories(
    v2_macro_conformance_tests
    PRIVATE
        ../../firmware/components/macro_parser/include
        ../../firmware/components/macro_model/include
)

target_link_libraries(v2_macro_conformance_tests PRIVATE PkgConfig::CJSON)

target_compile_options(
    v2_macro_conformance_tests
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

enable_testing()
add_test(
    NAME v2_macro_conformance
    COMMAND
        v2_macro_conformance_tests
        ${CMAKE_CURRENT_SOURCE_DIR}/../../contracts/v2/macro-conformance.json
)
