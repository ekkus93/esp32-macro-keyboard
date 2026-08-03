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

**Done — commit `43dab8d`** (58 files, +185/-6,637). All items below landed in
one commit; `./scripts/check-all.sh` exited 0 and 54/54 host tests pass.

**The tree-walker decision, as taken:** `storage_set_tree.c` and
`storage_repository_tree.c` (1,517 lines) were **deleted, not rewritten**. The
four production validators that called them now return `APP_ERROR_NONE` with a
comment saying so, which means import, replace and restore lose their
post-materialization tree-shape re-check. The incoming package is still fully
validated by `storage_package_validate()` before a single byte is written, so
what is lost is the belt-and-braces second check of a directory layout Phase 4
deletes outright. The function-pointer seam was kept deliberately, so the
transaction recovery paths stay testable with an injected failing validator and
so Phase 4 removes the plumbing on purpose rather than by accident.

**Two latent defects were found and fixed while cutting**, both left by the
earlier partial pass and neither caught by the compiler:

- `storage_atomic_validators.c` — deleting the `PROCEDURE_INDEX` case had also
  deleted `return validate_index;`, so `SET_INDEX` and `SET_MACRO_INDEX` fell
  through to `validate_set_metadata`. Every order file would have been rejected
  as corrupt during recovery. (This is what the earlier note calling
  `validate_index` "unused" had actually observed.)
- `storage_package_backup.c` — the `fold_skips()` call for macros had been cut
  along with the procedure block surrounding it, which would have silently
  stopped reporting skipped macros in partial backups.

**Coverage preserved rather than dropped**, in three places where deleting the
procedure test would have taken unrelated assertions with it — the same trap
2a's note warned about:

- `test_storage_object_json.c` — the order round-trip lived inside the progress
  test; extracted as `test_order_round_trip`.
- `test_storage_package_export.c` — the output-limit test had been reworked in
  the previous session to overflow via procedures. A maximal set of plain macro
  sources is only ~446 KiB, so it now drives the 512 KiB writer ceiling with
  sources made of quote characters, which double on JSON escaping.
- `test_storage_atomic_validators.c` — the classifier still asserts a
  procedures path now classifies as `UNKNOWN`, rather than dropping the case.

**Stack ratchet:** `storage_set_duplicate_locked` hit 6448 bytes against an
allowed 4736, because the now-smaller `write_duplicate_order` began inlining
into it. Fixed at source by heap-allocating the ~3.7 KB `storage_uuid_order_t`,
not by recording a larger number. Two allowlist entries naming deleted frames
were removed.

- [x] **CMake first.** Delete the `storage_procedure_repository_tests` and
  `storage_progress_repository_tests` targets and every reference to
  `storage_repository_procedures.c` / `storage_repository_progress.c` in
  `tests/host/CMakeLists.txt`, `tests/host/cmake/extra_tests.cmake`, and
  `firmware/components/storage/CMakeLists.txt`. Delete
  `tests/host/test_storage_procedures.c` and `test_storage_progress.c`.
- [x] Delete `storage_repository_procedures.c`,
  `storage_repository_progress.c`, and
  `storage_repository_procedures_internal.h` (1,250 lines).
- [x] Delete from `storage_repository.h`: `storage_procedure_list_t`,
  `storage_procedure_identity_t`, `storage_progress_status_t`,
  `storage_progress_snapshot_t`, `storage_reference_list_t`,
  `STORAGE_REFERENCE_DETAIL_MAX_IDS`, and all eleven procedure/progress entry
  points. `storage_macro_delete` loses its `out_references` out-parameter.
- [x] Delete the reference-integrity scan from `storage_repository_macros.c`
  and the `APP_ERROR_CONFLICT`-on-referenced-macro path, plus
  `reference_details_json` and the `"macro is referenced by procedures"`
  response in `web_api_macros.c`.
- [x] Delete `procedure_t`, `procedure_step_t`, `procedure_progress_t`,
  `procedure_step_type_t`, and `macro_model_free_procedure` from
  `macro_model.h`/`macro_model.c`, and `MACRO_KEYBOARD_LAYOUT_BYTES`.
