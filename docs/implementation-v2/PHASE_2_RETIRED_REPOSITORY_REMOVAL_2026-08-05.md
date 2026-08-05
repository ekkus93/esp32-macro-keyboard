# Phase 2 — Retired Repository Removal

**Status:** Complete  
**Task IDs:** V2-020, V2-021, V2-022  
**Implementation start SHA:** `757a4169fd40a22d0be791ad2fd1f44d4806b2b6`  
**Implementation evidence SHA:** `ce2434cff262d1fc04e963f7f30c34e996393d3e`  
**Evidence date:** 2026-08-05

## 1. Completion scope

Phase 2 removed the firmware-owned v1 package and macro repository architecture
before introducing the v2 opaque snapshot store. The firmware no longer owns,
indexes, validates, serializes, imports, exports, restores, or selects repository
packages or macros.

The completed Phase 2 tasks are:

- **V2-020:** delete firmware package and macro repositories;
- **V2-021:** delete obsolete HTTP resources;
- **V2-022:** delete obsolete tests and replace required coverage.

## 2. Production changes

### 2.1 Storage and startup

Removed the production v1 repository stack, including package and macro state,
active-package state, indexes, per-package and per-macro files, repository JSON,
revision and backup stores, restore/import/export/replace logic, repository
locking, repository recovery, and repository-specific incidents and health code.
The final orphaned `firmware/components/storage/storage_json.h` declaration file
was also deleted.

`app_core` no longer initializes, recovers, checks, locks, deinitializes, or cleans
up a firmware repository. Startup now retains only subsystems that have an
independent v2 responsibility.

The retained storage boundary contains only generic infrastructure:

- non-formatting LittleFS mounts for web assets and userdata;
- bounded filesystem wrappers;
- generic atomic file replacement primitives;
- storage mount state, health, and partition-capacity reporting.

### 2.2 HTTP API

Removed package CRUD, macro CRUD, firmware repository validation, repository
import/export/backup/restore/replace, repository-boundary, plural execution, and
stored-execution cancellation resources. Retired paths are not translated to a
new resource; route tests require them to return `404`.

The retained administration surface includes settings, password change, device
restart, reset settings, factory reset, and diagnostics. Phase 3 and later phases
will add the documented opaque blob and singular send resources.

### 2.3 Macro model and retained subsystems

Removed persisted package and macro structs and firmware active-package state.
Retained only execution-time source, timing, parser, executor, USB HID,
authentication, provisioning, Wi-Fi, static-server, and device-control behavior
that remains independently required by the v2 specifications.

## 3. Test and architectural coverage

Deleted tests and fixtures whose only purpose was the retired repository model,
including package backup/restore, package-order execution, repository fixtures,
and repository-bound cancellation/typing hardware scripts.

Preserved and adapted useful parser, executor, USB, authentication, Wi-Fi,
startup, static-server, reset, and HTTP concurrency coverage. Added or retained:

- negative route tests proving retired resources are absent;
- app-core startup failure and cleanup-path tests;
- strict request-ID, body-limit, JSON, and full-`uint32_t` parsing tests;
- `scripts/check-v2-phase2-architecture.py`, invoked by
  `scripts/check-all.sh`;
- forbidden-path and forbidden-symbol checks covering package repositories, macro
  repositories, `activePackageId`, retired CRUD routes, and firmware repository
  JSON declarations.

The architecture gate is fail closed. It does not suppress findings, translate
old APIs, or allow compatibility adapters.

## 4. Commands and results

The authoritative evidence was produced from a clean GitHub Actions checkout of
`ce2434cff262d1fc04e963f7f30c34e996393d3e` on Ubuntu 24.04.4.

| Command or gate | Result |
| --- | --- |
| `./scripts/run-tests.sh` | 33 of 33 native host tests passed; 0 failed |
| Host ASan and UBSan job | Passed the host suite with sanitizers |
| Native coverage job | Passed committed native coverage gates |
| `npm --prefix webapp test` | 26 of 26 files and 231 of 231 tests passed |
| Frontend typecheck, ESLint, stylelint, and Prettier | Passed with zero gate failures |
| Real Chrome workflows | Passed |
| ESP32-S3 device-test firmware build | Passed; build only, not device execution |
| `./scripts/check-all.sh` | Passed in the Quality workflow |

Toolchain verified by CI:

- ESP-IDF `v5.5.5`;
- target `esp32s3`;
- Node.js `v24.18.0`;
- Ubuntu `24.04.4` runner image.

## 5. Exact CI evidence

All primary workflows completed successfully on the implementation evidence SHA:

- [Browser Tests run 30989326830](https://github.com/ekkus93/esp32-macro-keyboard/actions/runs/30989326830)
- [Host Tests run 30989326802](https://github.com/ekkus93/esp32-macro-keyboard/actions/runs/30989326802)
- [Device Test Build run 30989327079](https://github.com/ekkus93/esp32-macro-keyboard/actions/runs/30989327079)
- [Quality run 30989326930](https://github.com/ekkus93/esp32-macro-keyboard/actions/runs/30989326930)

The Quality job completed `Run authoritative checks` successfully. Its failure-log
upload step was skipped because no failure occurred.

## 6. Hardware and observed-value applicability

No physical ESP32-S3R8, USB port, serial port, HID capture host, or Android device
was used for Phase 2 closeout. The Device Test Build workflow compiled firmware
but did not execute it on hardware. No physical-device claim is made from that
build.

Phase 2 is an architecture-deletion phase. It required no product timing, storage
capacity, memory, power-loss, persistence, or reconnect measurement. Those values
were therefore not collected or inferred from host fakes or CI builds.

## 7. Deferred work and limitations

The following work remains explicitly open and is not claimed by this report:

- Phase 3 opaque `/data/repository/` blob scanning and numeric ID allocation;
- atomic gzip upload, list, byte-identical download, and explicit deletion;
- capacity, `413`, `507`, interrupted-upload, and mount-failure behavior;
- physical power-cycle and storage hardware evidence;
- the singular `/api/v1/send` lifecycle and its hardware evidence;
- later React repository, snapshot, workflow, accessibility, and release phases.

## 8. Completion statement

Every Phase 2 checkbox in `docs/TODO_V2.md` is backed by the implementation and
reproducible evidence identified above. No unchecked task outside Phase 2 is being
claimed complete. No compatibility adapter, silent fallback, ignored error, or
warning suppression was introduced to satisfy the Phase 2 gate.
