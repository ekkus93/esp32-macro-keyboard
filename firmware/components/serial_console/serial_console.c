#include "serial_console.h"

#include <stdio.h>

#include "app_error.h"
#include "device_controls.h"
#include "esp_console.h"
#include "macro_executor.h"
#include "provisioning.h"
#include "wifi_ap.h"

#define WIFI_CONNECT_TIMEOUT_MS 15000U
#define MILLISECONDS_PER_SECOND 1000U

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
        /* Persist so the device rejoins on its own after a power cycle. Stored
         * in the same NVS record as everything else it remembers. */
        const app_error_code_t saved = provisioning_set_station(argv[1], argv[2]);
        printf(saved == APP_ERROR_NONE ? "credentials saved; will reconnect at boot\n"
                                       : "warning: connected but could not save credentials: %s\n",
               app_error_code_string(saved));
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
 * confirm and cancel buttons ever did - so a bare board with a USB cable is a
 * complete product. */
static int command_confirm(int argc, char **argv) {
    (void)argc;
    (void)argv;
    const app_error_code_t result = device_controls_signal_confirmation();
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
    esp_console_repl_t *repl = NULL;
    esp_console_repl_config_t repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
    repl_config.prompt = "keyboard> ";

    /* Bind to UART0 (the pins the devkit's separate USB-to-UART bridge port
     * exposes), NOT to USB-Serial-JTAG. This is load-bearing, not a
     * preference: on the ESP32-S3 the USB-Serial-JTAG peripheral and the
     * USB-OTG controller share one internal USB PHY and one pair of pins
     * (GPIO19/20). Binding this console to USB-Serial-JTAG holds that PHY
     * and stops TinyUSB from enumerating the device as a USB HID keyboard -
     * i.e. it silently disables the product's entire reason for existing.
     * Verified on real hardware: with the USB-Serial-JTAG console enabled
     * the native port stayed 303a:1001 (debug unit) and startup stalled at
     * the `usb` stage; with the console on UART0 the same build enumerates
     * as 303a:4001 "ESP32 Macro Keyboard", HID v1.11 Keyboard.
     *
     * Practical consequence: reaching this console needs a cable on the
     * devkit's UART port, while the native USB port is busy being the
     * keyboard. Both work at once; they are different connectors. */
    esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();
    if (esp_console_new_repl_uart(&uart_config, &repl_config, &repl) != ESP_OK) {
        return APP_ERROR_INTERNAL;
    }

    register_commands();

    return esp_console_start_repl(repl) == ESP_OK ? APP_ERROR_NONE : APP_ERROR_INTERNAL;
}
