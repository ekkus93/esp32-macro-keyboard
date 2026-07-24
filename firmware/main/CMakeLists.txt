idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES app_core
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion
    -Wformat=2 -Wdouble-promotion -Wmissing-declarations
    -Wstrict-prototypes
)
