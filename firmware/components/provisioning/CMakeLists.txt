idf_component_register(
    SRCS
    "provisioning.c"
    "provisioning_core.c"
    INCLUDE_DIRS
    "include"
    PRIV_INCLUDE_DIRS
    "."
    REQUIRES
    auth
    macro_model
    nvs_flash
    wifi_ap
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
