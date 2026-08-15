#!/usr/bin/env python3
from __future__ import annotations

import importlib.util
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[2]
HARDWARE = REPO / "tests" / "hardware"
sys.path.insert(0, str(HARDWARE))
SCRIPT = REPO / "scripts" / "run-h12-122-hardware.py"
spec = importlib.util.spec_from_file_location("h12_122", SCRIPT)
module = importlib.util.module_from_spec(spec)
assert spec.loader is not None
spec.loader.exec_module(module)


def main() -> None:
    manifest = {"diagnosticsBuildId": "a" * 39}
    observed = module.validate_diagnostics(
        manifest,
        {
            "firmwareVersion": "0.2.0",
            "buildId": "a" * 39,
            "resetReason": "power_on",
            "uptimeMs": 123456,
        },
    )
    assert observed == {
        "firmwareVersion": "0.2.0",
        "buildId": "a" * 39,
        "resetReason": "power_on",
        "uptimeMs": 123456,
    }

    valid_base = {
        "firmwareVersion": "0.2.0",
        "buildId": "a" * 39,
        "resetReason": "power_on",
        "uptimeMs": 123456,
    }
    for overrides in (
        {"buildId": "b" * 39},
        {"firmwareVersion": ""},
        {"buildId": "a" * 38},
        {"buildId": "not-hex"},
        {"resetReason": ""},
        {"uptimeMs": -1},
        {"uptimeMs": True},
    ):
        bad = {**valid_base, **overrides}
        try:
            module.validate_diagnostics(manifest, bad)
        except ValueError:
            pass
        else:
            raise AssertionError(f"invalid diagnostics unexpectedly accepted: {bad}")

    after_restart = {
        **observed,
        "resetReason": "software",
        "uptimeMs": 12000,
    }
    module.validate_restart_observation(observed, after_restart, 15000)
    for bad_after, elapsed_ms in (
        ({**after_restart, "resetReason": "power_on"}, 15000),
        ({**after_restart, "uptimeMs": 137000}, 15000),
        (after_restart, -1),
    ):
        try:
            module.validate_restart_observation(observed, bad_after, elapsed_ms)
        except ValueError:
            pass
        else:
            raise AssertionError(
                f"invalid restart observation unexpectedly accepted: {bad_after}, {elapsed_ms}"
            )

    original_resolve = module.provision_device.resolve_unprovisioned_address
    original_setup_state = module.provision_device.setup_state
    module.provision_device.resolve_unprovisioned_address = lambda ip, console: "192.0.2.10"
    module.provision_device.setup_state = lambda ip: (404, {})
    try:
        try:
            module.provision_device.provision(
                "192.0.2.10", "console", require_unprovisioned=True
            )
        except SystemExit:
            pass
        else:
            raise AssertionError(
                "H12 reprovision unexpectedly accepted an already-provisioned device"
            )
    finally:
        module.provision_device.resolve_unprovisioned_address = original_resolve
        module.provision_device.setup_state = original_setup_state

    original_runner_timeout = module.RESTART_TIMEOUT_S
    original_runner_connect = module.hil_state.connect_wifi
    module.RESTART_TIMEOUT_S = 0
    module.hil_state.connect_wifi = lambda console: (_ for _ in ()).throw(
        AssertionError("restart path attempted forbidden UART Wi-Fi recovery")
    )
    try:
        try:
            module.wait_for_authenticated_service(
                "192.0.2.10", "console", allow_uart_reconnect=False
            )
        except SystemExit:
            pass
        else:
            raise AssertionError("restart service wait unexpectedly succeeded")
    finally:
        module.RESTART_TIMEOUT_S = original_runner_timeout
        module.hil_state.connect_wifi = original_runner_connect

    original_provision_timeout = module.provision_device.RESTART_TIMEOUT_S
    original_provision_connect = module.provision_device.hil_state.connect_wifi
    module.provision_device.RESTART_TIMEOUT_S = 0
    module.provision_device.hil_state.connect_wifi = lambda console: (_ for _ in ()).throw(
        AssertionError("post-setup path attempted forbidden UART Wi-Fi recovery")
    )
    try:
        try:
            module.provision_device.wait_for_provisioned(
                "192.0.2.10", "console", allow_uart_reconnect=False
            )
        except SystemExit:
            pass
        else:
            raise AssertionError("post-setup service wait unexpectedly succeeded")
    finally:
        module.provision_device.RESTART_TIMEOUT_S = original_provision_timeout
        module.provision_device.hil_state.connect_wifi = original_provision_connect

    assert module.hil_state.console_argument("BenchWiFi") == '"BenchWiFi"'
    assert module.hil_state.console_argument("Revival Hall") == '"Revival Hall"'
    assert module.hil_state.console_argument(r"a\b") == '"a\\\\b"'
    assert module.hil_state.console_argument('a"b') == '"a\\"b"'
    try:
        module.hil_state.console_argument("bad\ncommand")
    except ValueError:
        pass
    else:
        raise AssertionError("console argument with newline unexpectedly accepted")

    clean = {
        "firmwareSha": "c" * 40,
        "manifestBuildId": "a" * 39,
        "steps": [{"name": "passwordChange", "result": "passed"}],
    }
    module.ensure_evidence_has_no_secret_keys(clean)

    app_main = (REPO / "firmware" / "main" / "app_main.c").read_text(encoding="utf-8")
    console_start = app_main.index("serial_console_start()")
    app_start = app_main.index("app_core_start()")
    assert console_start < app_start
    assert "return;" in app_main[console_start:app_start]

    serial_console = (
        REPO / "firmware" / "components" / "serial_console" / "serial_console.c"
    ).read_text(encoding="utf-8")
    command_start = serial_console.index("static int command_setup_code")
    command_end = serial_console.index("static int command_wifi_connect", command_start)
    command_body = serial_console[command_start:command_end]
    assert "xSemaphoreTake(setup_code_mutex, portMAX_DELAY)" in command_body
    take_position = command_body.index("xSemaphoreTake(setup_code_mutex, portMAX_DELAY)")
    format_position = command_body.index('snprintf(output, sizeof(output), "setup code: %s\\n", setup_code)')
    write_position = command_body.index("uart_write_bytes(UART_NUM_0, output, (size_t)output_length)")
    give_position = command_body.index("xSemaphoreGive(setup_code_mutex)", write_position)
    assert take_position < format_position < write_position < give_position
    assert 'printf("setup code: %s' not in command_body
    clear_start = serial_console.index("void serial_console_clear_setup_code")
    clear_end = serial_console.index("static void clear_local_text", clear_start)
    clear_body = serial_console[clear_start:clear_end]
    assert "configASSERT(xSemaphoreTake(setup_code_mutex, portMAX_DELAY) == pdTRUE)" in clear_body
    serial_start = serial_console.index("app_error_code_t serial_console_start")
    assert "if (!ensure_setup_code_mutex())" in serial_console[serial_start:]

    harness_source = SCRIPT.read_text(encoding="utf-8")
    assert '"flashDevice": args.flash_port' in harness_source
    assert '"consoleDevice": args.console' in harness_source
    assert '"buildType": manifest["buildType"]' in harness_source
    assert '"espIdfVersion": manifest["espIdfVersion"]' in harness_source
    assert harness_source.count("if not SHA40_RE.fullmatch(args.firmware_sha):") == 1
    assert "UART Wi-Fi recovery is forbidden for restart acceptance" in harness_source
    assert "wait_for_unprovisioned_uart(console)" in harness_source
    assert "require_unprovisioned=True" in harness_source
    assert "allow_post_setup_uart_reconnect=False" in harness_source
    assert "--flash-port and --console must identify distinct devices" in harness_source
    assert "H12 evidence is immutable" in harness_source
    assert "release manifest remained byte-identical through flashing" in harness_source
    assert 'steps.append({"name": "login", "result": "passed"})' in harness_source
    assert "pre-reset administrator password rejected after reprovision" in harness_source
    assert '"preRestartUptimeMs": pre_restart_diag["uptimeMs"]' in harness_source
    assert '"postRestartUptimeMs": restart_diag["uptimeMs"]' in harness_source

    provision_source = (HARDWARE / "provision_device.py").read_text(encoding="utf-8")
    no_reconnect = provision_source.index("if not allow_uart_reconnect:")
    uart_reconnect = provision_source.index("refreshed = hil_state.connect_wifi(console)")
    assert no_reconnect < uart_reconnect

    for forbidden in ("password", "setupCode", "session_token", "macroSource"):
        try:
            module.ensure_evidence_has_no_secret_keys({forbidden: "redacted"})
        except ValueError:
            pass
        else:
            raise AssertionError(f"secret-bearing evidence key unexpectedly accepted: {forbidden}")

    print("H12-122 hardware harness policy tests passed: 33")


if __name__ == "__main__":
    main()
