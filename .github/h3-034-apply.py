#!/usr/bin/env python3
from pathlib import Path

ROOT = Path.cwd()


def read(path: str) -> str:
    return (ROOT / path).read_text()


def write(path: str, text: str) -> None:
    target = ROOT / path
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text(text)


def replace_once(text: str, old: str, new: str, label: str) -> str:
    count = text.count(old)
    if count != 1:
        raise SystemExit(f"{label}: expected one anchor, found {count}")
    return text.replace(old, new, 1)


# ---------------------------------------------------------------------------
# Stable error and structured public reset-settings outcome.
# ---------------------------------------------------------------------------
p = "firmware/components/macro_model/include/app_error.h"
text = read(p)
text = replace_once(
    text,
    "    APP_ERROR_RESET_RECOVERY_REQUIRED\n} app_error_code_t;",
    "    APP_ERROR_RESET_RECOVERY_REQUIRED,\n"
    "    /* Reset-settings durably changed noncredential configuration but the\n"
    "     * operation could not establish the required restart boundary.\n"
    "     * Appended to preserve every existing numeric app-error value. */\n"
    "    APP_ERROR_RESET_SETTINGS_INCOMPLETE\n} app_error_code_t;",
    "app error enum",
)
write(p, text)

p = "firmware/components/macro_model/app_error.c"
text = read(p)
text = replace_once(
    text,
    '    case APP_ERROR_RESET_RECOVERY_REQUIRED:\n        return "reset_recovery_required";\n',
    '    case APP_ERROR_RESET_RECOVERY_REQUIRED:\n        return "reset_recovery_required";\n'
    '    case APP_ERROR_RESET_SETTINGS_INCOMPLETE:\n        return "reset_settings_incomplete";\n',
    "app error string",
)
write(p, text)

p = "firmware/components/device_controls/include/device_controls.h"
text = read(p)
anchor = """typedef struct {
    bool durably_accepted;
    bool recovery_required;
    app_error_code_t primary_error;
} device_controls_factory_reset_outcome_t;
"""
addition = anchor + """
/* H3-034 reset-settings transaction result. `settings_applied` is the durable
 * boundary. A failed session invalidation is recoverable by an owned reboot
 * because sessions are RAM-only; restart ownership failure is kept separate
 * so callers never represent a committed reset as if nothing changed. */
typedef struct {
    bool settings_applied;
    bool sessions_invalidated;
    bool restart_owned;
    app_error_code_t primary_error;
    app_error_code_t restart_error;
} device_controls_reset_settings_outcome_t;
"""
text = replace_once(text, anchor, addition, "device controls outcome type")
text = replace_once(
    text,
    "app_error_code_t device_controls_reset_settings(void);",
    "device_controls_reset_settings_outcome_t device_controls_reset_settings(void);\n\n"
    "/* True after a reset-settings durable commit until reboot. The web route\n"
    " * gate uses this RAM-only latch to prevent old sessions from exercising\n"
    " * normal API authority during the pre-reboot interval. */\n"
    "bool device_controls_reset_settings_restart_required(void);",
    "device controls reset public signature",
)
write(p, text)

# ---------------------------------------------------------------------------
# Reset engine: observable restart ownership, explicit reset-settings result,
# and safe factory-reset ordering (restart ownership before journal clear).
# ---------------------------------------------------------------------------
p = "firmware/components/device_controls/device_controls_reset.h"
text = read(p)
text = replace_once(
    text,
    "    void (*schedule_restart)(void *context, uint32_t delay_ms);",
    "    app_error_code_t (*schedule_restart)(void *context, uint32_t delay_ms);",
    "reset ops scheduler type",
)
text = replace_once(
    text,
    "app_error_code_t\ndevice_controls_reset_engine_reset_settings(const device_controls_reset_ops_t *operations,\n                                            uint32_t delay_ms);",
    "device_controls_reset_settings_outcome_t\n"
    "device_controls_reset_engine_reset_settings(const device_controls_reset_ops_t *operations,\n"
    "                                            uint32_t delay_ms);",
    "reset settings engine declaration",
)
text = replace_once(
    text,
    " * invalidates every session, and schedules a reboot. The H3 factory-reset\n * journal is intentionally untouched. */",
    " * invalidates every session, and establishes reboot ownership. The H3\n"
    " * factory-reset journal is intentionally untouched: unlike factory reset,\n"
    " * the durable settings record is already coherent after this operation,\n"
    " * and a reboot is sufficient to discard any remaining RAM-only sessions. */",
    "reset settings header comment",
)
write(p, text)

p = "firmware/components/device_controls/device_controls_reset.c"
text = read(p)
old = """app_error_code_t device_controls_reset_engine_restart(const device_controls_reset_ops_t *operations,
                                                      uint32_t delay_ms) {
    if (!device_controls_reset_ops_is_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    operations->schedule_restart(operations->context, delay_ms);
    return APP_ERROR_NONE;
}

app_error_code_t
device_controls_reset_engine_reset_settings(const device_controls_reset_ops_t *operations,
                                            uint32_t delay_ms) {
    if (!device_controls_reset_ops_is_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }

    const app_error_code_t settings_result =
        operations->reset_settings_noncredential(operations->context);
    if (settings_result != APP_ERROR_NONE) {
        return settings_result;
    }

    app_error_code_t first_error = APP_ERROR_NONE;
    record_first_error(operations->invalidate_all_sessions(operations->context), &first_error);
    operations->schedule_restart(operations->context, delay_ms);
    return first_error;
}
"""
new = """app_error_code_t device_controls_reset_engine_restart(const device_controls_reset_ops_t *operations,
                                                      uint32_t delay_ms) {
    if (!device_controls_reset_ops_is_valid(operations)) {
        return APP_ERROR_INVALID_ARGUMENT;
    }
    return operations->schedule_restart(operations->context, delay_ms);
}

device_controls_reset_settings_outcome_t
device_controls_reset_engine_reset_settings(const device_controls_reset_ops_t *operations,
                                            uint32_t delay_ms) {
    if (!device_controls_reset_ops_is_valid(operations)) {
        return (device_controls_reset_settings_outcome_t){
            .settings_applied = false,
            .sessions_invalidated = false,
            .restart_owned = false,
            .primary_error = APP_ERROR_INVALID_ARGUMENT,
            .restart_error = APP_ERROR_NONE,
        };
    }

    const app_error_code_t settings_result =
        operations->reset_settings_noncredential(operations->context);
    if (settings_result != APP_ERROR_NONE) {
        return (device_controls_reset_settings_outcome_t){
            .settings_applied = false,
            .sessions_invalidated = false,
            .restart_owned = false,
            .primary_error = settings_result,
            .restart_error = APP_ERROR_NONE,
        };
    }

    const app_error_code_t session_result =
        operations->invalidate_all_sessions(operations->context);
    const app_error_code_t restart_result =
        operations->schedule_restart(operations->context, delay_ms);
    return (device_controls_reset_settings_outcome_t){
        .settings_applied = true,
        .sessions_invalidated = session_result == APP_ERROR_NONE,
        .restart_owned = restart_result == APP_ERROR_NONE,
        .primary_error = session_result,
        .restart_error = restart_result,
    };
}
"""
text = replace_once(text, old, new, "reset engine restart/reset-settings block")
old = """    if (first_error == APP_ERROR_NONE) {
        record_first_error(operations->clear_factory_reset_pending(operations->context),
                           &first_error);
    }

    operations->schedule_restart(operations->context, delay_ms);
    return (device_controls_factory_reset_outcome_t){
"""
new = """    /* Establish restart ownership before clearing PENDING. If restart cannot
     * be owned, keeping the durable marker is safer than exposing ordinary
     * operation after a reset that has already been accepted. If an immediate
     * restart happens before the clear below, boot recovery sees PENDING and
     * safely replays the idempotent cleanup. */
    record_first_error(operations->schedule_restart(operations->context, delay_ms), &first_error);
    if (first_error == APP_ERROR_NONE) {
        record_first_error(operations->clear_factory_reset_pending(operations->context),
                           &first_error);
    }

    return (device_controls_factory_reset_outcome_t){
"""
text = replace_once(text, old, new, "factory reset restart/clear order")
write(p, text)

