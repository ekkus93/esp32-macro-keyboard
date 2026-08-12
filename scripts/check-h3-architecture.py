#!/usr/bin/env python3
"""Fail closed on H3 factory-reset journal/recovery regressions."""

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
    "operations->cleanup_temporary_files(operations->context)",
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
    fail("factory-reset H3 order changed: mark -> settings -> sessions -> blobs -> temp -> clear -> reboot")
if "if (marker_result != APP_ERROR_NONE)" not in reset_source:
    fail("factory reset no longer aborts before destruction when marker commit fails")

controls = read("firmware/components/device_controls/device_controls.c")
for required in (
    "factory_reset_state_mark_pending()",
    "factory_reset_state_clear()",
    "storage_blob_recover_startup()",
    ".mark_factory_reset_pending = adapter_reset_mark_pending",
    ".cleanup_temporary_files = adapter_reset_cleanup_temporary_files",
    ".clear_factory_reset_pending = adapter_reset_clear_pending",
    "static bool restart_scheduled",
):
    if required not in controls:
        fail(f"device-controls H3 binding is missing: {required}")

recovery = read("firmware/components/factory_reset_recovery/factory_reset_recovery.c")
for required in (
    "factory_reset_state_read(out_state)",
    "device_settings_factory_reset(&settings, &changed)",
    "storage_blob_delete_all(&deleted_count)",
    "storage_blob_recover_startup()",
    "factory_reset_state_clear()",
):
    if required not in recovery:
        fail(f"boot reset recovery binding is missing: {required}")

recovery_engine = read("firmware/components/factory_reset_recovery/factory_reset_recovery_engine.c")
ordered_recovery = (
    "operations->settings_init(operations->context)",
    "operations->erase_settings(operations->context)",
    "operations->settings_deinit(operations->context)",
    "operations->storage_mount(operations->context)",
    "operations->delete_blobs(operations->context)",
    "operations->cleanup_temporary_files(operations->context)",
    "operations->storage_unmount(operations->context)",
    "operations->clear_pending(operations->context)",
)
positions = []
for required in ordered_recovery:
    position = recovery_engine.find(required)
    if position < 0:
        fail(f"boot reset recovery stage is missing: {required}")
    positions.append(position)
if positions != sorted(positions):
    fail("boot reset recovery order changed")

recovery_cmake = read("firmware/components/factory_reset_recovery/CMakeLists.txt")
for required in ("factory_reset_state", "device_settings", "storage"):
    if required not in recovery_cmake:
        fail(f"factory-reset recovery dependency is missing: {required}")
for forbidden in ("auth", "web_server", "usb_keyboard", "device_controls"):
    if forbidden in recovery_cmake:
        fail(f"boot reset recovery gained an unsafe runtime dependency: {forbidden}")

app_core = read("firmware/components/app_core/app_core.c")
settings_init_start = app_core.find("static app_error_code_t adapter_settings_init")
if settings_init_start < 0:
    fail("app-core settings initialization adapter is missing")
recovery_call = app_core.find("factory_reset_recovery_run_if_pending(&recovered)", settings_init_start)
settings_init = app_core.find("device_settings_init()", settings_init_start)
if recovery_call < 0 or settings_init < 0 or recovery_call >= settings_init:
    fail("boot does not finish pending factory-reset recovery before ordinary settings initialization")
if "factory_reset_state_read" in app_core[settings_init_start:settings_init]:
    fail("app-core bypasses the dedicated reset recovery component")

if "factory_reset_recovery" not in read("firmware/components/app_core/CMakeLists.txt"):
    fail("app-core factory-reset recovery dependency is missing")
if "factory_reset_state" not in read("firmware/components/device_controls/CMakeLists.txt"):
    fail("device-controls reset journal dependency is missing")

storage_blob = read("firmware/components/storage/storage_blob.c")
delete_all_start = storage_blob.find("app_error_code_t storage_blob_delete_all(size_t *out_deleted_count)")
if delete_all_start < 0:
    fail("production bulk blob deletion is missing")
if "storage_fs_sync_parent_path(NULL, STORAGE_BLOB_DIRECTORY \"/.\")" not in storage_blob[delete_all_start:]:
    fail("bulk blob deletion does not durably sync the repository parent")

reset_test = read("tests/host/test_device_controls_reset.c")
for required in (
    "test_factory_reset_marker_failure_is_nondestructive",
    "test_factory_reset_settings_failure_keeps_marker_and_restarts",
    "test_factory_reset_cleanup_failure_keeps_marker_and_restarts",
    "test_factory_reset_temporary_cleanup_failure_keeps_marker_and_restarts",
    "test_factory_reset_marker_clear_failure_reboots_into_recovery",
    "test_factory_reset_replay_is_safe",
):
    if required not in reset_test:
        fail(f"H3 reset regression coverage is missing: {required}")

recovery_test = read("tests/host/test_factory_reset_recovery.c")
for required in (
    "test_pending_completes_and_reentry_is_noop",
    "test_every_interrupted_stage_is_retryable",
    "test_blob_and_temporary_cleanup_both_attempted",
):
    if required not in recovery_test:
        fail(f"H3 boot recovery regression coverage is missing: {required}")

settings_test = read("tests/host/test_device_settings_core.c")
if "test_factory_reset_is_idempotent" not in settings_test:
    fail("H3 repeated settings/credential erase regression is missing")

auth_test = read("tests/host/auth_additional_session_tests.inc")
if "H3 repeated invalidation remains a successful no-op" not in auth_test:
    fail("H3 repeated session invalidation regression is missing")

storage_test = read("tests/host/test_storage_blob.c")
if "H3 repeated temporary cleanup remains a successful no-op" not in storage_test:
    fail("H3 repeated temporary-debris cleanup regression is missing")

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

print("H3 factory-reset recovery architecture guard passed")
