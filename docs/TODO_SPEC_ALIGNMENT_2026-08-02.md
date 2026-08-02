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

### 4b — Move the active set into the index

**Not started.** The remaining half of 4.4.

`active_set_id` currently lives in the provisioning NVS blob, baked into its
binary wire format (`WIRE_ACTIVE_SET_OFFSET` in `provisioning_core.c`) and
surfaced through `/api/v1/settings` as `activeSetId`. SPEC §12.3 puts it in the
index, and SPEC §14 does not list it among what NVS stores.

It was left out of 4a on purpose: adding the field to the index while NVS still
owned it would give the device two authorities for which set is active, which is
worse than one commit with the field missing.

- [ ] Add `active_set_id` to `storage_set_index_t`, to the index JSON, and to
  its parse/serialize — including the rule that an active set not present in
  `set_ids` is a corrupt index, not a hint to drop silently.
- [ ] Add a repository entry point that selects the active set, and make
  `clear_matching_active_set` clear it in the index rather than through
  `provisioning_clear_active_set_if_matches`.
- [ ] Remove `has_active_set` / `active_set_id` from `settings_t` and from the
  provisioning NVS wire format, and bump/verify that format's own versioning.
- [ ] Point `/api/v1/settings` and `/api/v1/sets/{id}/select` at the index.
  The API shape does not change: `activeSetId` stays in the settings response.

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