# ---------------------------------------------------------------------------
# Production adapter: restart callback reports ownership; reset-settings sets
# a RAM authority latch immediately after its durable settings boundary.
# ---------------------------------------------------------------------------
p = "firmware/components/device_controls/device_controls.c"
text = read(p)
text = replace_once(
    text,
    "static bool restart_scheduled;\n",
    "static bool restart_scheduled;\nstatic bool reset_settings_restart_required;\n",
    "reset settings latch declaration",
)
old = """static void adapter_reset_schedule_restart(void *context, uint32_t delay_ms) {
    (void)context;
    portENTER_CRITICAL(&controls_lock);
    if (restart_scheduled) {
        portEXIT_CRITICAL(&controls_lock);
        return;
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
    /* A caller (e.g. the HTTP layer) that scheduled this already committed to
     * returning its "accepted" response; if the timer cannot even be created,
     * restarting immediately is safer than silently never rebooting into the
     * state that was just written to NVS. */
    if (esp_timer_create(&timer_args, &timer) != ESP_OK ||
        esp_timer_start_once(timer, (uint64_t)delay_ms * UINT64_C(1000)) != ESP_OK) {
        esp_restart();
    }
}

static app_error_code_t adapter_reset_settings_noncredential(void *context) {
    (void)context;
    app_v2_device_settings_t settings = {0};
    bool changed = false;
    return device_settings_reset_noncredential(&settings, &changed);
}
"""
new = """static app_error_code_t adapter_reset_schedule_restart(void *context, uint32_t delay_ms) {
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

static void mark_reset_settings_restart_required(void) {
    portENTER_CRITICAL(&controls_lock);
    reset_settings_restart_required = true;
    portEXIT_CRITICAL(&controls_lock);
}

bool device_controls_reset_settings_restart_required(void) {
    portENTER_CRITICAL(&controls_lock);
    const bool required = reset_settings_restart_required;
    portEXIT_CRITICAL(&controls_lock);
    return required;
}

static void secure_zero_reset_settings(app_v2_device_settings_t *settings) {
    volatile uint8_t *bytes = (volatile uint8_t *)settings;
    for (size_t index = 0U; index < sizeof(*settings); ++index) {
        bytes[index] = 0U;
    }
}

static app_error_code_t adapter_reset_settings_noncredential(void *context) {
    (void)context;
    app_v2_device_settings_t settings = {0};
    bool changed = false;
    const app_error_code_t result = device_settings_reset_noncredential(&settings, &changed);
    secure_zero_reset_settings(&settings);
    if (result == APP_ERROR_NONE) {
        /* This latch is deliberately RAM-only. The durable settings record is
         * already coherent; its purpose is solely to fail normal API authority
         * closed until the required reboot discards every RAM-only session. */
        mark_reset_settings_restart_required();
    }
    return result;
}
"""
text = replace_once(text, old, new, "production restart/reset-settings adapter")
text = replace_once(
    text,
    "app_error_code_t device_controls_reset_settings(void) {\n"
    "    const device_controls_reset_ops_t operations = reset_operations();\n"
    "    return device_controls_reset_engine_reset_settings(&operations,\n"
    "                                                       DEVICE_CONTROLS_RESTART_DELAY_MS);\n"
    "}",
    "device_controls_reset_settings_outcome_t device_controls_reset_settings(void) {\n"
    "    const device_controls_reset_ops_t operations = reset_operations();\n"
    "    return device_controls_reset_engine_reset_settings(&operations,\n"
    "                                                       DEVICE_CONTROLS_RESTART_DELAY_MS);\n"
    "}",
    "device controls reset settings definition",
)
write(p, text)

# ---------------------------------------------------------------------------
# Web business semantics for precommit, reboot-recovery, and committed restart
# failure. Preserve exact low-level primary and restart errors separately.
# ---------------------------------------------------------------------------
p = "firmware/components/web_server/web_device_actions.h"
text = read(p)
text = replace_once(
    text,
    "    app_error_code_t (*reset_settings)(void *context);",
    "    device_controls_reset_settings_outcome_t (*reset_settings)(void *context);",
    "web reset-settings callback type",
)
text = replace_once(
    text,
    "    WEB_DEVICE_RESET_SETTINGS_BACKEND_UNAVAILABLE,\n"
    "    WEB_DEVICE_RESET_SETTINGS_INTERNAL,",
    "    WEB_DEVICE_RESET_SETTINGS_BACKEND_UNAVAILABLE,\n"
    "    WEB_DEVICE_RESET_SETTINGS_REBOOT_RECOVERY_REQUIRED,\n"
    "    WEB_DEVICE_RESET_SETTINGS_COMMITTED_RESTART_INCOMPLETE,\n"
    "    WEB_DEVICE_RESET_SETTINGS_INTERNAL,",
    "web reset-settings enum",
)
text = replace_once(
    text,
    "typedef struct {\n"
    "    web_device_reset_settings_result_t result;\n"
    "    app_error_code_t detail;\n"
    "} web_device_reset_settings_outcome_t;",
    "typedef struct {\n"
    "    web_device_reset_settings_result_t result;\n"
    "    /* Primary settings/session error. */\n"
    "    app_error_code_t detail;\n"
    "    /* Restart-ownership error, kept separate from the primary failure. */\n"
    "    app_error_code_t restart_detail;\n"
    "} web_device_reset_settings_outcome_t;",
    "web reset-settings outcome details",
)
write(p, text)

