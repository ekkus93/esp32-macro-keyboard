# V2 device settings persistence foundation

**Phase:** 4 — Authentication, provisioning, and device settings

**Scope:** Canonical V2 settings persistence foundation for V2-040/V2-043

**Status:** Software foundation candidate; final exact-SHA permanent CI pending

## Boundary

This change establishes the canonical NVS-backed V2 device-settings store. It does not claim V2-040 provisioning completion, V2-043 live preference-route/UI completion, V2-044 Wi-Fi/reset completion, the V2-041 PBKDF2 hardware benchmark, or the Phase 4 exit gate.

The existing legacy `provisioning_config_t` runtime path is intentionally not adapted into this record. The next provisioning migration can instead move directly onto this canonical state and delete the legacy schema dependency rather than create another compatibility layer.

## Canonical record

The store persists the existing reviewed `app_v2_device_settings_t` fixed record from `app_contracts_v2`:

- record version 1;
- exactly 344 serialized bytes;
- exact credential/password-algorithm versions;
- Quick Send / Always Preview send mode;
- advisory snapshot-retention target;
- source-preview preference;
- serial-confirmation preference;
- opaque last-selected package UUID;
- device name;
- protected AP credentials;
- at most one configured station network;
- monotonic `next_blob_id` state.

The existing contract remains the single validator/encoder/decoder. The persistence layer does not duplicate or weaken that schema.

## Missing, corrupt, and unsupported records

A missing NVS record is the only condition that produces canonical unprovisioned defaults. The default object remains Quick Send, snapshot retention target 5, source previews hidden, and unprovisioned credentials/network fields empty.

All other storage-read failures propagate. Wrong record length, malformed data, and unsupported/corrupt serialized state fail closed as storage corruption. They are never silently replaced with defaults and are never reformatted or erased.

Caller-supplied invalid settings are deliberately classified separately as `APP_ERROR_INVALID_ARGUMENT`; they are not mislabeled as durable-storage corruption.

## Transactional update semantics

`device_settings_core_replace()` serializes the committed and candidate records canonically before deciding whether persistence is needed.

- Byte-identical canonical settings are a no-op: success with `changed=false` and no NVS write.
- A real update passes through one `replace_record_atomic` operation.
- The cached committed settings are updated only after that durable operation succeeds.
- A failed write or commit leaves the cached settings unchanged.
- Invalid candidates are rejected before persistence.

The ESP-IDF adapter writes through `nvs_set_blob()` followed by explicit `nvs_commit()`. If either stage fails, the adapter closes and reopens the namespace handle so staged handle state is discarded before later operations. No NVS erase, format, or destructive recovery fallback exists.

## Synchronization and secret handling

The production adapter owns a fixed mutex around public read/replace/reset operations. A mutex-release failure now returns `APP_ERROR_INTERNAL` unconditionally rather than being hidden when the protected operation had already returned another error.

Temporary serialized records and copied settings are explicitly zeroed through the injected secure-zero operation where they can contain credential material. No passwords, salts, verifiers, station credentials, or session material are logged by this component.

## Production build integration

`app_core` now explicitly depends on the `device_settings` ESP-IDF component. This makes the NVS adapter part of the production dependency graph and ensures normal firmware/Quality gates compile and analyze it even before V2-040 switches startup/provisioning reads onto the new store.

This is intentionally a build-time integration boundary, not a legacy-to-V2 runtime bridge.

## Permanent drift guard

`scripts/check-v2-device-settings-policy.py` is wired into both `scripts/check-all.sh` and `scripts/check-scripts.sh`.

It fails CI if the implementation drifts away from the reviewed invariants, including:

- only `NOT_FOUND` may enter the unprovisioned-default path;
- stored corruption remains a storage error;
- caller-invalid candidates remain distinct from storage corruption;
- duplicate canonical records are detected before persistence;
- cache advancement occurs after durable replacement;
- the NVS namespace/key remain `v2_settings` / `record`;
- reads use `nvs_get_blob` and writes use `nvs_set_blob` plus `nvs_commit`;
- failed write/commit paths reopen the NVS handle;
- mutex-release failure is surfaced;
- destructive `nvs_erase_all`, `nvs_flash_erase`, and `nvs_erase_key` fallbacks remain forbidden;
- the production app-core component explicitly depends on `device_settings`;
- regressions retain invalid-candidate, failed-write-preservation, and no-op-write coverage.

## Host regression coverage

`device_settings_core_tests` is a permanent Host Tests target. It covers:

- operation-table validation;
- absent-record canonical defaults without creating a record;
- repeated cached reads;
- no-op replacement without writes;
- valid fixed-record load;
- wrong-length rejection;
- record-version corruption rejection;
- successful replacement and reload;
- failed replacement preserving durable bytes and cached committed state;
- invalid candidate rejection before persistence;
- noncredential reset preserving credentials, protected AP configuration, and `next_blob_id` while restoring UI/station preferences to defaults.

## Ralph-loop findings

The focused integration loop found several defects and test-fixture errors before this foundation was accepted:

1. The first core build failed the repository's warning-as-error policy because an encoding helper carried an unused `core` parameter. The unnecessary parameter was removed; no warning was suppressed.
2. The first configured-settings fixture used credential version `7`, but the canonical record correctly requires `APP_V2_CREDENTIAL_VERSION == 1`. The fixture was repaired rather than weakening validation.
3. After caller-invalid settings were correctly separated from storage corruption at `60b013347f9d0233029ec6e371df8897b1305fe5`, permanent Host Tests exposed one stale expectation that still demanded `APP_ERROR_STORAGE_CORRUPT`. The regression was changed to require `APP_ERROR_INVALID_ARGUMENT`.
4. Source review found that mutex-release failure was visible only when the protected operation had otherwise succeeded. The adapter now returns `APP_ERROR_INTERNAL` for any release failure so synchronization failure cannot be silently masked.
5. The first production-hardening materializer failed before committing because its source-edit marker was indentation-sensitive. The marker was made semantic/regex-based; no production check was weakened or bypassed.

Focused materializer evidence includes:

- run `31202281333`: rejected the unused-parameter build defect;
- run `31202465052`: rejected the invalid V2 settings fixture;
- run `31202791579`, job `92946547669`: focused host target passed and permanently registered the test;
- Host Tests run `31202977934`: exposed the stale invalid-candidate error expectation after the production classification repair;
- run `31204791048`: rejected the brittle materializer marker before any permanent patch was committed;
- run `31204904346`, job `92953434594`: production hardening, policy guard, settings schema check, complete host test suite, and `git diff --check` all passed; the workflow then self-removed.

## Implementation commits

The permanent settings foundation culminates in:

- `5fb0a59b71f5fe6614655df39f07326200f68da9` — register the V2 settings host target;
- `60b013347f9d0233029ec6e371df8897b1305fe5` — separate caller validation from stored-record corruption;
- `803aea637ac797b62c2e20199b155b3814a285b4` — harden canonical V2 settings persistence, add the permanent policy guard, surface unlock failures, and add the production dependency.

The temporary materializer workflow is not part of the permanent tree.

## Exact-SHA validation boundary

The ordinary commit containing this report must pass all four permanent workflows on one exact SHA before this foundation is treated as settled:

- Browser Tests;
- Host Tests, including native tests, ASan/UBSan, native coverage, frontend tests, and frontend coverage;
- Device Test Build with ESP-IDF v5.5.5;
- Quality, including the authoritative checks, formatting, static analysis, script guards, production firmware build, and release checks.

No V2-043 checkbox is changed by this report. The live UI/preferences behavior and the invariant that preference changes never mutate repository blobs must still be wired and proven. V2-040 must still migrate first-run provisioning onto this canonical record. Hardware-dependent Phase 4 evidence remains explicitly deferred.
