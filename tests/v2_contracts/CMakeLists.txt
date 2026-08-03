cmake_minimum_required(VERSION 3.22)
project(esp32_macro_keyboard_v2_contracts C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(V2_CONTRACT_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/components/app_contracts_v2/include
)
set(V2_PARSER_SOURCE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/components/macro_parser
)
set(V2_PARSER_SOURCES
    ${V2_PARSER_SOURCE_DIR}/macro_parser_v2.c
    ${V2_PARSER_SOURCE_DIR}/macro_plan_v2.c
    ${V2_PARSER_SOURCE_DIR}/macro_keymap_us.c
    ${V2_PARSER_SOURCE_DIR}/macro_keymap_us_v2.c
)
set(V2_MACRO_CORPUS_JSON
    ${CMAKE_CURRENT_SOURCE_DIR}/../../contracts/v2/macro-conformance.json
)
set(V2_MACRO_CORPUS_GENERATOR
    ${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/generate-v2-macro-corpus.py
)
set(V2_GENERATED_MACRO_CORPUS
    ${CMAKE_CURRENT_BINARY_DIR}/generated_macro_corpus.inc
)

set_source_files_properties(
    ${V2_PARSER_SOURCE_DIR}/macro_parser_v2.c
    PROPERTIES COMPILE_DEFINITIONS
               "macro_keymap_us_named=macro_keymap_us_named_v2_canonical"
)

add_custom_command(
    OUTPUT ${V2_GENERATED_MACRO_CORPUS}
    COMMAND
        ${Python3_EXECUTABLE}
        ${V2_MACRO_CORPUS_GENERATOR}
        --input
        ${V2_MACRO_CORPUS_JSON}
        --output
        ${V2_GENERATED_MACRO_CORPUS}
    DEPENDS
        ${V2_MACRO_CORPUS_JSON}
        ${V2_MACRO_CORPUS_GENERATOR}
    COMMENT "Generating native v2 macro conformance cases"
    VERBATIM
)
set_source_files_properties(${V2_GENERATED_MACRO_CORPUS} PROPERTIES HEADER_FILE_ONLY TRUE)

add_executable(
    v2_macro_conformance_tests
    test_macro_conformance.c
    ${V2_GENERATED_MACRO_CORPUS}
    ${V2_PARSER_SOURCES}
)

target_include_directories(
    v2_macro_conformance_tests
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${V2_CONTRACT_INCLUDE_DIR}
        ${V2_PARSER_SOURCE_DIR}/include
        ../../firmware/components/macro_model/include
)

add_executable(
    v2_macro_canonical_tokens_tests
    test_macro_canonical_tokens.c
    ${V2_PARSER_SOURCES}
)

target_include_directories(
    v2_macro_canonical_tokens_tests
    PRIVATE
        ${V2_CONTRACT_INCLUDE_DIR}
        ${V2_PARSER_SOURCE_DIR}/include
        ../../firmware/components/macro_model/include
)

add_executable(
    v2_device_settings_tests
    test_device_settings.c
    ../../firmware/components/app_contracts_v2/device_settings_v2.c
)

target_include_directories(v2_device_settings_tests PRIVATE ${V2_CONTRACT_INCLUDE_DIR})

foreach(
    target
    v2_macro_conformance_tests
    v2_macro_canonical_tokens_tests
    v2_device_settings_tests
)
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
add_test(NAME v2_macro_conformance COMMAND v2_macro_conformance_tests)
add_test(NAME v2_macro_canonical_tokens COMMAND v2_macro_canonical_tokens_tests)
add_test(NAME v2_device_settings COMMAND v2_device_settings_tests)
