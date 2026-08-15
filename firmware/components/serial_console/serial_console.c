#include "serial_console.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "device_controls.h"
#include "device_settings.h"
#include "driver/uart.h"
#include "esp_console.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "macro_executor.h"
#include "serial_console_confirmation.h"
#include "setup_contract_v2.h"
#include "wifi_ap.h"

#define WIFI_CONNECT_TIMEOUT_MS 15000U
#define MILLISECONDS_PER_SECOND 1000U
#define SETUP_CODE_OUTPUT_BUFFER_BYTES 32U

static char current_setup_code[APP_V2_SETUP_CODE_BUFFER_BYTES];
static SemaphoreHandle_t setup_code_mutex;

static bool ensure_setup_code_mutex(void) {
    if (setup_code_mutex == NULL) {
        setup_code_mutex = xSemaphoreCreateMutex();
    }
    return setup_code_mutex != NULL;
}

static bool setup_code_valid(const char *setup_code) {
    if (setup_code == NULL) {
        return false;
    }
    for (size_t index = 0U; index < APP_V2_SETUP_CODE_DIGITS; ++index) {
        if (setup_code[index] < '0' || setup_code[index] > '9') {
            return false;
        }
    }
    return setup_code[APP_V2_SETUP_CODE_DIGITS] == '\0';
}