- [x] Delete `APP_PROCEDURES_PER_SET_MAX` and `APP_STEPS_PER_PROCEDURE_MAX`
  from `macro_limits.h` and from the `/api/v1/limits` response. SPEC §10.7.
- [x] Delete `STORAGE_PROCEDURE_FILE_MAX_BYTES`,
  `STORAGE_PROGRESS_FILE_MAX_BYTES`, and the now-vacuous
  `STORAGE_ORDER_MAX_IDS >= APP_PROCEDURES_PER_SET_MAX` assertion from
  `storage_object_json.h`; delete the procedure/progress/step parsers,
  serializers, and field tables from `storage_repository_objects_json.c`.
- [x] Delete `procedures` and `progress` from the package format, from
  `storage_package_summary_t`, and from `storage_package_object_kind_t`.
- [x] **Remove `include_progress`** from `storage_package_export_set`,
  `storage_package_export_backup`, and `storage_package_export_backup_detail`.
- [x] Delete `prune_dangling_procedures` and `procedure_included` from
  `storage_package_backup.c`; delete the procedure/progress writers from
  `storage_package_import.c`, `_replace.c`, and `_restore.c`; delete
  `write_duplicate_procedure` and the procedure half of
  `create_duplicate_staging` from `storage_repository_set_operations.c`.
- [x] Delete `procedure-order.json`, `procedures/`, and `progress/` from
  `storage_paths.c`, `storage_mount_topology.c`, and
  `storage_atomic_validators.c`.
- [x] **`storage_set_tree.c` and `storage_repository_tree.c`: deleted, not
  rewritten.** See the decision recorded above.
- [x] Strip procedure and progress fixtures from the remaining host test files.

**Gate before committing:** `./scripts/check-all.sh` exits 0.

### 2c — Web application (third commit)

Independent of the firmware once 2a has landed, because the API surface is
already gone.

**Done — commit `0b214fc`** (32 files, +69/-2,596). `./scripts/check-all.sh`
exited 0; 54/54 host and 118/118 webapp tests pass.

- [x] Delete `webapp/src/features/procedures/` (1,082 lines).
- [x] Delete `Procedure`, `ProcedureSummary`, `ProcedureStep`,
  `ProcedureProgress` and their guards from `types/models.ts` and
  `api/guards.ts`; delete the procedure and progress functions from
  `api/routes.ts`.
- [x] Remove the Procedures entry from the bottom navigation and the
  `#/procedures` route. The nav was **reordered** to `Macros | Sets | Settings`
  to match SPEC §9, not merely stripped of its middle entry.
- [x] Remove procedure context from the confirm-execution flow.
  `ExecutionConfirmationTarget` loses `sourceContext`, so `/confirm` now takes
  exactly one macro ID.
- [x] Update the screen list to SPEC §9's fifteen screens.
- [x] Delete `tests/app-procedures.test.tsx` and the procedure fixtures in
  `tests/appFixtures.ts`.
- [x] Remove procedure and progress routes from `docs/API.md`, and the
  `procedure`/`progress` definitions from
  `docs/schemas/macro-set-package.schema.json`. Also removed:
  `include_progress` from `docs/schemas/all-data-backup.schema.json`, and the
  now-orphaned `macro_step`/`manual_step` definitions that only the deleted
  `procedure` definition referenced.

**Two deletions made only after confirming nothing else used them:**
`pages/DeferredPage.tsx` existed solely as the procedure-editor stub, and
`.instruction-body` in `styles.css` was referenced only by the deleted
procedure workflow.

**Coverage kept rather than dropped:** `routing-confirmation.test.ts` had
asserted that a nested procedure context *parsed correctly*. That case is now
inverted into a rejection case — a stale bookmark carrying procedure context
must be refused outright, not silently reinterpreted as a plain macro
confirmation. The two identifiers it needs are declared locally in the test,
commented as the shape of a stale link rather than as fixtures.

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

**Done — commit `8b550c6`** (37 files, +590/-5,961). The storage component went
from 10,240 to 7,155 lines. `./scripts/check-all.sh` exited 0; 51/51 host tests
pass.

- [x] Delete `storage_transaction.c`, `storage_transaction_restore.c`,
  `storage_transaction_replace.c`, `storage_transaction_restore_recovery.c` and
  `storage_transaction_internal.h`.
