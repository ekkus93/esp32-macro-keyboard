idf_component_register(
    SRCS
    "factory_reset_recovery.c"
    "factory_reset_recovery_engine.c"
    INCLUDE_DIRS
    "include"
    PRIV_INCLUDE_DIRS
    "."
    REQUIRES
    macro_model
    factory_reset_state
    device_settings
    storage
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
