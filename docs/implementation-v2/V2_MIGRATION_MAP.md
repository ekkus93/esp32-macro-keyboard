# ESP32 Macro Keyboard v2 Migration Map

**Status:** Phase 0 implementation inventory  
**Baseline commit:** `ad75859f56986a81a2faf01832008b69b26a94e1`  
**Authoritative requirements:** [`../SPEC_V2.md`](../SPEC_V2.md) and
[`../UI_UX_SPEC_V2.md`](../UI_UX_SPEC_V2.md)

## 1. Classification rules

- **Retain** means the existing subsystem already implements a v2 responsibility
  and may remain after its tests are revalidated.
- **Adapt** means the subsystem remains conceptually valid but its contracts,
  limits, state model, or integration must change.
- **Rewrite** means the current implementation is built around the retired v1
  ownership model and should be replaced rather than incrementally preserved.
- **Delete** means the behavior is prohibited or has no v2 responsibility.

Existing code is never retained merely because it works under v1. Every retained
or adapted subsystem must pass v2 tests and acceptance evidence.

## 2. Firmware production inventory

### 2.1 `firmware/main`

| Subsystem | Decision | v2 treatment |
| --- | --- | --- |
| application entry point and component assembly | **Adapt** | Rewire startup around authentication, opaque blob storage, singular send state, static assets, and device settings. Remove package-repository startup dependencies. |

### 2.2 `firmware/components/support`

| Subsystem | Decision | v2 treatment |
| --- | --- | --- |
| operation results, lifecycle health, subsystem health | **Retain/Adapt** | Preserve generic error and health infrastructure; align codes and diagnostics with the fixed v2 API objects. |
| UUID utilities | **Retain/Adapt** | Continue canonical UUID validation and generation support. Firmware treats `lastSelectedPackageId` as an opaque UUID and does not resolve it against repository content. |
| CRC helpers | **Retain only for unrelated firmware uses** | Repository blobs must not gain a v2 CRC, hash, or digest. Remove any repository dependency on CRC helpers. |
| clocks, random, memory and bounded helpers | **Retain** | Revalidate warning-clean behavior and failure propagation. |

### 2.3 `firmware/components/macro_model`

| File group | Decision | v2 treatment |
| --- | --- | --- |
| `app_error.c`, `app_uuid.c` | **Retain/Adapt** | Preserve generic errors and canonical UUID handling. |
| package/macro repository object model in `macro_model.c` and headers | **Rewrite/Delete** | Keep only execution-time macro source/timing/action types needed by the parser and executor. Remove firmware-owned package objects, repository revisions, ordering, and persistence-oriented model fields. |

### 2.4 `firmware/components/macro_parser`

| Subsystem | Decision | v2 treatment |
| --- | --- | --- |
| C macro parser and US keymap | **Retain/Adapt** | Preserve complete-before-execute parsing, exact source locations, ASCII/directive/chord semantics, and action generation. Move all limits to the centralized v2 limits module and consume the shared conformance corpus. |

**Proof required:** shared C/TypeScript corpus, invalid source types nothing, exact
error positions, action and duration limits.

### 2.5 `firmware/components/macro_executor`

| Subsystem | Decision | v2 treatment |
| --- | --- | --- |
| executor engine, cancellation, timeout and health | **Retain/Adapt** | Keep one executor and release-all invariants. Replace plural execution/revision/package identity coupling with the singular `/api/v1/send` model, 60-second confirmation timeout, 300-second estimate ceiling, and 310-second absolute deadline. |

**Proof required:** cancellation during typing and delay, no queue, terminal
release-all, reload recovery from `GET /api/v1/send`.

### 2.6 `firmware/components/usb_keyboard`

| Subsystem | Decision | v2 treatment |
| --- | --- | --- |
| TinyUSB HID state and reports | **Retain/Adapt** | Preserve identity, ready-state gating, report generation, and release-all. Revalidate against v2 hardware acceptance tests. |

### 2.7 `firmware/components/auth`

