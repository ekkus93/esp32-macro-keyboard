# TODO: align the implementation with SPEC.md (2026-08-02 revision)

**Document status:** Execution plan
**Governs:** `docs/SPEC.md` as amended 2026-08-02 (commits `ff91125`, `c5cd4a2`)
**Supersedes for this work:** `docs/TODO.md`, which plans the pre-2026-08-02 product
**Created:** 2026-08-02

## What this is

`docs/SPEC.md` was amended on 2026-08-02 to describe the product that was
actually asked for: a generic USB HID keyboard that stores macro sets, where a
set is **a name and an ordered list of macros**. The implementation has not
caught up. This document is the ordered list of work to close that gap.

Two phases are already done and are recorded here only so the sequence reads
straight; do not redo them.

## Rules

These come from `CLAUDE.md` and apply to every item below.

1. **No checkbox is ticked without evidence** — the commit SHA, the exact
   command run, and its result. "Builds clean" is not evidence; `EXIT=0` from a
   named script is.
2. **`./scripts/check-all.sh` must exit 0 before any commit.** A commit that
   fails the gate is not a partial step, it is a defect.
3. **No suppression.** If a removal makes a warning appear, fix the cause. New
   entries in `docs/STATIC_ANALYSIS_EXCEPTIONS.md` require a written reason.
4. **The stack-usage ratchet is load-bearing.** Deleting code changes inlining
   and has already pushed two frames over their limits in this work. When
   `scripts/stack-usage-allowlist.txt` fails, heap-allocate the large local —
   do not record the larger number.
5. **Work directly on `master`, forward commits only.** No force-push, no
   history rewrite.
6. **Do not claim hardware validation** from a host test or a CI build.

## Sequencing rationale

The order below is not arbitrary, and reordering it costs real work:

- **Procedures before the storage rewrite.** Most of the transaction and
  reference-integrity machinery exists to keep procedure steps pointing at real
  macros. Rewriting storage first means porting procedures into the new layout
  and then deleting them.
- **Transactions before the layout change.** The flat layout's whole claim is
  that one atomic `rename()` replaces the transaction machinery. Removing the
  machinery first makes that claim testable instead of aspirational.
- **Restore last.** Its defect (task-watchdog reset) is a symptom of the tree
  rewrite. Phases 3 and 4 remove the cause; patching it earlier is wasted work.
  This is the standing instruction from `docs/HANDOFF_2026-08-02_SIMPLIFICATION.md`.

## Measurements

Taken 2026-08-02 at commit `52d74dc`. They exist so a phase that turns out
5x larger than stated gets questioned rather than absorbed.

| Area | Size |
| --- | --- |
| `firmware/components/storage` | 14,557 lines |
| Procedure/progress dedicated firmware files | 1,250 lines |
| Firmware files referencing procedures/progress | 41 |
| Transaction, atomic-recovery, and tree-walker files | 3,891 lines |
| `webapp/src/features/procedures/` | 1,082 lines |
| Webapp files referencing procedures/progress | 29 |
| Host test files referencing procedures/progress | 23 |

---

## Phase 0 — Done

- [x] **0.1 Buttons and added hardware removed.** `confirm` and `cancel` are
  serial-console commands; confirmation defaults off and every gated route
  honours the setting. Commit `5e6a381`. SPEC §19.
- [x] **0.2 PSRAM enabled** (ESP32-S3R8, octal). Free heap 208,804 → 8,465,455.
  Commit `863956f`. SPEC §5.2.
- [x] **0.3 Quarantine removed**; corrupt files are deleted and the failure
  reported. Commits `a6ec5a3`, `52d74dc`. SPEC §13.6.
- [x] **0.4 Global/shared macros removed**; every macro belongs to one set.
  Commit `a6ec5a3`. SPEC §7.2.

---

## Phase 1 — Trim the object model

Small, self-contained, and it shrinks every serializer that later phases touch.
Do it first so those phases are diffing less code.

**Depends on:** nothing.
**Estimated size:** ~300 lines across ~15 files.