- [x] Delete `/data/staging/`, `/data/trash/`, and `/data/transactions/` from
  `storage_mount_topology.c` and every path helper. SPEC §13.3.
- [x] Reduce `storage_atomic_recovery.c` (491 → 150 lines) to SPEC §13.4's
  entire recovery routine: delete any `*.tmp` under `/data`.
- [x] Delete `storage_atomic_validators.{c,h}`.
- [x] Set deletion removes the set and updates the index, with no trash rename.
  SPEC §8.6.
- [x] Set import/replace no longer stages and swaps. **See the caveat below.**

**`storage_atomic.c` was also simplified, 397 → 218 lines**, which the phase
brief implied ("there is no `.bak` file in the new design") without listing.
SPEC §13.4 permits nothing between verifying the temporary and renaming it, so
the `.bak` swap and its rollback ladder are gone. That ladder was not merely
redundant: renaming the destination aside before activating created a second
on-device copy of every object being written (SPEC §22, invariant 16) and a
window in which the canonical path did not exist at all. The temporary is now
`<target>.tmp` rather than `<target>.tmp.<uuid>` — every writer holds the
repository lock, so nothing can collide, and recovery can then be exactly
"unlink every `*.tmp`". The uuid-generator seam that existed for that naming is
removed from the atomic-write API.

**The ordering rule adopted at every mutation site:** the index is the
authority, so content is written before it is referenced and unreferenced
before it is removed. An interruption leaves an *unreferenced directory* —
invisible, because every reader enumerates through the index — rather than an
index entry pointing at a half-written or half-removed set.

**⚠ One place where crash-safety is temporarily weaker than what it replaced.**
Set replacement (`POST /api/v1/sets/import`) is not atomic in this phase. A set
is still a directory of files, so the old tree is removed and the new one
written in its place. An interruption leaves the set partially written while the
index still references it; it reads back as corrupt, which SPEC §13.6 handles by
reporting and deleting rather than silently substituting a default. The old
contents are gone either way, because the design forbids keeping a second copy
to roll back to. **Phase 4 closes this completely** — once a set is a single
file the whole replacement is one `storage_atomic_write`. The comment on
`replace_locked` says so at the call site.

**Restore now matches SPEC §13.5 instead of contradicting it.** It no longer
writes a manifest claiming an all-or-nothing guarantee across sets that the spec
explicitly does not want. Per-set outcome reporting and moving the loop off the
httpd task remain owed by Phase 5.

**Done means:** ✅ `/data` contains only `set-index.json` and `sets/`; ✅ boot
recovery is one function that unlinks `*.tmp`; ✅ `check-all.sh` exits 0.

**Test coverage this phase added** (SPEC §24.2), in
`test_storage_atomic_recovery.c` (rewritten) and `test_storage_parent_sync.c`:

- boot cleanup of a stray `.tmp`;
- a temporary whose destination never appeared is discarded, not activated;
- nested set directories are swept;
- only the exact `.tmp` suffix is removed — `tmp.json`, `a.tmp.json`,
  `b.json.tmpx` and `c.json.temp` all survive;
- a bare `".tmp"` names no destination and is left alone;
- interruption on either side of the `rename()`: old destination byte-for-byte
  intact on rename failure, nothing left behind on a failed create, and a
  parent-sync failure *after* the rename reported but **not** undone — undoing a
  committed rename is exactly the second-copy behaviour this phase removed.

**Two notes for later phases.** `storage_repository_discard_corrupt_file` was
accidentally cut alongside the manifest helpers it sat between and restored; the
linker caught it. `write_set_macros` hit the stack ratchet at 5152 bytes once it
began inlining into `materialize_sets`, fixed by heap-allocating the ~3.7 KB
`storage_uuid_order_t`.

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

### 4a — Flat layout — **Done, commit `317b795`**

30 files, +1,114/−1,575. Storage component 7,155 → 6,696 lines.
`./scripts/check-all.sh` exits 0; 51/51 host tests pass.

- [x] **4.1 Write the new repository.** `storage_repository_document.c` loads one
  set file into a `storage_set_document_t` and stores one back; every set and
  macro operation is written on top of those two functions. Macro create,
  update, delete, duplicate and reorder are each one write, where the old layout
  needed an object write plus an order-file write that could disagree with it.
