#include "serial_console.h"

#include <stdio.h>
#include <string.h>

#include "app_error.h"
#include "device_controls.h"
#include "esp_console.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "macro_executor.h"
#include "provisioning.h"
#include "serial_console_confirmation.h"
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

/* Clears the administrator password and AP credentials while retaining the
 * device settings and saved station network. Factory reset remains the
 * destructive path. Console-only access is deliberate: possession of the
 * board and UART connection is the authorization boundary. */
#define CREDENTIAL_RESET_CONFIRMATION "confirm"
#define CREDENTIAL_RESET_FLUSH_MS 250U

static int command_credential_reset(int argc, char **argv) {
    if (argc != 2 || strcmp(argv[1], CREDENTIAL_RESET_CONFIRMATION) != 0) {
        printf("usage: credential-reset " CREDENTIAL_RESET_CONFIRMATION "\n");
        printf("clears the administrator password and AP credentials, then restarts\n");
        printf("into setup. Settings and the saved Wi-Fi network are kept.\n");
        return 1;
    }
    const app_error_code_t result = provisioning_clear_credentials();
    if (result != APP_ERROR_NONE) {
        printf("credential reset failed: %s\n", app_error_code_string(result));
        return 1;
    }
    printf("credentials cleared; restarting into setup\n");
    fflush(stdout);
    vTaskDelay(pdMS_TO_TICKS(CREDENTIAL_RESET_FLUSH_MS));
    esp_restart();
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

    const esp_console_cmd_t credential_reset_command = {
        .command = "credential-reset",
        .help = "Clear the administrator password and AP credentials and restart into "
                "setup. Settings and the saved Wi-Fi network are kept.",
        .hint = CREDENTIAL_RESET_CONFIRMATION,
        .func = command_credential_reset,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&credential_reset_command));
}

app_error_code_t serial_console_start(void) {
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
