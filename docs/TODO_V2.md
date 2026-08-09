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

- [ ] Record the starting `master` commit SHA.
- [ ] Record toolchain versions and confirm ESP-IDF `v5.5.5`, target `esp32s3`,
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

- [ ] Inventory production firmware components under `firmware/`.
- [ ] Inventory React routes, feature modules, API clients, models, validators,
      and persistent-browser-storage use under `webapp/`.
- [ ] Inventory native host tests under `tests/host/`.
- [ ] Inventory browser and Vitest coverage under `webapp/tests/`.
- [ ] Inventory on-device Unity coverage under `firmware/test_app/`.
- [ ] Inventory hardware scripts under `tests/hardware/`.
- [ ] Inventory schemas, generated files, static assets, scripts, CI workflows,
      and documentation references.

## V2-002 — Create the explicit migration map

- [ ] Create `docs/implementation-v2/V2_MIGRATION_MAP.md`.
- [ ] Classify every production subsystem as **retain**, **adapt**, **rewrite**, or
      **delete**.
- [ ] Classify every test suite and fixture the same way.
- [ ] Identify all firmware-owned package and macro state.
- [ ] Identify all package/macro CRUD routes and validation routes.
- [ ] Identify all revision, ETag, optimistic-concurrency, restore, import,
      replace, and repository-index behavior from v1.
- [ ] Identify all frontend localStorage, sessionStorage, IndexedDB, Cache
      Storage, and service-worker repository persistence.
- [ ] Identify all standalone send-confirmation, send-progress, and result routes
      that conflict with the v2 primary workflow.
- [ ] Identify all documentation and scripts that still point to v1 authority.
- [ ] For every retained subsystem, name the v2 requirement and tests proving it
      remains valid.

## Phase 0 exit gate

- [ ] Baseline evidence is committed.
- [ ] Migration map covers production code, tests, scripts, schemas, and docs.
- [ ] Every known v1 feature has an explicit delete/adapt decision.
- [ ] No production behavior has changed in this phase.

---

## Phase 1 — Shared contracts, constants, and test fixtures

**Depends on:** Phase 0.

**Goal:** Define exact v2 contracts before firmware and UI implementation diverge.

## V2-010 — Repository model and schema contracts

- [ ] Define the strict TypeScript repository types:

```json
{
  "format": "esp32-macro-keyboard-repository",
  "schemaVersion": 1,
  "packages": []
}
```

- [ ] Ensure the root has exactly `format`, `schemaVersion`, and `packages`.
- [ ] Prohibit `activePackageId` and all unknown fields.
- [ ] Define package and macro types exactly as specified.
- [ ] Enforce canonical lowercase UUID v4 identifiers.
- [ ] Enforce repository-wide package-ID and macro-ID uniqueness.
- [ ] Add valid, boundary, malformed, duplicate-ID, prototype-bearing, sparse
      array, non-finite-number, and unknown-field fixtures.
- [ ] Add a checked-in canonical example repository and canonical compact JSON.

## V2-011 — API contract models

- [ ] Define shared request/response examples for every `/api/v1` route.
- [ ] Define the unprovisioned-only `GET /api/v1/setup` response with exactly
      `provisioned: false` and `deviceName`.
- [ ] Define the standard JSON error envelope with stable `code`, human-readable
      `message`, and optional `field`.
- [ ] Define exact setup, session, status, limits, settings, password-change,
      restart, reset-settings, factory-reset, diagnostics, blob, and send objects.
- [ ] Define strict unknown-field rejection for JSON requests.
- [ ] Define content types, maximum body sizes, success status codes, and error
      status codes per route.
- [ ] Add TypeScript types and C-side parsing/serialization contracts that match
      the same checked-in examples.

## V2-012 — Centralized limits

- [ ] Create one authoritative firmware limits module.
- [ ] Mirror client-relevant limits in generated or verified TypeScript types.
- [ ] Include at least:
  - [ ] package name: 64 UTF-8 bytes;
  - [ ] macro name: 64 UTF-8 bytes;
  - [ ] macro source: 4096 UTF-8 bytes;
  - [ ] compiled actions: 4096;
  - [ ] key press: 0–10,000 ms;
  - [ ] inter-key delay: 0–10,000 ms;
  - [ ] directive delay: 1–10,000 ms;
  - [ ] estimated macro duration: 300,000 ms;
  - [ ] absolute executor deadline: 310,000 ms;
  - [ ] JSON request body: 8192 bytes;
  - [ ] candidate repository blob maximum: 131,072 bytes;
  - [ ] active sessions: 8;
  - [ ] session idle lifetime: 86,400 seconds;
  - [ ] session absolute lifetime: 604,800 seconds;
  - [ ] serial-confirmation timeout: 60 seconds;
  - [ ] administrator password: 12–128 UTF-8 bytes.
