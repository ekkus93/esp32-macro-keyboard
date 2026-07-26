# FIX1 Phase 8 Handoff — Make Quarantine Recoverable

This document jump-starts a fresh session implementing **Phase 8** of the FIX1
program. It captures the scope, the current code to change, the couplings
(especially to the Phase 7 work), a suggested commit sequence, and the workflow
rules. Read this together with the authoritative sources:

- Spec: `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_SPEC.md`
- Plan: `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_TODO.md`
  (Phase 8 is section `## 8`)
- Operator decisions: `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_RESPONSES.md`
- Progress log: `docs/ESP32_MACRO_KEYBOARD_RUNTIME_INTEGRITY_AND_PRODUCT_COMPLETION_FIX1_PROGRESS.md`

## Starting state

- Branch `master`, work directly on it (RESPONSES Q4: no branch, no reset, no
  force-push, commit in coherent phase-sized units, run the phase gate before
  claiming done, record each commit + evidence in the progress doc).
- Phases 2–7 are complete and pushed. The last commit is the FIX1 §7.5
  crash-consistency matrix. The tree is clean and `check-all.sh`-green.
- **Do not** relax the fail-closed / no-false-success / no-suppression rules.
  Only the three registered `.clang-tidy` exceptions are allowed (RESPONSES Q2);
  no `|| true`, `NOLINT`, `eslint-disable`, `-Wno-*`, coverage-exclusion markers,
  or first-party formatting suppression (FIX1 §3.4).

## Phase 8 scope (from the TODO §8)

### 8.1 Change quarantine layout

Move from the current **flat-file** layout to a **directory-per-entry** layout:

```text
# staging (in-progress)
/data/staging/quarantine-<transaction-id>/
  record.json
  evidence

# committed
/data/quarantine/<quarantine-id>/
  record.json
  evidence
```

Update path helpers and limits.

### 8.2 Implement staged quarantine creation

Required sequence (make each filesystem step checked and fail-closed):

1. create unique staging directory;
2. copy or rename source into staged evidence;
3. create bounded record;
4. sync evidence and record;
5. validate record and evidence;
6. sync staging directory;
7. rename staging directory into quarantine;
8. sync quarantine parent;
9. return committed entry.

If the source must remain available until commit, **copy** it into staging and
remove the source only after activation. If LittleFS rename semantics are used,
document the exact rollback behavior.

### 8.3 Add quarantine recovery (startup)

- finish a provably complete staged quarantine;
- restore the source when activation never occurred and restoration is safe;
- preserve ambiguous staging as evidence;
- never delete an unmatched evidence file;
- never make the entire quarantine list unreadable because one entry is damaged;
  return valid entries plus a health error.

### 8.4 Tests

Power loss after every quarantine phase, plus corruption of: record only;
evidence only; directory name; record ID; source path; reason; duplicate
quarantine ID.

## Current implementation to rewrite

Primary file: `firmware/components/storage/storage_quarantine.c` (~573 lines).
Internal header: `firmware/components/storage/storage_quarantine_internal.h`.
Public header entries in `firmware/components/storage/include/storage.h`.

### Current on-disk layout (to be replaced)

Flat files under `/data/quarantine/`:

- `<uuid>.json` — the record
- `<uuid>.evidence` — the moved source bytes

Built by `record_path(uuid, suffix, ...)` →
`STORAGE_DATA_MOUNT "/quarantine/%s%s"`. The current create moves (renames) the
source directly to `<uuid>.evidence` and writes `<uuid>.json` — i.e. it is
**not** the staged/atomic dir-rename that §8.2 requires.

### Record JSON format (exact fields, keep or evolve deliberately)

```json
{"schema_version":1,"id":"<uuid>","source_path":"<...>","evidence_path":"<...>","reason":"<...>"}
```

`object_has_exact_fields()` enforces exactly those five keys. `parse_entry()`
additionally requires: `schema_version == 1`, `id` a valid UUID equal to the
expected id, `source_path` passing `safe_source_path()`, `evidence_path` a
string, `reason` a non-empty string, and lengths within the struct fields.

### Key structs / limits (storage.h)

```c
typedef struct {
    app_uuid_t id;
    char source_path[APP_PATH_MAX_BYTES];
    char evidence_path[APP_PATH_MAX_BYTES];
    char reason[STORAGE_QUARANTINE_REASON_MAX_BYTES];   // 96
} storage_quarantine_entry_t;

typedef struct {
    storage_quarantine_entry_t items[STORAGE_QUARANTINE_MAX_ENTRIES];  // 64
    size_t count;
} storage_quarantine_list_t;
```

