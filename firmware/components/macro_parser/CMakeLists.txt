idf_component_register(
    SRCS "macro_parser.c" "macro_keymap_us.c"
    INCLUDE_DIRS "include"
    REQUIRES macro_model
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion
    -Wformat=2 -Wundef -Wdouble-promotion -Wmissing-declarations
    -Wstrict-prototypes
)
