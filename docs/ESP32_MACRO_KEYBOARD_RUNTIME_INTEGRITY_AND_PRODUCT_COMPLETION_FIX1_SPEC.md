# ESP32 Macro Keyboard FIX1 Specification

> **⚠ Superseded in part (2026-08-02).** This document predates the
> specification revision that removed procedures, steps, checkpoints, progress
> tracking, the global macro library, quarantine, the multi-file transaction
> layer, and the per-set directory tree. Any task here that names one of those
> subsystems is **struck** — it describes work on code that no longer exists,
> and must not be implemented.
>
> What remains valid is everything about the parts that survived: storage
> durability, the repository lock, authentication, the executor, USB HID, Wi-Fi,
> and the web server. For current scope see `docs/SPEC.md`; for what was removed
> and why, see `docs/SPEC.md` §1.1 and
> `docs/TODO_SPEC_ALIGNMENT_2026-08-02.md`.

**Document type:** Authoritative corrective implementation specification
**Repository:** `ekkus93/esp32-macro-keyboard`
**Target branch:** `master`
**Review baseline:** `992f2a018aff97e5b167c98d6a0d469d6a7c84ff`
**Target hardware:** ESP32-S3 with at least 8 MiB flash
**Required ESP-IDF:** `v5.5.5`
**Companion implementation plan:**
`docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md`

## 1. Purpose

This specification converts the current code-review findings and the incomplete
items in `docs/TODO.md` into one corrective implementation program.

The current repository contains a strong foundation:

- strict macro parsing and bounded compilation;
- typed application errors;
- a single-owner macro executor;
- USB HID state and report abstractions;
- protected development SoftAP behavior;
- authentication, sessions, CSRF, and login throttling foundations;
- bounded HTTP helpers and static-file serving;
- set-level storage CRUD, transaction manifests, and quarantine foundations;
- host tests, sanitizers, coverage, frontend checks, and CI workflows.

The repository is not yet a safe or complete version `0.1` product. The
implementation must correct the runtime-integrity defects first, then complete
the missing persistent provisioning, storage repositories, APIs, frontend
workflows, hardware validation, and release evidence.

The implementation is complete only when the device performs a real,
server-owned macro execution workflow without silent fallback, false success,
partial cleanup, or ambiguous on-disk state.

## 2. Authority and precedence

This file and its companion TODO are authoritative for FIX1.

Use the following precedence when requirements conflict:

1. This FIX1 specification.
2. `docs/SPEC.md`.
3. The companion FIX1 TODO.
4. `docs/TODO.md`.
5. Existing implementation-status and review documents.
6. Existing code behavior.

Do not preserve existing behavior merely because tests currently encode it.
Tests that assert unsafe or false-success behavior must be corrected.

Do not mark a FIX1 task complete because a file, function, route, screen, or
test exists. Completion requires correct success behavior, correct failure
behavior, direct tests, clean quality gates, and synchronized documentation.

## 3. Non-negotiable implementation rules

The following rules apply to every FIX1 change.

### 3.1 Fail closed

- Never convert an infrastructure failure into a successful quality result.
- Never convert a cryptographic failure into an authentication mismatch.
- Never convert a cancelled execution into a successful execution.
- Never convert partial startup into a usable runtime.
- Never activate incomplete or ambiguous storage state.
- Never start an open AP or use a fixed default password as a fallback.
- Never continue normal operation after an unrecoverable ownership failure.
- Never accept a regular file where a required directory is expected.
- Never ignore a failed cleanup operation.

### 3.2 Preserve error causality

Every operation that can fail during cleanup or rollback must preserve:

- the primary operation error;
- the first cleanup error;
- whether cleanup remained incomplete;
- the affected subsystem or path;
- a stable request, execution, transaction, or recovery identifier when one
  exists.

Returning only a cleanup error is insufficient because it hides the event that
triggered cleanup. Returning only the primary error is insufficient when the
device remains partially initialized or on-disk state is ambiguous.

### 3.3 Explicit ownership

Every subsystem that allocates a task, queue, semaphore, driver, server handle,
netif, event handler, mounted filesystem, or open namespace must expose explicit
ownership and shutdown behavior.

