# V2-143 (partial) — development/API/test/hardware/recovery doc sync and status-vs-intent labeling

**Date:** 2026-08-09
**Scope:** Track J of Phase 14 / V2-143 — the "Update development, API, test,
hardware, and recovery documentation" and "Clearly label current
implementation status versus specification intent" sub-bullets only. Does not
cover `CLAUDE.md`, `docs/TODO.md`, or `docs/mockups/v2/` (other V2-143
sub-bullets, out of this track's file surface). Builds on, and does not
duplicate, the README-authority-pointer pass in
`docs/implementation-v2/V2_143_DOC_AUTHORITY_SYNC_2026-08-09.md`.

## What was investigated

Read every file under `docs/*.md`, cross-referenced against
`docs/implementation-v2/V2_MIGRATION_MAP.md` (the authoritative retain/
adapt/rewrite/delete decisions) and the actual firmware/webapp source where a
doc made a factual claim about current routes, storage layout, or physical
controls. Confirmed each finding below against source before editing:

- current `/api/v1/*` routes: `firmware/components/web_server/web_server_lifecycle.c`
  (`normal_routes`/`setup_routes` tables) and `contracts/v2/api/routes.json`;
- blob storage/recovery behavior: `firmware/components/storage/storage_blob*.c`,
  cross-checked against `docs/SPEC_V2.md` §10 (frozen; read-only, not edited);
- device-action/reset semantics: `firmware/components/device_controls/include/device_controls.h`
  and `firmware/components/device_controls/README.md` (whose own text states
  "Credential-reset and factory-reset gestures were never implemented and are
  not planned; a reset is a network request like any other");
- macro-language limits (delay range, duration ceiling):
  `firmware/components/app_contracts_v2/include/app_limits_v2.h`;
- on-device Unity test tags: `grep -oE '"\[[^"]*\]"' firmware/test_app/main/*.c`;
- host test file/suite counts: `find tests/host -name 'test_*.c'` /
  `-name '*.inc'`, `./scripts/run-tests.sh`;
- `docs/TODO_V2.md`'s current checkbox state for Phases 4 and 9-15, to judge
  whether a status claim in a handoff document was still accurate.

## What was changed and why

### `docs/API.md` — retirement banner, not a rewrite

Every route this file documents — `/api/v1/sets*`, `/api/v1/sets/{setId}/macros*`,
plural `/api/v1/executions`, `/api/v1/backup`, `/api/v1/restore` — was deleted
by the v1→v2 rebuild (`V2_MIGRATION_MAP.md` §2.14 "Delete", §8). None of it
exists in `web_server_lifecycle.c`'s route tables. This is not a partial
staleness; the entire document describes an architecture (firmware-owned
package/set/macro repository with revisions and ETags) that firmware no
longer implements at all.

Per this track's instruction not to invent new documentation content I'm not
fully confident in, I did not attempt a line-by-line rewrite of ~220 lines of
route documentation into the current v2 API surface. Instead I added a clear
retirement banner at the top naming the actually-deleted route families,
citing `V2_MIGRATION_MAP.md` §2.14/§8 as the source of that deletion, and
pointing to the two places the current v2 API is actually documented:
`docs/SPEC_V2.md` §13 (frozen, authoritative, not edited) and
`contracts/v2/api/routes.json` (machine-readable, consumed by
`webapp/src/v2/apiRouteManifest.ts`). The body below the banner is left intact
and marked historical rather than deleted, matching the pattern the prior
README track used for `docs/IMPLEMENTATION_STATUS.md`/`docs/UNIT_TESTS1_PROGRESS.md`.

Also fixed `docs/README.md`'s "Reference documents" line, which described
`API.md` as documenting "implemented routes" — true once, false now.

**Residual gap:** `docs/API.md` still has no accurate line-by-line v2 route
reference outside `docs/SPEC_V2.md` §13 itself. If a friendlier v2-specific
API reference doc is wanted, that is new-content authoring this track
deliberately did not do.

### `docs/RECOVERY.md` — rewritten to match the current blob model

The entire rule list described the deleted v1 architecture as current and
host-tested:

- cited `tests/host/test_storage_atomic_recovery.c` and
  `tests/host/test_storage_restore_transactions.c` — **neither file exists**
  in this worktree;
- "quarantine" for invalid persistent objects, `/data/trash/` for deleted
  sets, a backup location for in-progress replacement, and "durable
  transaction manifest[s]" — all deleted per `V2_MIGRATION_MAP.md` §2.13
  ("Delete" list: `storage_repository_*`, `storage_package_*`) and
  `docs/SPEC_V2.md` §10.1 ("There is no repository index file, checksum file,
  metadata file, package file, macro file, backup directory, staging
  directory, transaction directory, trash directory, or quarantine
  directory");
- closed with "requires physical confirmation or the documented boot
  gesture" — no boot gesture exists; confirmed against
  `firmware/components/device_controls/README.md`'s explicit statement that
  factory-reset gestures "were never implemented and are not planned."

Rewrote the rule list to describe the actual current behavior — atomic
`<id>.gz.tmp` → `<id>.gz` blob commit, boot-time temp-file-only cleanup,
single-blob deletion with no automatic replacement selection, no atomic
replace operation — citing `docs/SPEC_V2.md` §10.1/§10.3/§10.5/§10.6/§10.9 and
the real test names (`test_boot_recovery_removes_only_canonical_regular_temporaries`,
`test_boot_recovery_reports_unlink_failure` in `tests/host/test_storage_blob.c`,
verified present by `grep`). Replaced the boot-gesture closing sentence with
the real mechanism: `POST /api/v1/device/factory-reset` gated by admin
password plus the typed phrase `FACTORY RESET` (`docs/SPEC_V2.md` §11.4,
§13.9).

### `docs/PROVISIONING_SECURITY.md` — corrected the setup route list

Listed four routes — `GET /api/v1/setup-state`, `POST /api/v1/setup/credentials`,
`POST /api/v1/setup/complete`, `POST /api/v1/setup/restart` — that do not
exist. `firmware/components/web_server/web_server_lifecycle.c`'s
`setup_routes[]` table registers exactly `GET /api/v1/setup`,
`POST /api/v1/setup`, and `GET /*`, matching `docs/SPEC_V2.md` §13.4 (one GET
for state, one POST for the complete submission — no separate
credentials/complete/restart steps). Corrected the route list and added one
sentence describing the real two-route shape.

### `docs/HARDWARE_TEST_PLAN.md` — v1 terminology and a nonexistent gesture

Three findings, each verified before editing:

1. "Chromebook workflow dry run" used "sets"/"set selection"/"procedure
   order"/"manual checkpoints"/"resend"/"skip"/"reset" — v1 terminology.
   `docs/SPEC_V2.md` §6.3/§6.4 define the current term as "package"/"selected
   package," and §4 explicitly lists "procedures, workflow steps,
   checkpoints, or progress tracking" as a v0.2 **non-goal**. "Resend" and
   "skip" do not appear anywhere in `docs/SPEC_V2.md` or
   `docs/UI_UX_SPEC_V2.md`. Rewrote the paragraph in package/send terms and
   cited the non-goal explicitly rather than silently dropping the removed
   concepts.
2. "Persistence and fault tests" said to test interruption "after each
   transaction phase" and "progress updates." The v2 blob model has one
   atomic commit point (rename), not multiple transaction-manifest phases
   (those were deleted, see the `RECOVERY.md` section above), and send/
   execution state has no persistence or reboot-recovery contract in
   `docs/SPEC_V2.md` §13.10/§18.3 (unlike blobs and device settings, which do).
   Rewrote to name the actual commit point and note send state is RAM-only.
3. "Physical controls" said to "verify setup reset and factory reset gestures
   cannot be triggered by normal short presses" — no such gesture exists.
   `firmware/components/device_controls/README.md` states explicitly:
   "Credential-reset and factory-reset gestures were never implemented and
   are not planned; a reset is a network request like any other." Corrected
   to state that fact plainly instead of testing for a gesture that isn't
   there.

### `docs/FRONTEND_TESTS_PROGRESS.md` — retired-status banner

Described a pre-rebuild frontend test suite (execution/routing pages the v2
rebuild deleted or rewrote per `V2_MIGRATION_MAP.md` §3/§4.2) under
"Status: **Implemented and validated in pull-request CI**" with no historical
marking — unlike `docs/UNIT_TESTS1_PROGRESS.md`, `docs/IMPLEMENTATION_STATUS.md`,
`docs/RELEASE_NOTES.md`, and `docs/SECURITY_REVIEW.md`, which already carry
this kind of disclaimer. Not referenced from `README.md` or `docs/README.md`'s
curated document list. Added the same retired-banner pattern the prior
README track used, pointing to `docs/TODO_V2.md`/`docs/implementation-v2/`
for current status and leaving the historical body intact.

### `README.md` — on-device Unity test-menu `[benchmark]` gap

`docs/implementation-v2/V2_143_DOC_AUTHORITY_SYNC_2026-08-09.md` explicitly
flagged, but deliberately left unedited, that the on-device Unity test-menu
selector table is missing the `[benchmark]` tag
(`firmware/test_app/main/test_auth.c`'s PBKDF2 timing test carries
`[device][auth][benchmark]`), scoping that fix to this sub-bullet. Verified
by grepping every `TEST_CASE` tag in `firmware/test_app/main/*.c`
(`[auth]`, `[benchmark]`, `[device]`, `[executor]`, `[limits]`,
`[macro_parser]`, `[usb]`, `[uuid]` — all others already listed). Added the
`[benchmark]` row to the selector table and mentioned the benchmark in the
preceding prose paragraph, which listed every other on-device suite but this
one.

### `docs/CLAUDE_CODE_PHYSICAL_ESP32S3_V2_HANDOFF_2026-08-08.md` — superseded-status note

This handoff's baseline commit `34fb4bf4f3dfed94205b6294203dac3c05aabc3f` is
80 commits behind this worktree's `HEAD` (`git log --oneline
34fb4bf4..HEAD | wc -l`). It states as "the most important immediate fact for
continuation" that `POST /api/v1/setup` deliberately stubs a `503`; that stub
was replaced by the real transactional handler, recorded in
`docs/implementation-v2/V2_040_CUTOVER_B_TRANSACTIONAL_SETUP_SUBMISSION_2026-08-08.md`
(verified this file exists). `docs/TODO_V2.md` now shows V2-040 fully checked
and Phase 9 merged (`git log` shows "Merge Track G: Phase 9 Macros page and
Quick Send" as an ancestor of `HEAD`), so the document's "Phase 4 is not
complete" / "current Phase 4 state" framing is stale. This is exactly the
kind of status-vs-intent problem the second sub-bullet is about: a document
presenting a specific implementation snapshot as current when the project has
moved well past it.

Added a banner naming the superseded sections (3, 5, 6, 7, 9-12, 16) while
explicitly preserving the sections that remain generally applicable operating
guidance (1, 2, 4, 8, 13, 14, 15 — authority order, hard rules, toolchain, USB
port identification, evidence discipline, trap history). Did not rewrite the
900-line body; a full rewrite would require re-deriving the document's
"current Phase N state" from scratch, which is new-content authoring, not a
targeted correction.

### `docs/DEVELOPMENT.md` — investigated, no change

Read in full. Contains only toolchain/build/quality-gate commands, none of
which reference the retired v1 architecture, an old route, or `docs/SPEC.md`/
`docs/TODO.md`. Confirmed accurate against current `scripts/` entry points.
No edit made.

## What I deliberately left alone

- **`docs/SECURITY_REVIEW.md`** — already self-labeled "Historical snapshot
  from early implementation (pre-FIX1)" pointing to the FIX1 TODO. It lists
  v1 set/procedure/repository routes as a *stale finding already corrected
  inline* (not as current behavior), which is a materially different and
  already-honest presentation than the files above. Consistent with the
  prior README track's stated policy for self-labeled historical docs, left
  untouched.
- **`docs/UNIT_TESTS1_TODO.md`, `docs/RELEASE_NOTES.md`, `docs/TODO_EVIDENCE.md`**
  — each already carries its own historical/superseded disclaimer chaining to
  the FIX1 doc (not to v2). The prior README track considered re-pointing
  this chain past FIX1 straight to v2 and declined, reasoning that doing so
  "felt like more interpretation than this narrow track should make
  unilaterally." I make the same call for the same reason and leave these
  three alone.
- **`docs/SPEC_TEST_TRACEABILITY.md`** — already a clean one-paragraph
  retired-pointer to `docs/SPEC_V2_TEST_TRACEABILITY.md`. No change needed.
- **`docs/SPEC_V2_TEST_TRACEABILITY.md`** — generator output
  (`scripts/generate-spec-traceability.py`), explicitly "Do not edit by
  hand." Not touched.
- **`docs/STATIC_ANALYSIS_EXCEPTIONS.md`, `docs/CI_REPRODUCIBILITY.md`,
  `docs/MACRO_LANGUAGE.md`** — read in full; verified their factual claims
  (delay/duration limits, pinned toolchain versions, mt-unsafe `readdir()`
  compensating controls) against current source and found them accurate. No
  v1-era architecture claims, no dead routes, no stale authority pointers. No
  change made.
- **Dated historical narrative docs** (`FIX1_*` family,
  `RALPH_LOOP_PHASE19_HANDOFF_2026-07-31.md`,
  `HANDOFF_2026-08-02_SIMPLIFICATION.md`,
  `PROPOSAL_2026-08-03_*`, `TODO_SPEC_ALIGNMENT_2026-08-02.md`,
  `SPEC_CHANGE_AUDIT_2026-08-03.md`,
  `ESP32_MACRO_KEYBOARD_CODE_REVIEW_FIXES_*`,
  `UNIT_TESTS1_HANDOFF_STATUS_2026-07-24.md`,
  `docs/CLAUDE_CODE_HANDOFF_2026-07-31.md`, `CI_PHASE16_*`) — same reasoning
  as the prior README track: accurate as historical snapshots of what was
  authoritative when written; not touched.
- **README.md's host/frontend test-count table** ("53 `test_*.c` plus 26
  `.inc` fragments (50 CTest suites)") — re-measured in this worktree and
  found already drifted (55 `test_*.c` now present) due to unrelated,
  concurrently-merging test tracks (`git log` shows several `test(...)`
  commits landing after the README track's measurement). Left alone: this is
  ordinary count churn from parallel active development, not v1-era
  staleness, and would go stale again the moment another concurrent worktree
  merges — chasing it here would not leave the number more durably correct.
- **`webapp/README.md`, firmware headers referencing `SPEC.md`** — outside
  this track's file surface (`webapp/src/`, `firmware/` are explicitly
  excluded; `webapp/README.md` sits under `webapp/` and was left alone as
  out-of-surface).

## Commands run

```bash
export PATH="$HOME/.nvm/versions/node/v24.18.0/bin:$PATH"
node -v                                    # v24.18.0
npm --prefix webapp ci                     # 430 packages, 0 vulnerabilities

./scripts/check-docs.sh                    # exit 0; markdownlint 0 issues in 134 files;
                                            # only pre-existing, unrelated yamllint warnings
                                            # on .github/workflows/*.yml and
                                            # firmware/main/idf_component.yml (files this
                                            # change did not touch); "docs/SPEC_V2_TEST_TRACEABILITY.md
                                            # is current"

grep -oE '"\[[^"]*\]"' firmware/test_app/main/*.c | sort -u   # confirmed [benchmark] tag
find tests/host -name "test_*.c" | wc -l                      # 55 (README says 53; left alone, see above)
find tests/host -name "*.inc" | wc -l                          # 26
grep -n "setup_routes" -A4 firmware/components/web_server/web_server_lifecycle.c
git log --oneline 34fb4bf4f3dfed94205b6294203dac3c05aabc3f..HEAD | wc -l   # 80
```

## TODO_V2.md checkboxes closed

Two V2-143 sub-bullets, with evidence notes pointing at this report:

> - [x] Update development, API, test, hardware, and recovery documentation.
> - [x] Clearly label current implementation status versus specification intent.

The remaining V2-143 sub-bullets (`CLAUDE.md`, `docs/TODO.md` itself,
`docs/mockups/v2/` references) remain open/unchecked — none of that work was
done in this track.

## Statement

No task is being claimed complete beyond the two V2-143 sub-bullets checked
above. `docs/API.md` and
`docs/CLAUDE_CODE_PHYSICAL_ESP32S3_V2_HANDOFF_2026-08-08.md` were corrected
with retirement/superseded banners rather than full content rewrites, as
documented above — a fuller rewrite of either remains open work if wanted. No
unchecked V2-143 item and no Phase 14 exit-gate item is claimed satisfied by
this change.