- [x] **4.2 Delete per-object paths and order files.** `storage_repository_order.c`,
  `macro-order.json`, `set.json`, and `macros/` are gone. `storage_paths.c` drops
  from three path helpers to one. `schema.json` went too — the index carries
  `schema_version`, and a second file recording the same number is a second thing
  that can disagree.
- [x] **4.3 Delete the tree walkers.** Done in 2b; the vacuous validator seam
  they fed went with the transaction layer in Phase 3.
- [x] **4.4 Rewrite `index.json`** — *partially*. Renamed from `set-index.json`
  and rewritten to `schema_version` / `revision` / `set_ids`. **`active_set_id`
  is deliberately still missing — see 4b.**
- [x] **4.5 Migration: none.** No migration code was written.

**Two consequences worth recording:**

- **Replacement is atomic again.** Phase 3's one weak spot is closed: a set is
  one file, so `storage_package_replace_set` is a single `storage_atomic_write`
  and an interruption leaves either the complete old set or the complete new one.
- **A stored macro carries no `set_id`** (SPEC §12.2) — the file identifies the
  set, and parsing stamps it on. Packages and API responses still carry it as the
  envelope field §12.2 permits, so the macro JSON helpers survive and are now
  used only by the package layer.

**The size estimate was wrong, and here is why.** The phase predicted
`firmware/components/storage` well under 3,000 lines; it is 6,696. Roughly 2,900
of what remains is the package format (`storage_package*.c`), which 4a did not
touch — the estimate implicitly assumed the package layer shrank with the
repository, and it did not. The repository layer itself is now small.

**⚠ Not verified on hardware.** The "done means" measure below is a device
reading. By arithmetic the new layout should be ~8 KiB for `/data` + ~8 KiB for
`/data/sets` + one 4 KiB block per set file ≈ 28 KiB for 3 sets, against 98,304
before. That is a calculation, not a measurement. The device check is still owed
and is listed in Phase 5.4's hardware run.

### 4b — Move the active set into the index — **Done, commit `a2fdb4e`**

24 files, +300/−327. `./scripts/check-all.sh` exits 0; 51/51 host and 118/118
webapp tests pass. The index is now SPEC §12.3 in full.

- [x] `active_set_id` in `storage_set_index_t`, in the index JSON, and in its
  parse/serialize — including the rule that an active set absent from `set_ids`
  is a **corrupt index**, not a hint to drop silently.
- [x] `storage_set_select` and `storage_active_set_read`. Selection reads the set
  first, so it rejects an id that is in the index but whose file is missing or
  damaged; re-selecting the already-active set writes nothing.
- [x] Removed `has_active_set` / `active_set_id` from `provisioning_config_t`,
  `provisioning_settings_t`, and the NVS wire format, along with
  `provisioning_clear_active_set_if_matches`. `PROVISIONING_RECORD_BYTES`
  206 → 168.
- [x] `/api/v1/settings` and `/api/v1/sets/{id}/select` point at the index.

**What the move actually buys.** Deleting the active set now clears the
selection in the *same atomic index write* that removes the set from the order.
Before, that was a storage write plus an NVS commit — two durable operations
that could disagree if either failed.

**One deliberate API narrowing.** `GET /settings` still reports `activeSetId`
(clients want the whole operational state in one round trip), but `PUT
/settings` now **rejects** it as an unknown field. Accepting it would gate a
storage change on the settings revision, which is the two-authority problem
wearing a different hat. `POST /sets/{setId}/select` is the only way to move the
active set, and it no longer burns a settings revision. The webapp was only
echoing the current value back unchanged, so nothing it could do was lost.

**Old NVS blobs fail closed.** A 206-byte record read by this firmware trips the
existing exact-length check and is reported corrupt rather than misparsed —
confirmed by reading that path. No migration was written (4.5).

**A seam disappeared.** The host-side settings-ops hooks in
`storage_repository_sets.c` existed only to stub provisioning out of the set
repository. With the index owning the active set there is nothing to stub, so
the seam, its two test hooks, and the `ESP_PLATFORM` split around them are gone.