A function that returns an error may still own resources. Ownership must be
queryable or included in the returned state so the caller can clean it up.

### 3.4 No hidden check suppression

The project must not use any of the following to make a first-party check appear
green:

- `|| true`;
- ignored process exit status;
- hidden analyzer stderr;
- `NOLINT`;
- `eslint-disable`;
- `@ts-ignore`;
- `@ts-nocheck`;
- compiler diagnostic pragmas;
- `-Wno-*`;
- `-Wno-error=*`;
- coverage exclusion markers;
- first-party formatting suppression.

A tool may be excluded only for generated or third-party files, with the
exclusion path documented and tested.

### 3.5 No mock product behavior in production routes

Static screen scaffolding may remain only when it is visibly labeled as
unavailable and cannot be mistaken for working functionality.

The following are prohibited:

- a Send button that only changes the browser route;
- hardcoded USB readiness;
- hardcoded active-set state after login;
- a Save button that does not persist;
- a Delete button that does not delete;
- a diagnostics screen that reports hardcoded health;
- an import or export screen that describes behavior without invoking it;
- a result screen that labels cancellation as success.

## 4. Required implementation order

FIX1 must be implemented in this order:

1. Make quality gates fail closed.
2. Add complete subsystem ownership and reverse-order teardown.
3. Correct HTTP partial-start cleanup.
4. correct storage mount rollback and directory validation.
5. Add atomic-write artifact recovery.
6. Make quarantine creation recoverable.
7. Serialize repository mutations.
8. Separate authentication mismatch from authentication-system failure.
9. Correct Wi-Fi, executor, and controls cleanup/error visibility.
10. Implement secure persistent provisioning.
11. Complete macro, procedure, progress, and settings repositories.
12. Complete server-owned execution submission and resource APIs.
13. Replace frontend mock workflows with real API-backed workflows.
14. Complete integration, power-loss, and hardware validation.
15. Synchronize release documentation and enforce release budgets.

Later phases must not be used to work around defects in earlier phases.

## 5. Runtime architecture requirements

## 5.1 Application lifecycle state machine

`app_core_sequence_start()` must become an explicit ownership state machine.

Track at least:

- NVS initialized;
- storage web partition mounted;
- storage data partition mounted;
- storage recovery completed;
- repository initialized;
- authentication initialized;
- USB driver initialized;
- executor initialized and task running;
- controls initialized and task running;
- Wi-Fi resources partially or fully owned;
- HTTP resources partially or fully owned.

The cleanup path must:

1. preserve the primary failure;
2. stop accepting new user work;
3. request cancellation of any active execution;
4. release all USB keys;
5. stop HTTP;
6. stop Wi-Fi;
7. stop controls;
8. stop executor;
9. deinitialize USB;
10. deinitialize authentication;
11. deinitialize repository resources;
12. unmount data and web filesystems;
13. deinitialize NVS when appropriate;
14. set the fatal indicator using a channel that does not depend exclusively on
    the failing subsystem;
15. report all cleanup failures.

Cleanup must attempt every owned stage even when an earlier cleanup stage fails.

Production startup must not initialize task-owning subsystems and then discover
that secure provisioning is unavailable. Provisioning state must be loaded and
validated before USB, executor, controls, Wi-Fi, or HTTP are started.

## 5.2 Subsystem deinitialization APIs

Add idempotent shutdown APIs for all resource-owning subsystems.

Required public APIs include equivalent functionality to:

```c
app_error_code_t auth_deinit(void);
app_error_code_t usb_keyboard_deinit(void);
app_error_code_t macro_executor_deinit(void);
app_error_code_t device_controls_deinit(void);
app_error_code_t storage_repository_deinit(void);
bool web_server_owns_resources(void);
bool wifi_ap_owns_resources(void);
```

Calling a deinit function twice must not crash, leak, or destroy resources owned
by another subsystem. The second call may return `APP_ERROR_NONE` or a stable
not-initialized result, but behavior must be documented and tested.

Task-owning subsystems must use cooperative task shutdown. Do not delete a task
while it may still access a queue, semaphore, GPIO, driver, or engine object.

## 5.3 Structured operation result

Introduce a small common result structure for startup, cleanup, and destructive
storage operations.

The exact type name may differ, but it must represent:

```c
typedef struct {
    app_error_code_t primary_error;
    app_error_code_t cleanup_error;
    bool cleanup_incomplete;
} app_operation_result_t;
```

Do not replace the existing stable error-code API everywhere. Use the structured
result where two failures can coexist, and adapt to the existing API only at
boundaries that also log or persist the secondary error.

## 6. Quality-gate requirements

## 6.1 clang-tidy must fail closed

`scripts/check-firmware.sh` must preserve the exit status from
`run-clang-tidy`.

Before running the analyzer, verify:

- the clang compile database exists;
- it is valid JSON;
- it contains at least one first-party translation unit;
- the expected ESP-IDF clang tools are available.

The script must fail when:

- the analyzer returns nonzero;
- no first-party translation unit is selected;
- a first-party warning or error is reported;
- the compilation database is missing or empty;
- output parsing fails.

The script must print the complete analyzer output on failure.

Do not treat output grep as the sole source of truth.

## 6.2 First-party source must not use formatting suppression

Replace the `clang-format off` source-amalgamation pattern in:

- `firmware/components/auth/auth_core.c`;
- `firmware/components/web_server/web_server.c`;
- `firmware/components/web_server/web_server_adapter.c`.

Compile the implementation fragments as ordinary translation units or refactor
them into normally formatted source files.

Coverage and clang-tidy must attribute those files as first-party production
code.

## 6.3 CI pinning

Pin GitHub Actions to immutable commit SHAs before release. Record the human
readable action version in comments.

Pin or verify the versions of host packages used by the quality gate. A mutable
`ubuntu-latest` image is acceptable during development only when the documented
tool-version verifier rejects incompatible versions.

## 7. Storage integrity requirements

## 7.1 Mount ownership and rollback

`storage_mount_all()` must report partial ownership or guarantee complete
rollback before returning failure.

If `userdata` mount fails and `webfs` unmount also fails, the caller must know
that `webfs` remains mounted and must continue cleanup.

`mkdir_checked()` must use `stat()` after `EEXIST` and return
`APP_ERROR_STORAGE_CORRUPT` when the path is not a directory.

The repository must not initialize on an invalid directory topology.

## 7.2 Atomic-write artifact recovery

The current atomic writer uses:

- `<destination>.tmp.<uuid>`;
- `<destination>.bak.<uuid>`.

Startup must reconcile these artifacts before transaction manifests or
repository indexes are loaded.

Recovery must be deterministic and validator-driven. It must never guess that
unvalidated bytes are safe.

For each destination and operation identifier, evaluate:

| Canonical | Temporary | Backup | Required result |
| --- | --- | --- | --- |
| absent | present | absent | validate temporary; activate only when the destination type and operation permit roll-forward |
| absent | absent | present | restore validated backup |
| absent | present | present | restore validated backup; preserve temporary as evidence unless a transaction manifest proves roll-forward |
| present | present | absent | validate canonical; remove or quarantine temporary |
| present | absent | present | validate canonical and backup; keep canonical only when it represents a completed activation; otherwise restore backup |
| present | present | present | use the owning transaction manifest; absent proof means preserve old committed state and quarantine ambiguity |
| absent | absent | absent | no action |

Every rename and parent-directory durability barrier during recovery must be
checked.

Unknown, malformed, cross-directory, duplicate, or conflicting artifacts must
not be deleted. Preserve them as recovery evidence and enter degraded mode.

## 7.3 Manifest durability

Transaction manifests are recovery metadata and must not depend on a write
scheme that cannot recover its own interrupted state.

One of the following designs is required:

1. implement atomic-artifact recovery for transaction manifests before manifest
   enumeration; or
2. replace manifest persistence with an append-only, checksummed journal whose
   valid prefix is recoverable after power loss.

The chosen design must survive interruption after every write, sync, close,
rename, and parent-sync step.

## 7.4 Quarantine transaction

Quarantine creation must be all-or-nothing or recoverable.

The preferred format for the unreleased version `0.1` is:

```text
/data/quarantine/<quarantine-id>/
  record.json
  evidence
```

Create the complete entry under a staging directory, sync and validate both
files, then atomically rename the directory into the quarantine root.

A startup recovery pass must reconcile any quarantine staging directory.

