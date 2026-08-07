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

def require(source: str, fragment: str, description: str) -> None:
    if fragment not in source:
        raise SystemExit(f"V2 settings policy violation: {description}")

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
require(
    adapter,
    "const app_error_code_t reopen_result = reopen_settings_handle();",
    "failed NVS writes/commits must discard staged handle state by reopening",
)
require(
    adapter,
    "if (xSemaphoreGive(settings_mutex) != pdTRUE) {\n        return APP_ERROR_INTERNAL;\n    }",
    "mutex release failure must never be hidden behind an earlier result",
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

print("V2 device settings persistence policy checks passed")
