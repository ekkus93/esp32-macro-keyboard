idf_component_register(
    SRCS "macro_executor.c"
    INCLUDE_DIRS "include"
    REQUIRES macro_model macro_parser usb_keyboard freertos
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion
    -Wformat=2 -Wundef -Wdouble-promotion -Wmissing-declarations
    -Wstrict-prototypes
)
