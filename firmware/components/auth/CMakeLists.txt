idf_component_register(
    SRCS
        "auth.c"
        "auth_core.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "."
    REQUIRES macro_model esp_timer mbedtls freertos
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion
    -Wformat=2 -Wdouble-promotion -Wmissing-declarations
    -Wstrict-prototypes
)