p = "firmware/components/web_server/web_device_actions.c"
text = read(p)
old = """    const app_error_code_t result = ops->reset_settings(ops->context);
    if (result != APP_ERROR_NONE) {
        return (web_device_reset_settings_outcome_t){
            .result = WEB_DEVICE_RESET_SETTINGS_BACKEND_UNAVAILABLE,
            .detail = result,
        };
    }
    return (web_device_reset_settings_outcome_t){.result = WEB_DEVICE_RESET_SETTINGS_OK};
}"""
new = """    const device_controls_reset_settings_outcome_t reset = ops->reset_settings(ops->context);
    if (!reset.settings_applied) {
        if (reset.sessions_invalidated || reset.restart_owned || reset.primary_error == APP_ERROR_NONE ||
            reset.restart_error != APP_ERROR_NONE) {
            return (web_device_reset_settings_outcome_t){
                .result = WEB_DEVICE_RESET_SETTINGS_INTERNAL,
                .detail = APP_ERROR_INTERNAL,
            };
        }
        return (web_device_reset_settings_outcome_t){
            .result = WEB_DEVICE_RESET_SETTINGS_BACKEND_UNAVAILABLE,
            .detail = reset.primary_error,
        };
    }

    if (!reset.restart_owned) {
        if (reset.restart_error == APP_ERROR_NONE ||
            (reset.sessions_invalidated && reset.primary_error != APP_ERROR_NONE) ||
            (!reset.sessions_invalidated && reset.primary_error == APP_ERROR_NONE)) {
            return (web_device_reset_settings_outcome_t){
                .result = WEB_DEVICE_RESET_SETTINGS_INTERNAL,
                .detail = APP_ERROR_INTERNAL,
            };
        }
        return (web_device_reset_settings_outcome_t){
            .result = WEB_DEVICE_RESET_SETTINGS_COMMITTED_RESTART_INCOMPLETE,
            .detail = reset.primary_error,
            .restart_detail = reset.restart_error,
        };
    }
    if (reset.restart_error != APP_ERROR_NONE) {
        return (web_device_reset_settings_outcome_t){
            .result = WEB_DEVICE_RESET_SETTINGS_INTERNAL,
            .detail = APP_ERROR_INTERNAL,
        };
    }

    if (!reset.sessions_invalidated) {
        if (reset.primary_error == APP_ERROR_NONE) {
            return (web_device_reset_settings_outcome_t){
                .result = WEB_DEVICE_RESET_SETTINGS_INTERNAL,
                .detail = APP_ERROR_INTERNAL,
            };
        }
        return (web_device_reset_settings_outcome_t){
            .result = WEB_DEVICE_RESET_SETTINGS_REBOOT_RECOVERY_REQUIRED,
            .detail = reset.primary_error,
        };
    }
    if (reset.primary_error != APP_ERROR_NONE) {
        return (web_device_reset_settings_outcome_t){
            .result = WEB_DEVICE_RESET_SETTINGS_INTERNAL,
            .detail = APP_ERROR_INTERNAL,
        };
    }
    return (web_device_reset_settings_outcome_t){.result = WEB_DEVICE_RESET_SETTINGS_OK};
}"""
text = replace_once(text, old, new, "web reset-settings mapping")
write(p, text)

# ---------------------------------------------------------------------------
# HTTP: prebuild 202 before durable mutation; accepted reboot recovery remains
# 202; committed restart failure is explicit 409 reset_settings_incomplete.
# ---------------------------------------------------------------------------
p = "firmware/components/web_server/web_api_administration.c"
text = read(p)
text = replace_once(
    text,
    "static app_error_code_t device_actions_ops_reset_settings(void *context) {\n"
    "    (void)context;\n"
    "    return device_controls_reset_settings();\n"
    "}",
    "static device_controls_reset_settings_outcome_t device_actions_ops_reset_settings(void *context) {\n"
    "    (void)context;\n"
    "    return device_controls_reset_settings();\n"
    "}",
    "administration reset-settings adapter signature",
)
start = text.index("static app_error_code_t handle_device_reset_settings(")
end = text.index("\n/* -------------------------------------------------------------------------\n * POST /api/v1/device/factory-reset", start)
old = text[start:end]
new = """static app_error_code_t handle_device_reset_settings(const web_api_call_t *call,
                                                     web_api_response_t *response) {
    if (!call_has_body(call)) {
        return web_api_handler_error(response, APP_ERROR_INVALID_ARGUMENT,
                                     "invalid reset-settings request", NULL);
    }

    char *json = NULL;
    web_api_response_t accepted_response = {0};
    if (web_device_reset_accepted_json(false, true, &json) != APP_ERROR_NONE) {
        return web_api_handler_error(response, APP_ERROR_INTERNAL, "response encoding failed",
                                     NULL);
    }
    const app_error_code_t prepare_result =
        web_api_handler_success_json(&accepted_response, WEB_HTTP_STATUS_ACCEPTED, json);
    web_api_handler_json_free(json);
    if (prepare_result != APP_ERROR_NONE) {
        web_api_response_free(&accepted_response);
        return web_api_handler_error(response, APP_ERROR_INTERNAL, "response encoding failed",
                                     NULL);
    }

    const web_device_actions_ops_t ops = device_actions_ops();
    const web_device_reset_settings_outcome_t outcome =
        web_device_reset_settings_handle((char *)call->body, call->body_length + 1U, &ops);
    switch (outcome.result) {
    case WEB_DEVICE_RESET_SETTINGS_OK:
    case WEB_DEVICE_RESET_SETTINGS_REBOOT_RECOVERY_REQUIRED:
        *response = accepted_response;
        return APP_ERROR_NONE;
    case WEB_DEVICE_RESET_SETTINGS_INVALID_BODY:
        web_api_response_free(&accepted_response);
        return web_api_handler_error(response, APP_ERROR_INVALID_ARGUMENT,
                                     "invalid reset-settings request", NULL);
    case WEB_DEVICE_RESET_SETTINGS_INVALID_CONFIRMATION:
        web_api_response_free(&accepted_response);
        return web_api_handler_error(response, APP_ERROR_INVALID_ARGUMENT,
                                     "incorrect confirmation phrase", "confirmation");
    case WEB_DEVICE_RESET_SETTINGS_BACKEND_UNAVAILABLE:
        web_api_response_free(&accepted_response);
        return web_api_handler_error(response, outcome.detail, "reset-settings unavailable", NULL);
    case WEB_DEVICE_RESET_SETTINGS_COMMITTED_RESTART_INCOMPLETE:
        web_api_response_free(&accepted_response);
        return web_api_response_error(
            response,
            &(web_api_error_spec_t){
                .status = WEB_HTTP_STATUS_CONFLICT,
                .code = APP_ERROR_RESET_SETTINGS_INCOMPLETE,
                .message = "settings reset; automatic restart incomplete; restart the device",
            });
    case WEB_DEVICE_RESET_SETTINGS_INTERNAL:
    default:
        web_api_response_free(&accepted_response);
        return web_api_handler_error(response, APP_ERROR_INTERNAL, "reset-settings failed", NULL);
    }
}
"""
text = text[:start] + new + text[end:]
write(p, text)

p = "firmware/components/web_server/web_api_core.c"
text = read(p)
text = replace_once(
    text,
    "    case APP_ERROR_CONFLICT:\n"
    "    case APP_ERROR_AUTH_STATE_INCOMPLETE:\n",
    "    case APP_ERROR_CONFLICT:\n"
    "    case APP_ERROR_AUTH_STATE_INCOMPLETE:\n"
    "    case APP_ERROR_RESET_SETTINGS_INCOMPLETE:\n",
    "reset-settings incomplete HTTP status",
)
write(p, text)

# ---------------------------------------------------------------------------
# Fail normal API authority closed after a reset-settings durable commit until
# reboot, including the pathological restart-ownership failure case.
# ---------------------------------------------------------------------------
p = "firmware/components/web_server/web_server_lifecycle.c"
text = read(p)
text = replace_once(
    text,
    '#include "app_error.h"\n#include "factory_reset_state.h"\n',
    '#include "app_error.h"\n#include "device_controls.h"\n#include "factory_reset_state.h"\n',
    "lifecycle device controls include",
)
anchor = """    if (request == NULL || handler == NULL) {
        return ESP_FAIL;
    }

    factory_reset_state_t reset_state = FACTORY_RESET_STATE_NONE;
"""
replacement = """    if (request == NULL || handler == NULL) {
        return ESP_FAIL;
    }

    if (device_controls_reset_settings_restart_required()) {
        return web_api_send_status_error(request, 503U, APP_ERROR_RESET_SETTINGS_INCOMPLETE,
                                         "reset-settings restart required");
    }

    factory_reset_state_t reset_state = FACTORY_RESET_STATE_NONE;
"""
text = replace_once(text, anchor, replacement, "lifecycle reset-settings authority gate")
write(p, text)

# Host lifecycle target needs the public device-controls include after the new
# gate dependency.
p = "tests/host/CMakeLists.txt"
text = read(p)
needle = "add_executable(\n    web_server_lifecycle_tests"
start = text.index(needle)
end = text.find("\nadd_executable(", start + 1)
if end < 0:
    end = len(text)