Do not create a committed record before evidence ownership is durable.

## 7.5 Repository serialization

All repository mutations must be serialized across their complete
read-check-write transaction.

This includes:

- set create, update, delete, duplicate, and reorder;
- macro create, update, delete, duplicate, and reorder;
- procedure create, update, delete, and reorder;
- progress updates;
- settings changes;
- import and restore;
- transaction recovery;
- quarantine mutation;
- index repair.

Public repository functions must acquire the repository mutation lock.
Internal helper functions called while the lock is held must use `_locked` or
equivalent naming and must not reacquire a non-recursive lock.

Read-only operations may use the same lock initially. Reader/writer
optimization is out of scope until correctness is proven.

## 7.6 Complete object repositories

Implement bounded, revisioned repositories for:

- set-local macros;
- global macros;
- procedures;
- procedure progress;
- settings;
- active-set selection;
- recovery and diagnostic metadata.

Every object must have:

- exact schema version;
- strict field allowlist;
- bounded strings and arrays;
- strict UUID validation;
- nonzero revision;
- referential-integrity validation;
- atomic persistence;
- expected-revision mutation;
- quarantine on corrupt active data;
- deterministic ordering.

Deleting a set must not delete a global macro. Deleting a macro that is still
referenced must fail with a stable conflict and return the referencing object
IDs.

## 8. Authentication and provisioning requirements

## 8.1 Password verification API

Password verification must return an operation error separately from the match
result.

Required behavior:

- malformed arguments return `APP_ERROR_INVALID_ARGUMENT`;
- unsupported or corrupt password record returns
  `APP_ERROR_STORAGE_CORRUPT`;
- PBKDF2 failure returns `APP_ERROR_INTERNAL`;
- a correct derivation with different bytes returns
  `APP_ERROR_NONE` and `out_matches = false`;
- a correct derivation with equal bytes returns
  `APP_ERROR_NONE` and `out_matches = true`.

Only `APP_ERROR_NONE` with `out_matches = false` may increment the login-failure
counter.

## 8.2 Persistent encrypted provisioning

Production firmware must use encrypted NVS for device configuration.

The partition table must include an NVS key partition:

```text
nvs_keys, data, nvs_keys, , 0x1000, encrypted
```

The firmware configuration must enable the selected ESP-IDF NVS encryption
scheme. The release process must also define the required flash-encryption or
HMAC-key provisioning workflow.

Persist at least:

- provisioning-complete marker;
- AP SSID;
- AP passphrase;
- administrator password record;
- configuration revision;
- physical-confirmation setting;
- active-set selection policy;
- credential generation/version metadata.

Do not persist:

- plaintext administrator password;
- active session tokens;
- CSRF tokens;
- one-time setup code after setup completes.

NVS writes must use `nvs_commit()` and verify readback before reporting success.

## 8.3 First-run setup

When unprovisioned, the device must expose only the documented setup flow.

Setup must:

1. generate a one-time setup secret;
2. require physical presence or an explicit development-only build option;
3. establish a protected AP;
4. accept and validate the administrator password;
5. generate or accept a protected AP passphrase;
6. write encrypted NVS configuration;
7. read back and validate configuration;
8. invalidate the setup secret;
9. restart into normal operation.

A failed setup must not leave the device marked provisioned.

## 8.4 Credential logging

Production and ordinary development logs must not print plaintext AP or
administrator credentials.

A special manufacturing or bring-up build may expose one-time credentials only
when all of the following hold:

- a separate Kconfig option is enabled;
- the build prints a permanent warning banner;
- the option is rejected by the production release gate;
- credentials are printed once;
- the device invalidates them after successful provisioning;
- tests prove the option is absent from production configuration.

## 8.5 HTTP login security

Login must enforce:

- bounded JSON body;
- exact supported Content-Type;
- Host and Origin validation;
- global and per-client throttling;
- no failure-count increment on internal auth failure;
- stable error envelope;
- `Retry-After` when rate limited;
- no credential logging;
- session creation only after throttle reset succeeds.

The session cookie must include:

- `HttpOnly`;
- `SameSite=Strict`;
- `Path=/`;
- `Secure` when HTTPS is introduced.

## 9. Wi-Fi requirements

