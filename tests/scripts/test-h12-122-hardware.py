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
        manifest, {"firmwareVersion": "0.2.0", "buildId": "a" * 39}
    )
    assert observed == {"firmwareVersion": "0.2.0", "buildId": "a" * 39}

    for bad in (
        {"firmwareVersion": "0.2.0", "buildId": "b" * 39},
        {"firmwareVersion": "", "buildId": "a" * 39},
        {"firmwareVersion": "0.2.0", "buildId": "a" * 38},
        {"firmwareVersion": "0.2.0", "buildId": "not-hex"},
    ):
        try:
            module.validate_diagnostics(manifest, bad)
        except ValueError:
            pass
        else:
            raise AssertionError(f"invalid diagnostics unexpectedly accepted: {bad}")

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

    for forbidden in ("password", "setupCode", "session_token", "macroSource"):
        try:
            module.ensure_evidence_has_no_secret_keys({forbidden: "redacted"})
        except ValueError:
            pass
        else:
            raise AssertionError(f"secret-bearing evidence key unexpectedly accepted: {forbidden}")

    print("H12-122 hardware harness policy tests passed: 16")


if __name__ == "__main__":
    main()