Internal constants in `storage_quarantine.c`: `QUARANTINE_RECORD_MAX_BYTES 1024`,
`QUARANTINE_ID_ATTEMPTS 4`, `QUARANTINE_JSON_SUFFIX ".json"`,
`QUARANTINE_EVIDENCE_SUFFIX ".evidence"`.

### Public / internal API (callers depend on these signatures)

Public (`storage.h`):

- `storage_quarantine_file(const char *source_path, const char *reason, storage_quarantine_entry_t *out_entry)`
- `storage_quarantine_list(storage_quarantine_list_t *out_list)`

Internal / ops-seam (`storage_quarantine_internal.h`):

- `storage_quarantine_file_with_ops(source_path, reason, out_entry, ops, generate_uuid, uuid_context)`
- `storage_quarantine_list_with_ops(out_list, ops)`
- `storage_quarantine_read_record_with_ops(path, expected_id, out_entry, ops)`
  — **added in Phase 7** for the atomic-recovery QUARANTINE_RECORD validator.

Keeping the `storage_quarantine_file[_with_ops]` and
`storage_quarantine_list[_with_ops]` signatures stable minimizes churn — only the
internals and on-disk layout change. If the layout change makes
`storage_quarantine_read_record_with_ops` unnecessary (see coupling below),
remove it and its declaration together with its only caller.

### Directory creation

`firmware/components/storage/storage_mount_topology.c` (`storage_prepare_directories`)
creates `/data/quarantine` and `/data/staging` (among others). The new layout
needs no new top-level directory (entries are subdirectories created at
quarantine time), but confirm staging/quarantine roots exist.

## Callers that MUST migrate together (Q7: keep the tree internally consistent)

1. `firmware/components/storage/storage_repository_index.c`
   - `storage_quarantine_file(path, "invalid ordering index", &entry)`
   - `storage_quarantine_file(STORAGE_SCHEMA_FILE_PATH, "invalid storage schema marker", &entry)`
2. `firmware/components/storage/storage_repository_sets.c`
   - `storage_quarantine_file(path, "invalid set metadata", &entry)`
3. **Phase 7 coupling — `firmware/components/storage/storage_atomic_recovery.c`**
   - `quarantine_destination_artifacts()` calls
     `storage_quarantine_file_with_ops(artifact_path, "unreconcilable atomic-write artifact", ...)`.
     This must still work under the new layout (it quarantines a stray artifact
     file). Verify `safe_source_path()` still accepts the artifact source path
     and that the staged-dir creation handles a plain-file source.
4. **Phase 7 coupling — `firmware/components/storage/storage_atomic_validators.c`**
   - Classifier maps `/data/quarantine/<uuid>.json` →
     `STORAGE_ATOMIC_OBJECT_QUARANTINE_RECORD`, and `validate_quarantine_record()`
     calls `storage_quarantine_read_record_with_ops`.
   - **Under the new staged-dir creation there are no per-file `.tmp/.bak`
     quarantine artifacts** (records are created in staging then the whole
     directory is renamed), so this classification/validator is very likely
     **obsolete**. Recommended: remove `STORAGE_ATOMIC_OBJECT_QUARANTINE_RECORD`
     handling from the classifier + `validator_for_type` + delete
     `validate_quarantine_record`, and update
     `tests/host/test_storage_atomic_validators.c` accordingly (the classifier
     test currently expects a quarantine classification, and there is a quarantine
     validator test). Double-check nothing else writes a bare
     `/data/quarantine/<file>` that could still leave an atomic artifact.

## Tests to update / add

- `tests/host/test_storage_sets.c` — uses `storage_quarantine_list()` and asserts
  quarantine counts after corrupt-set/mismatch/duplicate scenarios. Update
  assertions to the new layout.
- `tests/host/test_storage_atomic_validators.c` — remove/replace the quarantine
  classifier + validator cases if that validator is dropped.
- `tests/host/test_storage_atomic_recovery.c` — the executor's QUARANTINE actions
  (`test_executor_quarantine_conflict`, `test_executor_quarantine_corrupt_backup`)
  assert `directory_entry_count(/data/quarantine) == 4` and `== 2` under the old
  two-files-per-entry scheme. Under the new one-directory-per-entry scheme these
  counts change (one subdirectory per quarantined artifact). Update them.
