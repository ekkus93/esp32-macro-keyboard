idf_component_register(
    SRCS
        "usb_keyboard.c"
        "usb_keyboard_state.c"
        "usb_descriptors.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "."
    REQUIRES macro_model esp_tinyusb tinyusb
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion
    -Wformat=2 -Wdouble-promotion -Wmissing-declarations
    -Wstrict-prototypes
)