- [x] **1.1 Remove Chromebook-era set metadata.** `a8ac67f`. Delete `description`,
  `manufacturer`, `model`, and `board` from `macro_set_t`
  (`firmware/components/macro_model/include/macro_model.h`), from the set JSON
  reader/writer (`storage_repository_objects_json.c`), from the web API resource
  parser and emitter (`web_api_json.c`, `web_api_handler_common.c`), from
  `MacroSet` in `webapp/src/types/models.ts` and its guard, and from every test
  fixture and package literal.
  SPEC §12.1. These four fields exist only because the founding spec mistook one
  user's Chromebook workflow for the product.
- [x] **1.2 Remove `keyboard_layout` from the set.** `a8ac67f`. Version 0.1 is US English
  device-wide (SPEC §10.1); a per-set copy is a field nothing reads.
- [x] **1.3 Remove `sort_order` from the set.** `a8ac67f`. Set order is `index.json`'s
  ordered `set_ids` array and nothing else. Two sources of truth for order is
  how order gets silently lost. SPEC §12.1, §12.3.
- [x] **1.4 Remove `favorite` from `macro_t`.** `a6eb528`. The user orders the list; a
  favourite flag is a second, weaker ordering. SPEC §12.2.
- [x] **1.5 Update the schemas.** `a6eb528`. `docs/schemas/macro-set-package.schema.json`
  and `docs/schemas/all-data-backup.schema.json`.

**Done means:** the four objects in SPEC §12 match the structs byte for byte, no
struct carries a field no code reads, and `check-all.sh` exits 0.

---

## Phase 2 — Remove procedures, steps, and progress

The largest single cut, and the one that makes Phase 3 tractable. A macro set is
a name and an ordered list of macros; there is no level between them.

**Depends on:** Phase 1.
**Size:** ~1,250 lines of dedicated firmware code, plus references in 41
firmware files, 29 webapp files, and 23 host test files.

**This phase lands in three commits, not one.** An attempt on 2026-08-02 tried
it as a single cut, ran out of session with the tree not compiling, and was
stashed rather than committed (`WIP Phase 2: procedures/progress removal,
incomplete (does not build)`). The seams below are where that attempt found the
build goes green on its own; they are the whole reason this phase is written
this way.

Before starting, read `## Phase 2 field notes` at the end of this section.

### 2a — Web API layer (first commit) — DONE `4ab338c`

Compiles green **before** the model types are touched, because nothing here
outlives `procedure_t`. Land it on its own.

- [x] Delete `web_api_procedures.c` and its `HANDLER_PROCEDURES` dispatch arm,
  and remove the source from `firmware/components/web_server/CMakeLists.txt` and
  from `web_api_repository_handlers_tests` in `tests/host/CMakeLists.txt`.
- [x] Delete `WEB_API_ROUTE_SET_PROCEDURES`, `_SET_PROCEDURE`,
  `_SET_PROCEDURES_REORDER`, `_PROCEDURE_PROGRESS`, `_PROGRESS_COMPLETE`,
  `_PROGRESS_SKIP` from `web_api_core.h`, plus `match_set_procedure_routes`,
  the `"procedures"` arms in `match_set_routes`, the method policy entries, the
  body-limit entries, and `has_procedure_id`/`procedure_id` from
  `web_api_path_t`.
- [x] Delete `web_api_json_parse_procedure_resource`,
  `_parse_progress_resource`, `_parse_progress_action`,
  `web_api_progress_action_t`, and `read_execution_source_context`. The
  execution submit body becomes exactly `{setId, macroId, macroRevision}`.
- [x] Delete `has_procedure_context`, `procedure_id`, `step_id`, and
  `procedure_read` from `web_execution_submit_request_t`/`web_execution_ops_t`,
  and `validate_procedure_context`/`procedure_context_matches` from
  `web_execution_submit.c`. An execution is a macro and a set (SPEC §18).
- [x] Delete `web_api_handler_procedure_json`, `_procedure_list_json`,
  `_progress_json`, and `procedure_summary` from `web_api_handler_common.c`.
- [x] Add a negative assertion that `/api/v1/sets/{id}/procedures` no longer
  resolves — the same class of leftover as the quarantine diagnostics route,
  which parsed for a full commit after its handler was deleted (`52d74dc`).

**Gate:** `./scripts/check-all.sh` exited 0 (58/58 host, 131/131 webapp).