| Subsystem | Decision | v2 treatment |
| --- | --- | --- |
| password verification | **Adapt** | Use PBKDF2-HMAC-SHA-256 with versioned metadata and a measured ESP32-S3R8 iteration count producing 250–500 ms derivation time. |
| RAM-only sessions | **Adapt** | Enforce 8 active sessions, 24-hour idle expiry, 7-day absolute expiry, 32-byte random tokens, `HttpOnly; SameSite=Strict; Path=/`. |
| login rate limiting | **Adapt** | Enforce five failures in 60 seconds followed by five-minute lockout with explicit errors. |
| auth health and constant-time comparison | **Retain/Adapt** | Preserve generic safety behavior and add v2 boundary tests. |

### 2.8 `firmware/components/provisioning`

| Subsystem | Decision | v2 treatment |
| --- | --- | --- |
| first-run state and NVS configuration | **Adapt** | Replace the v1 settings record with a versioned v2 record containing device/network/auth fields plus `sendMode`, advisory `snapshotRetentionTarget`, `showMacroSourcePreviews`, and opaque `lastSelectedPackageId`. Preserve unrelated fields during partial updates. |
| setup workflow | **Adapt** | Implement the exact `/api/v1/setup` contract, 8-digit serial setup code, reconnect semantics, and no secret echoing. |

### 2.9 `firmware/components/wifi_ap`

| Subsystem | Decision | v2 treatment |
| --- | --- | --- |
| protected AP and optional station operation | **Retain/Adapt** | Keep AP-first operation, bounded station retry, and explicit state. Align settings contracts and diagnostics; never expose passphrases. |

### 2.10 `firmware/components/serial_console`

| Subsystem | Decision | v2 treatment |
| --- | --- | --- |
| `confirm`, `cancel`, network commands and diagnostics | **Adapt** | Preserve trusted development-console semantics, align confirmation with singular send state, and ensure no setup code or credential leakage outside explicitly allowed provisioning output. |

### 2.11 `firmware/components/device_controls`

| Subsystem | Decision | v2 treatment |
| --- | --- | --- |
| restart, reset settings, factory reset | **Adapt** | Implement exact v2 preservation/erase behavior and accepted-response objects. Reset settings preserves blobs; factory reset erases configuration and repository blobs and requires reprovisioning. |

### 2.12 `firmware/components/app_core`

| Subsystem | Decision | v2 treatment |
| --- | --- | --- |
| startup sequencing and subsystem health | **Rewrite/Adapt** | Remove firmware repository/index/package startup sequencing. Start NVS/settings, userdata blob store, AP/station, USB, executor, auth, static server, and diagnostics in an explicit fail-visible order. |

### 2.13 `firmware/components/storage`

The current component registers low-level filesystem primitives together with a
large firmware-owned repository implementation. The two groups must be separated.

#### Retain or adapt

| Current files/areas | Decision | v2 treatment |
| --- | --- | --- |
| `storage_mount.c`, `storage_mount_core.c`, `storage_mount_topology.c` | **Adapt** | Keep LittleFS mounting and no-format-on-failure policy; mount the userdata partition and expose health. |
| `storage_fs_ops.c` | **Retain/Adapt** | Reuse bounded filesystem wrappers when they propagate every write, flush, close, sync, rename, read, and delete error. |
| generic pieces of `storage_atomic.c` | **Adapt** | Reuse only primitives that support `<id>.gz.tmp` to `<id>.gz` commit semantics without repository JSON or revision assumptions. |
| generic temporary-file recovery concepts from `storage_atomic_recovery.c` | **Rewrite/Adapt** | Restrict recovery to v2 upload `.tmp` files; never parse or delete final blobs because React cannot read them. |
| storage health/capacity reporting | **Adapt** | Report fixed v2 capacity fields, blob count, invalid names, and temporary files without repository parsing. |

#### Rewrite

| Area | Decision | v2 treatment |
| --- | --- | --- |
| storage paths and directory scanning | **Rewrite** | Implement `/data/repository/`, fixed-width decimal IDs, newest-first numeric listing, stale-counter recovery, and invalid-name diagnostics. |
| blob add/load/list/delete | **Rewrite** | Add a byte-oriented opaque blob module. It must not instantiate JSON, gzip, CRC, package, macro, revision, index, backup, restore, import, or replace logic. |

#### Delete

The following registered v1 files and their dedicated headers/tests have no v2
production responsibility and must be removed after replacement coverage exists:

- `storage_repository_io.c` where coupled to repository documents rather than
  generic opaque streams;
- `storage_repository_json.c`;
- `storage_json.c` for repository/package parsing;
- `storage_repository_objects_json.c`;
- `storage_repository_index.c`;
- `storage_repository_packages.c`;
- `storage_repository_package_operations.c`;
- `storage_repository_lock.c` where it exists for v1 revision/document locking;
- `storage_repository_document.c`;
- `storage_repository_macros.c`;
- `storage_package.c`;
- `storage_package_writer.c`;
- `storage_package_reader.c`;
- `storage_package_export.c`;
- `storage_package_backup.c`;
- `storage_package_restore.c`;
- `storage_package_replace.c`;
- `storage_package_import.c`;
- repository-specific incident and health logic that interprets package or JSON
  state.

### 2.14 `firmware/components/web_server`

#### Retain or adapt

| Current area | Decision | v2 treatment |
| --- | --- | --- |
| static file serving, path validation and content typing | **Retain/Adapt** | Preserve bounded streaming, traversal rejection, immutable hashed assets, and no userdata exposure. |
| HTTP adapter/body/auth/cookie primitives | **Retain/Adapt** | Preserve strict body handling and authentication; align limits, content types, cookie lifetime, and error envelope. |
| setup, login, logout, lifecycle and diagnostics infrastructure | **Adapt** | Replace v1 route shapes and response objects with exact v2 contracts. |
| JSON response helpers | **Adapt** | Emit the stable v2 error object and reject unknown fields. They must not parse repository blobs. |

#### Rewrite

- `web_server_status_limits.c` to expose exact v2 status and centralized limits;
- dispatch/route tables to contain only the documented `/api/v1` routes;
- execution submission/status/cancellation around singular `/api/v1/send`;
- administration/settings handlers around the versioned v2 NVS settings object;
- diagnostics around the fixed schema;
- blob list/add/load/delete handlers around opaque byte streams.

#### Delete

- `web_api_packages.c`;
- `web_api_macros.c`;
- package selection routes;
- firmware macro validation routes;
- repository restore/import/export/replace routes;
- plural `/api/v1/executions` resources;
- package/macro revision, `expectedRevision`, ETag, and optimistic-concurrency
  handling;
- any silent translation of old paths to v2 paths.

### 2.15 `firmware/test_app`

| Subsystem | Decision | v2 treatment |
| --- | --- | --- |
| on-device parser, executor, USB, auth, Wi-Fi, setup and storage tests | **Adapt** | Preserve useful device harness infrastructure; delete v1 package/repository tests and add v2 blob, singular-send, settings, timeout, and release-all cases. |

## 3. React production inventory

### 3.1 Application shell and startup

| Current file/area | Decision | v2 treatment |
| --- | --- | --- |
| `webapp/src/App.tsx` | **Rewrite** | Replace firmware package loading and route-level execution state with the v2 startup state machine, in-memory repository provider, dirty state, automatic newest-blob loading, selected-package resolution, and inline send recovery. |
| `features/auth/SessionBoundary` | **Adapt** | Preserve provisioned/session boundary concepts; add repository loading after authentication and reauthentication without discarding a live dirty working copy. |
| setup and login screens | **Adapt** | Align exact request objects, setup-complete/reconnect flow, rate-limit errors, and no per-phone onboarding. |
| `components/AppShell.tsx` | **Rewrite/Adapt** | Add device name, selected package, USB state, Saved/Unsaved indicator, Save snapshot, four-tab navigation, safe areas, and portrait overlay integration. |

### 3.2 Routing