app_error_code_t serial_console_publish_setup_code(const char *setup_code) {
    if (!setup_code_valid(setup_code)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    if (setup_code_mutex == NULL || xSemaphoreTake(setup_code_mutex, portMAX_DELAY) != pdTRUE) {
        return APP_ERROR_INTERNAL;
    }
    memcpy(current_setup_code, setup_code, sizeof(current_setup_code));
    xSemaphoreGive(setup_code_mutex);
    return APP_ERROR_NONE;
}

void serial_console_clear_setup_code(void) {
    if (setup_code_mutex == NULL) {
        memset(current_setup_code, 0, sizeof(current_setup_code));
        return;
    }
    /* Once a code can have been published, failure to acquire this mutex must
     * never degrade into retaining the secret silently. With portMAX_DELAY this
     * is an invariant failure, so panic/reboot rather than continuing with stale
     * disclosure authority. */
    configASSERT(xSemaphoreTake(setup_code_mutex, portMAX_DELAY) == pdTRUE);
    memset(current_setup_code, 0, sizeof(current_setup_code));
    xSemaphoreGive(setup_code_mutex);
}

static void clear_local_text(char *text, size_t length) {
    volatile char *bytes = text;
    for (size_t index = 0U; index < length; ++index) {
        bytes[index] = '\0';
    }
}

static int command_setup_code(int argc, char **argv) {
    (void)argv;
    if (argc != 1) {
        printf("usage: setup-code\n");
        return 1;
    }

    char setup_code[APP_V2_SETUP_CODE_BUFFER_BYTES] = {0};
    if (setup_code_mutex == NULL || xSemaphoreTake(setup_code_mutex, portMAX_DELAY) != pdTRUE) {
        printf("setup code unavailable; device is not in setup mode\n");
        return 1;
    }
    memcpy(setup_code, current_setup_code, sizeof(setup_code));
    if (!setup_code_valid(setup_code)) {
        clear_local_text(setup_code, sizeof(setup_code));
        xSemaphoreGive(setup_code_mutex);
        printf("setup code unavailable; device is not in setup mode\n");
        return 1;
    }

    /* Do not use stdout for this disclosure. ESP-IDF can mirror stdout/stderr
     * to a secondary USB-Serial-JTAG console even though the REPL input lives
     * on UART0. The setup code is intentionally available only on the trusted
     * physical UART command surface, so write its response directly to UART0.
     * Hold the mutex until the direct write has copied the data so a successful
     * HTTP setup cannot retire the authority while this command is emitting it. */
    char output[SETUP_CODE_OUTPUT_BUFFER_BYTES] = {0};
    const int output_length = snprintf(output, sizeof(output), "setup code: %s\n", setup_code);
    if (output_length <= 0 || (size_t)output_length >= sizeof(output)) {
        clear_local_text(output, sizeof(output));
        clear_local_text(setup_code, sizeof(setup_code));
        xSemaphoreGive(setup_code_mutex);
        printf("setup code output failed\n");
        return 1;
    }
    const int written = uart_write_bytes(UART_NUM_0, output, (size_t)output_length);
    clear_local_text(output, sizeof(output));
    clear_local_text(setup_code, sizeof(setup_code));
    xSemaphoreGive(setup_code_mutex);
    if (written != output_length) {
        printf("setup code output failed\n");
        return 1;
    }
    return 0;
}

static int command_wifi_connect(int argc, char **argv) {
    if (argc != 3) {
        printf("usage: wifi-connect <ssid> <password>\n");
        return 1;
    }
    printf("connecting to \"%s\" (up to %us)...\n", argv[1],
           WIFI_CONNECT_TIMEOUT_MS / MILLISECONDS_PER_SECOND);
    char ip_address[WIFI_STA_IP_STRING_BYTES];
    const app_error_code_t result = wifi_ap_connect_station(
        argv[1], argv[2], WIFI_CONNECT_TIMEOUT_MS, ip_address, sizeof(ip_address));
    if (result == APP_ERROR_NONE) {
        printf("connected, IP address: %s\n", ip_address);
        bool changed = false;
        const app_error_code_t saved = device_settings_set_station(argv[1], argv[2], &changed);
        if (saved != APP_ERROR_NONE) {
            printf("connection established but credentials were not persisted: %s; "
                   "reboot will not reconnect\n",
                   app_error_code_string(saved));
            return 1;
        }
        printf(changed ? "credentials saved; will reconnect at boot\n"
                       : "credentials already current; will reconnect at boot\n");
        return 0;
    }
    printf("connection failed: %s\n", app_error_code_string(result));
    return 1;
}

static int command_wifi_status(int argc, char **argv) {
    (void)argc;
    (void)argv;
    const wifi_ap_status_t status = wifi_ap_get_status();
    static const char *const state_names[] = {
        [WIFI_AP_STOPPED] = "stopped",
        [WIFI_AP_STARTING] = "starting",
        [WIFI_AP_READY] = "ready",
        [WIFI_AP_ERROR] = "error",
    };
    printf("AP state: %s, clients: %u, last error: %s\n", state_names[status.state],
           (unsigned)status.client_count, app_error_code_string(status.last_error));
    return 0;
}

/* The device deliberately requires no buttons and no added hardware. These two
 * commands provide, over the UART console, the only two things the physical
 * confirm and cancel buttons ever did. */
static app_error_code_t confirm_send_adapter(void *context) {
    (void)context;
    return macro_executor_confirm();
}

static app_error_code_t confirm_device_action_adapter(void *context) {
    (void)context;
    return device_controls_signal_confirmation();
}

static int command_confirm(int argc, char **argv) {
    (void)argc;
    (void)argv;
    const serial_console_confirmation_ops_t operations = {
        .context = NULL,
        .confirm_send = confirm_send_adapter,
        .confirm_device_action = confirm_device_action_adapter,
    };
    const app_error_code_t result = serial_console_route_confirmation(&operations);
    if (result == APP_ERROR_NONE) {
        printf("confirmation sent\n");
        return 0;
    }
    printf("confirmation failed: %s\n", app_error_code_string(result));
    return 1;
}

static int command_cancel(int argc, char **argv) {
    (void)argc;
    (void)argv;
    const app_error_code_t result = macro_executor_cancel();
    if (result == APP_ERROR_NONE) {
        printf("cancellation requested\n");
        return 0;
    }
    if (result == APP_ERROR_NOT_FOUND) {
        printf("nothing is executing\n");
        return 0;
    }
    printf("cancellation failed: %s\n", app_error_code_string(result));
    return 1;
}

static void register_commands(void) {
    const esp_console_cmd_t setup_code_command = {
        .command = "setup-code",
        .help = "Reveal the current one-time setup code on this physical UART console.",
        .hint = NULL,
        .func = command_setup_code,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&setup_code_command));

    const esp_console_cmd_t confirm_command = {
        .command = "confirm",
        .help = "Give physical confirmation for a pending request, in place of "
                "pressing the confirm button.",
        .hint = NULL,
        .func = command_confirm,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&confirm_command));

    const esp_console_cmd_t cancel_command = {
        .command = "cancel",
        .help = "Cancel the running macro, in place of pressing the cancel button.",
        .hint = NULL,
        .func = command_cancel,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&cancel_command));

    const esp_console_cmd_t wifi_connect_command = {
        .command = "wifi-connect",
        .help = "Join an existing Wi-Fi network in station mode (the device's own "
                "SoftAP keeps running alongside it).",
        .hint = "<ssid> <password>",
        .func = command_wifi_connect,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&wifi_connect_command));

    const esp_console_cmd_t wifi_status_command = {
        .command = "wifi-status",
        .help = "Show the device's own SoftAP state and client count.",
        .hint = NULL,
        .func = command_wifi_status,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&wifi_status_command));
}

app_error_code_t serial_console_start(void) {
    if (!ensure_setup_code_mutex()) {
        return APP_ERROR_INTERNAL;
    }

    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "keyboard> ";

    /* Bind to UART0 (the pins the devkit's separate USB-to-UART bridge port
     * exposes), NOT to USB-Serial-JTAG. On the ESP32-S3 the USB-Serial-JTAG
     * peripheral and the USB-OTG controller share one internal USB PHY and one
     * pair of pins. Binding this console to USB-Serial-JTAG prevents TinyUSB
     * from enumerating the device as a USB HID keyboard. */
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    if (esp_console_new_repl_uart(&uart_config, &repl_config, &repl) != ESP_OK) {
        return APP_ERROR_INTERNAL;
    }

    register_commands();

    return esp_console_start_repl(repl) == ESP_OK ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}
