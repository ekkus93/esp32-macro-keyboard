cmake_minimum_required(VERSION 3.20)
project(esp32_macro_keyboard_host_tests C)

enable_testing()
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(macro_parser_tests
    test_macro_parser.c
    ../../firmware/components/macro_model/app_error.c
    ../../firmware/components/macro_model/app_uuid.c
    ../../firmware/components/macro_model/macro_model.c
    ../../firmware/components/macro_parser/macro_parser.c
    ../../firmware/components/macro_parser/macro_keymap_us.c
)
target_include_directories(macro_parser_tests PRIVATE
    ../../firmware/components/macro_model/include
    ../../firmware/components/macro_parser/include
)
target_compile_options(macro_parser_tests PRIVATE
    -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion
    -Wformat=2 -Wundef -Wdouble-promotion -Wmissing-declarations
    -Wstrict-prototypes
)
add_test(NAME macro_parser COMMAND macro_parser_tests)
