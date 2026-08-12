idf_component_register(
    SRCS
    "factory_reset_state.c"
    "factory_reset_state_core.c"
    INCLUDE_DIRS
    "include"
    PRIV_INCLUDE_DIRS
    "."
    REQUIRES
    macro_model
    nvs_flash
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