- [ ] Make `GET /api/v1/limits` derive from the same constants.
- [ ] Add boundary tests for every numeric and byte-count limit.

## V2-013 — Shared macro-language conformance corpus

- [ ] Retain or replace the current corpus with one format consumed by C and
      TypeScript tests.
- [ ] Cover printable ASCII, line endings, tabs, directives, chords, escaping,
      delays, malformed braces, Unicode rejection, duplicate modifiers, unknown
      names, source positions, action limits, and duration limits.
- [ ] Define expected compiled actions for valid cases.
- [ ] Define expected code, byte offset, line, column, and message class for
      invalid cases.
- [ ] Make parser drift fail both local gates and CI.

## V2-014 — Device settings schema

- [ ] Define a versioned NVS settings record.
- [ ] Include device name, AP settings, optional station settings, password
      verifier metadata, physical-confirmation policy, provisioning state,
      credential version, optional next-blob counter, and UI preferences.
- [ ] Include:
  - [ ] `sendMode`, default `quick`;
  - [ ] `snapshotRetentionTarget`, default `5`, advisory only;
  - [ ] `showMacroSourcePreviews`, default `false`;
  - [ ] `lastSelectedPackageId`, default `null`.
- [ ] Validate `lastSelectedPackageId` only as an opaque canonical UUID or null.
- [ ] Specify reset-settings and factory-reset defaults exactly.
- [ ] Add wrong-version, wrong-length, truncated-record, invalid-enum, and invalid
      UUID tests.

## Phase 1 exit gate

- [ ] Contract fixtures are checked in and consumed by tests.
- [ ] C and TypeScript agree on every shared boundary and enum.
- [ ] No production route or repository serializer still depends on a v1 shape.
- [ ] Narrow contract test suites pass with zero warnings.

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

- [ ] Add a blob, power-cycle the board, and load byte-identical data.
- [ ] Add multiple blobs and verify numeric ordering.
- [ ] Delete one blob and verify every other blob is byte-identical.
- [ ] Interrupt an upload and verify no partial final file appears.
- [ ] Reboot and verify temporary cleanup.
- [ ] Fill storage and verify `507` leaves all final blobs unchanged.
- [ ] Simulate or induce mount failure and verify no formatting occurs.

## Phase 3 exit gate

- [ ] Host storage tests, image tests, and static analysis pass.
- [ ] Required hardware evidence is committed.
- [ ] Firmware remains completely unaware of repository contents.

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

## V2-041 — Password verifier and PBKDF2 benchmark

- [x] Use PBKDF2-HMAC-SHA-256 with a random per-password salt.
- [x] Store verifier version, salt, and iteration count.
- [x] Use constant-time verifier comparison.
- [x] Benchmark candidate iteration counts on the reference ESP32-S3R8.
- [ ] Record median, percentile, and worst observed time under representative
      memory and Wi-Fi load.
- [x] Select one exact iteration count yielding approximately 250–500 ms.
- [x] Freeze the selected constant in code and tests.
- [ ] Confirm the derivation does not trip watchdogs or starve critical tasks.
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
      Verified via the full `./scripts/run-tests.sh` / `check-all.sh` suite
      (50/50), including the `auth`, `startup`, `wifi`, and `web` labels.
- [ ] PBKDF2 hardware benchmark and selected iteration count are committed.
      Blocked on V2-041's two remaining hardware-only items (real-device
      timing percentiles, watchdog/starvation confirmation).
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
      policy-table level; no live HTTP-socket-level test exists (no
      `esp_http_server` fake exists anywhere in this codebase).

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
- [ ] Test the complete unprovisioned/provisioned route-access matrix.
      Guaranteed structurally by `check-setup-route-isolation.sh`, not by an
      executed test exercising every route/method pair in both states.

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
      routes (session, restart, settings, change-password, reset-settings,
      factory-reset, diagnostics, setup-conflict) are still tested only via
      direct function calls with narrow test doubles, plus
      `web_request_policy_evaluate()` unit coverage of the shared auth/
      content-type/body-limit/confirmation gate all of them pass through
      first (now including a `WEB_API_ROUTE_DIAGNOSTICS_FULL`-specific
      unauthorized/expired-session case, and a dispatch-wiring test proving
      that route reaches `web_diagnostics_handle()`) — no live end-to-end HTTP
      test wires `web_server_api.c`/`web_request_policy.c` to the httpd fake
      for that group; investigated and deliberately deferred, see
      `docs/implementation-v2/V2_057_FULL_HTTP_CONTRACT_MATRIX_2026-08-09.md`.