- New `tests/host/test_storage_quarantine.c` cases for §8.4 (power-loss after each
  staged phase; the seven corruption cases). The existing quarantine test already
  builds a fake-fs `storage_fs_ops_t` adapter — reuse that idiom, and the
  `fake_fs_backend` fault-injection API (`fake_fs_backend_fail_on` /
  `_add_failure`, per-operation + occurrence) for power-loss injection. For
  crash-state reconciliation prefer constructing the post-step on-disk state (as
  the Phase 7 §7.5 matrix does), because a mid-write fault triggers graceful
  rollback rather than the crash state recovery must handle.

## Startup ordering (already in place from §7.4)

`app_core`'s `adapter_storage_recover` currently runs:

```text
storage_atomic_recover_all();        // FIX1 §7.4
storage_transaction_recover_all();
```

Per the FIX1 §7.4 order (`mount → atomic recovery → transaction manifest recovery
→ quarantine staging recovery → repository init`), Phase 8's §8.3 quarantine
recovery slots **after** transaction recovery and **before** repository init. Add
a public `storage_quarantine_recover_all(void)` (+ `_with_ops`) and call it from
`adapter_storage_recover` after `storage_transaction_recover_all()`, or fold it
into the storage recovery step. Keep each step's error fail-closed.

## Suggested commit sequence

1. **§8.1** — new directory layout + path helpers + limits; migrate
   `storage_quarantine_file[_with_ops]` create and `storage_quarantine_list[_with_ops]`
   read to the directory format; update the three repository/atomic callers and
   the affected tests so the committed tree stays consistent. (If dropping the
   Phase 7 quarantine validator, do it here.)
2. **§8.2** — the 9-step staged creation (staging dir → staged evidence → record
   → sync → validate → sync dir → rename into quarantine → sync parent → return),
   every fs step checked; document LittleFS rename rollback behavior.
3. **§8.3** — `storage_quarantine_recover_all` implementing the five recovery
   rules, wired into the startup order; host tests for each rule.
4. **§8.4** — power-loss-after-each-phase matrix + the seven corruption cases.

Each commit must be internally consistent and pass the gate.

## Workflow / gate reminders

- Source the toolchain before firmware commands:
  `. "$HOME/esp/esp-idf-v5.5.5/export.sh"`. Frontend/docs need Node 24.18.0:
  `nvm use 24.18.0`.
- Per-change fast loop: `./scripts/run-tests.sh storage` (label), then
  `./scripts/run-tests.sh --sanitizers storage`, `./scripts/check-firmware.sh`
  (firmware build + fail-closed clang-tidy), `./scripts/check-format.sh`,
  `./scripts/check-docs.sh`.
- Full gate before claiming the phase done: `./scripts/check-all.sh`.
- `check-firmware.sh` rebuilds the ESP32-S3 image and runs esp-clang clang-tidy;
  it is slow (run it in the background). Watch for `readability-function-cognitive-complexity`
  (limit 25 — split helpers) and `misc-include-cleaner` (add direct includes;
  it rewrites `<cstddef>`→`<stddef.h>` if you use `--fix`, so prefer manual
  includes), and `bugprone-easily-swappable-parameters` (two adjacent same-type
  params must be **restructured**, not exempted — see the Phase 7 §7.2 struct
  fold as precedent).
- Commit messages: conventional style, subject < 72 chars, and **no
  `Co-Authored-By:` trailer** (a global commit-msg hook rejects it).
- Update the FIX1 TODO checkboxes and record each commit + validation evidence in
  the FIX1 progress doc as you go. Record any deviation from the TODO in the
  progress doc "Deviations from the TODO" section (per RESPONSES §8).

## Known residuals relevant to Phase 8

- `/data/quarantine/` is currently **not scanned** by the atomic-recovery
  executor (`storage_atomic_recovery.c`) because a quarantine artifact cannot be
  re-quarantined (`safe_source_path` excludes the quarantine root). The new
  staged-dir scheme should remove the need for per-file quarantine atomic
  artifacts entirely; confirm and update the progress-doc residual note.
- Object validators for macro/procedure/progress/settings remain deferred to
  Phase 15; unrelated to Phase 8 but present in the classifier.