Note for 2b: set duplication was covered only inside
`test_procedure_and_progress_routes`. That coverage moved to
`test_set_delete_and_persistent_readback` rather than being dropped —
check for the same pattern before deleting any other procedure test.

### 2b — Storage and model (second commit)

Nothing here compiles until all of it is done, so this is one commit by
necessity. Work in the order given: the CMake edits must come first or the
build fails to *configure* and hides every compile error behind it.

**Attempt 2 (2026-08-02) got the firmware roughly 80% cut and stopped.** The
work is on `stash@{0}` ("WIP 2b: …, firmware ~80% (does not build)"). What is
already done there, and what is left, in order:

**Done in the stash:** all CMake edits; the eleven file deletions (including
both tree-walkers and their internal headers); `storage_repository.h` trimmed
to SPEC §12; `macro_model.h` trimmed and `macro_model_free_procedure` gone;
the reference-integrity scan removed from `storage_repository_macros.c` and the
conflict path from `web_api_macros.c`; `storage_repository_objects_json.c` and
`storage_object_json.h` stripped; `macro_limits.h` procedure limits removed;
`web_server_adapter_json.c` limits response fixed;
`storage_atomic_validators.{c,h}` procedure/progress object kinds removed.

**Left to do, in dependency order:**

1. `storage_atomic_validators.c` — `validate_index` is now unused; it was only
   reachable through the procedure-order object kind.
2. `storage_package.c` — `validate_procedure_object`, `validate_progress_object`,
   `procedure_macro_references_valid`, `procedure_has_step`,
   `progress_step_references_valid`, the two field enumerators, the two array
   slots, the two summary counters, and their allocation-table entries.
3. `storage_package_export.c`, `_backup.c`, `_import.c`, `_replace.c`,
   `_restore.c` — the procedure/progress writers and readers, plus
   `prune_dangling_procedures` / `procedure_included` in `_backup.c`.
4. `storage_repository_set_operations.c` — `write_duplicate_procedure` and the
   procedure half of `create_duplicate_staging`.
5. `include_progress` on the three export entry points.
6. The ~20 host test files.

**A decision this attempt made, which needs stating in the commit message:**
the tree-walkers (`storage_set_tree.c`, `storage_repository_tree.c`, 1,517
lines) were **deleted rather than rewritten**, per the field note. Their callers
in `storage_package_import.c`, `_replace.c`, and `_restore.c` lose their
post-materialization tree-shape re-check. The incoming package is still fully
validated by `storage_package.c` before anything is written, so what is lost is
the belt-and-braces second check of a directory layout that Phase 4 deletes
outright. Say so plainly in the commit rather than letting it pass silently.

- [ ] **CMake first.** Delete the `storage_procedure_repository_tests` and
  `storage_progress_repository_tests` targets and every reference to
  `storage_repository_procedures.c` / `storage_repository_progress.c` in
  `tests/host/CMakeLists.txt`, `tests/host/cmake/extra_tests.cmake`, and
  `firmware/components/storage/CMakeLists.txt`. Delete
  `tests/host/test_storage_procedures.c` and `test_storage_progress.c`.
- [ ] Delete `storage_repository_procedures.c`,
  `storage_repository_progress.c`, and
  `storage_repository_procedures_internal.h` (1,250 lines).
- [ ] Delete from `storage_repository.h`: `storage_procedure_list_t`,
  `storage_procedure_identity_t`, `storage_progress_status_t`,
  `storage_progress_snapshot_t`, `storage_reference_list_t`,
  `STORAGE_REFERENCE_DETAIL_MAX_IDS`, and all eleven procedure/progress entry
  points. `storage_macro_delete` loses its `out_references` out-parameter.
- [ ] Delete the reference-integrity scan from `storage_repository_macros.c`
  (`step_references_macro`, `add_reference`, `parse_procedure_filename`,
  `read_reference_scan_procedure`, `procedure_references_macro`,
  `scan_procedure_entry`, `scan_set_procedure_references`,
  `find_macro_references`, `procedure_reference_scan_t`) and the
  `APP_ERROR_CONFLICT`-on-referenced-macro path, plus `reference_details_json`
  and the `"macro is referenced by procedures"` response in `web_api_macros.c`.
  **This is the single largest reason the transaction machinery exists** —
  removing it is what makes Phase 3 possible.