block = text[start:end]
inc = "            ../../firmware/components/device_controls/include\n"
if inc not in block:
    block = replace_once(
        block,
        "            ../../firmware/components/device_settings/include\n",
        "            ../../firmware/components/device_settings/include\n" + inc,
        "lifecycle CMake device-controls include",
    )
    text = text[:start] + block + text[end:]
write(p, text)

# ---------------------------------------------------------------------------
# Controls host tests: structured outcomes, restart failure, separate errors,
# and factory marker retention if restart ownership cannot be established.
# ---------------------------------------------------------------------------
p = "tests/host/test_device_controls_reset.c"
text = read(p)
text = replace_once(
    text,
    "    app_error_code_t clear_pending_result;\n",
    "    app_error_code_t clear_pending_result;\n    app_error_code_t schedule_restart_result;\n",
    "reset test restart result field",
)
old = """static void fake_schedule_restart(void *context, uint32_t delay_ms) {
    fake_reset_t *fake = context;
    ++fake->schedule_restart_calls;
    fake->restart_sequence = ++fake->sequence;
    fake->last_delay_ms = delay_ms;
}
"""
new = """static app_error_code_t fake_schedule_restart(void *context, uint32_t delay_ms) {
    fake_reset_t *fake = context;
    ++fake->schedule_restart_calls;
    fake->restart_sequence = ++fake->sequence;
    fake->last_delay_ms = delay_ms;
    return fake->schedule_restart_result;
}
"""
text = replace_once(text, old, new, "reset test scheduler signature")
# Validation macro now inspects structured reset-settings invalid-ops result.
old = """        TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,                                           \
                             device_controls_reset_engine_reset_settings(&operations, 500U));      \
        const device_controls_factory_reset_outcome_t factory_outcome =                            \
"""
new = """        const device_controls_reset_settings_outcome_t reset_outcome =                         \
            device_controls_reset_engine_reset_settings(&operations, 500U);                         \
        TEST_CHECK(!reset_outcome.settings_applied);                                                 \
        TEST_CHECK(!reset_outcome.sessions_invalidated);                                             \
        TEST_CHECK(!reset_outcome.restart_owned);                                                    \
        TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT, reset_outcome.primary_error);                \
        TEST_CHECK_APP_ERROR(APP_ERROR_NONE, reset_outcome.restart_error);                            \
        const device_controls_factory_reset_outcome_t factory_outcome =                              \
"""
text = replace_once(text, old, new, "reset ops validation structured outcome")
# Happy path.
old = """    TEST_CHECK_APP_ERROR(APP_ERROR_NONE,
                         device_controls_reset_engine_reset_settings(&operations, 500U));
    TEST_CHECK_EQ_U64(1U, fake.reset_settings_calls);
"""
new = """    const device_controls_reset_settings_outcome_t outcome =
        device_controls_reset_engine_reset_settings(&operations, 500U);
    TEST_CHECK(outcome.settings_applied);
    TEST_CHECK(outcome.sessions_invalidated);
    TEST_CHECK(outcome.restart_owned);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, outcome.primary_error);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, outcome.restart_error);
    TEST_CHECK_EQ_U64(1U, fake.reset_settings_calls);
"""
text = replace_once(text, old, new, "reset settings happy outcome")
# Settings failure.
old = """    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE,
                         device_controls_reset_engine_reset_settings(&operations, 500U));
    TEST_CHECK_EQ_U64(1U, fake.reset_settings_calls);
"""
new = """    const device_controls_reset_settings_outcome_t outcome =
        device_controls_reset_engine_reset_settings(&operations, 500U);
    TEST_CHECK(!outcome.settings_applied);
    TEST_CHECK(!outcome.sessions_invalidated);
    TEST_CHECK(!outcome.restart_owned);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, outcome.primary_error);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, outcome.restart_error);
    TEST_CHECK_EQ_U64(1U, fake.reset_settings_calls);
"""
text = replace_once(text, old, new, "reset settings precommit failure outcome")
# Session failure.
old = """    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL,
                         device_controls_reset_engine_reset_settings(&operations, 500U));
    TEST_CHECK_EQ_U64(1U, fake.reset_settings_calls);
    TEST_CHECK_EQ_U64(1U, fake.invalidate_sessions_calls);
    TEST_CHECK_EQ_U64(1U, fake.schedule_restart_calls);
}

static void test_factory_reset_happy_path_orders_durable_boundary_first(void) {
"""
new = """    const device_controls_reset_settings_outcome_t outcome =
        device_controls_reset_engine_reset_settings(&operations, 500U);
    TEST_CHECK(outcome.settings_applied);
    TEST_CHECK(!outcome.sessions_invalidated);
    TEST_CHECK(outcome.restart_owned);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, outcome.primary_error);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, outcome.restart_error);
    TEST_CHECK_EQ_U64(1U, fake.reset_settings_calls);
    TEST_CHECK_EQ_U64(1U, fake.invalidate_sessions_calls);
    TEST_CHECK_EQ_U64(1U, fake.schedule_restart_calls);
}

static void test_reset_settings_reports_restart_failure_after_commit(void) {
    fake_reset_t fake;
    fake_init(&fake);
    fake.schedule_restart_result = APP_ERROR_IO;
    const device_controls_reset_ops_t operations = fake_ops(&fake);
    const device_controls_reset_settings_outcome_t outcome =
        device_controls_reset_engine_reset_settings(&operations, 500U);
    TEST_CHECK(outcome.settings_applied);
    TEST_CHECK(outcome.sessions_invalidated);
    TEST_CHECK(!outcome.restart_owned);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, outcome.primary_error);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, outcome.restart_error);
    TEST_CHECK_EQ_U64(1U, fake.schedule_restart_calls);
}

static void test_reset_settings_preserves_session_and_restart_errors_separately(void) {
    fake_reset_t fake;
    fake_init(&fake);
    fake.invalidate_sessions_result = APP_ERROR_INTERNAL;
    fake.schedule_restart_result = APP_ERROR_IO;
    const device_controls_reset_ops_t operations = fake_ops(&fake);
    const device_controls_reset_settings_outcome_t outcome =
        device_controls_reset_engine_reset_settings(&operations, 500U);
    TEST_CHECK(outcome.settings_applied);
    TEST_CHECK(!outcome.sessions_invalidated);
    TEST_CHECK(!outcome.restart_owned);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, outcome.primary_error);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, outcome.restart_error);
}

static void test_factory_reset_happy_path_orders_durable_boundary_first(void) {
"""
text = replace_once(text, old, new, "reset settings failure tests")
# Factory successful sequence now owns restart before clearing PENDING.
text = replace_once(
    text,
    "    TEST_CHECK(fake.cleanup_temporary_sequence < fake.clear_pending_sequence);\n"
    "    TEST_CHECK(fake.clear_pending_sequence < fake.restart_sequence);",
    "    TEST_CHECK(fake.cleanup_temporary_sequence < fake.restart_sequence);\n"
    "    TEST_CHECK(fake.restart_sequence < fake.clear_pending_sequence);",
    "factory successful restart/clear order test",
)
text = replace_once(
    text,
    "    TEST_CHECK(fake.clear_pending_sequence < fake.restart_sequence);\n}",
    "    TEST_CHECK(fake.restart_sequence < fake.clear_pending_sequence);\n}",
    "factory clear failure order test",
)
# Add explicit factory schedule failure protection due shared scheduler seam.
anchor = "\nstatic void test_factory_reset_marker_clear_failure_reboots_into_recovery(void) {"
if text.count(anchor) != 1:
    raise SystemExit("factory marker-clear test anchor")
