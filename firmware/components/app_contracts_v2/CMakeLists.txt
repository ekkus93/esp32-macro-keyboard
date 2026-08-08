idf_component_register(
    SRCS
    "device_settings_v2.c"
    "setup_contract_v2.c"
    "settings_contract_v2.c"
    INCLUDE_DIRS
    "include"
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