- [x] Test the unprovisioned route surface contains only setup GET/POST and static
      setup assets.
- [ ] Test setup-state GET returns only the approved two fields and returns `404`
      after provisioning. The provisioned-mode `404` path
      (`setup_route_response()`'s GET branch in `web_api_administration.c`) is now
      tested; `setup_state_handler` (the unprovisioned-mode GET, in
      `web_server_setup.c`) is httpd-dependent and remains untested.
- [x] Test setup POST returns `409` after provisioning. `setup_route_response()`'s
      POST branch is now directly tested via
      `web_api_handle_administration(WEB_API_ROUTE_SETUP, POST, …)`, distinct from
      `test_already_provisioned_rejected`'s defense-in-depth check.
- [x] Test exact response schemas and status codes. Strong for
      status/limits/send/login/diagnostics/blob — status/limits/send are now
      also verified at the live-handler level, not just their pure JSON
      builders (see the matrix bullet above); session/restart/setup-conflict
      are now covered too via `web_api_administration_tests`.
- [x] Test that secret-like sentinel values never appear in responses or logs.
      Explicit checks exist for diagnostics and status; send's password material
      is secure-zeroed by construction; session (`handle_session()`'s JSON) and
      login (the cookie header `web_cookie_build_session_header()` composes) are
      now scanned with the same `check-secret-sentinel.py`-backed harness
      diagnostics/status use; blob responses carry raw gzip bytes and an ID/size
      pair only, with no secret-like field to scan.
- [ ] Consume the same checked-in examples from C and TypeScript tests. The
      TypeScript side genuinely validates against
      `contracts/v2/api/examples.json`; the C side never parses that file —
      one divergence this actually caught and fixed:
      `web_server_diagnostics.c`'s `resetReason` values used hyphens
      (`"power-on"`) against the contract's underscores (`"power_on"`).

## Phase 5 exit gate

- [x] Route table exactly matches the v2 specification.
- [x] Old routes are absent.
- [ ] Contract and security tests pass. The tests that exist pass (52/52),
      but this masks the real coverage gaps enumerated under V2-057 — passing
      tests is not the same as complete contract/security coverage.
- [ ] API documentation examples match observed responses. No script or test
      diffs actual handler output against `contracts/v2/api/examples.json`
      wholesale; the one concrete mismatch found during this audit
      (`diagnostics.resetReason`) is now fixed, but nothing guards against a
      recurrence of this class of drift.

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
- [ ] Measure real-device last-keystroke latency after cancellation.

Evidence: `docs/implementation-v2/V2_063_EXECUTOR_CANCELLATION_RESPONSIVENESS_2026-08-08.md`.
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

- [ ] Verify `303a:4001` and required manufacturer, product, and serial strings.
- [ ] Capture host HID reports rather than relying on text-editor output.
- [ ] Verify printable text exactly.
- [ ] Verify a chord sets modifier and usage concurrently.
- [ ] Verify every terminal path ends with an all-zero report.
- [ ] Verify invalid source types nothing.
- [ ] Verify cancellation during typing and delay.
- [ ] Verify disconnect and reconnect behavior.

## Phase 6 exit gate

- [x] C and TypeScript conformance suites pass the same corpus.
- [x] Executor host tests pass under sanitizers.
- [ ] Required HID and cancellation hardware evidence is committed.

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
- [ ] Open the empty Macros page.
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
- [ ] Real-browser tests cover first phone, refresh, expired session, no blobs,
      invalid newest blob, and send recovery.

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
- [ ] Show Add macro, Edit, Send, and overflow controls.
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

- [ ] Make preview available from overflow actions.
- [ ] Honor Always Preview when configured.
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

- [ ] Macros page browser tests cover idle, USB unavailable, quick send,
      confirmation, progress, cancel, complete, failure, timeout, release error,
      reload, and rapid repeated input.
- [x] Ordinary Quick Send never navigates to a standalone progress/result route.

---

## Phase 10 — Macro editing and package management

**Depends on:** Phases 7 and 9.

## V2-100 — Macro editor

- [ ] Implement name, source, key-press duration, and inter-key delay fields.
- [ ] Show UTF-8 byte counts.
- [ ] Implement directive insertion controls.
- [ ] Run live TypeScript validation against the shared corpus implementation.
- [ ] Show exact error location and Go to error.
- [ ] Show action count and estimated duration when valid.
- [ ] Save only to the in-memory working copy.
- [ ] Cancel without changing the working copy.

## V2-101 — Macro CRUD and ordering

- [ ] Create, edit, duplicate, move, reorder, and delete macros locally.
- [ ] Generate IDs with `crypto.randomUUID()`.
- [ ] Preserve global macro-ID uniqueness.
- [ ] Mark repository dirty after every actual content/order change.
- [ ] Avoid dirty transitions after no-op edits.

## V2-102 — Package management

- [ ] Create, rename, duplicate, reorder, and delete packages locally.
- [ ] Generate canonical UUID v4 package IDs.
- [ ] Mark repository dirty after actual changes.
- [ ] Identify destructive targets by name.
- [ ] Resolve and persist package selection after selected-package deletion.
- [ ] Do not make ordinary package switching dirty.

## V2-103 — Unsaved-change protection

- [ ] Keep Unsaved changes and Save snapshot visible on all operational screens.
- [ ] Register `beforeunload` while dirty where supported.
- [ ] Warn before Sign Out, snapshot load, import replacement, reset settings,
      and factory reset.
- [ ] Offer context-appropriate Cancel, Export working copy, Save snapshot, and
      Discard options.
- [ ] Never claim closed unsaved work can be recovered.

## Phase 10 exit gate

- [ ] Editing and package-management unit and browser tests pass.
- [ ] No edit calls a firmware package or macro route.
- [ ] Dirty-state transition matrix is fully tested.

---

## Phase 11 — Snapshots, import, and export UI

**Depends on:** Phases 7 and 10.

## V2-110 — Manual Save snapshot

- [ ] Validate the entire repository.
- [ ] Serialize compact UTF-8 JSON.
- [ ] Gzip in React.
- [ ] Enforce device limits before upload.
- [ ] Upload only after an explicit user action.
- [ ] Mark saved only after `201 Created`.
- [ ] Preserve dirty work after every failure.
- [ ] Never autosave after edits, sends, package selection, timers, or navigation.

## V2-111 — Snapshot management

- [ ] Show blob ID, stored size, loaded indicator, storage usage, and configured
      advisory target.
- [ ] Provide Load, Download, Delete, and Save current snapshot.
- [ ] Avoid device-generated dates.
- [ ] Permit manual loading at any time.
- [ ] Confirm deletion by exact blob ID and consequence.
- [ ] Never automatically delete a snapshot.

## V2-112 — Advisory retention target

- [ ] Default target to five.
- [ ] Show a non-blocking cleanup indicator when count exceeds target.
- [ ] Let the user choose which snapshots to delete.
- [ ] Permit a sixth or later snapshot when storage permits.
- [ ] Test that no save path triggers deletion.

## V2-113 — Dirty-work protection during load

- [ ] Warn when loading another snapshot while dirty.
- [ ] Offer Cancel, Export working copy, Save snapshot, and Discard changes and
      load.
- [ ] Validate the selected snapshot before replacing memory.
- [ ] Leave stored snapshots untouched.
- [ ] Resolve selected package after load.

## V2-114 — Unreadable snapshot recovery

- [ ] Show the failing blob and exact decompression/schema error.
- [ ] Keep it stored.
- [ ] Allow download, delete, or deliberate selection of another blob.
- [ ] Never silently fall back.

## V2-115 — Import and export

- [ ] Export the current working copy as `.emk-repository.json.gz`.
- [ ] Import bytes, decompress, decode, parse, and fully validate before
      replacement.
- [ ] Show package and macro counts before confirmation.
- [ ] Mark imported data dirty.
- [ ] Do not upload automatically.
- [ ] Exclude every device credential, session, key, and diagnostic field.

## V2-116 — Advanced non-atomic replace

- [ ] Keep normal saves additive.
- [ ] Implement replacement only as an explicitly advanced delete-then-add flow.
- [ ] Warn that a failed add does not restore the deleted blob.
- [ ] Test delete-success/add-failure behavior.

## Phase 11 exit gate

- [ ] Snapshot and import/export browser tests pass.
- [ ] No automatic snapshot creation or deletion exists.
- [ ] Dirty work survives all recoverable failure paths.

---

## Phase 12 — Settings, diagnostics, and destructive operations UI

**Depends on:** Phases 5 and 11.

## V2-120 — Settings UI

- [ ] Implement device name.
- [ ] Implement serial-confirmation policy.
- [ ] Implement AP and optional station configuration.
- [ ] Implement administrator password change.
- [ ] Implement Quick Send/Always Preview.
- [ ] Implement advisory retention target.
- [ ] Implement source-preview preference.
- [ ] Keep `lastSelectedPackageId` hidden from ordinary text editing.
- [ ] Ensure preference changes do not dirty the repository.

## V2-121 — Restart and reset flows

- [ ] Implement restart confirmation and reconnect guidance.
- [ ] Implement reset-settings confirmation with exact preservation behavior.
- [ ] Implement factory-reset confirmation with exact erase and reprovision
      behavior.
- [ ] Protect dirty work before every disruptive action.
- [ ] Handle connection loss and recovery explicitly.

## V2-122 — Diagnostics UI

- [ ] Render the fixed diagnostics schema.
- [ ] Show firmware/build, uptime, reset reason, memory, stack, USB, Wi-Fi,
      storage, blob count, send state, health, and invalid/temp filenames.
- [ ] Do not display package or macro data.
- [ ] Provide copy/download only after filtering sensitive content.

## Phase 12 exit gate

- [ ] Settings and diagnostics contract/browser tests pass.
- [ ] Destructive flows preserve or explicitly discard dirty work.
- [ ] Secret-leak tests pass.

---

## Phase 13 — Portrait phones, responsive layout, and accessibility

**Depends on:** Phases 8–12.

## V2-130 — Responsive layout

- [ ] Support a minimum 320 CSS-pixel viewport.
- [ ] Use single-column phone layouts.
- [ ] Support wider tablet and desktop layouts without changing workflow.
- [ ] Keep touch targets at least 44 by 44 CSS pixels.
- [ ] Respect display cutouts and gesture-navigation safe areas.
- [ ] Ensure bottom navigation does not cover final actions.

## V2-131 — Portrait-required phone surface

- [ ] Apply phone-landscape blocking using tested coarse-pointer, orientation, and
      short-viewport criteria, initially equivalent to:

```css
@media (orientation: landscape) and (pointer: coarse) and (max-height: 600px)
```

- [ ] Show Rotate your phone instead of ordinary operational content.
- [ ] Restore the exact route, draft, working copy, and dirty state on portrait.
- [ ] Do not reload, clear memory, restart a send, or duplicate callbacks.
- [ ] Add `orientation: "portrait-primary"` to the manifest as progressive
      enhancement only.

## V2-132 — Landscape active-send safety

- [ ] Show macro name and current send state/progress on the orientation surface.
- [ ] Keep Cancel and release all keys accessible.
- [ ] Test cancellation while landscape.
- [ ] Ensure tablets, foldables classified as tablets, laptops, and desktops are
      not incorrectly blocked.

## V2-133 — Accessibility

- [ ] Make all controls keyboard accessible.
- [ ] Preserve logical focus order.
- [ ] Trap and restore focus in dialogs.
- [ ] Use live regions without announcing every poll tick.
- [ ] Never use color as the only state indicator.
- [ ] Expose source-editor labels and exact validation locations.
- [ ] Provide Move first/up/down/last alternatives to drag and drop.
- [ ] Honor reduced motion.
- [ ] Prevent hidden source from leaking through accessible names.

## Phase 13 exit gate

- [ ] Automated accessibility checks pass.
- [ ] Manual keyboard and screen-reader checks are recorded.
- [ ] Real Android phone portrait/landscape tests pass.
- [ ] Tablet and desktop landscape tests pass.
- [ ] Active-send cancellation remains available in landscape.

---

## Phase 14 — Migration cleanup, static assets, scripts, and documentation

**Depends on:** Phases 2–13.

## V2-140 — Delete dead v1 code

- [ ] Remove obsolete firmware files and build registrations.
- [ ] Remove obsolete React routes, screens, API clients, guards, models, and
      fixtures.
- [ ] Remove obsolete schemas and generated artifacts.
- [ ] Remove compatibility types and migrations that have no released v2 input.
- [ ] Remove dead tests rather than skipping them.
- [ ] Verify no user-visible v1 wording remains.

## V2-141 — Static application production behavior

- [ ] Keep all production assets local.
- [ ] Use content-hashed Vite filenames.
- [ ] Generate gzip variants where specified.
- [ ] Serve correct content types.
- [ ] Cache hashed assets immutably and revalidate `index.html`.
- [ ] Reject path traversal.
- [ ] Never expose the userdata mount.
- [ ] Verify the built webfs fits the 1 MiB partition with recorded margin.

## V2-142 — Build, lint, and architectural guards

- [ ] Update scripts for the new source and test layout.
- [ ] Keep `./scripts/check-all.sh` authoritative.
- [ ] Add guards against old routes, firmware repositories, `activePackageId`,
      automatic snapshot deletion, mandatory send navigation, remote assets, and
      browser repository persistence.
- [ ] Keep all first-party warnings fatal.

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

- [ ] Repository-wide searches find no authoritative v1 references.
- [ ] No dead v1 production code or skipped v1 test remains.
- [ ] Static assets fit and pass local-only checks.
- [ ] Documentation accurately describes the implemented v2 system.

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
has happened yet. The items below reflect only what is independently,
currently, and reproducibly true as of commit `9bb47bb` (2026-08-09) — not a
claim that the project is close to final sign-off.

- [ ] `docs/SPEC_V2.md` matches production behavior. Cannot be true yet: Phases
      9–14 (Macros page, editing, snapshots, settings UI, accessibility,
      migration cleanup) are unstarted, so most of SPEC_V2's UI-facing
      requirements have no implementation to match against.
- [ ] `docs/UI_UX_SPEC_V2.md` matches production behavior. Same reason.
- [ ] `docs/TODO_V2.md` contains no falsely completed task. Not yet audited —
      that audit is V2-156's job and V2-156 has not been started.
- [x] `docs/TODO.md`, `README.md`, and `CLAUDE.md` point to the v2 authority
      set. `docs/TODO.md` and `CLAUDE.md` already did; `README.md`'s opening
      section was fixed to cite `SPEC_V2.md`/`UI_UX_SPEC_V2.md`/`TODO_V2.md`
      (V2-143's first sub-bullet, see
      `docs/implementation-v2/V2_143_DOC_AUTHORITY_SYNC_2026-08-09.md`). This
      is narrower than V2-143 as a whole, which also covers
      `docs/mockups/v2/`, API/test/hardware/recovery documentation, and
      status-vs-intent labeling — those remain open.
- [x] `./scripts/check-all.sh` passes from a clean checkout. Verified against
      an actual fresh `git clone` of commit `9bb47bb` from `origin/master`
      (not the working tree), with `npm ci` run in `webapp/` and
      `littlefs-python==0.15.0` installed into the sourced ESP-IDF virtualenv
      exactly as `.github/workflows/quality.yml` does it: full gate exit 0,
      50/50 host tests. This is a snapshot fact about the current commit, not
      a permanent one — it will need re-verification at the actual final
      sign-off after more work lands.
- [ ] Required ESP32-S3R8, USB HID, storage, Wi-Fi, authentication, and Android
      evidence is committed. Multiple hardware-only items remain open
      throughout this file (V2-035, part of V2-041, part of V2-044, V2-064,
      and all of Phase 15's V2-151 through V2-155 matrices) — none of that
      evidence exists yet.
- [x] No v1 compatibility architecture remains in production code. Phase 2's
      exit gate (all four items) is complete, and this is continuously
      enforced, not just asserted: `check-all.sh` runs
      `check-v2-phase2-architecture.py` ("phase 2 architecture: no
      firmware-owned package or macro repository") and
      `check-removed-features.sh` ("removed features: none of the SPEC 1.1
      rejections have reappeared") on every invocation, and both currently
      pass. This covers production firmware code specifically; it does not
      by itself certify the webapp or the rest of this checklist.
- [ ] No known silent failure, dangerous fallback, secret leak, automatic
      snapshot deletion, or inaccessible cancellation path remains. Cannot be
      claimed yet — the snapshot-management UI (Phase 11) that "automatic
      snapshot deletion" is about does not exist yet, and V2-156's audit
      hasn't walked the rest.
- [ ] Final implementation report records the accepted release commit SHA. No
      such report exists; it is V2-156's own deliverable.