| Current route | Decision | v2 treatment |
| --- | --- | --- |
| `packages` | **Adapt** | Becomes package chooser/management entry, not the mandatory startup screen. |
| `macros` | **Rewrite/Adapt** | Becomes the primary operating console with inline Quick Send. |
| `macro-editor` | **Adapt** | Operates on the in-memory repository and TypeScript parser only. |
| `confirm` | **Delete as mandatory route / retain optional preview surface** | Preview remains optional and may be a dialog or route; ordinary send must not navigate here. |
| `execution` | **Delete as ordinary route** | Progress and cancellation move inline to the Macros page. |
| `result` | **Delete as ordinary route** | Terminal acknowledgement and errors move inline. |
| `manage-packages`, `package-editor`, `delete-package` | **Rewrite/Adapt** | Operate only on the in-memory repository and dirty state. |
| `import`, `export` | **Rewrite** | Operate on complete gzip repository snapshots, not firmware package routes. |
| `settings`, `diagnostics` | **Adapt** | Consume exact v2 objects and protect dirty work before disruptive actions. |
| new routes/surfaces | **Add** | Repository loading, first repository/first package, Snapshots, snapshot recovery, portrait-required surface, and unsaved-change dialogs. |

### 3.3 API layer

| Current area | Decision | v2 treatment |
| --- | --- | --- |
| generic fetch client and error transport | **Retain/Adapt** | Preserve bounded requests, unauthorized notification, and typed validation; use exact v2 content types and error object. |
| `api/routes.ts` | **Rewrite** | Delete package/macro CRUD, validation, revision and plural execution calls. Add blob, singular send, exact settings, status, limits, setup, auth, device action, and diagnostics calls. |
| `api/packages.ts` and package-specific clients | **Delete** | React package operations are local working-copy operations. |
| guards and management guards | **Rewrite** | Match strict v2 response objects and reject extra fields. |
| execution guards | **Rewrite** | Match singular send objects. |

### 3.4 Models and repository core

| Current area | Decision | v2 treatment |
| --- | --- | --- |
| `types/models.ts` | **Rewrite** | Separate strict repository types from device/API types. Remove settings `activePackageId`, revisions, package API objects, and plural execution models. |
| current package/macro models | **Adapt** | Package and macro field shapes remain useful only inside the strict React repository model. IDs must be canonical lowercase UUID v4 and macro IDs globally unique. |
| repository provider/state | **Add** | Hold the loaded or new repository exclusively in memory; track clean snapshot bytes/state and dirty transitions. |
| schema validator | **Add** | Enforce exact fields, JSON-safe plain objects, byte limits, UUIDs, uniqueness, and macro-language validation. |
| compression/import/export | **Rewrite/Add** | Use browser `CompressionStream("gzip")` and `DecompressionStream("gzip")`; no uncompressed fallback. |

### 3.5 Package and macro features

| Current feature | Decision | v2 treatment |
| --- | --- | --- |
| `PackageSelectionPage.tsx` | **Rewrite** | Remove `localStorage` recent-package state and firmware `selectPackage`; use device-wide opaque `lastSelectedPackageId`. Selection does not dirty repository data. |
| `PackageManagementPage.tsx` | **Rewrite/Adapt** | Preserve user workflows but perform create/rename/duplicate/reorder/delete locally with dirty-state protection. |
| `MacroLibraryPage.tsx` | **Rewrite/Adapt** | Hide source by default, add reveal control, Quick Send, inline progress/cancel/acknowledgement, and no mandatory route change. |
| `MacroEditorPage.tsx` | **Rewrite/Adapt** | Preserve useful editing controls; remove firmware CRUD/revision and server validation. Use local TypeScript validation and working-copy updates. |
| package import/export settings feature | **Delete/Rewrite** | Replace package import/export with complete repository gzip import/export and snapshot workflows. |

### 3.6 Execution features

| Current feature | Decision | v2 treatment |
| --- | --- | --- |
| `ConfirmExecutionPage.tsx` | **Delete as required flow / adapt as optional preview** | No ordinary Send navigation. Optional preview may reuse presentation concepts but submits source/timing only. |
| `ExecutionPage.tsx` | **Delete/merge** | Polling, serial-confirmation display and cancellation become inline Macros-page state. |
| `ExecutionResultPage.tsx` | **Delete/merge** | Completion, failure, timeout, cancellation and release errors become inline acknowledgements. |

### 3.7 Settings, diagnostics, responsive and accessibility

