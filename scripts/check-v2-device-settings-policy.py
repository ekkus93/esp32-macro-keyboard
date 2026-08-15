#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

core = (ROOT / "firmware/components/device_settings/device_settings_core.c").read_text(
    encoding="utf-8"
)
adapter = (ROOT / "firmware/components/device_settings/device_settings.c").read_text(
    encoding="utf-8"
)
app_core_cmake = (ROOT / "firmware/components/app_core/CMakeLists.txt").read_text(
    encoding="utf-8"
)
host_test = (ROOT / "tests/host/test_device_settings_core.c").read_text(encoding="utf-8")
app_core_sequence = (ROOT / "firmware/components/app_core/app_core_sequence.c").read_text(
    encoding="utf-8"
)
app_core_test = (ROOT / "tests/host/test_app_core.c").read_text(encoding="utf-8")
serial_console = (ROOT / "firmware/components/serial_console/serial_console.c").read_text(
    encoding="utf-8"
)
serial_console_cmake = (ROOT / "firmware/components/serial_console/CMakeLists.txt").read_text(
    encoding="utf-8"
)
hil_state = (ROOT / "tests/hardware/hil_state.py").read_text(encoding="utf-8")


def require(source: str, fragment: str, description: str) -> None:
    if fragment not in source:
        raise SystemExit(f"V2 settings policy violation: {description}")


def require_order(source: str, fragments: tuple[str, ...], description: str) -> None:
    previous = -1
    for fragment in fragments:
        position = source.find(fragment, previous + 1)
        if position < 0 or position <= previous:
            raise SystemExit(f"V2 settings policy violation: {description}")
        previous = position


def section(source: str, start: str, end: str, description: str) -> str:
    start_index = source.find(start)
    if start_index < 0:
        raise SystemExit(f"V2 settings policy violation: {description} start missing")
    end_index = source.find(end, start_index + len(start))
    if end_index < 0:
        raise SystemExit(f"V2 settings policy violation: {description} end missing")
    return source[start_index:end_index]


require(
    core,
    "if (read_result == APP_ERROR_NOT_FOUND)",
    "only an absent record may enter the unprovisioned-default path",
)
require(
    core,
    "return APP_ERROR_STORAGE_CORRUPT;",
    "stored record length/decode corruption must remain a hard storage error",
)
require(
    core,
    "map_decode_result(decode_result)",
    "stored record decode failures must use storage-error mapping",
)
require(
    core,
    "map_candidate_result(result)",
    "caller-supplied settings validation must be distinct from storage corruption",
)
require(
    core,
    "memcmp(current_record, candidate_record, sizeof(current_record)) == 0",
    "duplicate settings records must be detected before persistence",
)
require(
    core,
    "core->ops.replace_record_atomic",
    "settings updates must pass through the transactional persistence operation",
)
replace_index = core.index("core->ops.replace_record_atomic")
cache_index = core.index("core->current = *settings;")
if cache_index <= replace_index:
    raise SystemExit(
        "V2 settings policy violation: cached settings advance before durable replacement succeeds"
    )

require(adapter, '#define DEVICE_SETTINGS_NAMESPACE "v2_settings"', "V2 NVS namespace drift")
require(adapter, '#define DEVICE_SETTINGS_RECORD_KEY "record"', "V2 NVS record key drift")
require(adapter, "nvs_get_blob", "V2 settings must load from NVS")
require(adapter, "nvs_set_blob", "V2 settings must write through NVS")
require(adapter, "nvs_commit", "V2 settings writes must commit explicitly")

replace_record = section(
    adapter,
    "static app_error_code_t replace_record_atomic",
    "static device_settings_core_ops_t settings_operations",
    "settings transactional replacement",
)
require(
    replace_record,
    "(void)reopen_settings_handle();",
    "failed NVS set must discard staged handle state",
)
require(
    replace_record,
    "return set_result;",
    "failed NVS set cleanup must preserve the initiating write error",
)
require(
    replace_record,
    "return reconcile_failed_commit(record, commit_result);",
    "failed NVS commit must reconcile canonical state before deciding the result",
)
require_order(
    replace_record,
    (
        "nvs_set_blob",
        "if (set_result != APP_ERROR_NONE)",
        "(void)reopen_settings_handle();",
        "return set_result;",
        "nvs_commit",
        "if (commit_result == APP_ERROR_NONE)",
        "return reconcile_failed_commit(record, commit_result);",
    ),
    "settings replacement must preserve set-error provenance and reconcile commit uncertainty",
)

