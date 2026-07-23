idf_component_register(
    SRCS "usb_keyboard.c" "usb_descriptors.c"
    INCLUDE_DIRS "include"
    REQUIRES macro_model esp_tinyusb tinyusb
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion
    -Wformat=2 -Wundef -Wdouble-promotion -Wmissing-declarations
    -Wstrict-prototypes
)