addition = r'''
static void test_factory_reset_restart_ownership_failure_keeps_pending(void) {
    fake_reset_t fake;
    fake_init(&fake);
    fake.schedule_restart_result = APP_ERROR_IO;
    const device_controls_reset_ops_t operations = fake_ops(&fake);
    const device_controls_factory_reset_outcome_t outcome =
        device_controls_reset_engine_factory_reset(&operations, 500U);
    TEST_CHECK(outcome.durably_accepted);
    TEST_CHECK(outcome.recovery_required);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, outcome.primary_error);
    TEST_CHECK_EQ_U64(1U, fake.schedule_restart_calls);
    TEST_CHECK_EQ_U64(0U, fake.clear_pending_calls);
}

'''
text = text.replace(anchor, "\n" + addition + anchor.lstrip("\n"), 1)
# Add new calls.
call_anchor = "    test_reset_settings_reports_session_failure_but_still_restarts();\n"
text = replace_once(
    text,
    call_anchor,
    call_anchor
    + "    test_reset_settings_reports_restart_failure_after_commit();\n"
    + "    test_reset_settings_preserves_session_and_restart_errors_separately();\n",
    "reset test main calls",
)
call_anchor = "    test_factory_reset_marker_clear_failure_reboots_into_recovery();\n"
text = replace_once(
    text,
    call_anchor,
    "    test_factory_reset_restart_ownership_failure_keeps_pending();\n" + call_anchor,
    "factory restart failure main call",
)
write(p, text)

# H3-033 integrated matrix scheduler now reports ownership. Delayed-timer failure
# represents the production immediate-reboot fallback and therefore succeeds.
p = "tests/host/test_factory_reset_failure_matrix.c"
text = read(p)
old = """static void live_schedule_restart(void *context, uint32_t delay_ms) {
    fixture_t *fixture = context;
    TEST_CHECK(delay_ms > 0U);
    if (fixture->delayed_restart_arm_fails) {
        fixture->immediate_restart_requested = true;
    } else {
        fixture->delayed_restart_requested = true;
    }
}
"""
new = """static app_error_code_t live_schedule_restart(void *context, uint32_t delay_ms) {
    fixture_t *fixture = context;
    TEST_CHECK(delay_ms > 0U);
    if (fixture->delayed_restart_arm_fails) {
        fixture->immediate_restart_requested = true;
    } else {
        fixture->delayed_restart_requested = true;
    }
    return APP_ERROR_NONE;
}
"""
text = replace_once(text, old, new, "failure matrix scheduler signature")
write(p, text)

# ---------------------------------------------------------------------------
# Web-device-action unit tests.
# ---------------------------------------------------------------------------
p = "tests/host/test_web_device_actions.c"
text = read(p)
text = replace_once(
    text,
    "    app_error_code_t reset_settings_error;\n",
    "    device_controls_reset_settings_outcome_t reset_settings_outcome;\n",
    "web actions fake reset outcome field",
)
old = """static app_error_code_t fake_reset_settings(void *context) {
    fake_t *fake = context;
    ++fake->reset_settings_calls;
    return fake->reset_settings_error;
}
"""
new = """static device_controls_reset_settings_outcome_t fake_reset_settings(void *context) {
    fake_t *fake = context;
    ++fake->reset_settings_calls;
    return fake->reset_settings_outcome;
}
"""
text = replace_once(text, old, new, "web actions fake reset callback")
# Success fixture.
text = replace_once(
    text,
    "static void test_reset_settings_success(void) {\n    fake_t fake = {0};",
    "static void test_reset_settings_success(void) {\n"
    "    fake_t fake = {.reset_settings_outcome =\n"
    "                       {.settings_applied = true,\n"
    "                        .sessions_invalidated = true,\n"
    "                        .restart_owned = true,\n"
    "                        .primary_error = APP_ERROR_NONE,\n"
    "                        .restart_error = APP_ERROR_NONE}};",
    "web actions reset success fixture",
)
old = """static void test_reset_settings_backend_failure(void) {
    fake_t fake = {.reset_settings_error = APP_ERROR_STORAGE_UNAVAILABLE};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"confirmation\":\"RESET SETTINGS\"}");

    const web_device_reset_settings_outcome_t outcome =
        web_device_reset_settings_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_RESET_SETTINGS_BACKEND_UNAVAILABLE, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, outcome.detail);
}
"""
new = """static void test_reset_settings_backend_failure(void) {
    fake_t fake = {.reset_settings_outcome =
                       {.settings_applied = false,
                        .sessions_invalidated = false,
                        .restart_owned = false,
                        .primary_error = APP_ERROR_STORAGE_UNAVAILABLE,
                        .restart_error = APP_ERROR_NONE}};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"confirmation\":\"RESET SETTINGS\"}");

    const web_device_reset_settings_outcome_t outcome =
        web_device_reset_settings_handle(body, sizeof(body), &ops);

    TEST_CHECK_EQ_INT(WEB_DEVICE_RESET_SETTINGS_BACKEND_UNAVAILABLE, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_STORAGE_UNAVAILABLE, outcome.detail);
}

static void test_reset_settings_session_failure_is_owned_by_reboot(void) {
    fake_t fake = {.reset_settings_outcome =
                       {.settings_applied = true,
                        .sessions_invalidated = false,
                        .restart_owned = true,
                        .primary_error = APP_ERROR_IO,
                        .restart_error = APP_ERROR_NONE}};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"confirmation\":\"RESET SETTINGS\"}");
    const web_device_reset_settings_outcome_t outcome =
        web_device_reset_settings_handle(body, sizeof(body), &ops);
    TEST_CHECK_EQ_INT(WEB_DEVICE_RESET_SETTINGS_REBOOT_RECOVERY_REQUIRED, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, outcome.detail);
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, outcome.restart_detail);
}

static void test_reset_settings_restart_failure_is_committed_partial(void) {
    fake_t fake = {.reset_settings_outcome =
                       {.settings_applied = true,
                        .sessions_invalidated = false,
                        .restart_owned = false,
                        .primary_error = APP_ERROR_INTERNAL,
                        .restart_error = APP_ERROR_IO}};
    const web_device_actions_ops_t ops = operations(&fake);
    char body[TEST_BODY_CAPACITY];
    build_body(body, sizeof(body), "{\"confirmation\":\"RESET SETTINGS\"}");
    const web_device_reset_settings_outcome_t outcome =
        web_device_reset_settings_handle(body, sizeof(body), &ops);
    TEST_CHECK_EQ_INT(WEB_DEVICE_RESET_SETTINGS_COMMITTED_RESTART_INCOMPLETE, outcome.result);
    TEST_CHECK_APP_ERROR(APP_ERROR_INTERNAL, outcome.detail);
    TEST_CHECK_APP_ERROR(APP_ERROR_IO, outcome.restart_detail);
}
"""
text = replace_once(text, old, new, "web actions reset failure tests")
# Main calls: insert after existing backend failure if present.
main_call = "    test_reset_settings_backend_failure();\n"
text = replace_once(
    text,
    main_call,
    main_call
    + "    test_reset_settings_session_failure_is_owned_by_reboot();\n"
    + "    test_reset_settings_restart_failure_is_committed_partial();\n",
    "web actions reset main calls",
)
write(p, text)

