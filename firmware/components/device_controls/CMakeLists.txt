idf_component_register(
    SRCS "device_controls.c"
    INCLUDE_DIRS "include"
    REQUIRES macro_model macro_executor driver
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion
    -Wformat=2 -Wundef -Wdouble-promotion -Wmissing-declarations
    -Wstrict-prototypes
)