**Done means:** a device with 3 sets of 5 macros reports `usedBytes` in the low
tens of KiB rather than 98,304, and `check-all.sh` exits 0.

---

## Phase 5 — Fix restore — **Done (5.1–5.3), commit `89479b5`**

- [x] **5.1 Restore writes each set file atomically** and is explicitly not
  atomic across sets. It no longer stops at the first set it cannot write:
  aborting threw away every set after a failure for no reason, when each file is
  independently atomic. Only the sets actually written go into the index, so the
  index never names a set whose file is missing.
- [x] **5.2 Restore runs on the worker task.** **The watchdog bug was still
  live, and not where this item implied.** Restore *did* move to the worker —
  but only when physical confirmation was enabled, and confirmation is **off by
  default**. In the default configuration the whole write loop still ran on the
  httpd task. The offload predicate now has two independent reasons: a
  confirmation wait, or work long enough to trip the watchdog on its own
  (`web_api_route_requires_worker`: restore, import, import-new).
  `web_api_request_requires_confirmation` was renamed to
  `..._requires_worker`, because it had stopped being about confirmation.
- [x] **5.3 The response reports per-set outcomes.** A complete restore returns
  200 with `setsRestored`/`setsFailed` and the per-set list. A partial restore is
  **not** a 200 and goes out through the error envelope with the same per-set
  detail attached. The status comes from the *first* per-set failure, so storage
  exhaustion still reads as 507 instead of being flattened to 500.
- [x] **5.4 Prove it on hardware. DONE 2026-08-02.** `GET /api/v1/backup` then
  `POST /api/v1/restore` of that same package, against the attached ESP32-S3:
  `HTTP 200 in 2.65 s`, `restored=true`, 14 of 14 sets restored, 0 failed, per-set
  outcomes reported, device alive afterwards, repository byte-identical. That is
  acceptance criterion 4.

  It took two detours worth recording, because both were mine.

  **A real firmware defect, found and fixed on the way (`280d61a`).**
  `web_server_async_dispatch` handed requests to the worker without reading the
  body first, and esp_http_server gives an async handler the request but not its
  unread payload. Both worker-routed body-carrying routes -- restore and the set
  imports -- reached their handlers with `body_length` 0. Fixed by reading on the
  httpd task and carrying the buffer across, with the limit decision split from
  the read so the policy ordering `test_body_limit_precedes_headers` pins is
  unchanged.

  **The blocker itself was the harness, not the firmware.** `device_client.post`
  serialised with `json.dumps(body)`, whose defaults insert `", "` and `": "`.
  The device's package scanner rejects whitespace between tokens, so every
  restore attempt through the harness was 422 while the same document validated
  on the host -- where I had, without noticing, pasted the device's *compact*
  output into the probe. Posting the device's raw backup bytes unmodified
  succeeds; re-serialising the identical object with spaces fails. The frontend
  was never affected: `JSON.stringify` emits compact JSON.

  Two things follow. The harness now serialises compactly. And the parser's
  whitespace intolerance is recorded as 5.6 below, because a user who opens an
  exported backup in an editor and saves it gets a 422 with nothing to go on.

  **Fixed and proved on hardware 2026-08-02.** A macro with `ab{DELAY 3000}cd`
  was planted through the API (accepted, 201, because creation does not compile)
  and `GET /api/v1/backup` returned 200 with `skipped: {total: 1, items:
  [{kind: macro, id: ..., set_id: ...}]}` instead of the 422 it used to answer.
  The uncompilable macro is dropped at snapshot time and recorded, so the
  emitted package still validates -- which matters, because the export validates
  what it wrote and that validation compiles every macro.

  The underlying asymmetry stands and is worth its own item: creation accepts a
  source the device cannot compile. Backup now tolerates it; SPEC 3.10 would
  rather it were never stored.

- [x] **5.6 The package parser accepts whitespace between tokens. DONE.** Any
  pretty-printed package is refused with `422 invalid_argument` and no
  indication why. `docs/SPEC.md` does not require packages to be compact, and
  `GET /api/v1/backup` is a file a user can reasonably open, inspect, and save.
  **Fixed.** `read_plain_string` and `read_u32` were the only token consumers in
  the scanner that did not skip leading whitespace, so a value had to begin
  immediately after its colon -- which the device's own writer satisfies and no
  pretty-printer does. Both now skip, and a host test covers spaces, newlines
  and tabs. Proved on hardware: a backup re-serialised with `indent=2` restores
  with 200, `restored: true`, 14 of 14 sets.

