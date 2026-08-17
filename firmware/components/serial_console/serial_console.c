#include "serial_console.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "api_contracts_v2.h"
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
#include "web_settings.h"
#include "web_settings_production_ops.h"
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

/* SPEC_V2 12.4: possession of the board is sufficient authorization for this
 * command, on either a provisioned or unprovisioned device -- no current
 * password, and no additional confirmation beyond the command itself. Never
 * echoes the password back; only whether the change succeeded. */
static int command_set_admin_password(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: set-admin-password <password>\n");
        return 1;
    }
    const app_v2_string_view_t new_password_view = {
        .data = argv[1],
        .length = strlen(argv[1]),
    };
    const web_settings_ops_t ops = web_settings_production_ops();
    const web_change_password_outcome_t outcome =
        web_change_password_apply_without_verification(new_password_view, &ops);
    switch (outcome.result) {
    case WEB_CHANGE_PASSWORD_OK:
        printf("administrator password updated; all sessions invalidated\n");
        return 0;
    case WEB_CHANGE_PASSWORD_INVALID_NEW_PASSWORD:
        /* A fixed 12-128 byte literal, not a %u conversion:
         * scripts/check-credential-logging.sh conservatively forbids any
         * output call combining a sensitive word with a runtime `%`
         * conversion, unable to tell a fixed bound from a real secret value
         * at that specifier. AUTH_PASSWORD_MIN_BYTES/MAX_BYTES are exactly
         * 12/128 (auth.h); there is nothing secret in either number. */
        printf("invalid password: must be 12-128 characters\n");
        return 1;
    case WEB_CHANGE_PASSWORD_COMMITTED_SESSION_INVALIDATION_INCOMPLETE:
        /* Matches web_api_administration.c's change_password_error(): the
         * durable credential and RAM verifier are already the new password,
         * so this must read as "changed, but tell the owner to sign in with
         * it" rather than as a no-op failure. Split across two calls, not
         * one interpolated message: scripts/check-credential-logging.sh
         * conservatively forbids any output call whose format string
         * combines a sensitive word with a runtime `%` conversion, so the
         * error detail (a %s conversion) and the word "password" (a fixed
         * literal) cannot share one printf. */
        printf("administrator credential updated, but session invalidation failed: %s\n",
               app_error_code_string(outcome.detail));
        printf("sign in with the new password\n");
        return 1;
    case WEB_CHANGE_PASSWORD_BACKEND_UNAVAILABLE:
        printf("administrator credential change failed: %s\n",
               app_error_code_string(outcome.detail));
        return 1;
    case WEB_CHANGE_PASSWORD_INVALID_BODY:
    case WEB_CHANGE_PASSWORD_INCORRECT_CURRENT_PASSWORD:
    case WEB_CHANGE_PASSWORD_INTERNAL:
    default:
        printf("administrator password change failed\n");
        return 1;
    }
}

/* SPEC_V2 12.4: replaces the device's own access-point passphrase directly.
 * Persisted like wifi-connect's station credentials, but this passphrase
 * requires a restart to take effect -- identical to a PUT /api/v1/settings
 * accessPoint change -- since wifi_ap_start() only reads it at boot. */
static int command_set_ap_passphrase(int argc, char **argv) {
    if (argc != 2) {
        printf("usage: set-ap-passphrase <passphrase>\n");
        return 1;
    }
    bool changed = false;
    const app_error_code_t result = device_settings_set_access_point_passphrase(argv[1], &changed);
    if (result == APP_ERROR_NONE) {
        printf(changed ? "access-point passphrase saved; restart required to take effect\n"
                       : "access-point passphrase already current\n");
        return 0;
    }
    /* "credential", not "passphrase": see the comment on the analogous
     * set-admin-password failure printf above. */
    printf("access-point credential change failed: %s\n", app_error_code_string(result));
    return 1;
}

/* Stack headroom for the two first-party tasks that own long-running work.
 *
 * This is deliberately a console command rather than a diagnostics field:
 * SPEC_V2 §13.13 fixes the `GET /api/v1/diagnostics` schema and states that the
 * committed contract definitions "may add fields only through an explicit
 * specification update", and no `stack` field exists there. The trusted UART
 * console carries no such contract, so the measurement is available to a
 * developer at the bench without changing the wire format.
 *
 * ESP-IDF's uxTaskGetStackHighWaterMark() reports the minimum free stack in
 * bytes, and xTaskCreate() takes its depth in bytes too, so both numbers below
 * are directly comparable. A value of 0 means the task is not running.
 */
static int command_stack(int argc, char **argv) {
    (void)argc;
    (void)argv;
    const size_t executor_free = macro_executor_stack_high_water_mark();
    const size_t controls_free = device_controls_stack_high_water_mark();
    printf("macro_executor: %u bytes free of %u (minimum observed)\n", (unsigned)executor_free,
           (unsigned)MACRO_EXECUTOR_TASK_STACK_BYTES);
    printf("controls:       %u bytes free of %u (minimum observed)\n", (unsigned)controls_free,
           (unsigned)DEVICE_CONTROLS_TASK_STACK_BYTES);
    return 0;
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

    const esp_console_cmd_t set_admin_password_command = {
        .command = "set-admin-password",
        .help = "Replace the administrator password. No current password is required; "
                "possession of this console is the authorization.",
        .hint = "<password>",
        .func = command_set_admin_password,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&set_admin_password_command));

    const esp_console_cmd_t set_ap_passphrase_command = {
        .command = "set-ap-passphrase",
        .help = "Replace the device's own access-point passphrase. Requires a restart "
                "to take effect.",
        .hint = "<passphrase>",
        .func = command_set_ap_passphrase,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&set_ap_passphrase_command));

    const esp_console_cmd_t wifi_status_command = {
        .command = "wifi-status",
        .help = "Show the device's own SoftAP state and client count.",
        .hint = NULL,
        .func = command_wifi_status,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&wifi_status_command));

    const esp_console_cmd_t stack_command = {
        .command = "stack",
        .help = "Show the minimum observed free stack, in bytes, for the executor "
                "and controls tasks.",
        .hint = NULL,
        .func = command_stack,
    };
    ESP_ERROR_CHECK(esp_console_cmd_register(&stack_command));
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
