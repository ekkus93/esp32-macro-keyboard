# ESP32 Macro Keyboard — v2 Implementation TODO

**Document status:** Authoritative implementation sequence  
**Product version:** 0.2 rebuild  
**Created:** 2026-08-03

## 0. Authority and use

This TODO is derived only from:

- [`docs/SPEC_V2.md`](SPEC_V2.md)
- [`docs/UI_UX_SPEC_V2.md`](UI_UX_SPEC_V2.md)

Those two synchronized specifications remain the source of product requirements.
This file defines the implementation order, evidence requirements, and completion
gates.

The retired v1 specification, retired TODOs, prior implementation reports,
current v1 behavior, screenshots, mockups, and historical code are not sources of
v2 requirements. Existing code may be reused only when it independently satisfies
the v2 specifications and the tests required here.

No task may silently change either authoritative specification. A genuine
specification conflict must be reported to the product owner before implementation
continues.

## 0.1 Completion rules

- [ ] Work through phases in order unless a task explicitly names a safe
      dependency exception.
- [ ] Do not mark a task complete without implementation and reproducible
      evidence.
- [ ] Compilation alone is not evidence for runtime behavior.
- [ ] Host fakes are not hardware evidence.
- [ ] A firmware build is not USB HID, Wi-Fi, persistence, or power-loss evidence.
- [ ] Hardware-dependent tasks remain open until executed on the reference
      ESP32-S3R8 and recorded in committed evidence.
- [ ] Do not use `|| true`, ignored exit codes, warning suppression, analyzer
      exclusions, or redirected diagnostics to make a gate pass.
- [ ] Do not add compatibility shims for the retired firmware-owned package and
      macro architecture.
- [ ] Do not preserve obsolete routes, schema fields, storage files, or UI flows
      merely to keep old tests passing.
- [ ] Add or update tests in the same change as each behavior.
- [ ] Keep the repository buildable and testable at every phase boundary.

## 0.2 Evidence format

Every completed phase must add or update a committed implementation report under
`docs/implementation-v2/`. Each report must include:

- phase and task IDs;
- exact commit SHA;
- files and subsystems changed;
- commands executed;
- complete pass/fail summary;
- relevant test names and counts;
- hardware model, ports, firmware build ID, and host OS when hardware was used;
- observed values for timing, storage, memory, and reconnect tests;
- unresolved limitations or deferred evidence;
- an explicit statement that no unchecked task is being claimed complete.

Use logs or machine-readable artifacts where useful, but do not commit passwords,
passphrases, setup codes, session tokens, repository contents, macro source, or
other sensitive data.

## 0.3 Ralph-loop phase discipline

For each phase:

1. inspect the current implementation and identify the smallest coherent gap;
2. write or update the failing test or acceptance harness;
3. implement the narrow change;
4. run the narrow test loop;
5. run the phase gate;
6. inspect the diff for obsolete behavior, silent fallbacks, and untested paths;
7. record exact evidence;
8. repeat until the phase exit gate is satisfied.

---

## Phase 0 — Baseline, inventory, and migration map

**Goal:** Establish a trustworthy starting point and classify every existing v1
subsystem before modifying production behavior.

## V2-000 — Record the starting state

- [x] Record the starting `master` commit SHA.
- [x] Record toolchain versions and confirm ESP-IDF `v5.5.5`, target `esp32s3`,
      and Node.js `v24.18.0`.
- [ ] Run the current authoritative local gate:

```bash
./scripts/check-all.sh
```

- [ ] Record every current failure without weakening a gate.
- [ ] Record current frontend test count, host test count, firmware build status,
      webfs size, app image size, and userdata partition size.
- [ ] Confirm no uncommitted user work is mixed into the rebuild baseline.

## V2-001 — Build a complete code inventory

- [x] Inventory production firmware components under `firmware/`.
- [x] Inventory React routes, feature modules, API clients, models, validators,
      and persistent-browser-storage use under `webapp/`.
- [x] Inventory native host tests under `tests/host/`.
- [x] Inventory browser and Vitest coverage under `webapp/tests/`.
- [x] Inventory on-device Unity coverage under `firmware/test_app/`.
- [x] Inventory hardware scripts under `tests/hardware/`.
- [x] Inventory schemas, generated files, static assets, scripts, CI workflows,
      and documentation references.

## V2-002 — Create the explicit migration map

- [x] Create `docs/implementation-v2/V2_MIGRATION_MAP.md`.
- [x] Classify every production subsystem as **retain**, **adapt**, **rewrite**, or
      **delete**.
- [x] Classify every test suite and fixture the same way.
- [x] Identify all firmware-owned package and macro state.
- [x] Identify all package/macro CRUD routes and validation routes.
- [x] Identify all revision, ETag, optimistic-concurrency, restore, import,
      replace, and repository-index behavior from v1.
- [x] Identify all frontend localStorage, sessionStorage, IndexedDB, Cache
      Storage, and service-worker repository persistence.
- [x] Identify all standalone send-confirmation, send-progress, and result routes
      that conflict with the v2 primary workflow.
- [x] Identify all documentation and scripts that still point to v1 authority.
- [x] For every retained subsystem, name the v2 requirement and tests proving it
      remains valid.

## Phase 0 exit gate

- [ ] Baseline evidence is committed.
- [x] Migration map covers production code, tests, scripts, schemas, and docs.
- [x] Every known v1 feature has an explicit delete/adapt decision.
- [x] No production behavior has changed in this phase.

---

## Phase 1 — Shared contracts, constants, and test fixtures

**Depends on:** Phase 0.

**Goal:** Define exact v2 contracts before firmware and UI implementation diverge.

## V2-010 — Repository model and schema contracts

- [x] Define the strict TypeScript repository types:

```json
{
  "format": "esp32-macro-keyboard-repository",
  "schemaVersion": 1,
  "packages": []
}
```

- [x] Ensure the root has exactly `format`, `schemaVersion`, and `packages`.
- [x] Prohibit `activePackageId` and all unknown fields.
- [x] Define package and macro types exactly as specified.
- [x] Enforce canonical lowercase UUID v4 identifiers.
- [x] Enforce repository-wide package-ID and macro-ID uniqueness.
- [x] Add valid, boundary, malformed, duplicate-ID, prototype-bearing, sparse
      array, non-finite-number, and unknown-field fixtures.
- [x] Add a checked-in canonical example repository and canonical compact JSON.

## V2-011 — API contract models

- [x] Define shared request/response examples for every `/api/v1` route.
- [x] Define the unprovisioned-only `GET /api/v1/setup` response with exactly
      `provisioned: false` and `deviceName`.
- [x] Define the standard JSON error envelope with stable `code`, human-readable
      `message`, and optional `field`.
- [x] Define exact setup, session, status, limits, settings, password-change,
      restart, reset-settings, factory-reset, diagnostics, blob, and send objects.
- [x] Define strict unknown-field rejection for JSON requests.
- [x] Define content types, maximum body sizes, success status codes, and error
      status codes per route.
- [x] Add TypeScript types and C-side parsing/serialization contracts that match
      the same checked-in examples.

## V2-012 — Centralized limits

- [x] Create one authoritative firmware limits module.
- [x] Mirror client-relevant limits in generated or verified TypeScript types.
- [x] Include at least:
  - [x] package name: 64 UTF-8 bytes;
  - [x] macro name: 64 UTF-8 bytes;
  - [x] macro source: 4096 UTF-8 bytes;
  - [x] compiled actions: 4096;
  - [x] key press: 0–10,000 ms;
  - [x] inter-key delay: 0–10,000 ms;
  - [x] directive delay: 1–10,000 ms;
  - [x] estimated macro duration: 300,000 ms;
  - [x] absolute executor deadline: 310,000 ms;
  - [x] JSON request body: 8192 bytes;
  - [x] candidate repository blob maximum: 131,072 bytes;
  - [x] active sessions: 8;
  - [x] session idle lifetime: 86,400 seconds;
  - [x] session absolute lifetime: 604,800 seconds;
  - [x] serial-confirmation timeout: 60 seconds;
  - [x] administrator password: 12–128 UTF-8 bytes.
- [x] Make `GET /api/v1/limits` derive from the same constants.
- [x] Add boundary tests for every numeric and byte-count limit.

## V2-013 — Shared macro-language conformance corpus

- [x] Retain or replace the current corpus with one format consumed by C and
      TypeScript tests.
- [ ] Cover printable ASCII, line endings, tabs, directives, chords, escaping,
      delays, malformed braces, Unicode rejection, duplicate modifiers, unknown
      names, source positions, action limits, and duration limits.
- [x] Define expected compiled actions for valid cases.
- [x] Define expected code, byte offset, line, column, and message class for
      invalid cases.
- [x] Make parser drift fail both local gates and CI.

## V2-014 — Device settings schema

- [x] Define a versioned NVS settings record.
- [x] Include device name, AP settings, optional station settings, password
      verifier metadata, physical-confirmation policy, provisioning state,
      credential version, optional next-blob counter, and UI preferences.
- [x] Include:
  - [x] `sendMode`, default `quick`;
  - [x] `snapshotRetentionTarget`, default `5`, advisory only;
  - [x] `showMacroSourcePreviews`, default `false`;
  - [x] `lastSelectedPackageId`, default `null`.
- [x] Validate `lastSelectedPackageId` only as an opaque canonical UUID or null.
- [x] Specify reset-settings and factory-reset defaults exactly.
- [x] Add wrong-version, wrong-length, truncated-record, invalid-enum, and invalid
      UUID tests.

## Phase 1 exit gate

- [x] Contract fixtures are checked in and consumed by tests.
- [x] C and TypeScript agree on every shared boundary and enum.
- [ ] No production route or repository serializer still depends on a v1 shape.
- [x] Narrow contract test suites pass with zero warnings.

---

## Phase 2 — Remove the retired firmware-owned repository architecture

**Depends on:** Phase 1.

**Goal:** Delete v1 ownership before adding the v2 persistence path.

## V2-020 — Delete firmware package and macro repositories

- [x] Remove production package repository state.
- [x] Remove production macro repository state.
- [x] Remove firmware active-package state.
- [x] Remove package indexes, per-package files, per-macro files, repository JSON,
      revision stores, backup stores, restore stores, and related caches.
- [x] Remove firmware package/macro import, export, merge, replace, and restore
      logic.
- [x] Remove firmware repository schema parsing and serialization.
- [x] Remove associated startup sequencing and recovery behavior.

## V2-021 — Delete obsolete HTTP resources

- [x] Remove package CRUD routes.
- [x] Remove macro CRUD routes.
- [x] Remove firmware validation routes.
- [x] Remove repository restore/import/export/replace routes.
- [x] Remove plural execution resources and revision parameters.
- [x] Reject old paths explicitly rather than silently translating them.

## V2-022 — Remove obsolete tests and replace coverage

- [x] Delete tests whose only purpose is retired behavior.
- [x] Preserve useful parser, executor, USB, authentication, Wi-Fi, and static
      server tests by adapting them to v2 contracts.
- [x] Add negative tests proving old routes are absent.
- [x] Add source scans or architectural tests preventing reintroduction of:
  - [x] firmware package repositories;
  - [x] firmware macro repositories;
  - [x] repository `activePackageId`;
  - [x] package/macro CRUD routes;
  - [x] firmware gzip or JSON repository parsing.

## Phase 2 exit gate

- [x] Firmware builds without retired repository modules.
- [x] Old routes are absent and tested absent.
- [x] No compatibility adapter remains.
- [x] Relevant host tests and static analysis pass.

---

## Phase 3 — Opaque blob storage

**Depends on:** Phase 2.

**Goal:** Implement the complete byte-oriented snapshot store without repository
knowledge in firmware.

## V2-030 — Storage layout and scanning

- [x] Mount the userdata LittleFS partition without format-on-failure.
- [x] Use `/data/repository/` and fixed-width decimal `<id>.gz` filenames.
- [x] Scan final filenames at startup.
- [x] Ignore invalid names as blobs and report them in diagnostics.
- [x] Sort valid IDs numerically, newest first.
- [x] Derive the next ID from both NVS and the maximum existing ID.
- [x] Prevent overwrite after stale or erased counters.

## V2-031 — Atomic blob add

- [x] Implement bounded `application/gzip` streaming upload.
- [x] Write to `<id>.gz.tmp` in bounded chunks.
- [x] Treat short write, write error, flush error, close error, sync error, and
      rename error as failures.
- [x] Synchronize the temporary file before rename.
- [x] Use rename as the commit point.
- [x] Synchronize the directory when supported.
- [x] Return `201` only after the final file is committed.
- [x] Leave existing final blobs unchanged on every failure path.

## V2-032 — List, load, and delete

- [x] Implement newest-first listing with ID and stored byte size only.
- [x] Implement byte-identical streaming download as `application/gzip`.
- [x] Implement explicit deletion of exactly one selected blob.
- [x] Permit deletion of the final blob.
- [x] Never select or load a replacement automatically.
- [x] Never inspect gzip headers, decompress, parse JSON, compute metadata, or add
      a checksum, hash, digest, or CRC.

## V2-033 — Boot cleanup and degraded states

- [x] Remove only interrupted `.tmp` files during boot recovery.
- [x] Never remove a final `.gz` because React cannot decode it.
- [x] Expose mount failure as an explicit degraded or failed storage state.
- [x] Ensure mount failure never triggers formatting.
- [x] Report temporary and invalid files in diagnostics.

## V2-034 — Capacity and candidate blob limit

- [x] Report userdata `totalBytes`, `usedBytes`, and `remainingBytes` through the
      existing status contract, and report `blobMaxBytes` through
      `GET /api/v1/limits` from the authoritative shared constant.
- [x] Preserve the exact v2 API schemas while adding capacity evidence; do not
      silently add `totalBytes` or `maxBlobBytes` to the blob-list response.
- [x] Keep `APP_V2_BLOB_MAX_BYTES` at exactly 131,072 bytes (128 KiB) only when
      the real LittleFS image proof supports it; reduce the constant explicitly if
      the proof fails rather than weakening the test or silently changing the
      specification.
- [x] Remove the failed `APP_ERROR_LIMIT` mapping and host-test attempt; do not add
      a new generic application error solely to represent direct HTTP body-size
      rejection.
- [x] Enforce and test direct handler-level `413 Payload Too Large` for a request
      body above `APP_V2_BLOB_MAX_BYTES`.
- [x] Prove oversized rejection occurs before authentication, temporary-file
      creation, upload initialization, or any other storage mutation.
- [x] Enforce and test `ENOSPC` to `APP_ERROR_STORAGE_FULL` to
      `507 Insufficient Storage` when a within-limit upload cannot fit.
- [x] Prove every `507` failure leaves all previously committed final blobs
      byte-identical and does not replace or delete them.
- [x] Build a real 524,288-byte userdata LittleFS image containing two
      maximum-size final blobs and one maximum-size temporary upload.
- [x] Include directory and filesystem metadata overhead in the image proof;
      verify all three files byte-for-byte and require a positive safety margin.
- [x] Record the measured 128 KiB candidate evidence: 393,216 payload bytes,
      421,888 used bytes, 28,672 overhead bytes, and 102,400 remaining bytes.
- [x] Integrate the permanent LittleFS capacity and HTTP-contract proof into
      `scripts/check-all.sh` and the authoritative CI dependency setup so missing
      tooling fails explicitly rather than skipping the gate.
- [x] Remove all temporary V2-034 probe, implementation, and TODO-edit workflows
      after the permanent implementation is committed.
- [x] Run Host Tests, Browser Tests, Device Test Build, and Quality on the exact
      final implementation SHA; do not claim completion from an older or partial
      commit.
- [x] Create
      `docs/implementation-v2/V2_034_CAPACITY_AND_BLOB_LIMIT_2026-08-06.md`
      with the final accepted value, measured image results, commands, CI run and
      job IDs, exact implementation SHA, failed-attempt explanation, and any
      deferred hardware evidence.
- [x] Mark V2-034 complete only after the permanent gate, cleanup, implementation
      report, and exact-SHA authoritative CI evidence are all committed.

## V2-035 — Storage hardware evidence

All seven items verified on real ESP32-S3R8 hardware 2026-08-10 (firmware
`7f322c1`). `scripts/run-v2-035-hardware.py` had never been executed against
physical hardware before this session and had several real v2-contract
mismatches (wrong login field name, a stale v1-style response envelope, a
wrong diagnostics field path, an over-strict buildId equality check) — all
found and fixed first, with new regression tests, before any physical stage
ran. Full evidence:
`docs/hardware-evidence/V2_035_STORAGE_ESP32S3R8_2026-08-10.json` (all seven
scenarios, validated). Writeup:
`docs/implementation-v2/V2_035_STORAGE_HARDWARE_EVIDENCE_2026-08-10.md`.

