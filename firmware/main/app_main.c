#include "app_core.h"
#include "esp_log.h"

static const char *const TAG = "app_main";

void app_main(void);

void app_main(void)
{
    const app_error_code_t result = app_core_start();
    if (result != APP_ERROR_NONE) {
        ESP_LOGE(TAG, "startup failed: %s", app_error_code_string(result));
    }
}
