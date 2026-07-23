idf_component_register(
    SRCS "app_error.c" "app_uuid.c" "macro_model.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_hw_support
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion
    -Wformat=2 -Wundef -Wdouble-promotion -Wmissing-declarations
    -Wstrict-prototypes
)
