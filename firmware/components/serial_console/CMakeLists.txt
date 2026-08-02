idf_component_register(
    SRCS
    "serial_console.c"
    INCLUDE_DIRS
    "include"
    REQUIRES
    macro_model
    device_controls
    macro_executor
    wifi_ap
    console
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
