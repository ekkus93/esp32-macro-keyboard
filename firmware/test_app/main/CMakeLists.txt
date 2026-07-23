idf_component_register(
    SRCS
        "test_main.c"
        "test_uuid.c"
        "test_macro_parser.c"
        "test_limits.c"
        "test_macro_executor.c"
        "test_auth.c"
    INCLUDE_DIRS "."
    REQUIRES
        unity
        macro_model
        macro_parser
        macro_executor
        auth
)

target_compile_options(${COMPONENT_LIB} PRIVATE
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