# ---------------------------------------------------------------------------
# HTTP administration unit tests and signature fakes.
# ---------------------------------------------------------------------------
p = "tests/host/test_web_api_administration.c"
text = read(p)
text = replace_once(
    text,
    "    app_error_code_t reset_settings_result;\n",
    "    device_controls_reset_settings_outcome_t reset_settings_outcome;\n",
    "administration fake reset field",
)
old = """app_error_code_t device_controls_reset_settings(void) {
    ++fake_device_controls.reset_settings_calls;
    return fake_device_controls.reset_settings_result;
}
"""
new = """device_controls_reset_settings_outcome_t device_controls_reset_settings(void) {
    ++fake_device_controls.reset_settings_calls;
    return fake_device_controls.reset_settings_outcome;
}
"""
text = replace_once(text, old, new, "administration fake reset signature")
# Default happy reset-settings outcome.
old = """    fake_device_controls = (fake_device_controls_t){
        .factory_reset_outcome =
"""
new = """    fake_device_controls = (fake_device_controls_t){
        .reset_settings_outcome =
            {
                .settings_applied = true,
                .sessions_invalidated = true,
                .restart_owned = true,
                .primary_error = APP_ERROR_NONE,
                .restart_error = APP_ERROR_NONE,
            },
        .factory_reset_outcome =
"""
text = replace_once(text, old, new, "administration reset default")
# Add reset-settings HTTP tests before factory-reset section.
anchor = """/* -------------------------------------------------------------------------
 * POST /api/v1/device/factory-reset — H3-032 accepted/recovery semantics.
 * ---------------------------------------------------------------------- */
"""
if text.count(anchor) != 1:
    raise SystemExit("administration factory section anchor")
tests = r'''/* -------------------------------------------------------------------------
 * POST /api/v1/device/reset-settings — H3-034 partial-completion semantics.
 * ---------------------------------------------------------------------- */

static void prepare_reset_settings_call(web_api_call_t *call, char *body, size_t body_size) {
    TEST_CHECK(call != NULL);
    TEST_CHECK(body != NULL);
    const int written = snprintf(body, body_size, "{\"confirmation\":\"RESET SETTINGS\"}");
    TEST_CHECK(written > 0 && (size_t)written < body_size);
    *call = (web_api_call_t){
        .method = WEB_API_METHOD_POST,
        .path = {.route = WEB_API_ROUTE_DEVICE_RESET_SETTINGS},
        .body = body,
        .body_length = (size_t)written,
    };
}

static void test_reset_settings_precommit_failure_is_not_202(void) {
    reset_fakes();
    fake_device_controls.reset_settings_outcome = (device_controls_reset_settings_outcome_t){
        .settings_applied = false,
        .sessions_invalidated = false,
        .restart_owned = false,
        .primary_error = APP_ERROR_STORAGE_UNAVAILABLE,
        .restart_error = APP_ERROR_NONE,
    };
    char body[128];
    web_api_call_t call;
    prepare_reset_settings_call(&call, body, sizeof(body));
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_handle_administration(&call, &response));
    TEST_CHECK_EQ_U64(503U, response.status);
    cJSON *root = parse_response_body(&response);
    const cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
    TEST_CHECK_EQ_STRING("storage_unavailable",
                         cJSON_GetObjectItemCaseSensitive(error, "code")->valuestring);
    cJSON_Delete(root);
    web_api_response_free(&response);
}

static void test_reset_settings_session_failure_stays_202_when_reboot_owned(void) {
    reset_fakes();
    fake_device_controls.reset_settings_outcome = (device_controls_reset_settings_outcome_t){
        .settings_applied = true,
        .sessions_invalidated = false,
        .restart_owned = true,
        .primary_error = APP_ERROR_IO,
        .restart_error = APP_ERROR_NONE,
    };
    char body[128];
    web_api_call_t call;
    prepare_reset_settings_call(&call, body, sizeof(body));
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_handle_administration(&call, &response));
    TEST_CHECK_EQ_U64(202U, response.status);
    cJSON *root = parse_response_body(&response);
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "accepted")));
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "connectionWillClose")));
    TEST_CHECK(cJSON_IsFalse(cJSON_GetObjectItemCaseSensitive(root, "reprovisioningRequired")));
    TEST_CHECK(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(root, "repositoryBlobsPreserved")));
    cJSON_Delete(root);
    web_api_response_free(&response);
}

static void test_reset_settings_restart_failure_is_explicit_409(void) {
    reset_fakes();
    fake_device_controls.reset_settings_outcome = (device_controls_reset_settings_outcome_t){
        .settings_applied = true,
        .sessions_invalidated = false,
        .restart_owned = false,
        .primary_error = APP_ERROR_INTERNAL,
        .restart_error = APP_ERROR_IO,
    };
    char body[128];
    web_api_call_t call;
    prepare_reset_settings_call(&call, body, sizeof(body));
    web_api_response_t response = {0};
    TEST_CHECK_APP_ERROR(APP_ERROR_NONE, web_api_handle_administration(&call, &response));
    TEST_CHECK_EQ_U64(409U, response.status);
    cJSON *root = parse_response_body(&response);
    const cJSON *error = cJSON_GetObjectItemCaseSensitive(root, "error");
    TEST_CHECK_EQ_STRING("reset_settings_incomplete",
                         cJSON_GetObjectItemCaseSensitive(error, "code")->valuestring);
    TEST_CHECK_EQ_STRING("settings reset; automatic restart incomplete; restart the device",
                         cJSON_GetObjectItemCaseSensitive(error, "message")->valuestring);
    cJSON_Delete(root);
    web_api_response_free(&response);
}

'''
text = text.replace(anchor, tests + anchor, 1)
main_anchor = "    test_handle_device_restart_backend_unavailable();\n"
text = replace_once(
    text,
    main_anchor,
    main_anchor
    + "    test_reset_settings_precommit_failure_is_not_202();\n"
    + "    test_reset_settings_session_failure_stays_202_when_reboot_owned();\n"
    + "    test_reset_settings_restart_failure_is_explicit_409();\n",
    "administration reset main calls",
)
write(p, text)

# Signature-only fixes in route/async fixtures.
p = "tests/host/test_web_server_async_confirmation.c"
text = read(p)
old = """app_error_code_t device_controls_reset_settings(void) {
    ++g_reset_settings_calls;
    return APP_ERROR_NONE;
}
"""
new = """device_controls_reset_settings_outcome_t device_controls_reset_settings(void) {
    ++g_reset_settings_calls;
    return (device_controls_reset_settings_outcome_t){
        .settings_applied = true,
        .sessions_invalidated = true,
        .restart_owned = true,
        .primary_error = APP_ERROR_NONE,
        .restart_error = APP_ERROR_NONE,
    };
}
"""
text = replace_once(text, old, new, "async confirmation reset signature")
write(p, text)

p = "tests/host/test_web_server_administration_route.c"
text = read(p)
text = replace_once(
    text,
    "static app_error_code_t g_reset_settings_result;\n",
    "static device_controls_reset_settings_outcome_t g_reset_settings_outcome;\n",
    "route reset fake field",
)
old = """app_error_code_t device_controls_reset_settings(void) {
    ++g_reset_settings_calls;
    return g_reset_settings_result;
}
"""
new = """device_controls_reset_settings_outcome_t device_controls_reset_settings(void) {
    ++g_reset_settings_calls;
    return g_reset_settings_outcome;
}
"""
text = replace_once(text, old, new, "route reset fake signature")
# Any reset helper assignment to old scalar becomes a happy structured default.
text = text.replace(
    "g_reset_settings_result = APP_ERROR_NONE;",
    "g_reset_settings_outcome = (device_controls_reset_settings_outcome_t){\n"
    "        .settings_applied = true,\n"
    "        .sessions_invalidated = true,\n"
    "        .restart_owned = true,\n"
    "        .primary_error = APP_ERROR_NONE,\n"
    "        .restart_error = APP_ERROR_NONE,\n"
    "    };",
)
write(p, text)

