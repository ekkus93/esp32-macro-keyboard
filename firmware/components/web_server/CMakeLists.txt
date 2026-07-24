idf_component_register(
    SRCS
        "web_server.c"
        "web_server_adapter.c"
        "web_cookie.c"
        "web_origin.c"
        "web_static_path.c"
        "web_content.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "."
    REQUIRES macro_model macro_executor usb_keyboard wifi_ap auth storage esp_http_server json
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion
    -Wformat=2 -Wdouble-promotion -Wmissing-declarations
    -Wstrict-prototypes
)