| Current area | Decision | v2 treatment |
| --- | --- | --- |
| `SettingsPage.tsx` | **Rewrite/Adapt** | Remove active-package policy and revisions. Add Quick Send/Always Preview, advisory retention target, source-preview visibility, exact network/auth/device actions, and dirty-work protection. |
| `DiagnosticsPage.tsx` | **Adapt** | Render the fixed v2 schema and filter copied/downloaded content. |
| existing CSS/mobile layout | **Adapt** | Preserve useful mobile-first tokens; implement 320 px minimum, 44 px targets, safe areas, tablet/desktop layouts, and portrait-required phone behavior. |
| drag-and-drop-only reordering | **Prohibit** | Always provide Move first/up/down/last controls. |

### 3.8 Browser persistence

| Current use | Decision | v2 treatment |
| --- | --- | --- |
| `localStorage` recent package state in `PackageSelectionPage.tsx` | **Delete** | Package selection is device-wide NVS state. |
| any repository data in localStorage/sessionStorage/IndexedDB/Cache Storage/service worker | **Delete/Prohibit** | Add architectural tests and retain only unrelated presentation preferences when allowed by the specifications. |
| `scripts/check-frontend-persisted-state.sh` | **Adapt/Strengthen** | Detect repository fields, IDs, names, source, JSON, compressed bytes, IndexedDB, Cache Storage and service-worker persistence. |

## 4. Test inventory and migration

### 4.1 Native host tests (`tests/host`)

| Test family | Decision | v2 treatment |
| --- | --- | --- |
| support, operation result, lifecycle/subsystem health | **Retain/Adapt** | Align error/health enums and diagnostics. |
| macro parser/model | **Retain/Adapt** | Remove package repository model assumptions; consume shared conformance corpus. |
| executor and executor health | **Retain/Adapt** | Singular send, confirmation timeout, deadlines, no queue, release-all. |
| auth and auth health | **Adapt** | New PBKDF2 metadata, session bounds, rate limits and lockout. |
| USB and USB health | **Retain/Adapt** | v2 ready gating and HID evidence. |
| Wi-Fi, provisioning, device controls, app core | **Adapt** | New settings schema, startup, reset semantics and exact contracts. |
| generic web security/static/adapter tests | **Retain/Adapt** | Exact v2 error/content/body/auth policies. |
| repository/package JSON, indexes, locks, package operations, backup/restore/import/replace | **Delete** | Replace with opaque blob list/add/load/delete/capacity/recovery tests. |
| storage mount/fs/atomic primitives | **Retain/Adapt** | Reuse only generic pieces; add v2 `.gz.tmp` failure matrix and no-format policy. |
| web package/macro/plural execution route tests | **Delete/Rewrite** | Add exact v2 route contracts and negative tests for old paths. |

### 4.2 Vitest (`webapp/tests`)

| Test family | Decision | v2 treatment |
| --- | --- | --- |
| auth/setup/session tests | **Adapt** | Exact v2 objects and startup sequence. |
| package/API CRUD tests | **Delete/Rewrite** | Test local repository operations and no firmware CRUD requests. |
| macro editor tests | **Adapt** | Local parser, bytes, dirty state, no server validation. |
| confirmation/execution/result route tests | **Delete/Rewrite** | Test inline Quick Send state machine and optional preview. |
| settings/diagnostics tests | **Adapt** | Exact v2 settings and fixed diagnostics. |
| fixtures and API guards | **Rewrite** | Strict repository and API fixtures, unknown-field rejection, no `activePackageId`. |
| `setup.ts` storage mocks | **Adapt** | Preserve test isolation while prohibiting repository persistence. |

### 4.3 Browser tests (`webapp/tests/browser`)

| Area | Decision | v2 treatment |
| --- | --- | --- |
| browser runner and static-server harness | **Retain/Adapt** | Keep real-browser execution; extend for Compression Streams and mobile viewports. |
| current v1 workflows | **Rewrite** | Add startup matrix, automatic newest snapshot, recovery, package selection, Quick Send inline states, unsaved warnings, manual snapshots, source privacy and portrait safety. |

### 4.4 On-device Unity (`firmware/test_app`)

**Adapt.** Preserve the test harness and hardware access, remove package/repository
cases, and add v2 blob/settings/send/security cases. Building is not execution
evidence.

### 4.5 Hardware scripts (`tests/hardware`)

