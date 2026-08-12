#!/usr/bin/env python3
"""Fail closed on H3 factory-reset recovery-boundary regressions."""

from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]


def fail(message: str) -> None:
    print(f"error: {message}", file=sys.stderr)
    raise SystemExit(1)


def read(relative: str) -> str:
    path = ROOT / relative
    if not path.is_file():
        fail(f"required source is missing: {relative}")
    return path.read_text(encoding="utf-8", errors="replace")


app_error_h = read("firmware/components/macro_model/include/app_error.h")
if "APP_ERROR_RESET_RECOVERY_REQUIRED" not in app_error_h:
    fail("factory-reset recovery lost its stable app error")
if app_error_h.index("APP_ERROR_RESET_RECOVERY_REQUIRED") < app_error_h.index(
    "APP_ERROR_AUTH_STATE_INCOMPLETE"
):
    fail("H3 app error must remain appended so existing numeric error values do not change")

state_source = read("firmware/components/factory_reset_state/factory_reset_state.c")
for required in (
    'FACTORY_RESET_STATE_NAMESPACE "reset_journal"',
    'FACTORY_RESET_STATE_KEY "factory_reset"',
    "nvs_set_u8",
    "nvs_commit",
    "nvs_erase_key",
    "NVS_READONLY",
    "NVS_READWRITE",
):
    if required not in state_source:
        fail(f"durable factory-reset journal binding is missing: {required}")
for forbidden in ("storage_blob", "repository", "macro_repository"):
    if forbidden in state_source:
        fail(f"factory-reset journal became coupled to repository semantics: {forbidden}")

state_cmake = read("firmware/components/factory_reset_state/CMakeLists.txt")
for required in ("macro_model", "nvs_flash"):
    if required not in state_cmake:
        fail(f"factory-reset journal dependency is missing: {required}")
for forbidden in ("storage", "device_settings", "web_server"):
    if forbidden in state_cmake:
        fail(f"factory-reset journal has an unrelated dependency: {forbidden}")

reset_source = read("firmware/components/device_controls/device_controls_reset.c")
factory_start = reset_source.find("device_controls_reset_engine_factory_reset")
if factory_start < 0:
    fail("factory-reset engine is missing")
ordered = (
    "operations->mark_factory_reset_pending(operations->context)",
    "operations->erase_all_settings(operations->context)",
    "operations->invalidate_all_sessions(operations->context)",
    "operations->delete_all_blobs(operations->context)",
    "operations->clear_factory_reset_pending(operations->context)",
    "operations->schedule_restart(operations->context, delay_ms)",
)
positions = []
for required in ordered:
    position = reset_source.find(required, factory_start)
    if position < 0:
        fail(f"factory-reset H3 step is missing: {required}")
    positions.append(position)
if positions != sorted(positions):
    fail("factory-reset H3 order changed: mark -> settings -> sessions -> blobs -> clear -> reboot")
if "if (marker_result != APP_ERROR_NONE)" not in reset_source:
    fail("factory reset no longer aborts before destruction when marker commit fails")

controls = read("firmware/components/device_controls/device_controls.c")
for required in (
    "factory_reset_state_mark_pending()",
    "factory_reset_state_clear()",
    ".mark_factory_reset_pending = adapter_reset_mark_pending",
    ".clear_factory_reset_pending = adapter_reset_clear_pending",
):
    if required not in controls:
        fail(f"device-controls reset journal binding is missing: {required}")

app_core = read("firmware/components/app_core/app_core.c")
settings_init_start = app_core.find("static app_error_code_t adapter_settings_init")
if settings_init_start < 0:
    fail("app-core settings initialization adapter is missing")
state_read = app_core.find("factory_reset_state_read(&reset_state)", settings_init_start)
settings_init = app_core.find("device_settings_init()", settings_init_start)
if state_read < 0 or settings_init < 0 or state_read >= settings_init:
    fail("boot does not check durable factory-reset state before settings initialization")
for required in (
    "FACTORY_RESET_STATE_PENDING",
    "APP_ERROR_RESET_RECOVERY_REQUIRED",
    "factory reset recovery is pending",
):
    if required not in app_core[settings_init_start:settings_init]:
        fail(f"factory-reset boot gate is missing: {required}")

for cmake_path in (
    "firmware/components/app_core/CMakeLists.txt",
    "firmware/components/device_controls/CMakeLists.txt",
):
    if "factory_reset_state" not in read(cmake_path):
        fail(f"production dependency is missing from {cmake_path}")

reset_test = read("tests/host/test_device_controls_reset.c")
for required in (
    "test_factory_reset_marker_failure_is_nondestructive",
    "test_factory_reset_settings_failure_keeps_marker_and_restarts",
    "test_factory_reset_cleanup_failure_keeps_marker_and_restarts",
    "test_factory_reset_marker_clear_failure_reboots_into_recovery",
):
    if required not in reset_test:
        fail(f"H3 reset boundary regression coverage is missing: {required}")

state_test = read("tests/host/test_factory_reset_state_core.c")
for required in (
    "test_missing_marker_is_none",
    "test_pending_marker_round_trips",
    "test_unknown_marker_fails_closed",
):
    if required not in state_test:
        fail(f"H3 journal regression coverage is missing: {required}")

adapter_test = read("tests/host/test_factory_reset_state_adapter.c")
for required in (
    "test_pending_read_and_corrupt_value_fail_closed",
    "test_mark_pending_requires_commit",
    "test_clear_requires_commit_unless_already_absent",
):
    if required not in adapter_test:
        fail(f"H3 production journal adapter coverage is missing: {required}")

app_core_test = read("tests/host/test_app_core.c")
if "test_factory_reset_pending_blocks_all_runtime_startup" not in app_core_test:
    fail("H3 boot recovery gate regression coverage is missing")

print("H3 factory-reset recovery architecture guard passed")