- [x] **5.5 Backup is now tolerant of a damaged object (SPEC 17). DONE.**
  Found on the same run. One macro the device itself accepted at creation
  (`ab{DELAY 3000}cd` -- the parser wants `DELAY:`) made the whole repository
  unbackupable: `422 macro_syntax`, because `storage_package_validate` compiles
  every macro in the assembled package. SPEC 17 is explicit that an individually
  unusable object is omitted and recorded in `skipped` rather than failing the
  export, "because a backup is most needed exactly when storage is damaged".

  The host test that covers this (`test_unreadable_macro_is_skipped_and_recorded`)
  passes because its fake `macro_list` skips the fault internally. The real
  `storage_macro_list_detail_locked` deliberately does not -- its comment says a
  set file "parses as a whole or not at all" -- and the package-level compile
  check downstream fails the export outright. The fake models tolerance the
  implementation does not have.

  There is a second defect underneath it: **macro creation accepts a source the
  device can never compile.** Creation returned 201 for `{DELAY 3000}`, and it
  is only rejected later, on export. SPEC 3.10 requires rejecting malformed
  state rather than storing it. Either creation validates, or backup tolerates;
  at present neither does.

---

## Phase 6 — Enforce the storage budget — **Done, commit `dc62e68`**

- [x] **6.1 Measure before committing a write.** `storage_repository_measure_user_data`
  walks the sets directory and sums the files actually there, rather than
  trusting the per-object limits — which alone permit 50 × 100 × 4096 = 20 MB on
  a 512 KiB partition. The set being rewritten is excluded, because its old bytes
  are being replaced rather than added to. Over-budget is `APP_ERROR_STORAGE_FULL`
  (507), not `APP_ERROR_MACRO_LIMIT` (422): it is a capacity refusal, and 422
  would tell the user their object was malformed when it was not.
- [x] **6.2 Publish remaining space.** `GET /api/v1/diagnostics/storage` carries
  `usedBytes`, `totalBytes`, `remainingBytes`, `setFileMaxBytes`.
  `remainingBytes` is clamped at zero rather than underflowing.
- [x] **6.3 Add the nominal limits** — `APP_SET_FILE_MAX_BYTES` (32 KiB) and
  `APP_USER_DATA_MAX_BYTES` (480 KiB) in `macro_limits.h`.
  `STORAGE_SET_FILE_MAX_BYTES` now points at the nominal limit instead of the
  512 KiB package bound it had been borrowing.

---

## Phase 7 — Close the remaining spec gaps — **Done, commit `fe27378`**

- [x] **7.1 Report *why* a file was discarded.** New `storage_incidents.{c,h}`:
  a bounded in-RAM table of path + error. The decision the item asked for:
  **recorded, not logged and not persisted.** These files compile for host tests
  where `ESP_LOG` does not exist, and a log line is not reachable through the API
  anyway; persisting it would rebuild the quarantine mechanism this replaced,
  competing with the user's own data on a 512 KiB partition (SPEC §13.6).
  `storage_repository_discard_corrupt_file` regained the `reason` parameter that
  `a6ec5a3` removed — live this time, and recorded *before* the unlink so the
  incident survives even if the unlink fails.
- [x] **7.2 Add the SPEC §20.3 diagnostics fields** — `temporariesRemovedAtBoot`,
  `discardedObjectCount`, and `discardedObjects[]` with path and error.
- [x] **7.3 Reconcile `docs/TODO.md`.** Retired. Its 1,481 lines planned
  procedures, steps, checkpoints, progress, global macros, quarantine,
  transactions, and the per-set directory tree; building from it would
  reintroduce everything these seven phases removed. Replaced with a pointer to
  `SPEC.md` and this document; the original stays in git history.
