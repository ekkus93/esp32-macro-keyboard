idf_component_register(
    SRCS
    "macro_parser.c"
    "macro_parser_v2.c"
    "macro_plan_v2.c"
    "macro_keymap_us.c"
    INCLUDE_DIRS
    "include"
    REQUIRES
    app_contracts_v2
    macro_model
)

target_include_directories(
    ${COMPONENT_LIB} SYSTEM PRIVATE "$ENV{IDF_PATH}/components/newlib/platform_include"
)

target_compile_options(
    ${COMPONENT_LIB}
    PRIVATE -Wall
            -Wextra
            -Werror
            -Wshadow
            -Wconversion
            -Wsign-conversion
            -Wformat=2
            -Wdouble-promotion
            -Wmissing-declarations
            -Wstrict-prototypes
)