- [ ] Delete `procedure_t`, `procedure_step_t`, `procedure_progress_t`,
  `procedure_step_type_t`, and `macro_model_free_procedure` from
  `macro_model.h`/`macro_model.c`, and `MACRO_KEYBOARD_LAYOUT_BYTES`, which
  Phase 1 left unused.
- [ ] Delete `APP_PROCEDURES_PER_SET_MAX` and `APP_STEPS_PER_PROCEDURE_MAX`
  from `macro_limits.h` and from the `/api/v1/limits` response. SPEC §10.7.
- [ ] Delete `STORAGE_PROCEDURE_FILE_MAX_BYTES`,
  `STORAGE_PROGRESS_FILE_MAX_BYTES`, and the now-vacuous
  `STORAGE_ORDER_MAX_IDS >= APP_PROCEDURES_PER_SET_MAX` assertion from
  `storage_object_json.h`; delete the procedure/progress/step parsers,
  serializers, and field tables from `storage_repository_objects_json.c`.
- [ ] Delete `procedures` and `progress` from the package format
  (`storage_package.c`: two field enumerators, two array slots, two summary
  counters, `validate_procedure_object`, `validate_progress_object`,
  `procedure_macro_references_valid`, `procedure_has_step`,
  `progress_step_references_valid`, and their allocation-table entries), from
  `storage_package_summary_t`, and from `storage_package_object_kind_t`.
- [ ] **Remove `include_progress`** from `storage_package_export_set`,
  `storage_package_export_backup`, and `storage_package_export_backup_detail`.
  It threads through the whole export API and becomes meaningless.
- [ ] Delete `prune_dangling_procedures` and `procedure_included` from
  `storage_package_backup.c`; delete the procedure/progress writers from
  `storage_package_import.c`, `_replace.c`, and `_restore.c`; delete
  `write_duplicate_procedure` and the procedure half of
  `create_duplicate_staging` from `storage_repository_set_operations.c`.
- [ ] Delete `procedure-order.json`, `procedures/`, and `progress/` from
  `storage_paths.c`, `storage_mount_topology.c`, and
  `storage_atomic_validators.c` (including the
  `STORAGE_ATOMIC_OBJECT_PROCEDURE*`/`_PROGRESS` object kinds).
- [ ] **`storage_set_tree.c` and `storage_repository_tree.c`: do not rewrite.**
  See the field notes below.
- [ ] Strip procedure and progress fixtures from the remaining 21 host test
  files.

**Gate before committing:** `./scripts/check-all.sh` exits 0.

### 2c — Web application (third commit)

Independent of the firmware once 2a has landed, because the API surface is
already gone.

- [ ] Delete `webapp/src/features/procedures/` (1,082 lines).
- [ ] Delete `Procedure`, `ProcedureSummary`, `ProcedureStep`,
  `ProcedureProgress` and their guards from `types/models.ts` and
  `api/guards.ts`; delete the procedure and progress functions from
  `api/routes.ts`.
- [ ] Remove the Procedures entry from the bottom navigation and the
  `#/procedures` route; the nav becomes `Macros | Sets | Settings`. SPEC §9,
  §9.1.
- [ ] Remove procedure context from the confirm-execution flow.
- [ ] Update the screen list to SPEC §9's fifteen screens.
- [ ] Delete `tests/app-procedures.test.tsx` and the procedure fixtures in
  `tests/appFixtures.ts`.
- [ ] Remove procedure and progress routes from `docs/API.md`, and the
  `procedure`/`progress` definitions from
  `docs/schemas/macro-set-package.schema.json`.

**Gate before committing:** `./scripts/check-all.sh` exits 0.

### Phase 2 field notes

From the abandoned 2026-08-02 attempt. These are the things that cost time.

**The tree-walkers are a trap.** `storage_set_tree.c` (674 lines) produced 105
compile errors the moment `procedure_t` disappeared — more than any other file,
by a factor of four. Both it and `storage_repository_tree.c` (843 lines) exist
to validate a per-set **directory tree**, and **Phase 4 deletes both outright**.
Rewriting them here means doing the work twice. Prefer: stop calling them in
2b (the callers are `storage_package_replace.c`, `storage_package_import.c`,
and `storage_package_restore.c`) and let Phase 4 delete the files. If that
proves impossible, say so in the commit message rather than quietly rewriting
1,500 lines that are about to be deleted.