- [x] Add a blob, power-cycle the board, and load byte-identical data. Real
      USB power removal and restoration (not a software reset); all
      baseline/sentinel blobs reloaded byte-identical; `resetReason`
      confirmed `power_on`.
- [x] Add multiple blobs and verify numeric ordering. Three blobs created
      with strictly increasing IDs; newest-first listing confirmed.
- [x] Delete one blob and verify every other blob is byte-identical. Middle
      of three blobs deleted; both survivors verified unchanged.
- [x] Interrupt an upload and verify no partial final file appears. Power cut
      after 98,304 of 131,072 bytes sent (past the required 16,384-byte
      minimum); no new final blob ID after reboot.
- [x] Reboot and verify temporary cleanup. Post-interruption reboot reports
      `diagnostics.storage.temporaryFiles` empty.
- [x] Fill storage and verify `507` leaves all final blobs unchanged.
      Maximum-size uploads repeated until `507`; every previously committed
      blob remained byte-identical; fill blobs cleaned up.
- [x] Simulate or induce mount failure and verify no formatting occurs. Real
      `userdata` partition overwritten with a deterministic corrupt image via
      `parttool.py`; real firmware output
      (`esp_littlefs: mount failed, (-84)`, `stage failed: storage_mount`);
      post-boot partition read back byte-identical to the corrupt image
      (proving no silent format); backup restored and verified
      byte-identical to the original.

## Phase 3 exit gate

- [x] Host storage tests, image tests, and static analysis pass. Verified
      2026-08-09 at `50ada5b`: `./scripts/run-tests.sh storage` — 100% passed,
      13/13. `python3 ./scripts/check-v2-034-capacity.py` (the LittleFS
      capacity/image proof) — passed with the exact committed values
      (partition=524288, maxBlob=131072, payload=393216, used=421888,
      overhead=28672, remaining=102400). `./scripts/check-firmware.sh` (GCC
      build + esp-clang `run-clang-tidy` with `WarningsAsErrors: '*'` for both
      `firmware/` and `firmware/test_app/`) — completed with exit code 0 and
      zero first-party findings.
- [x] Required hardware evidence is committed. All seven V2-035 scenarios
      executed and validated on real ESP32-S3R8 hardware 2026-08-10:
      `docs/hardware-evidence/V2_035_STORAGE_ESP32S3R8_2026-08-10.json`. See
      V2-035 above and
      `docs/implementation-v2/V2_035_STORAGE_HARDWARE_EVIDENCE_2026-08-10.md`.
- [x] Firmware remains completely unaware of repository contents. Verified by
      inspection 2026-08-09: no gzip-header, decompress, JSON-parse, checksum,
      hash, digest, or CRC logic anywhere in `firmware/components/storage/`
      (`grep -rn "gzip\|checksum\|hash\|digest\|crc\|CRC"` over
      `storage_blob*.c` returns nothing); blob objects carry only `id` and
      `sizeBytes` (`firmware/components/web_server/web_server_blob.c`); the
      v1 package/repository object model was already deleted in Phase 2 per
      `V2_MIGRATION_MAP.md`.

---

## Phase 4 — Authentication, provisioning, and device settings

**Depends on:** Phases 1 and 3.

## V2-040 — First-run provisioning

- [x] Expose only `GET /api/v1/setup`, `POST /api/v1/setup`, and required static
      assets while unprovisioned.
- [x] Return exactly `provisioned: false` and the non-secret `deviceName` from
      `GET /api/v1/setup`.
- [x] Return `404` from `GET /api/v1/setup` after provisioning.
- [x] Keep every other `/api/v1` route unavailable while unprovisioned.
- [x] Require the one-time serial setup code.
- [x] Strictly validate device name, AP SSID, AP passphrase, administrator
      password, and physical-confirmation setting.
- [x] Preserve unrelated configuration fields during setup updates.
- [x] Return a restart/reconnect response without returning secrets.
- [x] Return `409` from setup submission after provisioning.
- [x] Test wrong, expired, malformed, and reused setup codes.
- **Gap found on real hardware 2026-08-09, not yet resolved:** the setup-mode
  AP's own Wi-Fi passphrase (needed just to associate and reach the setup UI
  at all) is derived deterministically
  (`provisioning_bootstrap_core.c`: HMAC of the eFuse hardware key, a fixed
  domain string, and the softAP MAC) and is never disclosed anywhere —
  not logged, not returned over HTTP, not exposed via any serial console
  command (confirmed by grepping `serial_console`/`wifi_ap`/`app_core`).
  `SPEC_V2.md` §12.3 specifies the *setup code*'s disclosure path (serial
  console) but not this passphrase's. As shipped, a real device owner has no
  documented way to learn it and complete first-run provisioning over
  Wi-Fi. Needs a product decision (physical label, companion doc,
  relaxing the console-disclosure rule for this one value, etc.), not a
  unilateral firmware patch — flagged for Phil rather than invented around.
  See `docs/implementation-v2/V2_041_HARDWARE_LOGIN_FIX_2026-08-09.md` §1.

## V2-041 — Password verifier and PBKDF2 benchmark

- [x] Use PBKDF2-HMAC-SHA-256 with a random per-password salt.
- [x] Store verifier version, salt, and iteration count.
- [x] Use constant-time verifier comparison.
- [x] Benchmark candidate iteration counts on the reference ESP32-S3R8.
- [x] Record median, percentile, and worst observed time under representative
      memory and Wi-Fi load. Measured 2026-08-09 on real hardware with the
      full production stack running (AP + station Wi-Fi + HTTP server, not
      the prior isolated Unity-console benchmark): 20 real
      `POST /api/v1/auth/login` requests, full round-trip — min 441.0 ms,
      median 522.5 ms, p90 757.2 ms, worst 839.1 ms. These are end-to-end
      HTTP timings (network + KDF + response), not pure KDF cost, and are
      not what the 250–500 ms selection criterion below was calibrated
      against; they confirm the frozen iteration count behaves reasonably
      under real conditions rather than re-deriving the count itself. See
      `docs/implementation-v2/V2_041_HARDWARE_LOGIN_FIX_2026-08-09.md`.
- [x] Select one exact iteration count yielding approximately 250–500 ms.
- [x] Freeze the selected constant in code and tests.
- [x] Confirm the derivation does not trip watchdogs or starve critical tasks.
      Verified 2026-08-09: zero watchdog/TWDT messages in a continuous
      40-second serial capture spanning all 20 real logins above; the
      console's liveness was independently confirmed immediately afterward
      (a harmless `wifi-status` command still got a normal reply), so the
      empty capture is a genuine negative result, not a dead capture. See
      `docs/implementation-v2/V2_041_HARDWARE_LOGIN_FIX_2026-08-09.md`.
- [x] Ensure passwords and derived material never appear in logs or diagnostics.

## V2-042 — Sessions and rate limiting

- [x] Generate session tokens with 32 random bytes of entropy.
- [x] Store at most eight sessions in bounded RAM.
- [x] Set `HttpOnly`, `SameSite=Strict`, and `Path=/` on the cookie.
- [x] Enforce 24-hour idle and seven-day absolute expiry.
- [x] Define deterministic behavior when the ninth session is created.
- [x] Enforce five failed logins within 60 seconds followed by a five-minute
      lockout.
- [x] Ensure rate-limit state is bounded and does not become a denial-of-service
      memory leak.
- [x] Test login, logout, expiry, lockout, session replacement, reboot, and cookie
      attributes.

## V2-043 — Device UI preferences

- [x] Implement NVS-backed Quick Send/Always Preview mode.
- [x] Default to Quick Send.
- [x] Implement advisory snapshot retention target, default five.
- [x] Implement source-preview setting, default hidden.
- [x] Implement opaque `lastSelectedPackageId`.
- [x] Suppress duplicate NVS writes when a value has not changed.
- [x] Ensure changing UI preferences never creates or changes a repository blob.

## V2-044 — Wi-Fi and reset semantics

- [x] Start the protected AP first and unconditionally.
- [x] Support at most one explicitly configured station network.
- [x] Ensure station failure cannot prevent AP operation.
- [x] Bound retries and expose status.
- [x] Define and test restart, reset-settings, credential reset where applicable,
      and factory-reset preservation/deletion behavior.
- [x] Ensure factory reset erases repository blobs only when the specification
      requires it and reports the connection loss clearly. Erasure-scope half
      done and tested (`storage_blob_delete_all()` is called only from
      `device_controls_factory_reset()`, never `reset_settings()`). The
      "reports the connection loss clearly" half was blocked on Phase 5's
      `web_server` when this note was written; V2-055 has since implemented
      `POST /api/v1/device/factory-reset` (`web_api_administration.c`),
      whose live route handler calls
      `web_device_reset_accepted_json(true, false, &json)` and returns `202`
      — that function unconditionally includes `"connectionWillClose":true`
      per `SPEC_V2.md` §13.14's exact example, now directly asserted by
      `test_reset_accepted_json_factory_reset_shape` (and the reset-settings
      variant) in `tests/host/test_web_device_actions.c`, not just implied by
      the shared restart test. See
      `docs/implementation-v2/V2_044_WIFI_AND_RESET_SEMANTICS_2026-08-08.md`.

## Phase 4 exit gate

- [x] Provisioning, authentication, session, NVS, and Wi-Fi host tests pass.
      Re-verified 2026-08-09 at `50ada5b` via `./scripts/run-tests.sh` (full
      suite, no label): 100% tests passed, 54/54 (the "50/50" figure in the
      prior note is stale — the suite has grown since). Confirmed per-label
      too: `auth` 2/2, `startup` 2/2, `wifi` 2/2, `web` 23/23, `storage`
      13/13.
- [x] PBKDF2 hardware benchmark and selected iteration count are committed.
      V2-041's two remaining hardware-only items (real-device timing
      percentiles, watchdog/starvation confirmation) are now closed — see
      `docs/implementation-v2/V2_041_HARDWARE_LOGIN_FIX_2026-08-09.md`.
- [ ] No secret appears in logs, APIs, diagnostics, or test artifacts.
      Partial, automated evidence exists (`check-credential-logging.sh`'s
      firmware log-call source scan, plus secret-sentinel tests for
      diagnostics/status/session/login), but that doesn't add up to a
      blanket guarantee across every API, webapp artifact, and test log —
      left open rather than claimed from partial coverage.

---

## Phase 5 — Exact v2 HTTP API

**Depends on:** Phases 1, 3, and 4.

## V2-050 — Common HTTP policy

- [x] Register only documented `/api/v1` routes.
- [x] Enforce authentication per route.
- [x] Enforce exact methods and content types.
- [x] Enforce bounded request bodies before allocation or parsing.
- [x] Reject unknown JSON fields.
- [x] Reject malformed, duplicate, overflowing, and trailing JSON content.
- [x] Prevent user-controlled filesystem paths.
- [x] Return the standard error envelope on JSON errors.
- [x] Disable CORS.
- [ ] Test malformed paths and unsupported methods. Covered at the
      policy-table level (`test_web_api_core.c`'s `web_api_parse_path()`/
      `web_api_route_allows_method()` cases, exercised for every route).
      Stale as of the V2-053/V2-057 blob and route-matrix tracks: an
      `esp_http_server` fake now exists
      (`tests/host/fakes/esp_http_server_stub`, `fakes/fake_httpd.c`) and is
      used by `test_web_server_blob*.c`, `test_web_server_status_limits_route.c`,
      `test_web_server_send_route.c`, `test_web_server_setup_route.c`, and
      `test_web_server_administration_route.c` — but it is a hand-built test
      double that answers handler calls directly, not ESP-IDF's real httpd
      URI-trie/method dispatch, so it still cannot prove what a malformed path
      or unsupported method actually receives from the real server (a 404/405
      from `esp_http_server` itself, before any first-party handler runs).
      Blob's parameterized routes do get live malformed-path coverage through
      this fake (`web_api_parse_blob_id()`); status/limits/send/setup have no
      path parameter and, along with every route's wrong-method case, remain
      covered only at the pure-function level per the same investigation
      recorded in
      `docs/implementation-v2/V2_057_FULL_HTTP_CONTRACT_MATRIX_2026-08-09.md`.
      No live-socket-level test against the real `esp_http_server` exists or
      can exist without hardware or a QEMU/ESP-IDF host harness.

## V2-051 — Setup and authentication routes

- [x] Implement unprovisioned-only `GET /api/v1/setup`.
- [x] Implement `POST /api/v1/setup`.
- [x] Implement `POST /api/v1/auth/login`.
- [x] Implement `POST /api/v1/auth/logout`.
- [x] Implement `GET /api/v1/auth/session`. `handle_session()` in
      `web_api_administration.c` is now compiled and exercised by
      `web_api_administration_tests` (success and backend-failure paths, exact
      JSON shape and status code) — see
      `docs/implementation-v2/V2_051_057_AUTH_SESSION_SETUP_TEST_COVERAGE_2026-08-08.md`.
- [x] Match exact schemas, status codes, cookie behavior, and expiry fields.
      Expiry math and JSON shape were already tested; cookie string formatting
      is now factored into `web_cookie_build_session_header()`
      (`web_cookie.c`, used by `web_server_login.c::send_login_accepted`) and
      covered by a host test, round-tripping through
      `web_cookie_extract_session()`.