**Adapt.** Preserve useful USB report capture, serial-console, reboot, Wi-Fi and
power-cycle harnesses. Replace whole-package, restore/import/replace workflows
with source/timing send, blob byte-identity, interrupted upload, full partition,
manual deletion, authentication bounds, and Android workflow evidence.

## 5. Schemas, fixtures and generated artifacts

| Area | Decision | v2 treatment |
| --- | --- | --- |
| v1 package/repository JSON schemas and examples | **Delete** | They are not compatibility inputs. |
| shared v2 repository fixtures | **Add** | Valid compact example, boundary and invalid fixtures. |
| shared API examples | **Add** | One exact example per route plus standard error object. |
| shared macro conformance corpus | **Retain/Adapt or replace** | One checked-in format consumed by C and TypeScript. |
| generated flash manifest and webfs image tools | **Adapt** | Continue deterministic images and verify new partition budgets. |

## 6. Scripts and CI

| Area | Decision | v2 treatment |
| --- | --- | --- |
| `scripts/check-all.sh` | **Retain/Adapt** | Remains authoritative and fail-fast. Update constituent checks for v2. |
| toolchain, format, lint, static analysis, secret logging, USB identity | **Retain/Adapt** | Preserve warning-as-error policy and update paths/contracts. |
| layer/removed-feature checks | **Rewrite** | Forbid firmware package/macro repositories, old routes, `activePackageId`, repository parsing/compression, automatic snapshot deletion, mandatory send navigation, and browser repository persistence. |
| partition/release-budget/webfs scripts | **Adapt** | Enforce 128 KiB candidate blob and image/partition margins. |
| GitHub Actions workflows | **Adapt** | Continue invoking the same local gates; update artifacts and hardware-independent v2 suites. |

## 7. Documentation

| Document family | Decision | v2 treatment |
| --- | --- | --- |
| `docs/SPEC_V2.md`, `docs/UI_UX_SPEC_V2.md`, `docs/TODO_V2.md` | **Retain** | Authoritative v2 set. Do not silently amend during implementation. |
| `docs/SPEC.md`, retired TODOs, proposals, old handoffs and implementation reports | **Retain as history only / remove authority references** | Never use as requirements. Clearly label historical context where retained. |
| `README.md` | **Rewrite/Adapt** | Point to v2 authority, distinguish current v1 implementation from target v2 until migration is complete, remove stale product claims. |
| `CLAUDE.md` | **Rewrite/Adapt** | Point to both specs and TODO v2, remove old authority and stale active-work instructions. |
| `docs/TODO.md` | **Rewrite** | Point exclusively to `TODO_V2.md`. |
| development/API/test/hardware docs | **Adapt** | Match exact v2 commands, routes, evidence and device-port behavior. |

## 8. Explicit v1 behavior disposition

| v1 behavior | Decision |
| --- | --- |
| firmware package repository and macro repository | **Delete** |
| active package inside repository/settings revision model | **Delete**; use opaque NVS `lastSelectedPackageId` |
| package/macro CRUD HTTP routes | **Delete** |
| firmware macro validation route | **Delete** |
| repository/package JSON parsing in firmware | **Delete** |
| package backup, restore, import, export and replace | **Delete** |
| repository indexes, revisions, ETags and optimistic concurrency | **Delete** |
| plural execution API | **Delete**; replace with singular send API |
| mandatory preview → progress page → result page navigation | **Delete** as primary workflow |
| per-browser recent-package localStorage | **Delete** |
| automatic snapshot creation or deletion | **Prohibit** |
| low-level mount, atomic I/O and static-server safety | **Adapt and retain only after v2 proof** |
| macro parser, executor, USB HID, auth, Wi-Fi and provisioning | **Adapt and revalidate** |

## 9. Migration ordering constraints

1. Establish shared v2 contracts and failing tests before deleting production
   modules.
2. Remove firmware repository ownership before introducing opaque blobs.
3. Introduce singular send contracts before replacing the React workflow.
4. Build the React repository core before page-level CRUD migration.
5. Do not delete useful v1 tests until equivalent v2 failure coverage exists.
6. Do not claim Phase 0 complete until the local baseline gate and exact counts
   are recorded from a clean checkout.
7. Do not claim hardware behavior from host tests, CI builds, or this inventory.
