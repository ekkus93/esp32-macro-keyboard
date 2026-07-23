idf_component_register(
    SRCS "app_core.c"
    INCLUDE_DIRS "include"
    REQUIRES macro_model storage usb_keyboard macro_executor device_controls wifi_ap auth web_server nvs_flash
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion
    -Wformat=2 -Wundef -Wdouble-promotion -Wmissing-declarations
    -Wstrict-prototypes
)
