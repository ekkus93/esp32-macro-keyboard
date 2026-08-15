#include "app_core.h"

#include "app_error.h"
#include "esp_log.h"
#include "serial_console.h"

static const char *const TAG = "app_main";

void app_main(void);

void app_main(void) {
    /* The trusted physical maintenance console is a prerequisite for the v2
     * development-appliance profile (SPEC_V2.md §12.4): unprovisioned setup
     * depends on its explicit setup-code command, and confirmation/cancel use
     * the same local surface. Do not start network/application services if the
     * console itself is unavailable. */
    const app_error_code_t console_result = serial_console_start();
    if (console_result != APP_ERROR_NONE) {
        ESP_LOGE(TAG, "serial console startup failed: %s", app_error_code_string(console_result));
        return;
    }

    /* The console is already running when app_core_start() begins, so it stays
     * available for local diagnostics if application startup fails. It is on
     * UART0 rather than USB-Serial-JTAG; see serial_console.c. */
    const app_error_code_t result = app_core_start();
    if (result != APP_ERROR_NONE) {
        ESP_LOGE(TAG, "startup failed: %s", app_error_code_string(result));
    }
}
