idf_component_register(
    SRCS
    "app_operation_result.c"
    "subsystem_health.c"
    INCLUDE_DIRS
    "include"
    REQUIRES
    macro_model
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
