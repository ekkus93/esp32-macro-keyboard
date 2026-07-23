idf_component_register(
    SRCS "wifi_ap.c"
    INCLUDE_DIRS "include"
    REQUIRES macro_model esp_wifi esp_event esp_netif
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wall -Wextra -Werror -Wshadow -Wconversion -Wsign-conversion
    -Wformat=2 -Wundef -Wdouble-promotion -Wmissing-declarations
    -Wstrict-prototypes
)