reconcile_commit = section(
    adapter,
    "static app_error_code_t reconcile_failed_commit",
    "static app_error_code_t replace_record_atomic",
    "failed settings commit reconciliation",
)
require(
    reconcile_commit,
    "if (reopen_settings_handle() != APP_ERROR_NONE)",
    "failed commit reconciliation must reopen NVS before reading canonical state",
)
require(
    reconcile_commit,
    "return APP_ERROR_COMMIT_UNCERTAIN;",
    "unreadable canonical state after commit failure must remain commit-uncertain",
)
require(
    reconcile_commit,
    "read_record(NULL, persisted, sizeof(persisted), &persisted_length)",
    "failed commit reconciliation must reread authoritative persisted bytes",
)
require(
    reconcile_commit,
    "memcmp(persisted, candidate, APP_V2_SETTINGS_RECORD_BYTES) == 0",
    "failed commit reconciliation must compare exact candidate bytes",
)
require(
    reconcile_commit,
    "return candidate_is_canonical ? APP_ERROR_NONE : commit_error;",
    "commit reconciliation must distinguish exact success from a different canonical record",
)
require_order(
    reconcile_commit,
    (
        "reopen_settings_handle()",
        "read_record(NULL, persisted, sizeof(persisted), &persisted_length)",
        "memcmp(persisted, candidate, APP_V2_SETTINGS_RECORD_BYTES) == 0",
    ),
    "failed commit reconciliation must reopen, reread, then compare canonical bytes",
)

finish_locked = section(
    adapter,
    "static app_error_code_t finish_locked",
    "app_error_code_t device_settings_init",
    "settings mutex release",
)
require(
    finish_locked,
    "if (xSemaphoreGive(settings_mutex) != pdTRUE && result == APP_ERROR_NONE)",
    "mutex release failure may replace success but must not overwrite an initiating operation error",
)
require(
    finish_locked,
    "return result;",
    "mutex release path must preserve an existing operation error",
)

for forbidden in ("nvs_erase_all", "nvs_flash_erase", "nvs_erase_key"):
    if forbidden in adapter:
        raise SystemExit(
            f"V2 settings policy violation: forbidden destructive fallback {forbidden}"
        )

require(
    app_core_cmake,
    "    device_settings\n",
    "production app_core must explicitly depend on the V2 settings component",
)
require(
    host_test,
    "TEST_CHECK_APP_ERROR(APP_ERROR_INVALID_ARGUMENT,",
    "invalid caller settings must be covered as invalid arguments",
)
require(
    host_test,
    "device_settings_core_replace(&core, &failed, &changed));",
    "invalid caller settings replacement path must remain covered",
)
require(
    host_test,
    "TEST_CHECK_EQ_BUFFER(durable_before, fake.durable, sizeof(durable_before));",
    "failed replacement must prove durable settings preservation",
)
require(
    host_test,
    "TEST_CHECK_EQ_U64(0U, fake.replace_calls);",
    "no-op settings writes must be covered",
)
require(
    host_test,
    'device_settings_core_set_station(&core, "BenchWiFi", "bench-passphrase", &changed)',
    "serial-station mutation must have a direct core regression",
)
require(
    serial_console_cmake,
    "    device_settings\n",
    "serial console must depend on the authoritative V2 settings store",
)
if "provisioning\n" in serial_console_cmake:
    raise SystemExit(
        "V2 settings policy violation: serial console must not depend on retired provisioning storage"
    )
require(
    serial_console,
    "device_settings_set_station(argv[1], argv[2], &changed)",
    "wifi-connect must persist through the authoritative V2 settings store",
)
for forbidden in ("provisioning_set_station", "provisioning_clear_credentials", "credential-reset"):
    if forbidden in serial_console:
        raise SystemExit(
            f"V2 settings policy violation: serial console contains retired path {forbidden}"
        )
require(
    serial_console,
    "connection established but credentials were not persisted",
    "wifi-connect must surface durable-persistence failure explicitly",
)
require(
    serial_console,
    "return 1;",
    "wifi-connect persistence failure must be a command failure",
)
require(
    app_core_sequence,
    ".station_configured = secrets->settings.station_configured",
    "setup-mode reboot must reuse a UART-configured durable station",
)
require(
    app_core_test,
    'TEST_CHECK_EQ_STRING("Setup Station", fixture.observed_station_ssid);',
    "setup-mode durable-station reuse must remain host-tested",
)
require(
    hil_state,
    'persisted = b"will reconnect at boot" in buffer',
    "HIL Wi-Fi helper must require an explicit durable-persistence acknowledgement",
)
require(
    hil_state,
    "device joined Wi-Fi but did not confirm durable station persistence",
    "HIL Wi-Fi helper must fail visibly on temporary-only association",
)

print("V2 device settings persistence policy checks passed")
