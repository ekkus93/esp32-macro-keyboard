#!/usr/bin/env python3
from pathlib import Path

ROOT = Path.cwd()


def read(path: str) -> str:
    return (ROOT / path).read_text()


def write(path: str, text: str) -> None:
    (ROOT / path).write_text(text)


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected one anchor, found {count}")
    return text.replace(old, new, 1)


p = "firmware/components/device_controls/device_controls.c"
text = read(p)
text = replace_once(
    text,
    "static bool restart_scheduled;\nstatic bool reset_settings_restart_required;\n",
    "static bool restart_scheduled;\n"
    "static bool restart_schedule_in_progress;\n"
    "static bool reset_settings_restart_required;\n",
    "restart ownership state",
)
old = """static app_error_code_t adapter_reset_schedule_restart(void *context, uint32_t delay_ms) {
    (void)context;
    portENTER_CRITICAL(&controls_lock);
    if (restart_scheduled) {
        portEXIT_CRITICAL(&controls_lock);
        return APP_ERROR_NONE;
    }
    restart_scheduled = true;
    portEXIT_CRITICAL(&controls_lock);

    const esp_timer_create_args_t timer_args = {
        .callback = restart_timer_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "device_restart",
        .skip_unhandled_events = false,
    };
    esp_timer_handle_t timer = NULL;
    /* A caller that reaches this point may already have committed durable
     * state. Delayed timer failure therefore falls back to an immediate reboot
     * rather than silently leaving the device live in a partial state. A
     * non-NONE return is reachable only if esp_restart() itself unexpectedly
     * returns without establishing restart ownership. */
    if (esp_timer_create(&timer_args, &timer) != ESP_OK ||
        esp_timer_start_once(timer, (uint64_t)delay_ms * UINT64_C(1000)) != ESP_OK) {
        esp_restart();
        return APP_ERROR_INTERNAL;
    }
    return APP_ERROR_NONE;
}
"""
new = """static app_error_code_t adapter_reset_schedule_restart(void *context, uint32_t delay_ms) {
    (void)context;
    portENTER_CRITICAL(&controls_lock);
    if (restart_scheduled) {
        portEXIT_CRITICAL(&controls_lock);
        return APP_ERROR_NONE;
    }
    if (restart_schedule_in_progress) {
        portEXIT_CRITICAL(&controls_lock);
        return APP_ERROR_CONFLICT;
    }
    restart_schedule_in_progress = true;
    portEXIT_CRITICAL(&controls_lock);

    const esp_timer_create_args_t timer_args = {
        .callback = restart_timer_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "device_restart",
        .skip_unhandled_events = false,
    };
    esp_timer_handle_t timer = NULL;
    const esp_err_t create_result = esp_timer_create(&timer_args, &timer);
    const esp_err_t start_result =
        create_result == ESP_OK
            ? esp_timer_start_once(timer, (uint64_t)delay_ms * UINT64_C(1000))
            : create_result;

    if (create_result != ESP_OK || start_result != ESP_OK) {
        if (create_result == ESP_OK && timer != NULL && esp_timer_delete(timer) != ESP_OK) {
            ESP_LOGE(TAG, "restart timer cleanup failed");
        }
        portENTER_CRITICAL(&controls_lock);
        restart_schedule_in_progress = false;
        portEXIT_CRITICAL(&controls_lock);

        /* A caller may already have committed durable state. Delayed timer
         * failure therefore falls back to immediate reboot. If esp_restart()
         * unexpectedly returns, the in-progress flag is already cleared and
         * no false `restart_scheduled` ownership is left behind. */
        esp_restart();
        return APP_ERROR_INTERNAL;
    }

    portENTER_CRITICAL(&controls_lock);
    restart_scheduled = true;
    restart_schedule_in_progress = false;
    portEXIT_CRITICAL(&controls_lock);
    return APP_ERROR_NONE;
}
"""
text = replace_once(text, old, new, "restart scheduler ownership semantics")
write(p, text)

p = "scripts/check-h3-architecture.py"
text = read(p)
text = replace_once(
    text,
    '    "static bool reset_settings_restart_required",\n',
    '    "static bool restart_schedule_in_progress",\n'
    '    "static bool reset_settings_restart_required",\n',
    "H3-034 scheduler state guard",
)
for required in (
    "restart_schedule_in_progress = true",
    "restart_schedule_in_progress = false",
    "restart_scheduled = true",
    "esp_timer_delete(timer)",
):
    if required not in read("firmware/components/device_controls/device_controls.c"):
        raise SystemExit(f"scheduler hardening missing after followup: {required}")
write(p, text)
