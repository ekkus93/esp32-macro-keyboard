#include "app_core.h"

#include "app_error.h"
#include "esp_log.h"
#include "serial_console.h"

static const char *const TAG = "app_main";

void app_main(void);

void app_main(void) {
    const app_error_code_t result = app_core_start();
    if (result != APP_ERROR_NONE) {
        ESP_LOGE(TAG, "startup failed: %s", app_error_code_string(result));
    }

    /* Debug/development console on UART0, added at the repository owner's
     * explicit request. Started unconditionally - even after a startup
     * failure - so it stays reachable exactly when it is most needed.
     * Deliberately NOT on USB-Serial-JTAG; see serial_console.c. */
    const app_error_code_t console_result = serial_console_start();
    if (console_result != APP_ERROR_NONE) {
        ESP_LOGE(TAG, "serial console startup failed: %s", app_error_code_string(console_result));
    }
}