- [x] **7.4 Audit the FIX1 documents.** Six files, ~5,900 lines, 356 references
  to removed subsystems. Each carries a header marking anything naming those
  subsystems as struck and pointing at SPEC §1.1, and naming what still applies:
  storage durability, the repository lock, auth, the executor, USB HID, Wi-Fi,
  the web server. Striking 356 individual lines would have been change for its
  own sake.

---

## Phase 8 — Test against the specification, not the implementation

Opened after the `expectedRevision` defect: six tests asserted a requirement the
handler invented, and hardware found it in a minute. `docs/SPEC_TEST_TRACEABILITY.md`
is the worklist; it is generated, and `check-docs.sh` fails if it drifts.

- [x] **8.1 SPEC §7.1 / §10.1 — set order and active-set selection.** Three
  tests written from the requirement. The third failed on its first run:
  `storage_set_reorder_locked` rebuilt the index from a blank struct and dropped
  `has_active_set` / `active_set_id`, so reordering silently deselected the
  active set. Phase 4b added those fields without auditing every writer. Fixed
  and covered in `e906f18`.
- [x] **8.2 SPEC §14 / §15.2 — station credentials.** Nine tests: absent is the
  initial state, credentials survive a power cycle, storing one disturbs nothing
  else in the record, storing replaces rather than accumulates, an empty SSID
  clears and actually erases the passphrase bytes, oversized input is refused
  without side effects, undersized output buffers are refused, and the flag and
  SSID must agree. All nine passed on the first run, so each was checked by
  mutating the source: dropping an unrelated field, and skipping the passphrase
  erase, each fail exactly one test. A tenth was written and then deleted —
  `test_corrupt_persisted_records` already asserted it.