# ---------------------------------------------------------------------------
# Lifecycle host regression for the new RAM authority gate.
# ---------------------------------------------------------------------------
p = "tests/host/test_web_server_lifecycle.c"
text = read(p)
text = replace_once(
    text,
    "static app_error_code_t g_factory_reset_state_read_result = APP_ERROR_NONE;\n",
    "static app_error_code_t g_factory_reset_state_read_result = APP_ERROR_NONE;\n"
    "static bool g_reset_settings_restart_required;\n",
    "lifecycle test reset-settings state",
)
anchor = """esp_err_t web_api_send_status_error(httpd_req_t *request, unsigned int status,
"""
if text.count(anchor) != 1:
    raise SystemExit("lifecycle error handler anchor")
text = text.replace(
    anchor,
    "bool device_controls_reset_settings_restart_required(void) {\n"
    "    return g_reset_settings_restart_required;\n"
    "}\n\n" + anchor,
    1,
)
text = replace_once(
    text,
    "    g_factory_reset_state_read_result = APP_ERROR_NONE;\n",
    "    g_factory_reset_state_read_result = APP_ERROR_NONE;\n"
    "    g_reset_settings_restart_required = false;\n",
    "lifecycle reset helper latch",
)
# Add a focused route-gate test before journal-read failure test.
anchor = "static void test_reset_journal_read_failure_denies_normal_api(void) {"
if text.count(anchor) != 1:
    raise SystemExit("lifecycle journal failure test anchor")
test = r'''static void test_reset_settings_restart_required_denies_normal_api(void) {
    reset_all();
    g_reset_settings_restart_required = true;
    web_server_config_t configuration = make_normal_config();
    TEST_CHECK_EQ_INT(APP_ERROR_NONE, web_server_start(&configuration));

    esp_err_t (*resolved_handler)(httpd_req_t *) = NULL;
    TEST_CHECK_EQ_INT(FAKE_HTTPD_ROUTE_FOUND,
                      fake_httpd_router_resolve("/api/v1/auth/login", HTTP_POST,
                                                &resolved_handler));
    httpd_req_t request = {0};
    TEST_CHECK_EQ_INT(ESP_OK, resolved_handler(&request));
    TEST_CHECK_EQ_U64(503U, g_reset_gate_status);
    TEST_CHECK_APP_ERROR(APP_ERROR_RESET_SETTINGS_INCOMPLETE, g_reset_gate_code);
    TEST_CHECK_EQ_STRING("reset-settings restart required", g_reset_gate_message);

    TEST_CHECK_EQ_INT(APP_ERROR_NONE, web_server_stop());
}

'''
text = text.replace(anchor, test + anchor, 1)
main_call = "    test_pending_factory_reset_denies_every_normal_api_route();\n"
text = replace_once(
    text,
    main_call,
    main_call + "    test_reset_settings_restart_required_denies_normal_api();\n",
    "lifecycle reset-settings gate main call",
)
write(p, text)

# ---------------------------------------------------------------------------
# HTTP status regression.
# ---------------------------------------------------------------------------
p = "tests/host/test_web_api_core.c"
text = read(p)
# Insert an assertion next to existing conflict mapping if exact call exists;
# otherwise before the function's closing marker based on RESET recovery test.
needle = "web_api_http_status_for_error(APP_ERROR_RESET_RECOVERY_REQUIRED)"
pos = text.find(needle)
if pos < 0:
    raise SystemExit("web API core reset recovery status assertion anchor missing")
line_end = text.find(";", pos)
if line_end < 0:
    raise SystemExit("web API core status assertion terminator missing")
insert_at = line_end + 1
text = (
    text[:insert_at]
    + "\n    TEST_CHECK_EQ_U64(409U, web_api_http_status_for_error(APP_ERROR_RESET_SETTINGS_INCOMPLETE));"
    + text[insert_at:]
)
write(p, text)

# ---------------------------------------------------------------------------
# H3 architecture guard: update shared scheduler order and close H3-034 only.
# ---------------------------------------------------------------------------
p = "scripts/check-h3-architecture.py"
text = read(p)
# Existing H3 order becomes ... temp -> restart ownership -> clear.
old = """    "operations->cleanup_temporary_files(operations->context)",
    "operations->clear_factory_reset_pending(operations->context)",
    "operations->schedule_restart(operations->context, delay_ms)",
)"""
new = """    "operations->cleanup_temporary_files(operations->context)",
    "operations->schedule_restart(operations->context, delay_ms)",
    "operations->clear_factory_reset_pending(operations->context)",
)"""
text = replace_once(text, old, new, "H3 shared factory order tuple")
text = replace_once(
    text,
    '    fail("factory-reset H3 order changed: mark -> settings -> sessions -> blobs -> temp -> clear -> reboot")',
    '    fail("factory-reset H3 order changed: mark -> settings -> sessions -> blobs -> temp -> restart ownership -> clear")',
    "H3 shared order message",
)
text = replace_once(
    text,
    'schedule_start = controls.find("static void adapter_reset_schedule_restart")',
    'schedule_start = controls.find("static app_error_code_t adapter_reset_schedule_restart")',
    "H3 scheduler signature guard",
)
# Retire the H3-033 guard that required H3-034 to remain open.
old = """h3_034 = todo.split("### H3-034 — Reset-settings semantics", 1)[1].split(
    "### H3-035 — Hardware interruption evidence", 1
)[0]
if "- [ ]" not in h3_034:
    fail("H3-034 was incorrectly closed by H3-033 work")"""
