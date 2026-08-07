idf_component_register(
    SRCS
    "device_settings.c"
    "device_settings_core.c"
    INCLUDE_DIRS
    "include"
    PRIV_INCLUDE_DIRS
    "."
    REQUIRES
    app_contracts_v2
    macro_model
    nvs_flash
    freertos
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