- [x] **8.3 SPEC §8.1 / §8.4 / §17.** Three more prohibitions. §8.4 ("the next
  macro MUST NOT execute automatically") had no test: a new one drives every
  terminal outcome — completed, cancelled, failed — and asserts the engine never
  puts work on the queue that no caller submitted. Verified non-vacuous by
  mutation: making the engine re-queue after `plan_free` fails it at
  `executor_terminal_tests.inc:192`. §8.1 and §17 turned out to be already
  covered but uncited, so they gained citations plus the assertions that were
  missing (the shortest legal passphrase is still checked for WPA2/WPA3-PSK and
  PMF, so encryption is not something the weakest accepted credential trades
  away).

**Blind spot found in the tooling.** The generator scanned only
`tests/host/test_*.c`, so every citation in the 20 `.inc` fragments — where the
auth, executor, web-security, and web-server-adapter tests actually live — was
invisible and would have reported covered sections as unmapped. It now scans
those too.

### 8.4 SPEC §15.2 boot behaviour — proven on hardware

Not reachable from any host test: it lives in `app_core.c`, which has none. The
traceability matrix reports those §15.2 prohibitions as "referenced" only because
citations are section-level. Verified directly instead, on a **production** build
(manufacturing banner absent), after erasing NVS and re-provisioning:

```text
I (1272) wifi:mode : softAP (9c:13:9e:a8:77:39)
I (1352) esp_netif_lwip: DHCP server started on interface WIFI_AP_DEF with IP: 192.168.4.1
I (1452) wifi:mode : sta (9c:13:9e:a8:77:38) + softAP (9c:13:9e:a8:77:39)
I (1572) wifi:connected with <bench-ssid>, aid = 6, channel 6, BW20
I (5672) app_core: joined saved Wi-Fi network, IP address: 192.168.88.108
```

The access point is up and serving DHCP at 192.168.4.1 **before** station mode is
added, the AP is retained alongside it, and the join used credentials read from
NVS with no console command issued after the reset. That is SPEC §15.2's required
ordering and the persistence requirement, on real hardware.

**Still unproven:** the failure half of §15.2 — that a join which fails or times
out is logged and ignored, leaving the device AP-only. It cannot be reached from
the bench, because `wifi-connect` verifies credentials by joining *before* storing
them, so there is no supported way to persist a network that will not answer.
Closing it needs either an unreachable stored network injected some other way, or
a host test around `app_core`'s startup sequence.

**Device state:** unprovisioned (NVS was erased to clear the stale 168-byte
record), running a production build, station credentials stored and working.
Bootstrap AP passphrase and setup code are in
`~/.config/esp32-macro-keyboard/hil/`, mode 600, never in the repository.

- [x] **5.7 Macro creation now validates the source. DONE 2026-08-02.**
  `POST /api/v1/sets/{id}/macros` returned 201 for `ab{DELAY 3000}cd` -- the
  parser wants `DELAY:` -- and the macro was only ever rejected later, when
  something tried to compile it. It sat in the repository until an export tripped
  over it (5.5), and a user who ran it would have got a failure at send time with
  no earlier warning.

  SPEC 3.10 requires rejecting malformed state rather than storing it. There is
  already a `POST /api/v1/sets/{id}/macros/validate` route and the editor calls
  it, so the parser is reachable from the write path; creation simply does not
  use it.

  **Fixed by making the write path validate.** `macro_create_locked` and
  `macro_update_locked` compile the source before touching the document and
  return `APP_ERROR_MACRO_SYNTAX`, which the API already maps to 422. The parser
  is the authority on what a macro means, so it is the authority on whether one
  can be stored; the same check already backed the editor's validate route, and
  the write path simply did not use it.

  SPEC 12.2 now states the rule, so the code and the document agree.

  On hardware: `POST /api/v1/sets/{id}/macros` with `ab{DELAY 3000}cd` returns
  `422 macro_syntax`, where it previously returned 201; the corrected
  `ab{DELAY:3000}cd` returns 201.

  Note the ordering this creates with 5.5. Backup still tolerates an
  uncompilable macro and records it in `skipped`, which is right: a repository
  written before this change can still contain one, and a backup is most needed
  when storage is damaged (SPEC 17). The write path stops new ones; the read
  path survives the old ones.

- [x] **5.8 The hand-rolled JSON scanner is gone. DONE 2026-08-02.**
  `storage_package.c` carried a streaming scanner -- cursor, frame stack,
  `scan_json_*`, `process_object_*` -- whose only product was per-object text
  slices, which were then handed to cJSON one at a time. cJSON therefore parsed
  every object in a package already, and the scanner was a second parse of the
  envelope around them. Restore made it explicit: it validated with the scanner
  and parsed the identical bytes with cJSON in the same request.

  The usual justification is bounded memory, and it did not hold: packages cap
  at 512 KiB against 8 MB of PSRAM, and the restore path built the full tree
  anyway.

  The package is now parsed once by cJSON and validated by walking that tree.
  `storage_package.c` went from 1,032 lines to 459; the component from 7,133 to
  6,633. The object validators kept their logic -- duplicate ids, the set a
  macro belongs to, the per-set macro cap, compiling each source -- and gained
  node-based entry points (`storage_repository_parse_{set,macro}_node`).

  Two rules were dropped in the first pass and restored once the tests caught
  them: a set package holds exactly one set, and the macro count must fit the
  sets present. That is what those tests are for.

  Verified on hardware after flashing: backup and restore round-trip 14 sets,
  a pretty-printed package restores (whitespace is now cJSON's problem, not
  ours), and `{}` and a kind mismatch are still refused with 422.

- [x] **5.9 The two export paths share one package writer. DONE 2026-08-02.**
  `storage_package_export.c` and `storage_package_backup.c` each carried their
  own copy of the growing buffer a package is built in: identical structs
  (`char *data; size_t length; size_t capacity;`) and six functions --
  `writer_reserve`, `append_bytes`, `append_text`, `append_serialized`,
  `append_set`, `append_macro` -- differing only in local variable names.

  Extracted to `storage_package_writer.{c,h}`. Export went 301 -> 213 lines,
  backup 619 -> 532; the component 6,633 -> 6,545 after the new file's own 120.

  Verified on hardware: a set export returns 565 bytes with one set and two
  macros, a backup returns 6,136 with fourteen and twenty-one, and the
  backup/restore round trip still passes.

  This is the honest limit of consolidation-by-extraction. The remaining
  duplication between import, replace, and restore is not copied code but three
  similar *shapes* -- validate, lock, apply, report -- over different units
  (one set, one set with a known id, the whole repository) with different
  atomicity. Merging them means designing one apply-a-package operation and
  deriving the three from it, which is a redesign rather than a deduplication
  and deserves its own decision.


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