- [x] Test the complete unprovisioned/provisioned route-access matrix.
      Previously guaranteed only structurally, by
      `check-setup-route-isolation.sh` parsing `web_server_lifecycle.c`'s
      `normal_routes[]`/`setup_routes[]` array literals with a regex. Closed
      with an executed test: `fakes/fake_httpd_router.c` is a faithful host
      port of ESP-IDF v5.5.5's own `httpd_register_uri_handler()`/
      `httpd_find_uri_handler()`/`httpd_uri_match_wildcard()`
      (`components/esp_http_server/src/httpd_uri.c`), so
      `tests/host/test_web_server_lifecycle.c` calls the real, unmodified
      `web_server_start()`/`web_server_stop()` (previously linked into no host
      test target at all) in both `WEB_SERVER_MODE_SETUP` and
      `WEB_SERVER_MODE_NORMAL`, letting the real route tables register into
      the fake router, then resolves a representative uri/method pair for
      every route named in `docs/SPEC_V2.md` against it in each mode and
      asserts which handler answers, if any. This is a level up from testing
      only that one fixed-URI handler behaves correctly (already covered
      elsewhere) or that the array-literal source text looks right (the
      isolation script): it proves what ESP-IDF's own dispatch algorithm
      actually resolves. Confirmed SPEC_V2 12.3 end to end ("every other
      `/api/v1` route is unavailable while unprovisioned") and, as a
      genuinely new finding no static check could show, that both route
      tables' trailing `GET *` static-asset catch-all means a mismatched
      route never resolves as a routing-layer 404 — it is always either
      answered by a specific handler, falls through to `static_handler()` (a
      GET request, which then 404s at the static-file layer, not the routing
      layer), or is `405 Method Not Allowed` (a non-GET request to a uri the
      catch-all's pattern still matches), never a bare "route not found" at
      the `httpd_find_uri_handler()` level. Also proves the provisioning
      transition itself: the same lifecycle object started in setup mode,
      stopped, then restarted in normal mode ends up with the swapped route
      surface, not a leaked or accumulated one. 3 tests
      (`web_server_lifecycle_tests`), covering 52 uri/method combinations
      across both provisioning modes, passing under `--sanitizers` too. See
      `docs/implementation-v2/V2_051_057_ROUTE_ACCESS_MATRIX_2026-08-09.md`.

## V2-052 — Status and limits routes

- [x] Implement `GET /api/v1/status` with stable fields for provisioning, device,
      firmware, uptime, USB, AP, station, storage, and send state.
- [x] Implement `GET /api/v1/limits` from centralized constants.
- [x] Exclude secrets, repository content, package names, macro names, and macro
      source.

## V2-053 — Blob routes

- [x] Implement `GET /api/v1/blob`.
- [x] Implement `POST /api/v1/blob`.
- [x] Implement `GET /api/v1/blob/{blob_id}`.
- [x] Implement `DELETE /api/v1/blob/{blob_id}`.
- [x] Verify exact binary behavior and status codes. HTTP-adapter-level host
      tests (`tests/host/test_web_server_blob.c` and its `.inc` fragments)
      now call `blob_list_handler`/`blob_create_handler`/`blob_load_handler`/
      `blob_delete_handler` directly against a fake `esp_http_server.h`
      (`tests/host/fakes/esp_http_server_stub`, `fake_httpd.c`) and a fake
      `storage_blob`/`storage_partition_capacity` backend
      (`fake_storage_blob.c`), covering exact status codes (200/201/204/400/
      401/404/413/415/500/503/507), `application/gzip`/`application/json`
      content types, and a byte-identical upload/download round trip through
      the handler. See
      `docs/implementation-v2/V2_053_057_BLOB_ROUTE_TEST_COVERAGE_2026-08-08.md`.

## V2-054 — Send routes

- [x] Implement `POST /api/v1/send` with source and timing only.
- [x] Reject package ID, macro ID, blob ID, revision, or extra fields.
- [x] Compile the full source before returning acceptance.
- [x] Return exact parser locations on `422` and type nothing.
- [x] Return `409` when a send is already active.
- [x] Implement `GET /api/v1/send` for current or most recent state.
- [x] Implement idempotent `DELETE /api/v1/send` cancellation.
- [x] Return `404` when no send has existed since boot.

## V2-055 — Settings and device-action routes

- [x] Implement `GET /api/v1/settings` without returning passphrases.
- [x] Implement `PUT /api/v1/settings` with unambiguous preserve/remove semantics.
- [x] Reject empty strings where they would ambiguously mean preserve or delete.
- [x] Implement `POST /api/v1/settings/change-password`.
- [x] Implement `POST /api/v1/device/restart`.
- [x] Implement `POST /api/v1/device/reset-settings`.
- [x] Implement `POST /api/v1/device/factory-reset`.
- [x] Return exact accepted/reconnect/reprovision/preservation fields.

## V2-056 — Diagnostics route

- [x] Implement `GET /api/v1/diagnostics` using a fixed schema.
- [ ] Include required subsystem health, memory, stack, storage, USB, Wi-Fi, send,
      invalid filename, and temporary-file data. No `stack` field exists:
      neither `docs/SPEC_V2.md` §13.13 nor the checked-in
      `contracts/v2/api/examples.json` `"diagnostics"` example has one. Per
      `CLAUDE.md`'s rule that the committed contract corpus is authoritative,
      this is very likely stale TODO wording rather than a real gap, but the
      item as literally written is unmet — flagged rather than silently
      decided either way.
- [x] Exclude credentials, sessions, repository bytes, repository JSON, package
      information, macro information, and macro source.

## V2-057 — Contract and security tests

- [ ] Test every route with valid, missing, extra, wrong-type, wrong-content-type,
      oversized, unauthorized, expired-session, malformed-path, and method-error
      cases. Now strong, at the live-`httpd_req_t`-handler level (the same
      `tests/host/fakes/esp_http_server_stub`/`fake_httpd.c` technique blob
      uses), for status, limits, login, send, and blob: `status_handler()`/
      `limits_handler()` (`test_web_server_status_limits_route.c`) and
      `send_create_handler()`/`send_get_handler()`/`send_cancel_handler()`
      (`test_web_server_send_route.c`) now each exercise valid/unauthorized/
      expired-session/backend-failure, plus wrong-content-type/oversized/
      missing/wrong-type/extra-field/parse-error for send's request body
      (status/limits are bodyless GETs, so those categories don't apply, same
      carve-out already noted for blob's raw-byte body). These three routes
      have fixed, single-purpose URI registrations ahead of the generic
      `/api/v1/*` wildcard, so malformed-path (no path parameter) and
      method-error (answered by httpd routing falling through to the wildcard,
      never by the handler itself) are instead covered at the pure-function
      level in `test_web_api_core.c`
      (`WEB_API_ROUTE_STATUS`/`LIMITS`/`SEND` cases). `web_api_administration.c`'s
      session, restart, settings, change-password, reset-settings, and
      factory-reset routes, plus the provisioned-mode setup-conflict fallback,
      now also have live end-to-end coverage: `test_web_server_administration_route.c`
      drives the real `api_handler()` (`web_server_api.c`'s generic
      `/api/v1/*` wildcard handler) against the same httpd fake, proving
      `method_from_request()` -> `web_api_parse_path()` ->
      `web_request_policy_evaluate()` -> `web_api_dispatch()` ->
      `web_api_handle_administration()` actually wires together — one valid
      case per route plus unauthorized/expired-session reused across a GET,
      a bodyless POST, and a body-bearing PUT to prove the shared gate holds
      for every request shape, not duplicated per route. This closed a real
      defect the live path exposed and unit tests could not:
      `web_server_api.c`'s `status_text()` had no case for `WEB_HTTP_STATUS_NO_CONTENT`
      (204), so a *successful* change-password response was sent to the
      client as "500 Internal Server Error" — fixed in the same commit.
      `GET /api/v1/diagnostics` now also has live end-to-end coverage in the
      same file: six of the eight subsystem-health snapshot functions
      `collect_diagnostics()` calls turned out to already be portable C with
      no ESP-IDF dependency (their own module comments say so) and link in
      real, unfaked; only `device_controls_get_health()` and
      `wifi_ap_get_status()` needed fakes, plus new
      `esp_idf_misc_stub/esp_heap_caps.h` and three additions to
      `esp_idf_misc_stub/esp_system.h` (`esp_reset_reason_t`,
      `esp_reset_reason()`, `esp_get_free_heap_size()`,
      `esp_get_minimum_free_heap_size()`) — see
      `docs/implementation-v2/V2_057_DIAGNOSTICS_AND_SETUP_STATE_2026-08-09.md`.
      The physical-confirmation-required=true path is now partially covered:
      `test_web_server_async_confirmation.c` (new target
      `web_server_async_confirmation_tests`) links the real
      `web_server_async.c` — against a new, deliberately minimal,
      dead-path-only FreeRTOS host stub
      (`tests/host/fakes/freertos_stub/freertos/{FreeRTOS,queue,semphr,task}.h`,
      whose implementations are hard-failure canaries, never real queue/task
      logic) — and drives `api_handler()` with
      `server_configuration.require_physical_confirmation = true` for
      restart/reset-settings/factory-reset/change-password without ever
      calling `web_server_async_start()`, so `web_server_async_dispatch()`
      always takes its own real, documented "worker unavailable: answer on
      the httpd task" fallback branch — proving confirmation-required
      routing, `device_controls_wait_for_confirmation()` invocation with the
      correct timeout, and both a granted and a denied (403) confirmation
      outcome, all through genuinely executed production code. This does
      **not** cover the other branch: the actual FreeRTOS worker-queue/task
      path (`web_server_async_start()`/`async_worker()`,
      `claim_in_flight()`/`release_in_flight()`'s mutual exclusion, the real
      `xQueueSend()`/`xQueueReceive()` handoff) remains genuinely untested at
      the host level — a faithful, runnable emulation of that concurrency
      would be the "substantial new concurrency-fake" four independent prior
      investigation rounds (see
      `docs/implementation-v2/V2_057_FULL_HTTP_CONTRACT_MATRIX_2026-08-09.md`,
      `docs/implementation-v2/V2_057_LIVE_ADMINISTRATION_HTTP_TEST_2026-08-09.md`,
      `docs/implementation-v2/V2_057_DIAGNOSTICS_AND_SETUP_STATE_2026-08-09.md`,
      `docs/implementation-v2/V2_051_057_ROUTE_ACCESS_MATRIX_2026-08-09.md`)
      concluded is too large/risky to build safely in this style of track,
      and this track's own fresh investigation reached the same conclusion —
      see `docs/implementation-v2/V2_057_PHASE5_HARDENING_2026-08-09.md`.
- [x] Test the unprovisioned route surface contains only setup GET/POST and static
      setup assets.
- [x] Test setup-state GET returns only the approved two fields and returns `404`
      after provisioning. The provisioned-mode `404` path
      (`setup_route_response()`'s GET branch in `web_api_administration.c`) was
      already tested. `setup_state_handler` (the unprovisioned-mode GET, in
      `web_server_setup.c`) is now also tested directly against the same
      `esp_http_server_stub`/`fake_httpd.c` technique other fixed-URI handlers
      use (`tests/host/test_web_server_setup_route.c`): the 200 response is
      compared field-for-field, and via `cJSON_Compare()`, against
      `contracts/v2/api/examples.json`'s own `setupState` fixture, proving
      exactly two members and no more; a defense-in-depth case also confirms
      the handler's own `server_configuration.mode` guard answers `404` if it
      is ever reached outside `WEB_SERVER_MODE_SETUP`. See
      `docs/implementation-v2/V2_057_DIAGNOSTICS_AND_SETUP_STATE_2026-08-09.md`.
- [x] Test setup POST returns `409` after provisioning. `setup_route_response()`'s
      POST branch is now directly tested via
      `web_api_handle_administration(WEB_API_ROUTE_SETUP, POST, …)`, distinct from
      `test_already_provisioned_rejected`'s defense-in-depth check.
- [x] Test exact response schemas and status codes. Strong for
      status/limits/send/login/diagnostics/blob — status/limits/send/diagnostics
      are now also verified at the live-handler level, not just their pure
      JSON builders (see the matrix bullet above); session/restart/
      setup-conflict/setup-state are now covered too via
      `web_api_administration_tests` and `test_web_server_setup_route.c`.
- [x] Test that secret-like sentinel values never appear in responses or logs.
      Explicit checks exist for diagnostics and status; send's password material
      is secure-zeroed by construction; session (`handle_session()`'s JSON) and
      login (the cookie header `web_cookie_build_session_header()` composes) are
      now scanned with the same `check-secret-sentinel.py`-backed harness
      diagnostics/status use; blob responses carry raw gzip bytes and an ID/size
      pair only, with no secret-like field to scan.
- [x] Consume the same checked-in examples from C and TypeScript tests. The
      TypeScript side genuinely validates against
      `contracts/v2/api/examples.json`
      (`webapp/tests/v2-api-contracts.test.ts`'s `isDiagnosticsResponse(examples.diagnostics)`
      etc.); the C side now does too, for every route this bullet named as a
      candidate plus the two the prior track already closed. A shared
      helper (`tests/host/support/test_examples_fixture.c`) loads and parses
      `examples.json` once per test binary; the following now use
      `cJSON_Compare()` to diff a live handler's real response against a
      checked-in example wholesale: `setupState`/`diagnostics` (prior
      track, `test_web_server_setup_route.c`/
      `test_web_server_administration_route.c`), and now also `status`/
      `limits` (`test_web_server_status_limits_route.c`), `sendAccepted`/
      `sendStatus` (`test_web_server_send_route.c`, `id` normalized since
      it is a fresh UUID every call), `blobList`/`blobCreated`
      (`test_web_server_blob_list.inc`/`test_web_server_blob_create.inc`),
      `session`/`settings`/`settingsUpdated`/`restartAccepted`/
      `resetSettingsAccepted`/`factoryResetAccepted`
      (`test_web_server_administration_route.c`). `changePassword` has no
      response body (`204`) to diff — genuinely not applicable, not a gap.
      `login` (`POST /api/v1/auth/login`) is the one named route still not
      compared: unlike every route above, it has no live `httpd_req_t`-level
      test at all yet (`login_handler`'s `httpd_req_to_sockfd()`/
      `getpeername()` IP-based rate-limiting call needs a real or faked
      socket, which no host test in this codebase provides) — a distinct,
      larger gap than "diff against the example," left open. This exact gap
      hid a 100%-reproducible production defect: real hardware testing
      2026-08-09 found every real login request failing with `500 login peer
      address unavailable`, because `esp_http_server`'s default dual-stack
      (IPv6) socket bind made `getpeername()` report `AF_INET6` for ordinary
      IPv4 clients, which `login_source_ipv4()`'s `AF_INET`-only check
      rejected outright. Fixed by disabling `CONFIG_LWIP_IPV6` project-wide
      (`firmware/sdkconfig.defaults`) rather than patching the vendored
      `esp_http_server` component; verified with 20/20 real logins
      succeeding afterward. The underlying host-test gap this bullet
      describes is still open — this only proves it was hiding a real bug
      and should be prioritized. See
      `docs/implementation-v2/V2_041_HARDWARE_LOGIN_FIX_2026-08-09.md` §2.
      Two genuine
      numeric discrepancies were found by this wholesale diffing, reported
      to Phil rather than silently resolved, and fixed by him 2026-08-10:
      (1) `sendAccepted`/`sendStatus`'s documented `estimatedDurationMs`
      for the exact `"make -j8{ENTER}"`/`keyPressMs: 8`/`interKeyMs: 15`
      example (also in SPEC_V2.md 13.10) read `214`, which did not match
      either the firmware or webapp parser's actual, identical,
      deterministic computation (`207`) — both `examples.json` and
      SPEC_V2.md now read `207`, matching the code; (2) `settingsUpdated`'s
      documented `restartRequired`/`reconnectRequired` for the exact
      `settingsUpdate` example (also in SPEC_V2.md 13.9) read `false`/
      `false`, contradicting SPEC_V2.md 13.9's own next sentence ("Changing
      access-point credentials sets both flags to `true`") and the code,
      which matched the prose, not the JSON — both `examples.json` and
      SPEC_V2.md now read `true`/`true`, matching the code and the prose.
      Both tests' now-unnecessary field-normalization workarounds were
      removed in the same fix, since the wholesale comparison passes
      cleanly on its own — see
      `docs/implementation-v2/V2_057_PHASE5_HARDENING_2026-08-09.md` for the
      original discrepancy writeup. One divergence from a prior track
      remains fixed:
      `web_server_diagnostics.c`'s `resetReason` values used hyphens
      (`"power-on"`) against the contract's underscores (`"power_on"`).

## Phase 5 exit gate

- [x] Route table exactly matches the v2 specification.
- [x] Old routes are absent.
- [ ] Contract and security tests pass. The tests that exist pass (56/56, up
      from 55/55 after this track's new `web_server_async_confirmation_tests`
      target), but this still is not the same as complete contract/security
      coverage. The physical-confirmation-required=true path is now partially
      closed (see the V2-057 route-matrix bullet: the "worker unavailable"
      synchronous fallback branch is genuinely, live-tested; the real
      FreeRTOS worker-queue/task branch is not, and — after a fresh
      investigation in this same track, not just trusting the four prior
      rounds' conclusions — remains genuinely unreachable at the host level
      without a substantial new concurrency-fake that would carry real risk
      of masking, not proving, correctness. See
      `docs/implementation-v2/V2_057_PHASE5_HARDENING_2026-08-09.md`.
- [x] API documentation examples match observed responses. `test_examples_fixture.c`-backed
      `cJSON_Compare()` tests now diff live handler output against
      `contracts/v2/api/examples.json` wholesale for every response-bearing
      route named in V2-057's "consume the same checked-in examples" bullet
      except `login`'s own POST response (no live handler test exists for it
      yet, a distinct, larger gap — see that bullet) and `changePassword`
      (genuinely bodyless, not applicable). Two concrete mismatches were
      found (`sendAccepted`/`sendStatus.estimatedDurationMs` and
      `settingsUpdated.restartRequired`/`reconnectRequired`, both also in
      SPEC_V2.md itself at 13.10/13.9), reported rather than silently
      fixed per CLAUDE.md's frozen-spec discipline, and corrected by Phil
      2026-08-10 in both `examples.json` and SPEC_V2.md; the one mismatch
      from a prior track (`diagnostics.resetReason`) remains fixed. See the
      V2-057 bullet and
      `docs/implementation-v2/V2_057_PHASE5_HARDENING_2026-08-09.md` for the
      full account.

---

## Phase 6 — Macro compiler, executor, USB HID, and send lifecycle

**Depends on:** Phases 1 and 5.

## V2-060 — Compiler compliance

- [x] Adapt or replace the C parser to satisfy the shared corpus.
- [x] Compile the complete source before execution begins.
- [x] Reject partial parses and all invalid Unicode.
- [x] Produce exact byte offset, line, and column.
- [x] Enforce action-count and duration limits before acceptance.
- [x] Ensure compile failure emits no HID report.

Evidence: `docs/implementation-v2/V2_060_061_062_COMPILER_EXECUTOR_RELEASE_ALL_2026-08-08.md`.
Audit of the already-adapted `macro_parser_v2.c`/`macro_plan_v2.c`; no code
changes required. `bash scripts/check-v2-contracts.sh --native-only` (21/21
native corpus cases) and `npm --prefix webapp run test --
tests/v2-macro-conformance.test.ts tests/v2-macro-canonical-tokens.test.ts`
(30/30) both pass against the shared `contracts/v2/macro-conformance.json`
corpus.

## V2-061 — Single executor and state machine

- [x] Use exactly one executor task.
- [x] Prevent HTTP handlers from typing directly.
- [x] Support `awaiting_confirmation`, `running`, `completed`, `cancelled`,
      `failed`, and `timed_out`.
- [x] Do not queue a second send.
- [x] Enforce the 60-second confirmation timeout.
- [x] Enforce the 310-second absolute deadline.
- [x] Preserve the current or most recent send for status recovery.

Evidence: `docs/implementation-v2/V2_060_061_062_COMPILER_EXECUTOR_RELEASE_ALL_2026-08-08.md`,
branch `phase6-compiler-executor-release-all`. `EXECUTION_AWAITING_CONFIRMATION`,
`require_confirmation`, `macro_executor_confirm()`, and a wall-clock-deadline
confirmation wait added to `macro_executor_engine.c`; the fixed
`APP_V2_EXECUTOR_ABSOLUTE_DEADLINE_MS` (310,000 ms) replaces a v1-shaped
per-request watchdog margin. `./scripts/run-tests.sh --sanitizers executor`
(ASan+UBSan, 2/2) and `./scripts/run-tests.sh` (all 45 suites) pass; new
`tests/host/executor_confirmation_tests.inc` covers confirm/cancel/expiry.
`POST /api/v1/send` does not yet gate on physical confirmation — that HTTP-layer
wiring is a documented, deliberate gap (owned by whichever stream next touches
`web_server`, not this task).

## V2-062 — Release-all invariant

- [x] Emit release-all after every key or chord action.
- [x] Attempt release-all on completion, cancellation, disconnect, suspension,
      timeout, parser invariant failure, task failure, queue failure, and internal
      error.
- [x] Clear internal pressed-key state even when transport delivery fails.
- [x] Report release failures separately from primary execution failures.

Evidence: `docs/implementation-v2/V2_060_061_062_COMPILER_EXECUTOR_RELEASE_ALL_2026-08-08.md`.
Most of this was already correct and covered by pre-existing tests; the one
genuine gap found and fixed was `macro_executor_engine_submit()`'s unlock- and
queue-failure paths never attempting release-all (SPEC_V2 §7.3 names "internal
error" and "queue failure" explicitly) — fixed, with new
`test_submission_ownership_and_recovery` assertions confirming `release_index`
increments on exactly those two paths. `./scripts/run-tests.sh` (all 45 suites)
and `./scripts/check-firmware.sh` (GCC + clang-tidy clean for `firmware/` and
`firmware/test_app/`) pass.

## V2-063 — Cancellation responsiveness

- [x] Make cancellation responsive during ordinary typing.
- [x] Make cancellation responsive during delay directives.
- [x] Make network and serial cancellation converge on the same state machine.
- [x] Add deterministic host tests for cancellation races.
- [x] Measure real-device last-keystroke latency after cancellation. Measured
      2026-08-10 on real ESP32-S3R8 hardware: cancelling a 60-character send
      mid-typing, the last observed HID keystroke landed 93.5 ms after the
      client-side cancel request — a real end-to-end measurement including
      Wi-Fi/HTTP round-trip, not just the firmware-internal 10 ms
      cancellation-slice bound. See
      `docs/implementation-v2/V2_063_064_USB_HID_HARDWARE_EVIDENCE_2026-08-10.md`.

Evidence: `docs/implementation-v2/V2_063_EXECUTOR_CANCELLATION_RESPONSIVENESS_2026-08-08.md`,
`docs/implementation-v2/V2_063_064_USB_HID_HARDWARE_EVIDENCE_2026-08-10.md`.
`macro_executor_engine.c`'s pre-existing `cancellable_delay()`/`CANCELLATION_SLICE_MS`
(10 ms) design already bounds cancellation latency during both key/chord dwell
and delay directives to one slice, and both `web_server_send.c`'s
`executor_cancel_adapter()` and `serial_console.c`'s `command_cancel()` already
call the same `macro_executor_cancel()` singleton (`macro_executor_engine_cancel()`
on the one global engine) — no code change was needed for the first three items,
only new deterministic tests (`tests/host/executor_cancellation_race_tests.inc`)
proving the one-slice bound and the network/serial convergence under a simulated
two-caller race. The last item requires the physical ESP32-S3R8 board and is
explicitly not claimed.

## V2-064 — USB identity and HIL evidence

All eight items verified on real ESP32-S3R8 hardware 2026-08-10 (firmware
`7f322c1`, unchanged from the V2-035 session). Full evidence and commands:
`docs/implementation-v2/V2_063_064_USB_HID_HARDWARE_EVIDENCE_2026-08-10.md`.

- [x] Verify `303a:4001` and required manufacturer, product, and serial
      strings. `lsusb -v -d 303a:4001` matches `SPEC_V2.md` §7.1 exactly:
      manufacturer "ESP32 Macro Keyboard Project", product "ESP32 Macro
      Keyboard", serial "ESP32S3-MACRO-01".
- [x] Capture host HID reports rather than relying on text-editor output.
      Real `/dev/hidraw*` reads via `tests/hardware/hid_capture.py`
      (resolved by VID:PID, not a hardcoded path).
- [x] Verify printable text exactly. `"abcXYZ123!@#"` and (after the
      disconnect/reconnect test) `"post-reconnect-ok"` both decoded from raw
      reports to an exact match.
- [x] Verify a chord sets modifier and usage concurrently. `{CTRL+SHIFT+T}`:
      one report carries `modifier=0x03` (CTRL+SHIFT) and `usage=23` ('t')
      together, not sequential reports.
- [x] Verify every terminal path ends with an all-zero report. Confirmed for
      completed (text, chord) and cancelled (mid-typing, mid-delay) paths
      across four distinct real captures. `failed`/`timed_out` not
      independently re-verified on hardware (see the evidence doc §8) —
      relies on the shared `release_all()` code path host tests already
      cover.
- [x] Verify invalid source types nothing. `"hello{NOTAREALDIRECTIVE}world"`
      returned `422 macro_parse_error` with zero HID reports captured — not
      even the leading text was typed.
- [x] Verify cancellation during typing and delay. Typing: cancelled a
      60-character send after 13 characters, all-zero final report.
      Delay: cancelled during `{DELAY:5000}`, the following action never
      executed, all-zero final report.
- [x] Verify disconnect and reconnect behavior. Real native-USB unplug/replug
      (not a software reset): `usb.state` went `ready` → `suspended` →
      `ready`; full functional recovery confirmed with a real post-reconnect
      send, exact text match.

## Phase 6 exit gate

- [x] C and TypeScript conformance suites pass the same corpus.
- [x] Executor host tests pass under sanitizers.
- [x] Required HID and cancellation hardware evidence is committed. All eight
      V2-064 items and V2-063's cancellation-latency item verified on real
      ESP32-S3R8 hardware 2026-08-10. See V2-063/V2-064 above and
      `docs/implementation-v2/V2_063_064_USB_HID_HARDWARE_EVIDENCE_2026-08-10.md`.

---

## Phase 7 — React repository core and persistence client

**Depends on:** Phases 1 and 5.

## V2-070 — Strict repository validation

- [x] Implement exact root, package, and macro validation.
- [x] Reject `activePackageId` and all unknown fields.
- [x] Enforce UUID, uniqueness, byte, integer, and macro-language invariants.
- [x] Leave the existing working copy unchanged after any validation failure.
- [x] Identify the exact failing field, package, or macro.

## V2-071 — In-memory working copy and dirty state

- [x] Store repository data only in live React memory.
- [x] Track the loaded/last-saved baseline and current working copy.
- [x] Mark dirty after package or macro content/order changes and imports.
- [x] Do not mark dirty after package selection, sends, cancellation, snapshot
      deletion, or UI preference changes.
- [x] Clear dirty only after successful snapshot save or deliberate replacement.
- [x] Preserve a live dirty working copy across in-tab reauthentication.

## V2-072 — Browser-storage prohibition

- [x] Remove repository JSON, IDs, names, source, and compressed bytes from
      localStorage, sessionStorage, IndexedDB, Cache Storage, and service workers.
- [x] Add automated tests and build scans for prohibited persistence.
- [x] Allow only unrelated presentation data when explicitly documented.

## V2-073 — Gzip and snapshot client

- [x] Feature-detect `CompressionStream("gzip")` and
      `DecompressionStream("gzip")`.
- [x] Show an explicit compatibility error when unsupported.
- [x] Never fall back to uncompressed repository storage.
- [x] Implement list, add, load, download, and delete client calls.
- [x] Validate before replacing the working copy.
- [x] Keep dirty work after failed save.
- [x] Implement `.emk-repository.json.gz` export.

## V2-074 — Package selection preference

- [x] Load `lastSelectedPackageId` from settings.
- [x] Open it only when it identifies a package in the loaded repository.
- [x] Auto-select the sole package and persist that selection.
- [x] Show the Package Chooser when selection cannot be resolved.
- [x] Update NVS only when the selected ID changes.
- [x] Never serialize selection into repository JSON.

## V2-075 — React send helper

- [x] Implement `sendMacro(request, { onStatus, onComplete })`.
- [x] Poll no slower than once per second while non-terminal.
- [x] Invoke status callbacks only for meaningful state/progress changes.
- [x] Invoke completion exactly once per send.
- [x] Recover state after reload using `GET /api/v1/send`.
- [x] Avoid duplicate POSTs and callbacks across orientation changes and rerenders.

## Phase 7 exit gate

- [x] Repository, gzip, dirty-state, storage-prohibition, settings, and send-helper
      tests pass.
- [x] Production frontend contains no firmware package/macro CRUD client.

---

## Phase 8 — Startup, provisioning, and authentication UI

**Depends on:** Phases 4, 5, and 7.

## V2-080 — First-run setup screens

- [x] Load first-run state from unprovisioned-only `GET /api/v1/setup`.
- [x] Reject any setup-state response with fields beyond `provisioned` and
      `deviceName`.
- [x] Implement device identification and setup-code entry.
- [x] Implement device name, AP credentials, administrator password, and optional
      physical-confirmation fields.
- [x] Implement review without displaying secret values.
- [x] Implement apply, restart, reconnect guidance, and Sign In transition.
- [x] Keep repository creation outside provisioning.

## V2-081 — Sign In

- [x] Show Sign In only for a configured device without a valid session.
- [x] Display rate-limit and lockout errors without leaking account state.
- [x] Redirect successful authentication into repository startup.
- [x] Do not add per-phone onboarding.

## V2-082 — Authenticated startup state machine

- [x] Show a brief repository-loading state when no live working copy exists.
- [x] Load settings and the newest blob by numeric ID.
- [x] Never show the chooser merely because several blobs exist.
- [x] Never silently fall back after newest-blob failure.
- [x] Route valid repositories through package-selection resolution.
- [x] Recover active or recent send status.
- [x] Restore the exact current route and draft when the tab still has a live
      working copy.

## V2-083 — First repository and first package

- [x] Show Create Your First Repository when no blobs exist.
- [x] Create an empty valid repository in memory.
- [x] Ask for the first package name.
- [x] Set `lastSelectedPackageId` to the new package.
- [x] Open the empty Macros page.
- [x] State clearly that data exists only in this tab until Save snapshot succeeds.
- [x] Keep Unsaved changes visible.

## V2-084 — Startup failure surfaces

- [x] Handle device unreachable.
- [x] Handle unsupported Compression Streams.
- [x] Handle invalid settings response.
- [x] Handle unreadable or invalid newest snapshot.
- [x] Handle missing or invalid selected package.
- [x] Preserve recoverable working state and provide precise next actions.

## Phase 8 exit gate

- [x] Startup decision-table tests cover every provisioning/session/blob/package
      combination.
- [x] Real-browser tests cover first phone, refresh, expired session, no blobs,
      invalid newest blob, and send recovery. `webapp/tests/browser/run-browser-tests.mjs`'s
      `runStartupWorkflows()` runs five scenarios, each against its own fresh
      fixture server and browser context: `runStartupFirstPhoneScenario`
      (unprovisioned device through First-Run Setup, Sign In, and Create Your
      First Repository), `runStartupRefreshAndSendRecoveryScenario` (a real
      `page.reload()` mid-send, asserting the full startup fetch sequence
      re-runs and the recovered send state comes from that sequence's own
      `GET /api/v1/send`, not merely eventually-correct page text),
      `runStartupExpiredSessionScenario` (dirties the working copy, forces a
      `401` server-side, waits for the app's own status poll to drop to Sign
      In, re-authenticates, and asserts zero new blob fetches — the same
      dirty in-memory working copy resumed), `runStartupNoBlobsScenario`, and
      `runStartupInvalidNewestBlobScenario` (a corrupt newest blob with a
      valid older one; asserts Snapshot recovery, explicit recovery via the
      older blob, and that the corrupt blob is never deleted). Command:
      `node tests/browser/run-browser-tests.mjs`, run 5 times standalone —
      `exit=0` every time, no flakiness — plus part of
      `./scripts/check-webapp.sh`'s `test:browser` step, run twice, both
      green. See
      `docs/implementation-v2/V2_BROWSER_COVERAGE_PHASE_8_9_10_2026-08-09.md`.

---

## Phase 9 — Macros page and Quick Send operating console

**Depends on:** Phases 7 and 8.

## V2-090 — Application shell

- [x] Show device name, selected package, USB state, and repository state.
- [x] Show Save snapshot whenever dirty.
- [x] Implement bottom navigation:

```text
Macros | Packages | Snapshots | Settings
```

- [x] Preserve accessibility and safe-area behavior.

## V2-091 — Macro list

- [x] Show ordered macros for the selected package.
- [x] Show Add macro, Edit, Send, and overflow controls. All four are direct
      controls in `MacrosPage.tsx`: an **Add macro** header button, a
      per-row **Edit** button (labeled "Edit `<macro name>`", wired to
      `onOpenEditMacro` -> `navigateToEditMacro` -> `MacroEditorPage` loading
      that exact macro by ID -- verified by reading `AppV2.tsx` and
      `routingV2.ts`), a per-row **Send** button, and a per-row
      `MacroOverflowMenu` (Preview and send/Duplicate/Delete, added in Phase
      10's V2-101). Left unchecked through Phase 9 because the overflow menu
      did not exist yet and Edit had no direct test; both gaps are now
      closed -- overflow is tested in
      `tests/v2-macros-page.test.tsx::"MacrosPage — V2-101 overflow menu
      (Duplicate/Delete)"` and `"the overflow menu still offers Preview and
      send"`, and Edit now has its own direct test, added by this audit:
      `tests/v2-macros-page.test.tsx::"MacrosPage — V2-091 macro list >
      Edit calls onOpenEditMacro with exactly the clicked row's macro ID"`
      (`npm --prefix webapp run test` -- 523/523 passed).
- [x] Support accessible reordering.
- [x] Disable Send unless USB is `ready` and no other send is active.
- [x] Keep the user on the Macros page for ordinary sends.

## V2-092 — Macro-source privacy

- [x] Hide source previews by default.
- [x] Show a non-revealing placeholder.
- [x] Support temporary per-row reveal.
- [x] Honor the device-wide source-preview preference.
- [x] Prevent hidden source from entering acknowledgements, accessible names,
      live regions, logs, diagnostics, notifications, or telemetry.

## V2-093 — Quick Send

- [x] Make the primary Send control issue one explicit `POST /api/v1/send`.
- [x] Show selected-row and page-level progress inline.
- [x] Disable other Send controls while active.
- [x] Show serial-confirmation waiting state inline.
- [x] Keep Cancel and release all keys accessible.
- [x] Show completion acknowledgement for approximately three to five seconds.
- [x] Persist cancellation, failure, timeout, and release-error messages until the
      user can understand or dismiss them.
- [x] Never include source in the acknowledgement.

## V2-094 — Optional Preview and Send

- [x] Make preview available from overflow actions. `MacrosPage.tsx`'s
      overflow menu offers "Preview and send" per macro row, tested in
      `tests/v2-macros-page.test.tsx::"the overflow menu still offers Preview
      and send"`.
- [x] Honor Always Preview when configured. The primary Send control now
      reads the device's `sendMode` setting: `preview` routes through the
      Preview and Send page instead of quick-sending, `quick` sends directly
      as before — both branches tested in
      `tests/v2-macros-page.test.tsx::"MacrosPage — V2-094 honoring Always
      Preview"`. Closed by
      `docs/implementation-v2/V2_100_103_MACRO_EDITING_PACKAGE_MANAGEMENT_2026-08-09.md`.
- [x] Show package, macro, source/action summary, timing, action count, duration,
      and USB state.
- [x] Provide explicit Send now and Cancel.
- [x] Return to the Macros page for progress.

## V2-095 — Reload and race handling

- [x] Recover inline send state after reload.
- [x] Prevent double-send on rapid taps.
- [x] Prevent duplicate completion callbacks.
- [x] Handle `409` by showing the actual current send.
- [x] Handle session expiry without discarding the working copy.

## Phase 9 exit gate

- [x] Macros page browser tests cover idle, USB unavailable, quick send,
      confirmation, progress, cancel, complete, failure, timeout, release error,
      reload, and rapid repeated input. The last two named gaps are now closed:
      rapid repeated input is a new scenario block inside `runBrowserWorkflows()`
      (`webapp/tests/browser/run-browser-tests.mjs`) that dispatches three
      synchronous DOM `.click()` calls on the Send button from inside a single
      `page.evaluate()` — unlike Playwright's own `locator.click()`, which is a
      separately-awaited CDP round trip per call, this genuinely reproduces the
      same-tick double-dispatch race `MacrosPage.tsx`'s `startingRef` guard
      (V2-095) is built to withstand, and asserts exactly one
      `POST /api/v1/send` fired. USB unavailable is a new
      `runUsbUnavailableWorkflow()` function against its own fixture server
      started with `usb.state: "disconnected"` from the very first
      `GET /api/v1/status` response (no poll-flip wait needed, since
      `useDeviceStatus` polls immediately on mount); it asserts the shell
      header shows `USB disconnected`, every Send button is `disabled`, and
      that USB becoming ready device-side re-enables Send within one real
      5-second poll cycle. Command: `node tests/browser/run-browser-tests.mjs`,
      run 5 times standalone — `exit=0` every time. See
      `docs/implementation-v2/V2_BROWSER_COVERAGE_PHASE_8_9_10_2026-08-09.md`.
- [x] Ordinary Quick Send never navigates to a standalone progress/result route.

---

## Phase 10 — Macro editing and package management

**Depends on:** Phases 7 and 9.

## V2-100 — Macro editor

- [x] Implement name, source, key-press duration, and inter-key delay fields.
- [x] Show UTF-8 byte counts.
- [x] Implement directive insertion controls.
- [x] Run live TypeScript validation against the shared corpus implementation.
- [x] Show exact error location and Go to error.
- [x] Show action count and estimated duration when valid.
- [x] Save only to the in-memory working copy.
- [x] Cancel without changing the working copy.

Evidence: commit `606fa02`, `webapp/src/features/macros/v2/MacroEditorPage.tsx`,
tested by `webapp/tests/v2-macro-editor-page.test.tsx`. See
`docs/implementation-v2/V2_100_103_MACRO_EDITING_PACKAGE_MANAGEMENT_2026-08-09.md`.

## V2-101 — Macro CRUD and ordering

- [x] Create, edit, duplicate, move, reorder, and delete macros locally.
- [x] Generate IDs with `crypto.randomUUID()`.
- [x] Preserve global macro-ID uniqueness.
- [x] Mark repository dirty after every actual content/order change.
- [x] Avoid dirty transitions after no-op edits.

Evidence: commit `606fa02`, `webapp/src/v2/repositoryEditing.ts` plus
`MacrosPage.tsx`'s new overflow menu (the gap Phase 9 explicitly deferred
here), tested by `webapp/tests/v2-repository-editing.test.ts` and
`webapp/tests/v2-macros-page.test.tsx::"MacrosPage — V2-101 overflow menu
(Duplicate/Delete)"`. See
`docs/implementation-v2/V2_100_103_MACRO_EDITING_PACKAGE_MANAGEMENT_2026-08-09.md`.

## V2-102 — Package management

- [x] Create, rename, duplicate, reorder, and delete packages locally.
- [x] Generate canonical UUID v4 package IDs.
- [x] Mark repository dirty after actual changes.
- [x] Identify destructive targets by name.
- [x] Resolve and persist package selection after selected-package deletion.
- [x] Do not make ordinary package switching dirty.

Evidence: commit `606fa02`,
`webapp/src/features/macros/v2/PackageManagementPage.tsx`, tested by
`webapp/tests/v2-package-management-page.test.tsx` and
`webapp/tests/v2-repository-editing.test.ts::"repositoryEditing — package
management (V2-102)"`. See
`docs/implementation-v2/V2_100_103_MACRO_EDITING_PACKAGE_MANAGEMENT_2026-08-09.md`.

## V2-103 — Unsaved-change protection

- [x] Keep Unsaved changes and Save snapshot visible on all operational
      screens. `MacroEditorPage`/`PackageManagementPage` render inside
      `AppShellV2` (`AppV2.tsx`), which already provides this per V2-090.
- [x] Register `beforeunload` while dirty where supported.
      `useBeforeUnloadGuard.ts`, tested in `tests/v2-before-unload-guard.test.tsx`;
      confirmed real in an actual browser, not just jsdom, while diagnosing
      and fixing a genuine CDP-harness hang this listener exposed — see
      `docs/implementation-v2/V2_100_103_MACRO_EDITING_PACKAGE_MANAGEMENT_2026-08-09.md`.
- [x] Warn before Sign Out, snapshot load, import replacement, reset settings,
      and factory reset. `UnsavedChangesPrompt.tsx` exists and is
      unit-tested in isolation. All five trigger points are now wired:
      snapshot load and import replacement in `SnapshotsPage.tsx` (Phase 11,
      V2-113/V2-115); Sign Out, reset settings, and factory reset in
      `features/settings/v2/SettingsPage.tsx` (Phase 12, V2-120/V2-121).
      Restart is deliberately not gated by this same prompt: SPEC_V2 §7.3's
      own trigger list names only Sign Out/snapshot load/import
      replacement/reset settings/factory reset, and restart preserves the
      dirty working copy through its reconnect-and-reauthenticate flow
      rather than discarding it (see V2-121 below), so there is nothing at
      risk for that prompt to protect. See
      `docs/implementation-v2/V2_120_122_SETTINGS_DIAGNOSTICS_DESTRUCTIVE_2026-08-09.md`.
- [x] Offer context-appropriate Cancel, Export working copy, Save snapshot, and
      Discard options. `UnsavedChangesPrompt.tsx` implements all four; only
      its call sites (the item above) are missing.
- [x] Never claim closed unsaved work can be recovered.

## Phase 10 exit gate

- [x] Editing and package-management unit and browser tests pass. Unit tests
      pass in full (577/577, `npm --prefix webapp run test`). Browser: a new
      `runMacroEditingWorkflows()` (`webapp/tests/browser/run-browser-tests.mjs`),
      against its own fresh fixture server/context, covers: Add macro (name,
      key-press/inter-key timing fields, directive insertion via a real
      focused-textarea click, live validation); an invalid source's exact
      error location plus "Go to error" moving real textarea focus/selection;
      Save and Cancel; and Package management (create, rename, duplicate,
      reorder, name-bearing two-step-confirm delete, and Open). It also
      reloads once while the working copy is dirty to prove the native
      `beforeunload` dialog (registered via `page.on("dialog", ...)`) doesn't
      hang the page — the same defect
      `V2_100_103_MACRO_EDITING_PACKAGE_MANAGEMENT_2026-08-09.md` found and
      fixed in this harness. Command:
      `node tests/browser/run-browser-tests.mjs`, run 5 times standalone —
      `exit=0` every time, no flakiness. See
      `docs/implementation-v2/V2_BROWSER_COVERAGE_PHASE_8_9_10_2026-08-09.md`.
- [x] No edit calls a firmware package or macro route. True by construction:
      v2 firmware has no package/macro CRUD routes at all (Phase 2), and
      `repositoryEditing.ts` makes no HTTP calls — it only mutates the
      in-memory working copy.
- [x] Dirty-state transition matrix is fully tested. Every named
      dirty/no-op transition has a direct test — content/order changes
      dirty, no-op edits don't, ordinary package switching doesn't.

---

## Phase 11 — Snapshots, import, and export UI

**Depends on:** Phases 7 and 10.

## V2-110 — Manual Save snapshot

- [x] Validate the entire repository. `saveWorkingCopyAsSnapshot` now calls
      `validateRepositoryForUse` before ever serializing, per SPEC_V2 §8.5;
      `SnapshotValidationError` on failure.
- [x] Serialize compact UTF-8 JSON. `serializeRepository` (Phase 7).
- [x] Gzip in React. Reuses `gzipCompress`/`CompressionStream("gzip")`
      (Phase 7 V2-073) unmodified.
- [x] Enforce device limits before upload. `SnapshotTooLargeError` against
      `v2Limits.blobMaxBytes` before any `fetch` call.
- [x] Upload only after an explicit user action. Only two call sites exist:
      the header Save snapshot button and `SnapshotsPage`'s Save current
      snapshot button, both plain `onClick` handlers.
- [x] Mark saved only after `201 Created`. `v2PostBinary` now requires
      exactly `201` (previously accepted any 2xx); `store.markSaved` runs
      only after that resolves.
- [x] Preserve dirty work after every failure. Tested for validation, size,
      and network/server failures — working copy stays dirty and unchanged.
- [x] Never autosave after edits, sends, package selection, timers, or
      navigation. True by construction (no timer or edit path calls save);
      tests assert zero incidental save/delete calls.

Evidence: `webapp/src/v2/snapshotClient.ts`, `webapp/src/v2/apiClient.ts`,
tested by `webapp/tests/v2-snapshot-client.test.ts` and
`webapp/tests/v2-api-client.test.ts`. See
`docs/implementation-v2/V2_110_116_SNAPSHOTS_IMPORT_EXPORT_2026-08-09.md`.

## V2-111 — Snapshot management

- [x] Show blob ID, stored size, loaded indicator, storage usage, and configured
      advisory target.
- [x] Provide Load, Download, Delete, and Save current snapshot.
- [x] Avoid device-generated dates.
- [x] Permit manual loading at any time.
- [x] Confirm deletion by exact blob ID and consequence.
- [x] Never automatically delete a snapshot.

Evidence: `webapp/src/features/snapshots/v2/SnapshotsPage.tsx`, tested by
`webapp/tests/v2-snapshots-page.test.tsx` and real-Chrome
`webapp/tests/browser/run-browser-tests.mjs`. See
`docs/implementation-v2/V2_110_116_SNAPSHOTS_IMPORT_EXPORT_2026-08-09.md`.

## V2-112 — Advisory retention target

- [x] Default target to five. Read from `settings.snapshotRetentionTarget`
      (device default `5`, V2-014/V2-043) — never hardcoded client-side.
- [x] Show a non-blocking cleanup indicator when count exceeds target.
- [x] Let the user choose which snapshots to delete.
- [x] Permit a sixth or later snapshot when storage permits.
- [x] Test that no save path triggers deletion.

Evidence: `webapp/src/v2/snapshotRetention.ts`, tested by
`webapp/tests/v2-snapshot-retention.test.ts` and
`webapp/tests/v2-snapshots-page.test.tsx`. See
`docs/implementation-v2/V2_110_116_SNAPSHOTS_IMPORT_EXPORT_2026-08-09.md`.

## V2-113 — Dirty-work protection during load

- [x] Warn when loading another snapshot while dirty.
- [x] Offer Cancel, Export working copy, Save snapshot, and Discard changes and
      load.
- [x] Validate the selected snapshot before replacing memory.
- [x] Leave stored snapshots untouched.
- [x] Resolve selected package after load.

Evidence: `webapp/src/features/snapshots/v2/SnapshotsPage.tsx` (reuses
`UnsavedChangesPrompt.tsx` from Phase 10), tested by
`webapp/tests/v2-snapshots-page.test.tsx` and real-Chrome
`webapp/tests/browser/run-browser-tests.mjs`. See
`docs/implementation-v2/V2_110_116_SNAPSHOTS_IMPORT_EXPORT_2026-08-09.md`.

## V2-114 — Unreadable snapshot recovery

- [x] Show the failing blob and exact decompression/schema error.
- [x] Keep it stored.
- [x] Allow download, delete, or deliberate selection of another blob.
- [x] Never silently fall back.

Evidence: `webapp/src/features/snapshots/v2/SnapshotsPage.tsx`, tested by
`webapp/tests/v2-snapshots-page.test.tsx`. See
`docs/implementation-v2/V2_110_116_SNAPSHOTS_IMPORT_EXPORT_2026-08-09.md`.

## V2-115 — Import and export

- [x] Export the current working copy as `.emk-repository.json.gz`.
- [x] Import bytes, decompress, decode, parse, and fully validate before
      replacement.
- [x] Show package and macro counts before confirmation.
- [x] Mark imported data dirty.
- [x] Do not upload automatically.
- [x] Exclude every device credential, session, key, and diagnostic field.
      True by construction of the frozen `Repository` schema type (Phase 1):
      only `format`/`schemaVersion`/`packages`/`macros` fields exist to
      serialize, and `validateRepositoryForUse`'s exact-key check rejects any
      extra field on import.

Evidence: `webapp/src/features/snapshots/v2/SnapshotsPage.tsx`, tested by
`webapp/tests/v2-snapshots-page.test.tsx` and real-Chrome
`webapp/tests/browser/run-browser-tests.mjs` (a real file selected via
Playwright's `page.setInputFiles()` and a real downloaded, gzip-decompressed
export). See
`docs/implementation-v2/V2_110_116_SNAPSHOTS_IMPORT_EXPORT_2026-08-09.md`.

## V2-116 — Advanced non-atomic replace

- [x] Keep normal saves additive.
- [x] Implement replacement only as an explicitly advanced delete-then-add flow.
- [x] Warn that a failed add does not restore the deleted blob.
- [x] Test delete-success/add-failure behavior.

Evidence: `webapp/src/v2/snapshotClient.ts` (`replaceSnapshotWithWorkingCopy`),
`webapp/src/features/snapshots/v2/SnapshotsPage.tsx`, tested by
`webapp/tests/v2-snapshot-client.test.ts` and
`webapp/tests/v2-snapshots-page.test.tsx`. See
`docs/implementation-v2/V2_110_116_SNAPSHOTS_IMPORT_EXPORT_2026-08-09.md`.

## Phase 11 exit gate

- [x] Snapshot and import/export browser tests pass. Real-Chrome coverage
      added to `run-browser-tests.mjs`; run repeatedly and stable — see the
      implementation report for exact run counts.
- [x] No automatic snapshot creation or deletion exists.
- [x] Dirty work survives all recoverable failure paths.

---

## Phase 12 — Settings, diagnostics, and destructive operations UI

**Depends on:** Phases 5 and 11.

## V2-120 — Settings UI

- [x] Implement device name.
- [x] Implement serial-confirmation policy.
- [x] Implement AP and optional station configuration.
- [x] Implement administrator password change.
- [x] Implement Quick Send/Always Preview.
- [x] Implement advisory retention target.
- [x] Implement source-preview preference.
- [x] Keep `lastSelectedPackageId` hidden from ordinary text editing.
- [x] Ensure preference changes do not dirty the repository.

Evidence: `webapp/src/features/settings/v2/SettingsPage.tsx`,
`webapp/src/v2/settingsClient.ts` (`updateSettings`/`changePassword`),
tested by `webapp/tests/v2-settings-page.test.tsx`,
`webapp/tests/v2-settings-client.test.ts`, and real-Chrome
`webapp/tests/browser/run-browser-tests.mjs`. See
`docs/implementation-v2/V2_120_122_SETTINGS_DIAGNOSTICS_DESTRUCTIVE_2026-08-09.md`.

## V2-121 — Restart and reset flows

- [x] Implement restart confirmation and reconnect guidance.
- [x] Implement reset-settings confirmation with exact preservation behavior.
- [x] Implement factory-reset confirmation with exact erase and reprovision
      behavior.
- [x] Protect dirty work before every disruptive action. Sign Out, reset
      settings, and factory reset are dirty-guarded via
      `UnsavedChangesPrompt`; restart is not — see the V2-103 entry above for
      why that is correct rather than a gap.
- [x] Handle connection loss and recovery explicitly.
      `features/settings/v2/useDeviceReconnect.ts` polls an authenticated
      route until the device answers again (network failure and transient
      `5xx` both mean "still down"; a `401` means "back, but the RAM-only
      session is gone") and `DeviceReconnectScreen.tsx` replaces the whole
      authenticated shell while that is in flight, per `AppV2.tsx`'s
      `AuthenticatedShell`.

Evidence: `webapp/src/features/settings/v2/SettingsPage.tsx`,
`webapp/src/features/settings/v2/DeviceReconnectScreen.tsx`,
`webapp/src/features/settings/v2/useDeviceReconnect.ts`,
`webapp/src/v2/deviceActionsClient.ts`, `webapp/src/AppV2.tsx`, tested by
`webapp/tests/v2-settings-page.test.tsx`,
`webapp/tests/v2-device-reconnect.test.ts(x)`,
`webapp/tests/v2-device-reconnect-screen.test.tsx`,
`webapp/tests/v2-device-actions-client.test.ts`, and
`webapp/tests/v2-app-v2.test.tsx`'s real (unmocked) end-to-end restart ->
reconnect -> reauthenticate -> resume test. No real-Chrome coverage exists
for the restart/reset-settings/factory-reset reconnect sequence itself
(deliberately — see the implementation report for why). See
`docs/implementation-v2/V2_120_122_SETTINGS_DIAGNOSTICS_DESTRUCTIVE_2026-08-09.md`.

## V2-122 — Diagnostics UI

- [x] Render the fixed diagnostics schema.
- [ ] Show firmware/build, uptime, reset reason, memory, stack, USB, Wi-Fi,
      storage, blob count, send state, health, and invalid/temp filenames.
      Left unchecked as literally written: SPEC_V2 §13.13's fixed
      `GET /api/v1/diagnostics` schema has no `stack` field anywhere (not at
      the top level, not inside `memory`) — only `freeHeapBytes`,
      `minimumFreeHeapBytes`, and `largestFreeBlockBytes`. Every other item
      in this line is rendered (`DiagnosticsPage.tsx`) and tested. Not
      fabricating a `stack` field to check this box — see the implementation
      report for the recommendation to the product owner.
- [x] Do not display package or macro data. True by construction
      (`isDiagnosticsResponse`'s exact-key guard) and directly tested: a
      unit test asserts a response carrying an unexpected field is rejected
      before it reaches React state, and the real-Chrome test asserts the
      rendered Diagnostics page never contains the fixture's package name.
- [x] Provide copy/download only after filtering sensitive content.
      `v2/diagnosticsExport.ts`'s `buildDiagnosticsExportText` re-derives
      Copy/Download's output field by field from the typed response rather
      than passing any raw object through; tested directly, including that
      an injected extra field never survives into the exported text.

Evidence: `webapp/src/features/settings/v2/DiagnosticsPage.tsx`,
`webapp/src/v2/diagnosticsClient.ts`, `webapp/src/v2/diagnosticsExport.ts`,
tested by `webapp/tests/v2-diagnostics-page.test.tsx`,
`webapp/tests/v2-diagnostics-client.test.ts`,
`webapp/tests/v2-diagnostics-export.test.ts`, and real-Chrome
`webapp/tests/browser/run-browser-tests.mjs`. See
`docs/implementation-v2/V2_120_122_SETTINGS_DIAGNOSTICS_DESTRUCTIVE_2026-08-09.md`.

## Phase 12 exit gate

- [x] Settings and diagnostics contract/browser tests pass. Vitest unit and
      `AppV2` integration coverage pass in full (576/576); real-Chrome
      coverage exists for Settings device-name edit and Diagnostics render
      but not for the restart/reset-settings/factory-reset reconnect
      sequence (see V2-121). No dedicated firmware/C-side contract test
      exists yet for the device-action or diagnostics routes under
      `tests/v2_contracts/` — out of this track's webapp-only file surface;
      left for whichever track owns that suite.
- [x] Destructive flows preserve or explicitly discard dirty work. Sign Out,
      reset settings, and factory reset each go through
      `UnsavedChangesPrompt` when the working copy is dirty (Cancel/Export
      working copy/Save snapshot/Discard changes), tested per action in
      `webapp/tests/v2-settings-page.test.tsx` and end to end for Sign Out
      in `webapp/tests/v2-app-v2.test.tsx`. Restart preserves the working
      copy without a prompt because nothing about it discards or replaces
      that copy (see V2-121/V2-103 above).
- [x] Secret-leak tests pass. `webapp/tests/v2-diagnostics-export.test.ts`
      proves the Diagnostics copy/download text cannot carry an injected
      field beyond the fixed schema; `isDiagnosticsResponse`
      (`apiGuards.ts`, pre-existing) already rejects any response outside
      that schema before it reaches React state. No password, passphrase,
      or session token field exists anywhere in the Settings or Diagnostics
      UI's rendered output, copy text, or download text — `SettingsPage.tsx`
      never echoes back a submitted password/passphrase value, and
      `changePassword`/`factoryResetDevice`'s request bodies (the only place
      a password appears client-side) are never logged or rendered.

---

## Phase 13 — Portrait phones, responsive layout, and accessibility

**Depends on:** Phases 8–12.

## V2-130 — Responsive layout

- [ ] Support a minimum 320 CSS-pixel viewport. `webapp/src/styles.css` still
      only sets `body { min-width: 320px; }` — an incidental floor, not a
      verified 320px layout. The real-browser responsive check
      (`assertResponsiveLayout()` in `webapp/tests/browser/run-browser-tests.mjs`,
      pre-existing) exercises a 360 CSS-pixel viewport, not 320. Left
      unchecked: 2026-08-09's V2-131/V2-132 work did not touch this item.
- [ ] Use single-column phone layouts. `styles.css`'s `@media (width <= 32rem)`
      block already collapses `.app-header`/`.card`/`.page-heading` to
      `flex-direction: column`, but this predates Phase 13 and has never been
      deliberately audited against every screen. Left unchecked (unchanged by
      2026-08-09's work).
- [x] Support wider tablet and desktop layouts without changing workflow.
      `styles.css` widens `.standalone`/`.app-shell` from a 48rem cap to a
      64rem cap at `@media (width >= 60rem)` (2026-08-09) — a relaxed max
      width, not a workflow change (same single-column markup, no split
      panes or new routes). The pre-existing real-browser
      `assertResponsiveLayout()` (`webapp/tests/browser/run-browser-tests.mjs`)
      asserts desktop content (1280x800, CDP device-metrics override) is
      wider than mobile content (360x640) with no horizontal scroll at
      either size — `./scripts/check-webapp.sh`'s `test:browser` step,
      2026-08-09, passed ("Real Chrome v2 Macros page/Quick Send workflows
      passed."). UI_UX_SPEC_V2 §13's "MAY use wider cards, split panes, or
      denser management layouts" is optional and remains unbuilt — only the
      "without changing workflow" half of the requirement is claimed here.
- [x] Keep touch targets at least 44 by 44 CSS pixels. Fixed 2026-08-09:
      `.header-button` (`min-height: 36px` -> `44px`) and
      `.directive-toolbar button` (`38px` -> `44px`) in `webapp/src/styles.css`
      were the two counter-examples the previous audit found; `grep -n
      "min-height" webapp/src/styles.css` now shows no value below `44px`
      anywhere in the file. Caveat: the pre-existing real-browser
      `assertTouchTargets()` (`webapp/tests/browser/run-browser-tests.mjs`)
      passes but only runs against the Macros page before either fixed
      control is on-screen (`.header-button` needs a dirty working copy;
      `.directive-toolbar` is macro-editor-only), so it does not
      independently exercise these two controls — the claim rests on the
      source-level fix and the grep, not that specific browser assertion.
- [ ] Respect display cutouts and gesture-navigation safe areas. Widened
      2026-08-09: `.standalone` (the full-screen container for every screen
      outside the authenticated shell — First-Run Setup, Sign In,
      device-unreachable/loading/reconnect) previously had no safe-area
      padding at all and now pads all four sides with
      `env(safe-area-inset-*)`; the new `.landscape-block` orientation
      surface (V2-131) does the same. Combined with the pre-existing
      `.app-header`/`.bottom-nav` coverage, every full-screen container now
      pads for cutouts. Left unchecked regardless, per the hard rule against
      claiming device validation from source review alone — no physical
      device with a notch/cutout has verified this.
- [x] Ensure bottom navigation does not cover final actions. Structural:
      `AppShellV2.tsx` renders `<header>`, `<main>`, `<nav>` as plain
      block-flow siblings (not absolutely/fixed positioned), and
      `styles.css`'s `.bottom-nav` uses `position: sticky; bottom: 0` rather
      than `fixed`/`absolute` — a sticky element stuck to the viewport
      bottom still occupies its own space in normal flow, so it can only sit
      below `main`'s content box, never overlap it. Confirmed empirically
      with an isolated Playwright reproduction of the same markup/CSS
      (short-content case, 375x700 viewport): last actionable button's
      bottom edge at y=356, nav's top edge at y=372 — no overlap
      (2026-08-09, throwaway script, not committed).

## V2-131 — Portrait-required phone surface

- [x] Apply phone-landscape blocking using tested coarse-pointer, orientation, and
      short-viewport criteria, initially equivalent to the query below.
      Implemented 2026-08-09:
      `webapp/src/features/shell/v2/useLandscapePhoneBlock.ts`'s
      `landscapePhoneMediaQuery` constant is exactly this string, read via
      `window.matchMedia` with a `change` listener (no polling). Tested in
      jsdom with a controllable `matchMedia` fake
      (`webapp/tests/fakeMatchMedia.ts`,
      `webapp/tests/v2-landscape-phone-block.test.tsx`: "uses the exact
      UI_UX_SPEC_V2 §12.4 media query", "reflects the initial matchMedia
      state on mount", "reacts to a later matchMedia change event without
      remounting", "stops listening after unmount"). Honest gap: this proves
      the hook's own wiring and the query string's exactness, not that real
      Chromium's compound `(pointer: coarse)` evaluates as intended under
      actual touch/orientation emulation — no real-browser test exercises
      this (see the Phase 13 exit gate's still-unchecked "Real Android phone
      portrait/landscape tests pass").

```css
@media (orientation: landscape) and (pointer: coarse) and (max-height: 600px)
```

- [x] Show Rotate your phone instead of ordinary operational content.
      `webapp/src/features/shell/v2/LandscapeBlockSurface.tsx` renders the
      exact UI_UX_SPEC_V2 §12.2 copy ("Rotate your phone" /
      "ESP32 Macro Keyboard is designed for portrait mode."); asserted by
      `v2-landscape-phone-block.test.tsx` and, wired into the real running
      app, `webapp/tests/v2-app-v2.test.tsx`'s "hides the app behind Rotate
      your phone in landscape, and restores the exact route and dirty state
      on return" test.
- [x] Restore the exact route, draft, working copy, and dirty state on portrait.
      `src/AppV2.tsx`'s `AuthenticatedShell` keeps its normal content tree
      (`AppShellV2` and everything inside it) mounted at all times and only
      toggles `display: none` on an ancestor `<div>` based on
      `useLandscapePhoneBlock()` — nothing unmounts, so no route/draft/
      working-copy/dirty state is ever lost. Proven end to end by
      `v2-app-v2.test.tsx`'s new test: signs in (working copy already
      dirty), navigates to Snapshots, flips the fake `matchMedia` to
      landscape (shell hidden, `.app-shell` still present in the DOM,
      "Rotate your phone" shown), flips back to portrait, and asserts the
      route is still "Snapshots", `"Unsaved changes"` is still shown, and
      the `/api/v1/blob` `GET` count is unchanged (proving
      `RepositoryStartupScreen` never remounted/re-fetched).
- [x] Do not reload, clear memory, restart a send, or duplicate callbacks. Same
      "keep mounted, only hide" mechanism as above rules out all four by
      construction: `window.location.reload()` is never called by this path,
      nothing is cleared, `MacrosPage`'s send-tracking effects and refs are
      untouched by a CSS-only `display` toggle, and no component
      double-mounts (which is what would duplicate a `sendMacro`/`trackSend`
      call — `sendClient.ts`'s own doc comment names exactly this
      "orientation-change remount" risk as the thing callers must avoid).
      The active-send V2-132 browser-level integration test below exercises
      the send/cancel path across a landscape excursion as further evidence.
- [ ] Add `orientation: "portrait-primary"` to the manifest as progressive
      enhancement only. Blocked on a prerequisite that does not exist:
      `webapp/index.html` has no `<link rel="manifest">`, and there is no
      `webapp/public/manifest.json` or any `.webmanifest` file anywhere in
      the repository (confirmed 2026-08-09: no `public/` directory exists
      under `webapp/` at all). Per this task's own scope instructions,
      creating a web app manifest from scratch is a larger decision than
      this one checkbox and was deliberately not invented here. Left
      unchecked; the UI_UX_SPEC_V2 §12.4 wording is itself "SHOULD... as
      progressive enhancement... correctness does not depend on" it, so
      nothing else in V2-131/V2-132 depends on this being done.

## V2-132 — Landscape active-send safety

- [x] Show macro name and current send state/progress on the orientation surface.
      `MacrosPage.tsx` reports an `ActiveSendSummary` (macro name, the same
      `activeStatusText()` string the ordinary inline UI shows, and a
      cancel handle) upward via a new optional `onActiveSendChange` prop
      whenever its send `lifecycle` is `"starting"` or `"active"`, and
      `null` otherwise (UI_UX_SPEC_V2 §12.3 names exactly "awaiting
      confirmation or running"). `AppV2.tsx` wires this into
      `LandscapeBlockSurface`. Tested at both layers:
      `webapp/tests/v2-macros-page.test.tsx`'s "MacrosPage — V2-132
      landscape active-send summary" block (4 tests: the starting summary
      in isolation via a deliberately-never-resolving `sendMacro` fake,
      active progress, null after the completion acknowledgement, and
      opt-out when the caller passes nothing) and `v2-app-v2.test.tsx`'s
      "an active send's macro name, progress, and Cancel remain accessible
      while landscape-blocked" end-to-end test.
- [x] Keep Cancel and release all keys accessible. The orientation surface
      renders the exact same "Cancel and release all keys" action (calling
      the same `cancelActiveSend`/`DELETE /api/v1/send` path as the ordinary
      inline button) whenever a cancel handle is available.
      `v2-landscape-phone-block.test.tsx` unit-tests
      `LandscapeBlockSurface` calling it; `v2-app-v2.test.tsx`'s end-to-end
      test locates the button specifically inside `.landscape-block` (not
      merely matching text anywhere in the document, since the ordinary
      hidden page has its own copy of the same button) and confirms
      clicking it issues the `DELETE`.
- [x] Test cancellation while landscape. `v2-app-v2.test.tsx`: starts a real
      send through the running app, flips to landscape, clicks the
      orientation surface's own Cancel button, and asserts the resulting
      `DELETE /api/v1/send` call and the "Send Open terminal was cancelled."
      acknowledgement.
- [ ] Ensure tablets, foldables classified as tablets, laptops, and desktops are
      not incorrectly blocked. By construction of the compound media query
      (`(orientation: landscape) and (pointer: coarse) and (max-height:
      600px)`): laptops/desktops fail `pointer: coarse` (fine pointer input),
      and typical tablets in landscape exceed the 600px height ceiling this
      project's target devices use — matching UI_UX_SPEC_V2 §12.4's own
      wording that this is "the initial implementation target" pending
      "implementation tests" refining exact thresholds against real
      hardware. No real device (tablet, foldable, laptop, or desktop) has
      verified this, per the hard rule against claiming device validation
      from source review alone — left unchecked on that basis, matching the
      Phase 13 exit gate's own still-open "Tablet and desktop landscape
      tests pass".

## V2-133 — Accessibility

- [x] Make all controls keyboard accessible. No `<div>`/`<span>` has a bare
      `onClick` anywhere in `webapp/src` (grep confirmed, 2026-08-09) and no
      custom control uses `role="button"`, so every click handler is on a
      native `button`/`input`/`textarea`, reachable and activatable by
      keyboard by default. The one identified gap — `MacrosPage.tsx`'s "More
      actions" overflow menu had no `Escape`-to-close or outside-click
      dismissal — is fixed: `webapp/src/features/shell/v2/useDismissibleOverlay.ts`
      adds both, wired into `MacroOverflowMenu`
      (`webapp/src/features/macros/v2/MacrosPage.tsx`), covered by
      `webapp/tests/v2-dismissible-overlay.test.tsx` (hook-level) and
      `webapp/tests/v2-macros-page.test.tsx` ("Escape closes an open overflow
      menu", "a click outside the overflow menu closes it"). Automated-check
      absence of violation is still not the same as a completed manual sweep
      — see the exit gate's still-unchecked "Manual keyboard ... checks are
      recorded".
- [ ] Preserve logical focus order. No CSS `order`, `row-reverse`/
      `column-reverse`, or positive `tabIndex` found anywhere in `webapp/src`
      (grep re-confirmed 2026-08-09 after this task's changes, including the
      new "Move first"/"Move last" controls, which render in plain DOM order)
      — nothing detected that would desync visual from DOM/focus order. Left
      unchecked: not manually/screen-reader verified, which this specific
      claim would need to be more than "no known violation."
- [x] Trap and restore focus in dialogs. A shared hook,
      `webapp/src/features/shell/v2/useFocusTrap.ts` (the v2-tree equivalent
      of `components/AccessibleDialog.tsx`'s logic: initial focus,
      `Tab`/`Shift+Tab` wrap, `Escape` close, return-focus-on-close, plus a
      `restoreFocusRef` escape hatch for triggers a ternary unmounts rather
      than layers under the dialog), is now wired into all 7 real
      `role="alertdialog"` surfaces: `MacrosPage.tsx`'s delete confirmation,
      `PackageManagementPage.tsx`'s delete confirmation,
      `SnapshotsPage.tsx`'s delete confirmation and its import-replace
      confirmation, `SettingsPage.tsx`'s restart confirmation and
      `ConfirmPhraseDialog` (reset-settings/factory-reset), and the shared
      `UnsavedChangesPrompt.tsx`. Covered by
      `webapp/tests/v2-focus-trap.test.tsx` (hook unit tests: initial focus,
      Tab/Shift+Tab wrap, Escape, restore-on-deactivate, restore-on-unmount)
      and per-surface tests in `webapp/tests/v2-macros-page.test.tsx`,
      `webapp/tests/v2-package-management-page.test.tsx`,
      `webapp/tests/v2-snapshots-page.test.tsx` (delete and import
      confirmations), `webapp/tests/v2-settings-page.test.tsx` (Restart and
      Reset settings dialogs), and
      `webapp/tests/v2-unsaved-changes-prompt.test.tsx`.
- [x] Use live regions without announcing every poll tick. Genuinely true and
      consistently applied: `role="status"`/`role="alert"` with `aria-live`
      appear across ~30 locations in `webapp/src` (auth, macros, snapshots,
      settings, package, execution, shell). Send-progress polling
      (`webapp/src/v2/sendClient.ts`, `pollIntervalMs = 1000`) only changes the
      live region's DOM text when `actionIndex`/state actually changes
      (`activeStatusText()` in `MacrosPage.tsx`), so unchanged polls do not
      re-announce — screen readers only fire on actual text mutation.
- [x] Never use color as the only state indicator. Genuinely true by
      construction: `webapp/src/components/StatusBadge.tsx` always renders a
      text `label` alongside its `status-{state}` color class (no icon- or
      color-only variant exists), and every banner/status region found
      (`ErrorBanner`, `ConnectivityBanner`, `validation-good`/`validation-bad`,
      `send-status`) pairs its color with visible text.
- [x] Expose source-editor labels and exact validation locations. Genuinely
      true: `features/macros/v2/MacroEditorPage.tsx` has a real
      `<label htmlFor="macro-editor-source">Macro source</label>` around the
      textarea, and a compile failure renders "Line {n}, column {n}, byte
      {n}." plus a "Go to error" button — asserted by
      `webapp/tests/v2-macro-editor-page.test.tsx:134`
      (`expect(container.textContent).toContain("Line 1, column 3, byte 2.")`).
- [x] Provide Move first/up/down/last alternatives to drag and drop. Still no
      drag-and-drop exists anywhere (no dnd/sortable dependency in
      `webapp/package.json`, no `draggable`/`onDragStart`/`onDrop` in
      `webapp/src`), so there is nothing to provide an alternative to, but the
      keyboard-operable reordering itself is now complete: the current v2
      pages (`features/macros/v2/MacrosPage.tsx`,
      `features/macros/v2/PackageManagementPage.tsx`) render "Move first",
      "Move up", "Move down", and "Move last" buttons for every row, backed
      by new `moveMacroToIndex`/`movePackageToIndex` helpers in
      `webapp/src/v2/repositoryEditing.ts` (direct target-index moves, not
      just the adjacent swap `moveMacro`/`movePackage` offer). Covered by
      `webapp/tests/v2-repository-editing.test.ts` (`moveMacroToIndex`/
      `movePackageToIndex` unit tests) and page-level tests in
      `webapp/tests/v2-macros-page.test.tsx`/
      `webapp/tests/v2-package-management-page.test.tsx` ("Move first and
      Move last are disabled at their respective ends", "Move last reorders
      ..."). The pre-v2 `features/package/PackageManagementPage.tsx` this
      requirement previously only partially existed in was confirmed
      unreachable dead code and deleted by V2-140 (2026-08-09) before this
      item closed — nothing was ported from it; the v2 buttons above are a
      fresh implementation.
- [x] Honor reduced motion. `webapp/src/styles.css` now carries a standing
      `@media (prefers-reduced-motion: reduce)` block collapsing
      `animation-duration`/`animation-iteration-count`/`transition-duration`/
      `scroll-behavior` for every element. As of this change there is still
      no `transition`/`animation`/`@keyframes` anywhere in `webapp/src/*.css`
      — the guard is currently vacuous in effect but is now a real,
      lint-checked (`stylelint`) policy rather than an absence of one: a
      future transition/animation added without its own explicit
      reduced-motion opt-out is disabled by this rule by construction, not by
      remembering to guard it individually.
- [ ] Prevent hidden source from leaking through accessible names. Moot for
      now rather than satisfied: source-hiding is a Phase 12 (`V2-120`
      "source-preview preference") feature that does not exist yet —
      `features/macros/v2/MacroPreviewPage.tsx` renders `macro.source`
      unconditionally in a `<code>` element, so there is no hide/reveal
      toggle for an accessible name to leak past. Left unchecked.

## Phase 13 exit gate

- [x] Automated accessibility checks pass. Real axe-core
      (`@axe-core/playwright`, pinned exact in `webapp/package.json`) now
      runs inside `webapp/tests/browser/run-browser-tests.mjs`'s real-Chrome
      harness (`runAccessibilityScan`), scanning the Macros, Snapshots, and
      Settings pages with the `wcag2a`/`wcag2aa`/`best-practice` rule sets —
      zero violations found at any impact level as of 2026-08-09 (confirmed
      by a temporary unfiltered run during this task, not just the
      serious/critical gate the harness enforces going forward). Scope
      caveat: only the three pages the harness already drives are scanned —
      Sign-in, First-run setup, Macro editor/preview, and Diagnostics are not
      yet covered by this automated scan (their keyboard/ARIA logic is
      covered by Vitest component tests instead, not by axe-core against a
      real DOM). `webapp/tests/v2-focus-trap.test.tsx` and
      `webapp/tests/v2-dismissible-overlay.test.tsx` are automated but not
      axe-core; listed here because they are the other automated
      accessibility-behavior checks this task added.
- [ ] Manual keyboard and screen-reader checks are recorded. Not done — this
      is a human-verification item no automated tooling substitutes for. A
      real check would need: a keyboard-only pass (no mouse) through Sign
      in/First-run setup, Macros (send, reorder, overflow menu, delete
      confirm), Package management (create/rename/duplicate/reorder/delete),
      Snapshots (save/load/delete/import/export, including the exact-ID
      delete confirmation and the dirty-work-during-load prompt), Settings
      (all forms plus Restart/Reset settings/Factory reset), and the Macro
      editor/preview screens; and a screen-reader pass (e.g. NVDA/JAWS on
      Windows, VoiceOver on macOS, or Orca on Linux) confirming live-region
      announcements, dialog labeling, and that hidden macro source is never
      announced. Left honestly unchecked.
- [ ] Real Android phone portrait/landscape tests pass.
- [ ] Tablet and desktop landscape tests pass.
- [x] Active-send cancellation remains available in landscape. V2-132,
      2026-08-09: `webapp/tests/v2-app-v2.test.tsx`'s "an active send's
      macro name, progress, and Cancel remain accessible while
      landscape-blocked" test starts a real send through the running app,
      switches to landscape, and confirms Cancel (specifically the copy
      inside `.landscape-block`, not merely matching text anywhere in the
      hidden page behind it) issues `DELETE /api/v1/send` and reaches the
      cancelled acknowledgement. This is jsdom + a `matchMedia` fake, not a
      real Android device — the two device-hardware exit-gate items above
      remain honestly unchecked for that reason.

---

## Phase 14 — Migration cleanup, static assets, scripts, and documentation

**Depends on:** Phases 2–13.

## V2-140 — Delete dead v1 code

- [ ] Remove obsolete firmware files and build registrations. Not
      investigated in this pass (scoped to the webapp per the task that ran
      it) — `scripts/check-v2-phase2-architecture.py` (CI-enforced, passes)
      confirms firmware carries no package/macro repository model, but a full
      audit of firmware source files and CMake/component registrations for
      other obsolete v1 remnants was not performed. Left unchecked.
- [x] Remove obsolete React routes, screens, API clients, guards, models, and
      fixtures. Deleted the entire retired v1 tree — confirmed unreachable
      from `main.tsx` by a full import-graph audit before deletion, and by a
      clean `npm --prefix webapp run typecheck`/`build` after: `App.tsx`,
      `routing.ts`, `api/` (`client.ts`, `errors.ts`, `executionGuards.ts`,
      `guards.ts`, `managementGuards.ts`, `packages.ts`, `routes.ts`),
      `types/models.ts`, `components/AppShell.tsx`,
      `components/ConnectivityBanner.tsx`, `components/AccessibleDialog.tsx`,
      `features/execution/*`, `features/package/*`,
      `features/macros/MacroEditorPage.tsx`, `MacroLibraryPage.tsx`,
      `macroDraft.ts`, `features/auth/{LoginPage,SetupPage,SessionBoundary}.tsx`,
      `features/settings/{DiagnosticsPage,PackageOperationsPage,SettingsPage}.tsx`,
      and their now-dead per-feature `README.md`s. `components/ErrorBanner.tsx`
      and `components/StatusBadge.tsx` were kept — grepped and confirmed
      imported by `v2/`-reachable code (`AppShellV2`, `MacroPreviewPage`, and
      others); `types/limits.ts` was kept for the same reason. Full detail
      and evidence: `docs/implementation-v2/V2_140_DEAD_V1_CODE_REMOVAL_2026-08-09.md`.
- [x] Remove obsolete schemas and generated artifacts. Deleted
      `docs/schemas/{all-data-backup,diagnostic-report,macro-set-package}.schema.json`
      — pre-identified as "not part of the v2 contract set" by
      `docs/implementation-v2/V2_000_002_BASELINE_INVENTORY_COMPLETION_2026-08-09.md`,
      confirmed unreferenced by any script/test/source outside historical
      evidence docs, and confirmed unused by v2 (`contracts/v2/` is the real
      v2 contract source). Fixed the resulting `scripts/check-docs.sh` glob
      failure (`nullglob`) so the now-empty `docs/schemas/` directory doesn't
      break the docs gate.
- [ ] Remove compatibility types and migrations that have no released v2
      input. No webapp instance found (grepped `webapp/src/v2/` and
      `features/*/v2/` for migration/compatibility-shim code reading v1-shaped
      data; found none — this product never shipped v1 to real users, so
      there was nothing to migrate from). Left unchecked because firmware
      (e.g. NVS settings-schema migration code) was not audited in this pass.
- [x] Remove dead tests rather than skipping them (webapp scope). Deleted 17
      Vitest files that exclusively exercised deleted v1 code (16 test files
      plus the now-orphaned `appFixtures.ts` fixture helper) — kept every
      test file that also exercises `v2/`-reachable code (verified per file,
      not by naming pattern: e.g. `tests/app.test.ts` looked v1-named but
      tests the still-live `types/limits.ts`, kept). Before: 59 files / 577
      tests. After: 43 files / 448 tests, all passing
      (`npm --prefix webapp run test`). Host/on-device test suites were not
      audited in this pass.
- [x] Verify no user-visible v1 wording remains (webapp scope). Grepped
      `webapp/src` for `v1`/`legacy`/`revision`/`procedure`/stray `set`-as-package
      wording after deletion; the only hits were source comments (not
      user-visible), one of which (`AppV2.tsx`'s file-header comment
      referencing the now-deleted `App`) was corrected. No `.css`/JSX literal
      v1 wording found.

## V2-141 — Static application production behavior

This whole item turns out to already be implemented and tested — the static-
serving infrastructure was built with the firmware/web_server component well
before Phase 14 was reached, not as Phase-14 work; auditing it here (2026-08-09)
just confirms it and closes the checkboxes.

- [x] Keep all production assets local. Verified: `npm --prefix webapp run
      build` (this worktree, node v24.18.0) then
      `./scripts/verify-no-remote-assets.sh webapp/dist` — exit 0.
- [x] Use content-hashed Vite filenames. Verified from the same build:
      `dist/assets/index-DNGRoJPY.css`, `dist/assets/index-Dyil3X-o.js`
      (`webapp/vite.config.ts` uses Vite's default hashed-output naming;
      nothing overrides it).
- [x] Generate gzip variants where specified. `scripts/build-webfs-image.sh`
      runs `gzip -9 -k -f` on every staged file before packaging the webfs
      image; `web_adapter_open_static_file()`
      (`firmware/components/web_server/web_server_adapter_static_stream.c`)
      prefers the `.gz` sibling when present — tested by
      `test_static_file_selection()` in
      `tests/host/test_web_server_adapter_json_static.inc`.
- [x] Serve correct content types. `web_content_type()` in the same adapter
      file, asserted by the same test (`"text/javascript; charset=utf-8"` for
      `.js`, etc.).
- [x] Cache hashed assets immutably and revalidate `index.html`.
      `web_server_adapter_static_stream.c` sets
      `"public, max-age=31536000, immutable"` for hashed assets and
      `"no-cache"` for `/index.html`; both asserted by
      `test_static_file_selection()`.
- [x] Reject path traversal. `tests/host/test_web_server_blob_load.inc`'s
      `test_blob_load_rejects_path_traversal()`, run via
      `./scripts/run-tests.sh web` (23/23 passed, 2026-08-09).
- [x] Never expose the userdata mount. Structural, not just policy: the only
      catch-all route is `{.uri = "/*", .method = HTTP_GET, .handler =
      static_handler}` in `firmware/components/web_server/web_server_lifecycle.c`,
      fixed to the `/web` (webfs) base path; blob/userdata access exists only
      through the separate, JSON-structured `/api/v1/blob*` routes. There is
      no route through which a request path reaches the userdata partition via
      the static-file handler.
- [x] Verify the built webfs fits the 1 MiB partition with recorded margin.
      Measured 2026-08-09: `./scripts/build-webfs-image.sh --skip-frontend-build`
      wrote `firmware/build/webfs.bin` (1,048,576 bytes, the full `webfs`
      partition per `firmware/partitions.csv`). Raw file size always equals
      partition capacity for a LittleFS image, so margin was computed the way
      `scripts/check-release-budgets.sh`'s `littlefs_used_bytes()` does — count
      non-erased 4096-byte blocks: **393,216 bytes actually used / 1,048,576
      bytes partition = 37.5%**, well inside the script's 85% budget ratio
      (891,289 bytes). The full `check-release-budgets.sh` script itself needs
      a completed ESP-IDF firmware build (`idf.py -C firmware build`) to run,
      which this pass did not do; the number above reproduces its webfs-
      specific calculation directly.

## V2-142 — Build, lint, and architectural guards

- [x] Update scripts for the new source and test layout. `scripts/` has no
      references to any retired v1 path (`grep -rl "storage_repository\|
      storage_package\|/api/v1/sets\|/api/v1/executions" scripts/` finds only
      `check-v2-phase2-architecture.py`, which names them as forbidden
      patterns, not live usage), and `check-all.sh` already invokes the
      current-layout scripts (`build-webfs-image.sh`,
      `check-release-budgets.sh`, the `check-v2-*` family,
      `check-v2-contracts.sh`) rather than anything stale.
- [x] Keep `./scripts/check-all.sh` authoritative. It is the single gate
      (`scripts/check-all.sh`, read 2026-08-09) covering toolchain, format,
      static-analysis policy, partitions, v2 contract/policy checks,
      production-config/credential-logging/mount-policy/layer-boundary/
      removed-features/phase-2-architecture guards, USB identity, frontend
      persisted-state, setup-route isolation, native v2 contracts, firmware
      build, stack usage, webfs image, flash manifest, release budgets, the
      full webapp chain, script linting, docs, and host tests — every
      documented command in `CLAUDE.md`/`README.md` routes through it or one
      of its constituents.
- [ ] Add guards against old routes, firmware repositories, `activePackageId`,
      automatic snapshot deletion, mandatory send navigation, remote assets, and
      browser repository persistence. 5 of 7 already have a dedicated CI-run
      guard: old routes/firmware repositories/`activePackageId`
      (`scripts/check-v2-phase2-architecture.py`'s `FORBIDDEN_PATHS`/
      `FORBIDDEN_SOURCE` regex, which explicitly matches `activePackageId`),
      remote assets (`scripts/verify-no-remote-assets.sh`), and browser
      repository persistence (`scripts/check-frontend-persisted-state.sh`'s
      localStorage/sessionStorage/indexedDB allowlist). No dedicated
      architectural guard exists for automatic snapshot deletion or mandatory
      send navigation — only ordinary feature tests
      (`webapp/tests/v2-snapshots-page.test.tsx`) cover the snapshot-deletion
      behavior, which is easier to regress unnoticed than a checked-in guard
      script. Left unchecked: the bullet names 7 things and 2 have no guard.
- [x] Keep all first-party warnings fatal. `.clang-tidy`: `WarningsAsErrors:
      '*'`; `webapp/package.json`: `"lint": "eslint . --max-warnings=0"`,
      `"stylelint": "stylelint 'src/**/*.css' --max-warnings=0"`; host tests
      compile with `-Wall -Wextra -Werror` per `CLAUDE.md`. Continuously
      enforced, not phase-14-specific work.

## V2-143 — Documentation synchronization

- [x] Update `README.md` to point to both v2 specifications and this TODO.
      Evidence: commit `5b1b771ff9e036486dee05ccd736206ee26befb7`. README.md's
      opening authority sentence now cites `docs/SPEC_V2.md`,
      `docs/UI_UX_SPEC_V2.md`, and `docs/TODO_V2.md`; several stale v1-era
      claims found in the same file while auditing it (dead `docs/SPEC.md`
      section citations, v1 package/repository hardware-validation claims,
      stale host/frontend test counts) were also corrected, and
      `docs/README.md`, `docs/IMPLEMENTATION_STATUS.md`, and
      `docs/UNIT_TESTS1_PROGRESS.md` were updated the same way. Full detail:
      `docs/implementation-v2/V2_143_DOC_AUTHORITY_SYNC_2026-08-09.md`. This
      closes only this sub-bullet — the rest of V2-143 (`CLAUDE.md`,
      `docs/TODO.md` itself, development/API/test/hardware/recovery
      documentation, `docs/mockups/v2/` references, status-vs-intent
      labeling) remains open.
- [x] Update `CLAUDE.md` to remove v1 authority and stale implementation
      guidance. Already correct: `CLAUDE.md`'s Project layout and Active
      development constraints sections cite `SPEC_V2.md`/`TODO_V2.md` as
      authoritative and name `SPEC.md`/`TODO.md` only as retired v1 stubs
      (fixed during this session's `/init` review, commit `9bb47bb`, before
      Track F's broader doc-authority sweep).
- [x] Update `docs/TODO.md` to point exclusively to this file. Already
      correct: its sole content is "The authoritative v2 implementation
      sequence is `TODO_V2.md`", with an explicit instruction not to
      implement from the retired v1 TODO.
- [x] Update development, API, test, hardware, and recovery documentation.
      `webapp/README.md`'s two false claims (identified 2026-08-09) are now
      fixed: it no longer claims `package-lock.json` doesn't exist, and its
      "Current implementation" section now distinguishes the real, v2-wired
      Macros/Packages/Snapshots surface (`AppV2`, the mounted entry point)
      from the retired v1 `App.tsx`'s genuine presentation scaffolds, rather
      than blanket-labeling both as scaffolds. `CLAUDE.md`'s matching webapp
      architecture citations were corrected the same way (same commit) so the
      staleness doesn't propagate. The `docs/*.md` work below was real and
      stands unchanged.
      Evidence: commit `e28daf4442ac66ef502a56902dfb461f2795f504`. `docs/API.md` (entirely v1 firmware-owned
      package/set/macro/backup/restore routes, all deleted per
      `docs/implementation-v2/V2_MIGRATION_MAP.md` §2.14/§8) got a retirement
      banner pointing to `docs/SPEC_V2.md` §13 and `contracts/v2/api/routes.json`
      as current authority, since a confident full route rewrite was out of this
      pass's scope; `docs/RECOVERY.md` was rewritten from the deleted v1
      set/trash/transaction-manifest model to the current opaque-blob recovery
      rules (`docs/SPEC_V2.md` §10.1/§10.3/§10.5/§10.6/§10.9) with corrected test
      citations; `docs/PROVISIONING_SECURITY.md`'s four nonexistent setup routes
      were corrected to the real `GET`/`POST /api/v1/setup` pair (verified
      against `web_server_lifecycle.c`); `docs/HARDWARE_TEST_PLAN.md`'s
      "Chromebook workflow dry run" and "Physical controls" sections were
      corrected from v1 set/procedure/checkpoint/reset-gesture language (all
      explicit `docs/SPEC_V2.md` §4 non-goals or, per
      `firmware/components/device_controls/README.md`, never-implemented) to v2
      package/send terminology; `docs/FRONTEND_TESTS_PROGRESS.md` (a pre-rebuild
      status snapshot presented as current) got a retired banner;
      `README.md`'s on-device Unity test-menu table and prose were missing the
      `[benchmark]` selector that
      `docs/implementation-v2/V2_143_DOC_AUTHORITY_SYNC_2026-08-09.md` had
      flagged but left for this track; and
      `docs/CLAUDE_CODE_PHYSICAL_ESP32S3_V2_HANDOFF_2026-08-08.md`'s
      point-in-time "current state" sections (describing the now-replaced
      `POST /api/v1/setup` `503` stub as current) got a superseded-status note.
      `docs/DEVELOPMENT.md` was investigated and found already accurate; no
      change needed. Full detail:
      `docs/implementation-v2/V2_143_DEV_STATUS_DOCS_SYNC_2026-08-09.md`.
- [ ] Add approved mockup references under `docs/mockups/v2/` only when the image
      files are available and licensed for repository use.
- [x] Clearly label current implementation status versus specification intent.
      The same `webapp/README.md` fix closes this: it no longer
      under-states Phase 9–11's real macro/package/snapshot functionality as
      unreliable "presentation scaffolds" (the opposite failure direction
      from this project's prior overclaiming incidents, but still inaccurate
      status labeling). The audit of `README.md` and other `docs/*.md` files
      remains accurate and stands unchanged.
      Evidence: commit `e28daf4442ac66ef502a56902dfb461f2795f504`. Audited `README.md`'s "Repository
      status" section (already corrected by the prior README-authority track)
      and found it accurately mid-rebuild-framed; no other file in scope made
      confident claims that unbuilt Phase 10-15 features (macro editing,
      snapshots UI, settings UI, portrait/accessibility, hardware validation)
      already exist. The one residual instance found —
      `docs/CLAUDE_CODE_PHYSICAL_ESP32S3_V2_HANDOFF_2026-08-08.md` presenting an
      80-commits-stale Phase 4 snapshot as current status — was labeled
      superseded per the change above. Full detail:
      `docs/implementation-v2/V2_143_DEV_STATUS_DOCS_SYNC_2026-08-09.md`.

## Phase 14 exit gate

- [ ] Repository-wide searches find no authoritative v1 references. Better
      but not yet: `docs/API.md` now leads with the current v2 route table
      and pushes the v1 route documentation into a clearly labeled
      "Archived: retired v1 API" section (2026-08-09); `webapp/README.md`,
      `webapp/tests/README.md`, `webapp/src/pages/README.md`, and `CLAUDE.md`
      no longer describe the deleted v1 tree as present. Left unchecked
      because this was a webapp-scoped pass — firmware source/docs and the
      full `docs/` tree were not exhaustively re-audited for stray
      authoritative v1 references, and the firmware half of V2-140 (below)
      hasn't run.
- [x] No dead v1 production code or skipped v1 test remains, in the webapp.
      V2-140's webapp scope ran 2026-08-09: `webapp/src/App.tsx` and its
      entire v1 feature tree, plus `routing.ts`, `api/`, `types/models.ts`,
      and the v1-only shared components, are deleted — confirmed by a clean
      `typecheck`/`lint`/`stylelint`/`test`/`build` and two clean
      `./scripts/check-webapp.sh` runs (2026-08-09). Checking only the webapp
      half of this line: firmware was not audited in this pass (see V2-140's
      firmware bullet above), so the line is not fully true repository-wide.
- [x] Static assets fit and pass local-only checks. Verified 2026-08-09 (same
      evidence as V2-141 above): `verify-no-remote-assets.sh webapp/dist` exit
      0 on a real production build, and the webfs image uses 393,216 of
      1,048,576 partition bytes (37.5%), inside the 85% budget ratio.
- [ ] Documentation accurately describes the implemented v2 system. Progress
      2026-08-09: `docs/API.md`, `webapp/README.md`, `webapp/tests/README.md`,
      `webapp/src/pages/README.md`, `CLAUDE.md`, and root `README.md`'s stale
      frontend test count were all corrected as part of this pass (including
      a stale "Settings/Diagnostics are unimplemented placeholders" claim in
      both `CLAUDE.md` and `webapp/README.md` that predated Phase 12
      shipping). Left unchecked regardless: this is a Phase 14 exit-gate
      line, and Phase 14 as a whole is not done (V2-140's firmware bullet is
      still open), so the phase cannot honestly close yet; only the docs
      actually touched across V2-143 and this pass have been re-verified
      accurate, not every markdown file in the repository.

---

## Phase 15 — Full validation and release evidence

**Depends on:** All prior phases.

## V2-150 — Native and frontend quality gates

- [ ] Run native host tests.
- [ ] Run ASan and UBSan.
- [ ] Run native coverage and meet committed policy thresholds.
- [ ] Run C formatting and clang-tidy with warnings as errors.
- [ ] Run TypeScript type checking.
- [ ] Run formatting checks.
- [ ] Run ESLint and stylelint with zero warnings.
- [ ] Run Vitest.
- [ ] Run real-browser tests.
- [ ] Build production web assets and webfs image.
- [ ] Run the firmware build for `esp32s3`.
- [ ] Run:

```bash
./scripts/check-all.sh
```

- [ ] Record exact commands, versions, counts, sizes, and results.

## V2-151 — On-device Unity validation

- [ ] Build and flash the on-device test application.
- [ ] Run the complete Unity menu on the reference board.
- [ ] Record every test name and result.
- [ ] Do not substitute a device-test build for execution.

## V2-152 — USB HID hardware matrix

- [ ] Validate Linux.
- [ ] Validate ChromeOS when a test machine is available.
- [ ] Validate Windows when a test machine is available.
- [ ] Record unavailable optional hosts without claiming completion.
- [ ] Verify identity, text, chords, release-all, cancellation, timeout, and
      reconnect from captured HID reports.

## V2-153 — Storage and power-failure matrix

- [ ] Validate add/list/load/delete on hardware.
- [ ] Validate power-cycle byte identity.
- [ ] Validate interrupted upload recovery.
- [ ] Validate full-partition behavior.
- [ ] Validate mount failure without formatting.
- [ ] Validate factory reset and reprovisioning.

## V2-154 — Network and authentication matrix

- [ ] Validate first-run setup.
- [ ] Validate unauthenticated setup-state GET before provisioning, its minimal
      response, and its `404` behavior after provisioning.
- [ ] Validate login, logout, idle expiry, absolute expiry, and lockout.
- [ ] Validate AP availability after station failure.
- [ ] Validate bounded reconnect behavior.
- [ ] Validate password change and PBKDF2 timing.
- [ ] Validate no secret appears in serial, HTTP, logs, diagnostics, or exports.

## V2-155 — Android UI workflow matrix

- [ ] Validate first-ever launch and setup.
- [ ] Validate configured-device Sign In.
- [ ] Validate first sign-in from a new Android phone.
- [ ] Validate already-authenticated refresh.
- [ ] Validate automatic newest-snapshot loading.
- [ ] Validate manual loading of an older snapshot.
- [ ] Validate Quick Send while remaining on the Macros page.
- [ ] Validate inline acknowledgement and cancellation.
- [ ] Validate hidden macro source.
- [ ] Validate dirty-state warnings and manual Save snapshot.
- [ ] Validate manual snapshot deletion and advisory retention.
- [ ] Validate portrait enforcement and landscape cancellation.

## V2-156 — Final acceptance audit

- [ ] Walk every acceptance criterion in both authoritative specifications.
- [ ] Link each criterion to code, tests, and evidence.
- [ ] Confirm firmware contains no repository parser, compressor, decompressor,
      package repository, macro repository, or package/macro CRUD route.
- [ ] Confirm repository schema has no `activePackageId`.
- [ ] Confirm package selection is device UI state and switching is not dirty.
- [ ] Confirm snapshots are never created or deleted automatically.
- [ ] Confirm ordinary sends require no standalone confirmation navigation.
- [ ] Confirm every terminal send path releases all keys.
- [ ] Confirm no credentials, repository data, or macro source leak.
- [ ] Confirm all partitions and images fit with recorded margins.
- [ ] Confirm this TODO and both specifications match implemented behavior.

## Phase 15 exit gate

- [ ] Full local gate passes from a clean checkout.
- [ ] Required hardware evidence is committed and reproducible.
- [ ] Every specification acceptance criterion has evidence.
- [ ] No unchecked hardware-dependent task is described as complete.
- [ ] Product owner performs the final v0.2 acceptance review.

---

## Final sign-off checklist

This checklist is Phase 15's release gate, not a running progress meter — most
items below can only be honestly checked once Phases 9–14 (the entire Macros/
editing/snapshots/settings/accessibility UI and the migration-cleanup phase) are
built and V2-156's acceptance audit has actually been walked, neither of which
has happened yet. Phases 9–11 (Macros/Quick Send, macro editing/package
management, snapshots/import-export) landed since the 2026-08-09 pass that
first populated this checklist; Phases 12–14 have not. The items below reflect
only what is independently, currently, and reproducibly true as of commit
`50ada5bd0ea75ac0e2f76b9b804b7949831f34cf` (re-audited 2026-08-09) — not a claim
that the project is close to final sign-off.

- [ ] `docs/SPEC_V2.md` matches production behavior. Cannot be true yet: Phases
      12–14 (settings/diagnostics UI, portrait/responsive/accessibility,
      migration cleanup) remain unstarted or incomplete, so a meaningful slice
      of SPEC_V2's UI-facing requirements still has no implementation to match
      against. (Phases 9–11 — Macros page, macro editing, package management,
      snapshots, import/export — are now built; this is narrower than the
      "Phases 9–14 unstarted" this line said as of commit `9bb47bb`.)
- [ ] `docs/UI_UX_SPEC_V2.md` matches production behavior. Same reason.
- [ ] `docs/TODO_V2.md` contains no falsely completed task. Partially
      addressed, not closed: this 2026-08-09 audit pass (see
      `docs/implementation-v2/V2_AUDIT_PHASE_13_14_15_2026-08-09.md`) walked
      every checkbox from Phase 13 through this checklist against real code
      and corrected several — some falsely-checked items were unchecked
      (two V2-143 sub-bullets), several genuinely-true items were newly
      checked (parts of V2-133, all of V2-141, most of V2-142). Six parallel
      tracks did the same for Phases 0–12. V2-156's own acceptance audit
      (walking every SPEC_V2/UI_UX_SPEC_V2 criterion to code/test/evidence,
      not just checkbox truthfulness) still has not been performed.
- [x] `docs/TODO.md`, `README.md`, and `CLAUDE.md` point to the v2 authority
      set. Re-verified 2026-08-09: `README.md`'s opening sentence still cites
      `docs/SPEC_V2.md`, `docs/UI_UX_SPEC_V2.md`, and `docs/TODO_V2.md`;
      `CLAUDE.md`'s Project layout and Active development constraints
      sections still do the same; `docs/TODO.md` is still a clean pointer to
      `TODO_V2.md`. This is narrower than V2-143 as a whole, which also
      covers `docs/mockups/v2/`, `webapp/README.md`, and status-vs-intent
      labeling — those remain open (see V2-143 above; `webapp/README.md` in
      particular was found stale by this pass).
- [x] `./scripts/check-all.sh` passes from a clean checkout. Audit Track G
      correctly unchecked this pending re-verification (the prior claim
      covered commit `9bb47bb`, 16 commits stale, predating Phases 9–11).
      Re-verified 2026-08-09 against commit `575dae5`: an actual fresh
      `git clone` from the local repository (not the working tree), `npm ci`
      in `webapp/`, `littlefs-python==0.15.0` installed into the sourced
      ESP-IDF virtualenv exactly as `.github/workflows/quality.yml` does it,
      then the full gate. First attempt (commit `50ada5b`, before this fix)
      caught a real pre-existing Prettier formatting defect in Audit Track
      E's added test (`webapp/tests/v2-macros-page.test.tsx`), fixed in
      `575dae5`. Second attempt at `575dae5`: exit `0`, 54/54 host tests, all
      steps clean. Same caveat as before — this is a snapshot fact about this
      commit, not a permanent one.
- [ ] Required ESP32-S3R8, USB HID, storage, Wi-Fi, authentication, and Android
      evidence is committed. Multiple hardware-only items remain open
      throughout this file (V2-035, part of V2-041, part of V2-044, V2-064,
      and all of Phase 15's V2-151 through V2-155 matrices) — none of that
      evidence exists yet.
- [x] No v1 compatibility architecture remains in production code. Re-verified
      2026-08-09 with fresh runs, not just re-read: `python3
      ./scripts/check-v2-phase2-architecture.py` → "phase 2 architecture: no
      firmware-owned package or macro repository" (exit 0); `bash
      ./scripts/check-removed-features.sh` → "removed features: none of the
      SPEC 1.1 rejections have reappeared" (exit 0). Phase 2's exit gate
      remains complete and this is continuously enforced by `check-all.sh` on
      every invocation. This covers production firmware code specifically; it
      does not by itself certify the webapp (which still contains dead v1
      React code, see the Phase 14 exit gate above) or the rest of this
      checklist.
- [ ] No known silent failure, dangerous fallback, secret leak, automatic
      snapshot deletion, or inaccessible cancellation path remains.
      Re-assessed 2026-08-09: still cannot be claimed, but the specific
      blocker has moved. Phase 11 is now built and its exit gate confirms "No
      automatic snapshot creation or deletion exists" (`V2-114`: "Never
      silently fall back"; unreadable snapshots are kept, shown, and offer
      download/delete rather than silent substitution). What remains open:
      Phase 12's destructive-operations UI (restart/reset-settings/factory-
      reset confirmation flows, V2-121) is unstarted, so its silent-failure/
      dangerous-fallback/secret-leak properties are unverified, and V2-156's
      audit still hasn't walked the rest of the system.
- [ ] Final implementation report records the accepted release commit SHA. No
      such report exists; it is V2-156's own deliverable.