Wi-Fi cleanup must attempt all owned cleanup stages even when one fails.

Ownership flags must be cleared only after the corresponding cleanup operation
succeeds.

`wifi_ap_stop()` must report both:

- the original runtime/start failure, when one exists;
- the cleanup failure.

The status API must expose a redacted, stable Wi-Fi health state and cleanup
state.

No cleanup path may start or reinitialize Wi-Fi merely to stop it.

## 10. Executor and USB requirements

## 10.1 Cooperative executor shutdown

`macro_executor_deinit()` must:

1. reject new submissions;
2. request cancellation;
3. notify the executor task;
4. wait for the executor task to exit using a bounded synchronization primitive;
5. attempt `usb_keyboard_release_all()`;
6. free any owned plan;
7. delete the queue and semaphore only after task exit;
8. reset engine state;
9. report primary and release/cleanup errors separately.

Do not use `vTaskDelete()` on a running worker that may still access shared
objects.

## 10.2 Terminal status integrity

Every accepted execution must end in exactly one terminal state:

- completed;
- cancelled;
- failed;
- timed out.

A key-release failure must be retained separately and must make the user-visible
result unsafe, even if the primary action sequence completed.

The executor task must never cast a cleanup result to `void`.

## 10.3 Execution identity

Execution status must include:

- execution ID;
- set ID;
- macro ID;
- macro revision;
- state;
- action index and count;
- current action summary;
- primary error;
- release error;
- accepted timestamp;
- started timestamp;
- completed timestamp.

Polling must return the status for the server-owned current execution.

## 10.4 USB hardware validation

The release is blocked until USB enumeration, text, chords, reconnect,
suspend/resume, cancellation, disconnect-mid-run, and release-all behavior are
observed on Linux and ChromeOS.

Windows remains recommended when hardware is available.

## 11. Device-controls requirements

`device_controls_deinit()` must stop the controls task cooperatively before
deleting the confirmation semaphore.

Controls health must retain and expose:

- GPIO read/configuration failure;
- GPIO output failure;
- semaphore failure;
- task-start failure;
- task-stop failure;
- last cancel-button error;
- last confirmation-button error.

The LED may reflect health, but the LED is not the sole error-reporting channel.

Factory-reset and credential-reset gestures must require deliberate,
documented, physically unlikely sequences and must be tested against accidental
short presses.

## 12. HTTP API requirements

All API responses must use the documented success/error envelope.

All mutable routes must require session, CSRF, Host, Origin, and supported
Content-Type unless the route is explicitly part of first-run setup.

## 12.1 Required resource routes

Implement and test at least:

### Sets

- `GET /api/v1/sets`
- `POST /api/v1/sets`
- `GET /api/v1/sets/{setId}`
- `PUT /api/v1/sets/{setId}`
- `DELETE /api/v1/sets/{setId}`
- `POST /api/v1/sets/{setId}/duplicate`
- `PUT /api/v1/sets/order`
- `POST /api/v1/sets/{setId}/select` (the bounded active-set mutation)

### Macros

- `GET /api/v1/sets/{setId}/macros`
- `POST /api/v1/sets/{setId}/macros`
- `GET /api/v1/sets/{setId}/macros/{macroId}`
- `PUT /api/v1/sets/{setId}/macros/{macroId}`
- `DELETE /api/v1/sets/{setId}/macros/{macroId}`
- `POST /api/v1/sets/{setId}/macros/{macroId}/duplicate`
- `POST /api/v1/sets/{setId}/macros/reorder`
- corresponding global macro routes under `/api/v1/global/macros`
- per-resource validation routes ending in `/validate`

### Procedures and progress

- procedure list/create/read/update/delete/order routes;
- progress read/update/reset routes;
- explicit skip and complete-step routes;
- reference-validation route or validation in every mutation.

### Execution

- `POST /api/v1/executions`
- `GET /api/v1/executions/current`
- `POST /api/v1/executions/current/cancel`
- `POST /api/v1/executions/{executionId}/cancel` as an exact-identity alias

Execution submission must accept object identity, not arbitrary executable
source. The server loads the persisted macro, verifies the expected revision,
compiles the complete source, verifies USB readiness, and transfers plan
ownership to the executor before returning `202 Accepted`.