**CMake fails before the compiler runs.** Deleting a `.c` file that a test
target names produces `No SOURCES given to target: …` at *configure* time, which
aborts the build before a single compile error is printed. Every real error
stays invisible until the CMake edits land. Do them first — that is why 2b
lists them as step one.

**Compile-error counts after the model types go**, as a work estimate:
`storage_set_tree.c` 105, `storage_repository_objects_json.c` 24,
`storage_atomic_validators.c` 22, `storage_package_import.c` 18,
`storage_package_replace.c` 14, `web_execution_submit.c` 2, `macro_limits.h` 2.

**Watch the stack ratchet.** Phase 1 tripped it three times: shrinking a struct
changes inlining, and a ~16 KB `procedure_progress_t` or ~4 KB
`storage_uuid_order_t` that used to sit in a separate frame suddenly lands in
its caller. Phase 2 deletes `procedure_progress_t` entirely, so the pressure
should fall — but check, do not assume, and fix at source.

**Done means:** `grep -ri "procedure\|progress" firmware webapp/src tests/host`
returns only execution-progress hits (SPEC §18's `action index and total`), and
`check-all.sh` exits 0.

## Phase 3 — Remove transactions, staging, and trash

**Depends on:** Phase 2. Attempting this first means preserving reference
integrity across a tree rewrite, which is most of what the machinery does.
**Estimated size:** ~2,000 lines removed of the 3,891 measured; the atomic
write primitive stays.

**Keep, do not delete:** `storage_atomic.c` (397 lines) is the `.tmp` +
`rename()` primitive that SPEC §13.4 requires. Only the multi-file transaction
layer above it goes.

- [ ] Delete `storage_transaction.c` (677 lines) and
  `storage_transaction_restore.c` (335 lines): manifests, phases, staging
  activation, and startup transaction recovery.
- [ ] Delete `/data/staging/`, `/data/trash/`, and `/data/transactions/` from
  `storage_mount_topology.c` and every path helper. SPEC §13.3.
- [ ] Reduce `storage_atomic_recovery.c` (491 lines) to SPEC §13.4's entire
  recovery routine: **delete any `*.tmp` under `/data`**. The reconcile
  decision table, artifact enumeration, backup restoration, and
  `.bak` handling all go with it — there is no `.bak` file in the new design.
- [ ] Delete `storage_atomic_validators.c` (360 lines). It validates candidate
  files by object type during recovery; with recovery reduced to "delete stray
  `.tmp`", nothing calls it.
- [ ] Change set deletion to remove the set file and update the index, with no
  trash rename. SPEC §8.6 — deletion is permanent, and the typed-name
  confirmation is the safeguard.
- [ ] Change set import/replace to a single `rename()` over the target set file.
  SPEC §8.7.

**Done means:** `/data` contains only `index.json` and `sets/`; boot recovery is
one function that unlinks `*.tmp`; `check-all.sh` exits 0.

**Test coverage this phase must add** (SPEC §24.2): interruption between writing
`.tmp` and `rename()` in both orders, and boot cleanup of a stray `.tmp`.

---

## Phase 4 — Flat storage layout

The change the whole sequence exists for.

**Depends on:** Phases 2 and 3.
**Estimated size:** a rewrite of the repository layer; expect
`firmware/components/storage` to end well under 3,000 lines against today's
14,557.

Target layout (SPEC §13.3):

```text
/data/
├── index.json              schema version, active set, set order
└── sets/
    └── <set-id>.json       set name and its ordered macros
```

- [ ] **4.1 Write the new repository.** One file per set holding the set's name
  and its `macros` array inline. Read = parse one file. Write = serialize one
  file, `.tmp`, `rename()`.
- [ ] **4.2 Delete per-object paths and order files.** `macro-order.json`,
  `set.json`, `macros/`, and `storage_repository_order.c` all disappear: array
  order in the set file **is** the order. SPEC §12.1.
- [ ] **4.3 Delete the tree walkers.** `storage_repository_tree.c` (843 lines)
  and `storage_set_tree.c` (674 lines) validate a directory tree that will no
  longer exist.
- [ ] **4.4 Rewrite `index.json`** to SPEC §12.3: `schema_version`, `revision`,
  `active_set_id`, `set_ids`. Firmware MUST NOT reconstruct it from a directory
  listing — that discards the user's set order, the one thing it holds.
- [ ] **4.5 Migration: none.** There is no released version and no v0.1.0 tag.
  A device holding the old layout is reflashed. Do not write migration code.

**Done means:** a device with 3 sets of 5 macros reports `usedBytes` in the low
tens of KiB rather than 98,304, and `check-all.sh` exits 0.

---

## Phase 5 — Fix restore

**Depends on:** Phase 4. Do not attempt earlier.

`POST /api/v1/restore` has never worked. The stack overflow is fixed; it now
trips the task watchdog because it performs the whole tree rewrite
synchronously on the HTTP server task, starving the idle task for over five
seconds.

- [ ] **5.1 Restore writes each set file atomically** and is explicitly not
  atomic across sets. SPEC §13.5.
- [ ] **5.2 Restore runs on the worker task** (`web_server_async.c`), not the
  httpd task. SPEC §13.5.
- [ ] **5.3 The response reports per-set outcomes.** A partial restore
  enumerates which sets were written and which were not, and MUST NOT return
  `200` for a run that failed to write some of them. SPEC §17.
- [ ] **5.4 Prove it on hardware.** `tests/hardware/test_backup_restore.py`
  against a real device, with the transcript recorded. Host tests and CI builds
  do not count.

---

## Phase 6 — Enforce the storage budget

Today the limits table permits 50 sets × 100 macros × 4096 bytes = 20 MB on a
512 KiB partition. Nothing measures actual bytes.

- [ ] **6.1 Measure before committing a write** and reject an over-budget write
  with `507`, rather than filling the filesystem. SPEC §10.7, §17.
- [ ] **6.2 Publish remaining space** through `/api/v1/limits` or
  `/api/v1/diagnostics/storage` so the web app can gate the user before the
  request is made.
- [ ] **6.3 Add the nominal limits** (32 KiB per set file, 480 KiB total) to the
  centralized limits header, not as scattered literals. SPEC §10.7.

---

## Phase 7 — Close the remaining spec gaps

Small items that SPEC requires and the code does not yet do. Each is
independent; none blocks anything else.

- [ ] **7.1 Report *why* a file was discarded.** SPEC §13.6 requires the object
  **and its error** to be surfaced. Today the path is reported and the reason is
  not: `discard_progress`/`discard_procedure` took a `reason` string that was
  already being discarded, and the dead parameter was removed in `a6ec5a3`
  rather than left to imply it was used. These files compile for host tests too,
  so `ESP_LOG` is not available in them — this needs a decision about where the
  reason goes (returned error detail, a bounded in-RAM record, or a diagnostics
  field).
- [ ] **7.2 Add the diagnostics fields SPEC §20.3 now names:** stray temporary
  files removed at boot, and objects deleted as corrupt since boot with their
  paths and errors. Neither exists.
- [ ] **7.3 Reconcile `docs/TODO.md`.** It is 1,481 lines planning the
  pre-2026-08-02 product — procedures, buttons, the directory tree. Either
  retire it in favour of this document or rewrite it. Leaving two contradictory
  plans in `docs/` is how this project got here.
- [ ] **7.4 Audit the FIX1 documents** (`*_FIX1_*.md`, ~5 files) for tasks that
  reference removed subsystems, and mark them struck rather than leaving them to
  read as open work.

---

## Acceptance

The sequence is complete when all of SPEC §25 holds, and specifically:

1. `./scripts/check-all.sh` exits 0.
2. `/data` contains exactly `index.json` and `sets/`.
3. A macro set round-trips through write, reboot, export, and restore with its
   macro order byte-identical (SPEC §25.6).
4. `POST /api/v1/restore` completes on real hardware without a watchdog reset,
   and reports per-set outcomes.
5. No struct, route, screen, or schema field exists that SPEC §1.1 rejects.
6. `firmware/components/storage` is under 3,000 lines.

Item 6 is a proxy, not a goal — but a storage component still over 10,000 lines
after this sequence means something was preserved that should have been deleted,
and is worth stopping to explain.
