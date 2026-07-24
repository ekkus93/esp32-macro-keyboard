idf_component_register(
    SRCS
        "macro_executor.c"
        "macro_executor_engine.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "."
    REQUIRES macro_model macro_parser usb_keyboard freertos
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion
    -Wformat=2 -Wdouble-promotion -Wmissing-declarations
    -Wstrict-prototypes
)