### Administration

- settings read/update;
- storage health;
- quarantine list;
- redacted diagnostics;
- export;
- import as new;
- transactional replace;
- full backup;
- full restore;
- restart;
- credential reset;
- factory reset with physical confirmation.

## 12.2 Status-code behavior

Use stable status mappings:

- `200` for successful reads and completed synchronous mutations;
- `201` for created resources;
- `202` only after the executor owns a valid plan or a documented asynchronous
  operation owns its work;
- `400` for malformed input;
- `401` for missing or invalid authentication;
- `403` for valid authentication without required physical or administrative
  authorization;
- `404` for absent resources;
- `409` for revision conflict, busy executor, reference conflict, or state
  conflict;
- `413` for payload too large;
- `415` for unsupported Content-Type;
- `422` for syntactically valid but semantically invalid object or macro source;
- `429` for rate limiting;
- `507` for storage full;
- `500` for internal errors;
- `503` for unavailable subsystems or incomplete provisioning.

Do not map every cancellation error to `409`.

## 13. Frontend requirements

## 13.1 Server state

The frontend must load and render real server state for:

- session;
- provisioning;
- active set;
- sets;
- macros;
- procedures;
- progress;
- USB state;
- storage health;
- current execution;
- settings;
- quarantine and diagnostics.

Hardcoded example data must be removed from production routes.

## 13.2 Execution workflow

The Send workflow must:

1. load the selected persisted macro;
2. show active set, macro identity, revision, decoded action summary, estimated
   duration, USB state, procedure context, and physical-confirmation
   requirement;
3. disable Send when USB is not ready or validation is stale/failed;
4. submit `POST /api/v1/executions`;
5. retain the accepted execution ID;
6. poll current execution;
7. display completed, cancelled, failed, timed-out, and release-failed states
   distinctly;
8. never infer success from `202`;
9. never automatically execute the next procedure step.

A cancellation result must use the word `Cancelled`, not `Finished`.

## 13.3 CRUD workflows

Every production button must either:

- perform its labeled action;
- open a functional form/dialog that performs the action; or
- be disabled with a visible explanation.

Implement real create, edit, duplicate, reorder, import, export, delete,
settings, backup, restore, and diagnostics flows.

## 13.4 Runtime validation

Do not cast unknown JSON payloads directly to domain types.

Validate API payloads using explicit type guards or a small pinned schema
validator. Invalid payloads must produce a visible `invalid_response` error.

## 13.5 Accessibility

Required controls include:

- keyboard-accessible dialogs;
- focus trapping and restoration;
- visible labels;
- live status/error regions;
- reorder alternatives: Move Up, Move Down, Move First, Move Last;
- touch targets suitable for mobile use;
- no color-only state indication.

Component and browser-level tests must cover primary workflows.

## 14. Import, export, backup, and restore requirements

## 14.1 Import validation

Validate the entire package before active mutation:

- schema and version;
- total package size;
- object counts;
- IDs and revisions;
- exact allowed fields;
- string and array bounds;
- macro syntax and duration;
- keyboard layout;
- duplicate IDs;
- procedure references;
- progress references;
- set/global macro scope;
- path safety;
- available storage;
- unsupported future fields;
- secret exclusion.

## 14.2 Transactional replace

Transactional replace must:

1. stage the complete replacement;
2. fsync and validate every staged file;
3. write durable recovery metadata;
4. move current data to backup;
5. sync the parent directory;
6. activate staged data;
7. sync the parent directory;
8. update indexes;
9. validate active readback;
10. mark the transaction complete;
11. remove backup and transaction metadata.

Power loss after every step must produce either the old complete state or the
new complete state, never a mixed active state.

## 14.3 Export and backup secret exclusion

Exports and backups must exclude:

- AP passphrase;
- administrator password record;
- setup secret;
- session and CSRF tokens;
- NVS encryption material;
- device-unique provisioning secrets.

Tests must scan generated packages for both field names and known secret values.

## 15. Diagnostics requirements

Diagnostics must be redacted and operationally useful.

Expose:

- firmware and schema versions;
- build identifier;
- uptime;
- reset reason;
- USB state;
- Wi-Fi state and client count;
- storage mount and recovery state;
- repository revision/health;
- quarantine count;
- current execution state;
- last primary and cleanup error per subsystem;
- task stack high-water marks;
- current and minimum heap;
- webfs and userdata capacity/free space.

Do not expose credentials, salts, hashes, tokens, raw macro source, or
unredacted HTTP headers.

## 16. Test requirements

## 16.1 Host tests

Add deterministic tests for every FIX1 defect, including:

- clang-tidy tool failure and empty translation-unit selection;
- startup failure after each initialization stage;
- cleanup failure at every reverse-order stage;
- partial HTTP start plus stop failure;
- partial filesystem mount rollback;
- `EEXIST` regular-file directory collision;
- every atomic temporary/backup/canonical combination;
- interrupted manifest update;
- interrupted quarantine creation;
- concurrent repository mutations using the same revision;
- PBKDF2 failure versus incorrect password;
- Wi-Fi cleanup continuing after each injected failure;
- executor terminal-publication and reset failure;
- controls GPIO and semaphore failure visibility;
- execution submission ownership;
- cancelled result text;
- release-error result text;
- malformed API success payload;
- secret exclusion from export and diagnostics.

All injected failures must assert the final ownership and persistent state, not
only the returned error.

## 16.2 Device tests

The device-test firmware must test:

- USB state transitions;
- keyboard report press/release;
- task startup and shutdown;
- AP startup and shutdown;
- encrypted NVS persistence;
- physical confirmation and cancel buttons;
- fatal/degraded indicator behavior;
- storage mount failure without formatting.

## 16.3 Browser tests

Add browser-level tests for:

- first-run setup;
- login, rate limiting, logout, and session expiry;
- set selection;
- macro validation and save;
- procedure progress;
- execution submission;
- cancellation;
- failed and release-failed results;
- disconnected USB;
- storage full;
- stale revision;
- import validation;
- offline/reconnect;
- keyboard-only operation.

## 16.4 Power-loss tests

Use deterministic host fault injection first, then real hardware power
interruption.

Record results for interruption after each transaction and atomic-write phase.

## 17. Release requirements

A production release is blocked until all of the following are true:

- the complete FIX1 TODO is checked off with evidence;
- `./scripts/check-all.sh` passes from a clean checkout;
- analyzer infrastructure failures make CI fail;
- all required APIs and frontend workflows are functional;
- no production screen contains hardcoded product state;
- no known silent fallback or false-success path remains;
- encrypted persistent provisioning is operational;
- storage recovery is deterministic;
- Linux and ChromeOS USB validation passes;
- SoftAP/browser integration passes;
- power-loss tests pass;
- storage-full tests pass;
- cancellation latency is measured;
- release-all behavior is observed;
- firmware, webfs, heap, stack, and userdata budgets are enforced;
- documentation matches the exact implementation.

## 18. Documentation synchronization

Update at least:

- `README.md`;
- `docs/API.md`;
- `docs/DEVELOPMENT.md`;
- `docs/IMPLEMENTATION_STATUS.md`;
- `docs/SECURITY_REVIEW.md`;
- `docs/RECOVERY.md`;
- `docs/HARDWARE_TEST_PLAN.md`;
- `docs/RELEASE_NOTES.md`;
- `docs/TODO.md`;
- `docs/UNIT_TESTS1_TODO.md` when its evidence is superseded.

Do not delete historical evidence. Correct stale claims and identify the commit
where behavior changed.

Do not reference any generated review, response, image, template, or companion
file unless it is committed at the exact path named.

## 19. Definition of done

FIX1 is done only when a clean, production-configured ESP32-S3 can:

1. boot with complete ownership accounting;
2. recover storage deterministically after interrupted writes;
3. provision protected persistent credentials;
4. expose an authenticated same-origin web application;
5. select a real persisted set;
6. create or load a persisted macro;
7. validate and submit it by ID and revision;
8. require the configured confirmation policy;
9. type it through USB HID;
10. cancel and release keys safely;
11. report the exact terminal state;
12. survive reboot with consistent data;
13. export and restore data without secrets;
14. expose redacted diagnostics;
15. shut down or enter fatal/degraded mode without leaked resources or hidden
    cleanup failures.

Anything less is incomplete and must remain visibly release-blocking.