text = replace_once(text, old, "", "retire H3-033 H3-034-open assertion")
# Append H3-034 guard.
guard = r'''

# H3-034 reset-settings partial-completion semantics.
app_error_h = read("firmware/components/macro_model/include/app_error.h")
if "APP_ERROR_RESET_SETTINGS_INCOMPLETE" not in app_error_h:
    fail("H3-034 stable committed-partial reset-settings error is missing")
if app_error_h.index("APP_ERROR_RESET_SETTINGS_INCOMPLETE") < app_error_h.index(
    "APP_ERROR_RESET_RECOVERY_REQUIRED"
):
    fail("H3-034 app error must remain appended to preserve existing numeric values")

controls_public = read("firmware/components/device_controls/include/device_controls.h")
for required in (
    "device_controls_reset_settings_outcome_t",
    "bool settings_applied;",
    "bool sessions_invalidated;",
    "bool restart_owned;",
    "app_error_code_t primary_error;",
    "app_error_code_t restart_error;",
    "device_controls_reset_settings_restart_required",
):
    if required not in controls_public:
        fail(f"H3-034 structured reset-settings contract is missing: {required}")

reset_header = read("firmware/components/device_controls/device_controls_reset.h")
if "app_error_code_t (*schedule_restart)" not in reset_header:
    fail("H3-034 restart ownership is not observable through the reset-engine seam")
reset_source = read("firmware/components/device_controls/device_controls_reset.c")
reset_start = reset_source.find("device_controls_reset_engine_reset_settings")
factory_start = reset_source.find("device_controls_reset_engine_factory_reset")
if reset_start < 0 or factory_start < 0:
    fail("H3-034 reset engine functions are missing")
reset_block = reset_source[reset_start:factory_start]
for required in (
    "settings_applied = true",
    "sessions_invalidated = session_result == APP_ERROR_NONE",
    "restart_owned = restart_result == APP_ERROR_NONE",
    "primary_error = session_result",
    "restart_error = restart_result",
):
    if required not in reset_block:
        fail(f"H3-034 reset-settings outcome lost semantic field: {required}")
factory_block = reset_source[factory_start:]
restart_pos = factory_block.find("operations->schedule_restart(operations->context, delay_ms)")
clear_pos = factory_block.find("operations->clear_factory_reset_pending(operations->context)")
if restart_pos < 0 or clear_pos < 0 or restart_pos >= clear_pos:
    fail("factory reset must retain PENDING until restart ownership is established")

controls = read("firmware/components/device_controls/device_controls.c")
for required in (
    "static bool reset_settings_restart_required",
    "mark_reset_settings_restart_required()",
    "device_controls_reset_settings_restart_required(void)",
    "secure_zero_reset_settings(&settings)",
    "esp_restart();",
    "return APP_ERROR_INTERNAL;",
):
    if required not in controls:
        fail(f"H3-034 production reset-settings/restart binding is missing: {required}")

web_actions_h = read("firmware/components/web_server/web_device_actions.h")
for required in (
    "WEB_DEVICE_RESET_SETTINGS_REBOOT_RECOVERY_REQUIRED",
    "WEB_DEVICE_RESET_SETTINGS_COMMITTED_RESTART_INCOMPLETE",
    "app_error_code_t restart_detail;",
):
    if required not in web_actions_h:
        fail(f"H3-034 web outcome classification is missing: {required}")
web_actions_c = read("firmware/components/web_server/web_device_actions.c")
for required in (
    "reset.settings_applied",
    "reset.sessions_invalidated",
    "reset.restart_owned",
    "reset.primary_error",
    "reset.restart_error",
    "WEB_DEVICE_RESET_SETTINGS_REBOOT_RECOVERY_REQUIRED",
    "WEB_DEVICE_RESET_SETTINGS_COMMITTED_RESTART_INCOMPLETE",
):
    if required not in web_actions_c:
        fail(f"H3-034 web reset-settings mapping is missing: {required}")

administration = read("firmware/components/web_server/web_api_administration.c")
handler_start = administration.find("static app_error_code_t handle_device_reset_settings")
handler_end = administration.find("static app_error_code_t handle_device_factory_reset", handler_start)
if handler_start < 0 or handler_end < 0:
    fail("H3-034 reset-settings HTTP handler is missing")
handler = administration[handler_start:handler_end]
prepare_pos = handler.find("web_api_handler_success_json(&accepted_response")
backend_pos = handler.find("web_device_reset_settings_handle")
if prepare_pos < 0 or backend_pos < 0 or prepare_pos >= backend_pos:
    fail("reset-settings 202 response is not fully prepared before durable mutation")
for required in (
    "WEB_DEVICE_RESET_SETTINGS_REBOOT_RECOVERY_REQUIRED",
    "WEB_DEVICE_RESET_SETTINGS_COMMITTED_RESTART_INCOMPLETE",
    "APP_ERROR_RESET_SETTINGS_INCOMPLETE",
    "settings reset; automatic restart incomplete; restart the device",
):
    if required not in handler:
        fail(f"H3-034 HTTP partial-completion handling is missing: {required}")

lifecycle = read("firmware/components/web_server/web_server_lifecycle.c")
for required in (
    "device_controls_reset_settings_restart_required()",
    "APP_ERROR_RESET_SETTINGS_INCOMPLETE",
    "reset-settings restart required",
):
    if required not in lifecycle:
        fail(f"H3-034 pre-reboot authority gate is missing: {required}")

settings_test = read("tests/host/test_device_settings_core.c")
if "test_reset_preserves_credentials_and_blob_counter" not in settings_test:
    fail("H3-034 lost proof that noncredential reset preserves credentials/blob counter")
reset_test = read("tests/host/test_device_controls_reset.c")
for required in (
    "test_reset_settings_reports_session_failure_but_still_restarts",
    "test_reset_settings_reports_restart_failure_after_commit",
    "test_reset_settings_preserves_session_and_restart_errors_separately",
    "test_factory_reset_restart_ownership_failure_keeps_pending",
):
    if required not in reset_test:
        fail(f"H3-034 controls failure injection is missing: {required}")
web_test = read("tests/host/test_web_device_actions.c")
for required in (
    "test_reset_settings_session_failure_is_owned_by_reboot",
    "test_reset_settings_restart_failure_is_committed_partial",
):
    if required not in web_test:
        fail(f"H3-034 web business regression is missing: {required}")
admin_test = read("tests/host/test_web_api_administration.c")
for required in (
    "test_reset_settings_precommit_failure_is_not_202",
    "test_reset_settings_session_failure_stays_202_when_reboot_owned",
    "test_reset_settings_restart_failure_is_explicit_409",
):
    if required not in admin_test:
        fail(f"H3-034 HTTP regression is missing: {required}")
lifecycle_test = read("tests/host/test_web_server_lifecycle.c")
if "test_reset_settings_restart_required_denies_normal_api" not in lifecycle_test:
    fail("H3-034 pre-reboot normal-authority regression is missing")

web_api_core = read("firmware/components/web_server/web_api_core.c")
conflict_block = web_api_core.split("case APP_ERROR_CONFLICT:", 1)[1].split(
    "return WEB_HTTP_STATUS_CONFLICT;", 1
)[0]
if "APP_ERROR_RESET_SETTINGS_INCOMPLETE" not in conflict_block:
    fail("reset-settings committed-partial error no longer maps to HTTP 409")

todo = read("docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_2026-08-10.md")
h3_034 = todo.split("### H3-034 — Reset-settings semantics", 1)[1].split(
    "### H3-035 — Hardware interruption evidence", 1
)[0]
if "- [ ]" in h3_034:
    fail("H3-034 TODO still contains unchecked implementation/evidence items")
h3_035 = todo.split("### H3-035 — Hardware interruption evidence", 1)[1].split(
    "### Phase H3 exit gate", 1
)[0]
if "- [ ]" not in h3_035:
    fail("H3-035 was incorrectly closed by H3-034 work")

print("H3-034 reset-settings semantics guard passed")
'''
text = text.rstrip() + guard + "\n"
write(p, text)

# ---------------------------------------------------------------------------
# TODO: close exactly H3-034 and leave H3-035/hardware exit evidence open.
# ---------------------------------------------------------------------------
p = "docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_2026-08-10.md"
text = read(p)
start = text.index("### H3-034 — Reset-settings semantics")
end = text.index("### H3-035 — Hardware interruption evidence", start)
block = text[start:end]
if block.count("- [ ]") != 3:
    raise SystemExit(f"expected exactly three open H3-034 boxes, found {block.count('- [ ]')}")
block = block.replace("- [ ]", "- [x]")
evidence = (
    "\n- Evidence (2026-08-12): reset-settings now distinguishes precommit failure from "
    "durably-applied partial completion, retains session and restart failures separately, "
    "prebuilds the exact SPEC 202 response before mutation, and fails normal API authority "
    "closed until reboot. A session-invalidation failure remains accepted only when reboot "
    "ownership is established; restart-ownership failure is an explicit 409 "
    "`reset_settings_incomplete`. AP/admin credentials, provisioning state, and repository "
    "blobs remain preserved. See `docs/implementation-v2/H3_034_RESET_SETTINGS_SEMANTICS_2026-08-12.md`.\n\n"
)
block = block.rstrip() + evidence
text = text[:start] + block + text[end:]
write(p, text)
