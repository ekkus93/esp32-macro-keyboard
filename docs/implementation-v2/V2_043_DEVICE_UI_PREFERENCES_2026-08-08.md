# V2-043 — Device UI preferences

**Phase:** 4 — Authentication, provisioning, and device settings

**Scope:** `docs/TODO_V2.md` §"V2-043 — Device UI preferences"

**Depends on:** `V2_DEVICE_SETTINGS_PERSISTENCE_FOUNDATION_2026-08-07.md`, which built the
canonical NVS-backed `app_v2_device_settings_t` store and explicitly deferred both the
V2-043 checklist and the repository-isolation invariant ("No V2-043 checkbox is changed
by this report. ... the invariant that preference changes never mutate repository blobs
must still be wired and proven.").

## Audit-first summary

Per the task instructions this work started with an audit of the existing
`device_settings_v2`/`device_settings` code against the V2-043 checklist, item by item,
before writing anything. Five of six checklist items were already correctly implemented
by the persistence-foundation work; they lacked only the specific regression evidence
called for by this checklist item. One item (the cross-subsystem repository-isolation
invariant) had no coverage at all and required a new test.

| Checklist item | Already correct? | Evidence added this change |
| --- | --- | --- |
| NVS-backed Quick Send/Always Preview, default Quick Send | Yes | Existing + new regressions below |
| Advisory snapshot retention target, default five | Yes | Existing + new regressions below |
| Source-preview setting, default hidden | Yes | Existing regression already covered it |
| Opaque `lastSelectedPackageId` | Yes (by construction — no repository code exists in firmware to couple to) | New explicit opaqueness regression |
| Suppress duplicate NVS writes | Yes | New regression against a *previously-set* (non-default) value |
| Preferences never touch a repository blob | **No — unimplemented as evidence** | New cross-subsystem test |

No production code changed. All work was additive test evidence, except that none of the
audited behavior needed a fix — the persistence foundation had already implemented it
correctly.

## Where the behavior lives

- `firmware/components/app_contracts_v2/include/device_settings_v2.h` /
  `device_settings_v2.c` — the fixed-layout `app_v2_device_settings_t` record, its
  `send_mode`, `snapshot_retention_target`, `show_macro_source_previews`, and
  `last_selected_package_id` fields, `app_v2_device_settings_init_unprovisioned()`
  (defaults: `APP_V2_SEND_MODE_QUICK`, retention `5`, previews hidden, package id
  empty), and `app_v2_device_settings_reset_noncredential()` (restores the same
  defaults on **Reset settings**, per SPEC_V2 §11.4).
- `firmware/components/device_settings/device_settings_core.c` — durability-agnostic
  core: canonical-record no-op-write detection (`memcmp` of the encoded current vs.
  candidate record before ever calling `replace_record_atomic`), missing-record
  defaulting without a write, and reset semantics.
- `firmware/components/device_settings/device_settings.c` — the real ESP-IDF NVS
  adapter (`nvs_open`/`nvs_get_blob`/`nvs_set_blob`/`nvs_commit`, namespace
  `v2_settings`, key `record`), exercised by `./scripts/check-firmware.sh` (clang-tidy
  over the esp-clang compile database) since host tests cannot link real NVS.
- `firmware/components/device_settings/CMakeLists.txt` — `REQUIRES app_contracts_v2
  macro_model nvs_flash freertos`. **No dependency on `storage`.** This is the
  structural half of the repository-isolation evidence: the component that persists
  UI preferences cannot call into the blob repository even if it wanted to, because it
  is never linked against it.
- Nothing in `firmware/components/storage/*` references `retention`, `send_mode`, or
  `last_selected_package_id` (confirmed by repository-wide grep); the blob-scan/list/
  delete code paths have no notion of user preferences and cannot act on them.

## Checklist evidence

### NVS-backed Quick Send/Always Preview mode, defaulting to Quick Send

- Default: `test_missing_record_uses_defaults_without_write` asserts
  `loaded.send_mode == APP_V2_SEND_MODE_QUICK` on an absent record
  (`tests/host/test_device_settings_core.c`).
- Persisted change + reload: `test_replace_and_failure_preservation` sets
  `APP_V2_SEND_MODE_PREVIEW`, replaces, reloads through a fresh core instance, and
  asserts the reloaded value.
- Real NVS wiring (`nvs_open`/`nvs_get_blob`/`nvs_set_blob`/`nvs_commit`) is exercised
  by `./scripts/check-firmware.sh` (compiles and clang-tidies `device_settings.c`) —
  host tests substitute a fake `device_settings_core_ops_t` because NVS itself is not
  host-linkable, matching the existing pattern used by `provisioning_core`.

### Advisory snapshot retention target, default five

- Default: same `test_missing_record_uses_defaults_without_write` asserts
  `loaded.snapshot_retention_target == 5`.
- Range validation (0..100, `APP_V2_SNAPSHOT_RETENTION_TARGET_MAX`) already lived in
  `app_v2_device_settings_validate()`.
- "Advisory" (never causes automatic deletion): proven structurally, not just by
  absence of a test — `storage_blob_*` (the only code that deletes blobs) never reads
  `app_v2_device_settings_t` at all, and the new
  `test_preference_changes_never_touch_repository` test (below) exercises retention
  values `0` and `100` against a live repository directory and shows zero blob
  deletion or mutation.

### Source-preview setting, default hidden

- `test_missing_record_uses_defaults_without_write` asserts
  `!loaded.show_macro_source_previews`. No further work was needed; this was already
  fully covered.

### Opaque `lastSelectedPackageId`

- Firmware performs **no** lookup, resolution, or existence check of this field
  against any repository, package, or blob concept — because v2 firmware carries no
  repository model at all (deleted per the v1→v2 migration; see
  `docs/implementation-v2/V2_MIGRATION_MAP.md`). `app_v2_device_settings_validate()`
  only checks UUID *syntax* (`valid_uuid_or_empty`), never membership.
- New `test_last_selected_package_id_is_opaque`
  (`tests/host/test_device_settings_core.c`) sets the field to a UUID that identifies
  nothing anywhere in the test, round-trips it, then clears it back to empty — proving
  both a set value and an unset value are accepted purely on syntax with no other
  side effect.
- The new cross-subsystem isolation test (below) additionally sets it while a real
  repository directory exists on disk and shows the directory is untouched.

### Suppress duplicate NVS writes when a value has not changed

- This was already implemented by `device_settings_core_replace()`: it encodes both
  the currently-cached record and the candidate record and does a byte-for-byte
  `memcmp` before ever invoking `replace_record_atomic` (see
  `firmware/components/device_settings/device_settings_core.c`).
- The existing `test_missing_record_uses_defaults_without_write` only proved this for
  the *all-defaults* case (never-written record vs. itself). New
  `test_duplicate_write_suppressed_after_prior_value` proves the stronger and more
  realistic case: an already-written, non-default record replayed unchanged (same
  struct instance, then an independently-constructed but field-identical copy) still
  produces `out_changed == false` and no additional `replace_record_atomic` call, while
  changing exactly one field does trigger exactly one additional write.

### Changing UI preferences must never create or change a repository blob

**This was the one checklist item with no existing evidence** — the persistence
foundation report explicitly flagged it as still open. New
`tests/host/test_device_settings_repository_isolation.c`
(`test_preference_changes_never_touch_repository`):

1. Creates a real on-disk repository directory (`storage_blob_prepare_directory_with_ops`)
   and writes two real blob files with known byte content, using the same
   `storage_blob_*` primitives and POSIX filesystem ops the firmware uses.
2. Snapshots the repository: a real `storage_blob_scan_with_ops` (entry ids and byte
   counts) plus a raw read of both files' exact bytes.
3. Initializes a `device_settings_core_t` backed by a completely separate in-memory
   fake NVS record — no shared state, path, or type with the repository directory —
   and drives it through `send_mode` (quick → preview → quick), `snapshot_retention_target`
   (0, then 100), `show_macro_source_previews` (on), `last_selected_package_id` (set to
   an arbitrary UUID), and a full `device_settings_core_reset_noncredential()`.
4. Re-snapshots the repository and asserts: identical `valid_count`,
   `invalid_name_count`, `temporary_file_count`, `max_id`; identical entry
   ids/byte-counts; and byte-for-byte identical file contents (`TEST_CHECK_EQ_BUFFER`)
   for both blobs.

Combined with the structural fact that `device_settings`'s `CMakeLists.txt` never
`REQUIRES storage`, this closes the checklist item with both a build-time (cannot link)
and a runtime (byte-identical before/after) proof.

## Commands run

All commands were run from the repository root
(`/home/phil/work/esp32-macro-keyboard/.claude/worktrees/agent-a93d09a07326d1dde`) on
branch `v2-043-device-ui-preferences`.

```console
$ ./scripts/run-tests.sh storage
...
100% tests passed, 0 tests failed out of 13
(includes: device_settings_core, device_settings_repository_isolation)

$ ./scripts/run-tests.sh
...
100% tests passed, 0 tests failed out of 40

$ python3 scripts/check-v2-device-settings-policy.py
V2 device settings persistence policy checks passed

$ ./scripts/check-format.sh
... (clang-format, cmake-format/cmake-lint, shfmt, prettier all clean)

$ . "$HOME/esp/esp-idf-v5.5.5/export.sh" && ./scripts/check-firmware.sh
...
SCRIPT_EXIT=0
(zero clang-tidy warning:/error: matches across firmware/ and firmware/test_app/,
confirmed by two independent synchronous runs and by grepping the captured log for the
`:LINE:COL: (warning|error):` pattern used by `run_first_party_clang_tidy` — no matches)

$ . "$HOME/esp/esp-idf-v5.5.5/export.sh" && export NVM_DIR="$HOME/.nvm" && nvm use && ./scripts/check-all.sh
... (verify-toolchain, check-format, static-analysis-policy, partitions, v2-034-capacity,
v2-device-settings-policy, production-config, credential-logging, mount-policy,
layer-boundaries, removed-features, v2-phase2-architecture, usb-identity,
frontend-persisted-state, setup-route-isolation, v2-auth-policy, v2-contracts
--native-only, check-firmware, stack-usage, build-webfs-image, generate-flash-manifest,
release-budgets, check-webapp [ci, format:check, typecheck, lint, stylelint, test
(298/298), test:coverage, build, test:browser, local-assets], check-scripts,
check-docs, run-tests all ran)
CHECK_ALL_EXIT=0
100% tests passed, 0 tests failed out of 40
```

`check-all.sh` was run as a real, synchronous, blocking process (`bash
run_check_all.sh > check_all.log 2>&1; echo "CHECK_ALL_EXIT=$?"`). It exceeded the
10-minute synchronous cap of the invoking shell tool once (that invocation was
transparently continued to completion by the same process on disk rather than
restarted), and the final captured log — read directly, not summarized from memory —
ends with `CHECK_ALL_EXIT=0` and the CTest summary `100% tests passed, 0 tests failed
out of 40`. The full log was additionally swept for `error:`, `fail`, `✗`, `✖`, and
`npm ERR` with zero genuine matches (the only `fail`-containing lines are the
`check-scripts.sh` self-tests' own descriptive names for scenarios the analyzer
harness must correctly reject, e.g. `ok analyzer nonzero exit with no warnings fails`,
each reported `ok`).

## What the integrator needs to know

- No production firmware behavior changed in this change — every checklist item but
  one was already correctly implemented by the persistence-foundation work; this change
  is entirely new regression evidence plus one new cross-subsystem test file.
- `tests/host/test_device_settings_repository_isolation.c` is a new CTest target
  (`device_settings_repository_isolation`, label `storage`) registered in
  `tests/host/CMakeLists.txt` right after `device_settings_core_tests`. It links
  `device_settings_core.c` + `device_settings_v2.c` alongside
  `storage_fs_ops.c`/`storage_blob_core.c`/`storage_blob_recovery.c`/`storage_blob.c`,
  mirroring the source lists already used by `storage_blob_tests` and
  `device_settings_core_tests`.
- V2-043 does not include the `/api/v1/settings` HTTP routes — those are `V2-055`
  (Phase 5, a separate stream) and were intentionally left untouched, along with
  `web_server`, `wifi_ap`, and `device_controls` per the task's explicit exclusions.
- The pre-existing `firmware/components/provisioning/` component (`provisioning_core.c`,
  `provisioning_settings_t` with `always_select_package`/`require_physical_confirmation`)
  is a distinct, older settings surface unrelated to the V2-043 checklist (it predates
  the canonical `app_v2_device_settings_t` record) and was not touched or audited as
  part of this change; it may be worth reconciling in a future pass but is out of this
  checklist's scope.
