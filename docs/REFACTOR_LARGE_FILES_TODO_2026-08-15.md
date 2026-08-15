# Large-file split and refactor TODO

**Document status:** Implementation plan. **Non-normative.**
**Date:** 2026-08-15
**Base commit:** `a68f6dc` (`master`, clean, `./scripts/check-all.sh` green)

This document contains **no product requirements**. It does not add, remove, or
reinterpret anything in `docs/SPEC_V2.md` or `docs/UI_UX_SPEC_V2.md`, and
nothing in it may be cited as a requirement. Every task here is a
behaviour-preserving code move. If any task appears to require a behaviour
change, that is a defect in this document — stop and raise it rather than
changing behaviour to match it.

## 0. Where the target comes from

Phil, 2026-08-15:

> "Create a comprehensive TODO list for splitting and refactoring these 10
> files. Try to keep the files under 800 lines. Do not try to split and refactor
> them all at once. Do them one by one."

So: **800 lines is the target**, it is Phil's number, and it is a target rather
than a gate. No script enforces it and this document does not propose adding
one. "One by one" is a constraint on execution, not a suggestion — see §2.

The ten files are the output of `/top-large-files` at `a68f6dc`, ranked by line
count over git-tracked source files (excluding generated and vendored code).

| # | Lines | File | Task |
| ---: | ---: | --- | --- |
| 1 | 2401 | `webapp/tests/browser/run-browser-tests.mjs` | [T10](#t10) |
| 2 | 1502 | `webapp/tests/v2-snapshots-page.test.tsx` | [T6](#t6) |
| 3 | 1229 | `tests/host/test_web_server_administration_route.c` | [T8](#t8) |
| 4 | 1220 | `tests/host/test_web_settings.c` | [T7](#t7) |
| 5 | 1098 | `webapp/tests/v2-macros-page.test.tsx` | [T5](#t5) |
| 6 | 1081 | `scripts/run-v2-035-hardware.py` | [T9](#t9) |
| 7 | 1048 | `webapp/src/features/settings/v2/SettingsPage.tsx` | [T3](#t3) |
| 8 | 966 | `webapp/src/features/macros/v2/MacrosPage.tsx` | [T2](#t2) |
| 9 | 956 | `webapp/tests/v2-app-v2.test.tsx` | [T4](#t4) |
| 10 | 836 | `webapp/src/features/snapshots/v2/SnapshotsPage.tsx` | [T1](#t1) |

All line numbers in this document were measured at `a68f6dc` and **will drift**
as tasks land. Re-derive them before starting each task; do not trust a stale
range.

## 1. Why these splits and not others

Every proposed boundary below is an existing seam in the file — a component, a
`describe` block, a test-name prefix, a helper cluster — not an arbitrary cut at
800 lines. A split that lands mid-concept to hit a number makes the code worse
while making the metric better. Where a file has no seam that gets it under 800,
the task says so instead of inventing one.

## 2. Rules of engagement

1. **One file per task, one task at a time, one commit per task.** Do not batch.
   This mirrors the discipline in the hardening trackers and exists so that a
   regression bisects to a single move.
2. **Behaviour-preserving.** A split moves code; it does not rename exported
   symbols, change props, alter test assertions, or "clean up while in there".
   If a genuine bug is spotted mid-task, record it and fix it in a *separate*
   commit — before or after, never inside the move.
3. **The test suite must pass unchanged.** For the six test files, the number of
   tests before and after must be identical. Record both counts in the commit
   message. A split that changes the test count has lost or duplicated a test.
4. **`./scripts/check-all.sh` must exit 0 on the resulting tree** before the task
   is checked off. Not "should pass" — run it.
5. **Evidence per task**, in this document: commit SHA, the exact command run,
   the result, and the before/after line counts of every file touched.
6. **No new suppressions.** If a split trips clang-tidy or ESLint, fix the cause.
   `docs/STATIC_ANALYSIS_EXCEPTIONS.md` is not a release valve for a refactor.

## 3. What the split must not break — verified constraints

Each of these was checked against the tree at `a68f6dc`, not assumed.

**3.1 `.inc` fragments are not format-checked.** `scripts/check-format.sh`
globs only `-name '*.c' -o -name '*.h'`, so the 26 existing `.inc` fragments are
outside clang-format enforcement — and 2 of them have already drifted
(`auth_additional_rate_tests.inc`, `auth_existing_tests.inc`). T7 and T8 would
add roughly 1,800 more unchecked lines. **T0 closes this first.**
**Closed by `75a2c66`.**

**3.1a The format authority is esp-clang 19.1.2, not apt 18.1.3.** Found while
doing T0, and it invalidated an earlier claim in `CLAUDE.md`.
`.github/workflows/quality.yml:85-86` sources `export.sh` immediately before
`./scripts/check-all.sh`, and `check-format.sh` does a bare `PATH` lookup — so
CI formats with esp-clang's clang-format. Measured at `a68f6dc` over the 314
first-party `.c`/`.h` files: **0 dirty under esp-clang 19, 2 dirty under apt
18**. Run every format command from a shell that has sourced `export.sh`. An
apt-only shell falsely flags
`tests/host/fakes/esp_http_server_stub/esp_http_server.h` and
`tests/host/fakes/freertos_stub/freertos/FreeRTOS.h`.
Corrected in `4b78391` (`CLAUDE.md`) and `75db7c2` (the format-on-edit hook,
which `a280deb` had pinned to the wrong version).

**3.2 Two gate scripts read `.inc` files by exact path and match on content.**
`scripts/check-h3-architecture.py:178` reads
`tests/host/auth_additional_session_tests.inc`; `scripts/check-v2-auth-policy.py:60-61`
reads `auth_additional_session_tests.inc` and `auth_additional_rate_tests.inc`.
Renaming or reformatting those files can break a gate. None of the ten files
here are among them, but T0 reformats one of them — see T0's verification.

**3.3 `scripts/run-v2-035-hardware.py` is loaded by file path, not imported.**
`tests/scripts/test-v2-035-hardware.py:17` uses
`importlib.util.spec_from_file_location`, and `scripts/run-h5-055-hardware.py`
depends on it. The path is also cited in ~10 evidence documents under
`docs/implementation-v2/`. **The entry-point path must not move.** A module
loaded by `spec_from_file_location` has no package context, so sibling imports
need `scripts/` on `sys.path` — this is the single hardest task here and it is
sequenced last but one.

**3.4 Coverage gates are unaffected.** The native coverage policy list in
`scripts/generate-native-coverage.sh:18-32` names firmware `.c` files only —
none of the ten. Frontend thresholds in `webapp/vite.config.ts:21-26` are global
(60% branches/functions/lines/statements over `src/**`), not per file, so
splitting a component cannot trip a per-file threshold.

**3.5 Traceability and removed-feature scans already cover the new extensions.**
`scripts/generate-spec-traceability.py:69` includes `.inc`;
`scripts/check-removed-features.sh:55` covers `.c .h .inc .ts .tsx .js .mjs`.
Moved content stays in scope, so long as it is moved and not dropped.

**3.6 Two of the production files have open Round 2 findings.** `MacrosPage.tsx`
is R4-040 and `SettingsPage.tsx` is R4-041 in
`docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_ROUND2_2026-08-10.md`.
See §5 for the sequencing recommendation — this is a decision for Phil, not one
this document makes.

---

## T0 — Prerequisite: bring `.inc` fragments under format enforcement

**Blocks:** T7, T8. Not required by any other task.
**Size:** small.

Without this, T7 and T8 move ~1,800 lines of C out of clang-format's reach,
which turns a refactor into a silent reduction in enforcement.

- [x] **T0-001** Extend `scripts/check-format.sh` so the first-party C glob
      includes `*.inc` alongside `*.c` and `*.h`. Same directory list, same
      prune rules. — `75a2c66`.
- [x] **T0-002** Reformat the 2 fragments that currently fail:
      `tests/host/auth_additional_rate_tests.inc`,
      `tests/host/auth_existing_tests.inc`. — `75a2c66`. **The instruction this
      task originally carried was wrong**: it said to use `/usr/bin/clang-format`
      because "CI checks with 18". See §3.1a — CI checks with esp-clang 19.
      Reformatted with esp-clang 19; as it happens both versions produce
      identical output for these two files (pure line-wrapping), so the result
      is version-agnostic.
- [x] **T0-003** Re-run the two gates that read `.inc` content by exact path,
      because T0-002 reformats one of the files they parse:
      `python3 scripts/check-h3-architecture.py` and
      `python3 scripts/check-v2-auth-policy.py`. — both exit 0 after the
      reformat.
- [x] **T0-004** Add a regression test under `tests/scripts/` asserting that a
      deliberately misformatted `.inc` file makes `check-format.sh` fail. —
      `tests/scripts/test-check-format-inc-coverage.sh`, registered in
      `check-scripts.sh`, 3 assertions. **Proven to fail when the `*.inc` glob
      is reverted** (`FAIL: check-format.sh passed with a misformatted .inc
      fragment present`), so it is not a vacuous test.

**Verify:** `./scripts/check-format.sh && ./scripts/check-scripts.sh && ./scripts/check-all.sh`
**Done when:** all four boxes checked, `check-all.sh` exit 0, evidence recorded.

**Evidence:** commit `75a2c66`. `./scripts/check-all.sh` → `EXIT=0`
(3,615-line log; `check-format .inc coverage regression tests passed: 3`;
`100% tests passed, 0 tests failed out of 66`). Related corrections outside
this task: `75db7c2`, `4b78391`.

---

## T1 — `SnapshotsPage.tsx` (836 → ~600) <a id="t1"></a>

Smallest overage, one dominant seam, no open findings. Best first task: it
proves the pattern for T2 and T3 at the lowest risk.

**Seams** (`webapp/src/features/snapshots/v2/SnapshotsPage.tsx`):

| Lines | Contents | Destination |
| --- | --- | --- |
| 106–135 | `loadFailureMessage`, `importFailureMessage`, `formatBytes` | `snapshotMessages.ts` (~30) |
| 136–347 | `SnapshotRowProps` + `SnapshotRow` | `SnapshotRow.tsx` (~212) |
| rest | deps, props, `PendingReplace`, `SnapshotsPage` | stays (~600) |

- [x] **T1-001** Extract `snapshotMessages.ts`. Pure functions, no React import.
      — 34 lines. `formatBytes` had to come here rather than travel with the
      row: it is used by `SnapshotRow` *and* by the page body (former lines
      637–638).
- [x] **T1-002** Extract `SnapshotRow.tsx` with its props interface. — 217 lines.
- [x] **T1-003** Confirm `SnapshotsPage.tsx` is under 800 and that no export
      outside the module changed. — **598 lines**. Only `SnapshotsPage` and
      `SnapshotsPageDependencies` are imported externally (`AppV2.tsx`,
      `v2-snapshots-page.test.tsx`); both unchanged.

**Risk:** low. `SnapshotRow` is referenced only by this page.
**Verify:** `npm --prefix webapp run test -- v2-snapshots-page` then
`./scripts/check-webapp.sh`, then `./scripts/check-all.sh`.
**Test count must be unchanged: 41.**

**Evidence:** commit `6e0f55a`. 836 → 598 + 217 + 34.
`npm --prefix webapp run test -- v2-snapshots-page` → **41 passed (41)**.
`./scripts/check-all.sh` → `EXIT=0` (49 frontend test files, 66/66 host tests).
Move verified byte-exact by diffing the extracted ranges against the pre-edit
file: only the 4 added `export` keywords and one collapsed blank line.

---

## T2 — `MacrosPage.tsx` (966 → ~620) <a id="t2"></a>

**Sequencing:** carries open finding **R4-040** (send trackers started by the
`initialSend` recovery effect, `recoverActiveSend()` and `startSend()` without a
matching stop on unmount). Recommendation: **fix R4-040 first, then split.** The
finding names specific call sites; splitting first invalidates its line
references and makes the fix harder to review. Phil's call.

**Seams** (`webapp/src/features/macros/v2/MacrosPage.tsx`):

| Lines | Contents | Destination |
| --- | --- | --- |
| 77–147, 189 | `MacroIdentity`, `SendLifecycle`, `completionAckMs`, `terminalIssueReason`, `activeStatusText`, `terminalIssueText`, `MoveAction` | `macroSendStatus.ts` (~75) |
| 148–171 | `DismissibleBanner` + props | `DismissibleBanner.tsx` (~24) |
| 204–325 | `MacroOverflowMenu` | `MacroOverflowMenu.tsx` (~122) |
| 172–188, 326–433 | `MacroRowProps`, `MacroRow` | `MacroRow.tsx` (~125) |
| rest | deps, props, `MacrosPage` | stays (~620) |

- [x] **T2-001** Extract `macroSendStatus.ts` (types + pure status text helpers).
      This is the highest-value piece: it is the send-lifecycle vocabulary and
      is currently buried in a page component. — 75 lines.
- [x] **T2-002** Extract `DismissibleBanner.tsx`. — 23 lines.
- [x] **T2-003** Extract `MacroOverflowMenu.tsx`. — 139 lines. Note `MoveAction`
      (line 189) goes to `macroSendStatus.ts`, **not** here — measured: it is
      used by `MacroRowProps` (183) and by `MacrosPage` (756), so it is shared,
      not menu-local. The original version of this table had it in the wrong
      file.
- [x] **T2-004** Extract `MacroRow.tsx` (imports `MacroOverflowMenu`). — 127 lines.
- [x] **T2-005** Confirm under 800 and no behaviour change. — **619 lines**.
      Three imports became unused in the page and were dropped
      (`useDismissibleOverlay`, `useFocusTrap`, `SendState`); each moved with
      the component that used it.

**Risk:** medium — `MacroRow` and `MacroOverflowMenu` are coupled, and the
overflow menu has dismissal behaviour covered by its own `describe` block.
**Verify:** `npm --prefix webapp run test -- v2-macros-page`, then
`./scripts/check-webapp.sh`, then `./scripts/check-all.sh`.
**Test count must be unchanged: 44** (corrected from 41 — see §7).

**Evidence:** commit `79e2fd9`. 966 → 619 + 75 + 139 + 127 + 23.
`npm --prefix webapp run test -- v2-macros-page` → **44 passed (44)**.
`./scripts/check-all.sh` → `EXIT=0`. Each extracted range diffs byte-identical
against the pre-edit file apart from the added `export` keywords.

**Sequencing note:** this split ran **before** the R4-040 fix, contrary to the
recommendation above. R4-040's line references in the Round 2 tracker now point
into the pre-split file and must be re-derived — the send-tracker call sites
(`initialSend` recovery effect, `recoverActiveSend()`, `startSend()`) are all
still in `MacrosPage.tsx`, none moved to `MacroRow.tsx`. The split neither fixes
nor closes R4-040.

---

## T3 — `SettingsPage.tsx` (1048 → ~500) <a id="t3"></a>

**Sequencing:** carries open finding **R4-041** (submitting the AP or Station
form silently discards unsaved Identity edits). Same recommendation as T2: **fix
first, then split** — though note this split is also the structural fix for the
*class* of bug, since it forces each form's state ownership to become explicit
at a module boundary. If Phil prefers, splitting first is defensible here in a
way it is not for T2; the two orders are both reasonable and the choice is his.

**Seams** (`webapp/src/features/settings/v2/SettingsPage.tsx`):

| Lines | Contents | Destination |
| --- | --- | --- |
| 47–51, 99–102 | byte-limit constants, `byteCountLabel` | `settingsFieldLimits.ts` (~15) |
| 103–256 | `IdentityForm` + props | `IdentityForm.tsx` (~154) |
| 257–326 | `AccessPointForm` + props | `AccessPointForm.tsx` (~70) |
| 327–439 | `StationForm` + props | `StationForm.tsx` (~113) |
| 440–534 | `PasswordForm` + props | `PasswordForm.tsx` (~95) |
| 535–640 | `ConfirmPhraseDialog` + props | `ConfirmPhraseDialog.tsx` (~106) |
| rest | deps, props, `DirtyGuardedAction`, `SettingsPage` | stays (~500) |

- [x] **T3-001** Extract `settingsFieldLimits.ts` first — the forms share it, so
      extracting it up front avoids four rounds of import churn. — 14 lines.
      Needed one import the region map missed: `byteCountLabel` calls
      `utf8ByteLength`. Typecheck caught it on the first run.
- [x] **T3-002** Extract `IdentityForm.tsx`. — 162 lines.
- [x] **T3-003** Extract `AccessPointForm.tsx`. — 79 lines.
- [x] **T3-004** Extract `StationForm.tsx`. — 122 lines.
- [x] **T3-005** Extract `PasswordForm.tsx`. — 98 lines.
- [x] **T3-006** Extract `ConfirmPhraseDialog.tsx`. — 110 lines.
- [x] **T3-007** Confirm under 800 and no behaviour change. — **499 lines**.
      Four imports became unused in the page and were dropped (`useEffect`,
      `v2Limits`, `utf8ByteLength`, `isSettingsUpdateRequest`).

**Risk:** medium-high — five extractions in one file, and the dirty-state guard
(`DirtyGuardedAction`) spans page and forms. Extract one form per commit-sized
step even though the task lands as one commit; if the dirty-state wiring
resists a clean move, stop and record why rather than reshaping it.
**Verify:** `./scripts/check-webapp.sh`, then `./scripts/check-all.sh`. Settings
behaviour is also covered by browser workflows — `npm --prefix webapp run test:browser`.

**Evidence:** commit `abd93ff`. 1048 → 499 + 162 + 122 + 110 + 98 + 79 + 14.
Settings suites **23 passed (23)**, unchanged. `./scripts/check-all.sh` →
`EXIT=0` (49 frontend test files, 66/66 host tests, browser workflows green).
Every extracted range diffs byte-identical against the pre-edit file apart from
the added `export` keywords and a trailing blank line.

**Sequencing note:** as with T2, this ran **before** the R4-041 fix. The
Identity/AP/Station forms are now separate modules, which makes the finding's
shape clearer — each form owns its own state and submits independently — but
nothing about the dirty-edit-discard behaviour changed. R4-041 remains open and
its line references need re-deriving.

---

## T4 — `v2-app-v2.test.tsx` (956 → 4 files) <a id="t4"></a>

First test-file split, and the cleanest: a 253-line shared prelude followed by
four independent `describe` blocks.

| Lines | Contents | Destination |
| --- | --- | --- |
| 1–253 | imports, fakes, harness setup | `helpers/appV2TestHarness.tsx` (~253) |
| 254–526 | "wiring RepositoryStartupScreen into the running app shell" | `v2-app-v2-startup.test.tsx` (~275) |
| 527–819 | Phase 10 wiring + Phase 12 wiring | `v2-app-v2-wiring.test.tsx` (~300) |
| 820–956 | V2-131/V2-132 phone-landscape orientation | `v2-app-v2-orientation.test.tsx` (~140) |

Phase 10 (81 lines) and Phase 12 (212 lines) are merged into one wiring file
rather than split into two — an 81-line test file is a worse outcome than a
300-line one, and both describe the same concern.

- [x] **T4-001** Extract the prelude to `webapp/tests/appV2Harness.tsx`.
      The harness must be **shared, not copied** — duplicating 244 lines of fakes
      across four files is a worse result than the 956-line file. — 235 lines.
      **Path correction:** flat in `tests/`, not a `tests/helpers/`
      subdirectory. No such directory exists; `fakeFetch.ts`, `fakeMatchMedia.ts`
      and `render.tsx` all sit flat, and vitest's `include` only collects
      `*.test.*`, so a helper beside them is not picked up as a suite.
- [x] **T4-002** Split the four `describe` blocks into the three test files above.
      — 303 / 320 / 163 lines.
- [x] **T4-003** Confirm the total test count is unchanged and every new file is
      under 800. — **14 passed (14)**, unchanged; largest new file 320.

**Risk:** low-medium. Watch for shared mutable state in the prelude — if any
fake holds state across `describe` blocks, splitting changes reset timing. If a
test only passed because of ordering within the old file, this task will expose
it; that is a real bug, so fix it separately and note it.
**Verify:** `npm --prefix webapp run test`.
**Test count must be unchanged: 14.**

**Evidence:** commit `a549dd9`. 956 → 235 + 303 + 320 + 163.
`npm --prefix webapp run test -- v2-app-v2` → **3 files, 14 passed (14)**.
`./scripts/check-all.sh` → `EXIT=0`; frontend test files 49 → 51.
Each `describe` body diffs byte-identical against the pre-split file apart from
one trailing separator blank line; the harness differs only by added `export`
keywords. The `beforeEach`/`afterEach` fake-timer hooks are repeated explicitly
in each suite rather than relying on an imported module registering hooks into
the importing file's root suite — that works in vitest but is implicit enough to
mislead.

---

## T5 — `v2-macros-page.test.tsx` (1098 → 4 files) <a id="t5"></a>

Nine `describe` blocks, 132-line prelude. Grouped by concern:

| Source describes | Destination | ~Lines |
| --- | --- | ---: |
| 1–132 prelude | `helpers/macrosPageHarness.tsx` | 132 |
| V2-091 list, V2-133 overflow dismissal, V2-092 source privacy (133–407) | `v2-macros-page-list.test.tsx` | 275 |
| V2-093 Quick Send, V2-132 landscape summary, send-tracker lifetime (408–876) | `v2-macros-page-send.test.tsx` | 470 |
| V2-095 reload/race, V2-101 overflow menu, V2-094 Always Preview (877–1098) | `v2-macros-page-actions.test.tsx` | 222 |

- [x] **T5-001** Extract `macrosPageHarness.tsx` (flat in `tests/`, per T4-001).
      — 117 lines.
- [x] **T5-002** Split into the three test files above. — list 279, send 502,
      actions 235.
- [x] **T5-003** Confirm count unchanged and all files under 800. —
      **44 passed (44)**; largest new file 502.

**Note:** the "send tracker lifetime" describe is the coverage for R4-040. If T2
runs after this, its regression test lands in `v2-macros-page-send.test.tsx`.
**Verify:** `npm --prefix webapp run test`.
**Test count must be unchanged: 44** (see §7 — one `test.each` block at line 542
expands to four cases, so the file has 44 tests from 41 declarations).

**Evidence:** commit `052f679`. 1098 → 117 + 279 + 502 + 235.
`npm --prefix webapp run test -- v2-macros-page` → **3 files, 44 passed (44)**.
`./scripts/check-all.sh` → `EXIT=0`; frontend test files 51 → 53.
Method note: the per-file import lists were derived by giving every split file
the full original import block and letting `eslint --max-warnings=0` name the
unused ones, rather than by reasoning about which symbol each `describe` needs.
Two rounds were required — the first `eslint` output was truncated by a `head`
and hid three findings in the `send` suite.

---

## T6 — `v2-snapshots-page.test.tsx` (1502 → 5 files) <a id="t6"></a>

Largest test file; ten `describe` blocks, 151-line prelude.

| Source describes | Destination | ~Lines |
| --- | --- | ---: |
| 1–151 prelude | `helpers/snapshotsPageHarness.tsx` | 151 |
| V2-111 management, V2-112 retention, V2-110/111 manual load (152–426) | `v2-snapshots-page-management.test.tsx` | 275 |
| V2-113 dirty-work protection, V2-114 unreadable recovery (427–706) | `v2-snapshots-page-protection.test.tsx` | 280 |
| V2-111 download/delete, V2-116 non-atomic replace (707–1001) | `v2-snapshots-page-replace.test.tsx` | 295 |
| V2-115 export, V2-115 import, H8 selection resolution (1002–1502) | `v2-snapshots-page-transfer.test.tsx` | 500 |

- [x] **T6-001** Extract `snapshotsPageHarness.tsx`. — 137 lines.
- [x] **T6-002** Split into the four test files above. — management 289,
      protection 301, replace 308, transfer 526.
- [x] **T6-003** Confirm count unchanged and all files under 800. —
      **41 passed (41)**; largest new file 526.

**Risk:** the V2-115 import block alone is ~296 lines (1101–1396). If
`transfer` lands over 800 once imports are added, split import out to
`v2-snapshots-page-import.test.tsx` rather than trimming tests.
**Outcome:** not needed — `transfer` landed at 526, so export, import and the
H8 selection tests stay together.
**Verify:** `npm --prefix webapp run test`.
**Test count must be unchanged: 41.**

**Evidence:** commit `0f20761`. 1502 → 137 + 289 + 301 + 308 + 526.
`npm --prefix webapp run test -- v2-snapshots-page` → **4 files, 41 passed (41)**.
`./scripts/check-all.sh` → `EXIT=0`; frontend test files 53 → 56.

**Webapp milestone:** with T1–T6 done, no file under `webapp/src` or
`webapp/tests` exceeds 800 lines. The largest remaining is
`webapp/src/v2/apiGuards.ts` at 760, which was never in the top ten.

---

## T7 — `test_web_settings.c` (1220 → ~120 + fragments) <a id="t7"></a>

**Depends on T0.** Follows the established convention — `test_auth.c` includes a
fixture fragment plus topic fragments (`tests/host/test_auth.c:13-18`). CMake
lists only the `.c` (`tests/host/CMakeLists.txt:138`), so fragments need no
build change and the suite stays one CTest binary.

Structure: fixture 1–188, tests 189–1157 (59 tests: 5 `get`, 39 `put`, 15
`change`), `main` 1158–1220.

| Contents | Destination | ~Lines |
| --- | --- | ---: |
| fixture and helpers (1–188, minus includes) | `web_settings_test_fixture.inc` | 150 |
| 5 `test_get_*` | `web_settings_get_tests.inc` | 75 |
| 39 `test_put_*`, first half | `web_settings_put_field_tests.inc` | 300 |
| 39 `test_put_*`, second half | `web_settings_put_policy_tests.inc` | 300 |
| 15 `test_change_*` (≈865–1157) | `web_settings_change_password_tests.inc` | 293 |
| includes, `#include` lines, `main` | `test_web_settings.c` | 120 |

- [x] **T7-001** Extract the fixture fragment. — 170 lines.
- [x] **T7-002** Extract the `get` fragment. — 75 lines.
- [x] **T7-003** Split the 39 `put` tests into two fragments **at a topical
      boundary**, not at test 20. — **Kept as one fragment (601 lines).** There
      is no topical boundary inside the PUT section: the only section comments
      in the file are the three GET / PUT / change-password headers. 601 is
      already under the 800 target, so an arbitrary cut would buy nothing. This
      task explicitly allowed the outcome.
- [x] **T7-004** Extract the `change_password` fragment. — 294 lines.
- [x] **T7-005** Confirm the `main` runner still registers all 59 tests and the
      CTest suite count is unchanged. — `main()` byte-identical, 59 tests
      registered, web suite 31/31.

**Verify:** `./scripts/run-tests.sh web`, then `./scripts/check-format.sh` (now
covering `.inc` thanks to T0), then `./scripts/check-all.sh`.

**Evidence:** commit `373b0e8`. 1220 → 85 + 601 + 294 + 170 + 75.
`./scripts/check-all.sh` → `EXIT=0`, 66/66 host tests.
**T0 paid off immediately:** `check-format.sh` rejected all four new fragments
for trailing blank lines. Before T0 it would not have looked at them, and the
drift would have entered the tree exactly as it had for the two auth fragments.

---

## T8 — `test_web_server_administration_route.c` (1229 → ~90 + fragments) <a id="t8"></a>

**Depends on T0.** Same convention as T7. Note the fixture here is unusually
large — 492 lines before the first test — so most of the win is in isolating it.

Structure: fixture 1–492, tests 493–1188 (29 tests), `main` 1189–1229.

| Contents | Destination | ~Lines |
| --- | --- | ---: |
| fixture and helpers (1–492, minus includes) | `admin_route_test_fixture.inc` | 440 |
| 3 `session`, 2 `restart`, 2 `setup` | `admin_route_session_tests.inc` | 180 |
| 5 `settings`, 4 `change` | `admin_route_settings_tests.inc` | 230 |
| 7 `diagnostics` | `admin_route_diagnostics_tests.inc` | 190 |
| 4 `factory`, 2 `reset` | `admin_route_reset_tests.inc` | 150 |
| includes, `#include` lines, `main` | `test_web_server_administration_route.c` | 90 |

- [x] **T8-001** Extract the fixture fragment (the single biggest win here). —
      388 lines.
- [x] **T8-002** Extract the four topic fragments above. — settings 239,
      reset 176, diagnostics 173, session 108. **Grouping changed from the
      table:** suites are grouped by *contiguous* source range —
      session+restart, settings+change-password,
      reset-settings+factory-reset+setup, diagnostics — so each fragment stays a
      pure move. The table put `setup` with `session`, but `setup`'s tests sit
      at 979–1014, between factory-reset and diagnostics; honouring the table
      would have made that fragment a non-contiguous gather for no benefit.
- [x] **T8-003** Confirm `main` registers all 29 tests, CTest count unchanged. —
      `main()` byte-identical, 29 registered, web suite 31/31.

**Verify:** `./scripts/run-tests.sh web`, then `./scripts/check-format.sh`, then
`./scripts/check-all.sh`.

**Evidence:** commit `6f6f52d`. 1229 → 149 + 388 + 239 + 176 + 173 + 108.
`./scripts/check-all.sh` → `EXIT=0`, 66/66 host tests. The `.c` lands at 149
rather than the estimated ~90 because it keeps a 65-line file doc comment and
33 includes.

---

## T9 — `run-v2-035-hardware.py` (1081 → ~150 entry + package) <a id="t9"></a>

**Highest risk in this document. Do not start it before T1–T8 are done.**

Three hard constraints, all verified in §3.3:

1. `scripts/run-v2-035-hardware.py` **must keep its exact path** — ~10 evidence
   documents cite it, `scripts/run-h5-055-hardware.py` depends on it, and
   `tests/scripts/test-v2-035-hardware.py:17` loads it via
   `importlib.util.spec_from_file_location`.
2. A module loaded by `spec_from_file_location` has **no package context**, so
   `from v2_035_support import …` will not resolve without `scripts/` on
   `sys.path`. Solve this deliberately, in the entry point, and document it in a
   comment — do not discover it at CI time.
3. `scripts/run-h5-055-hardware.py` imports from this module. Whatever it
   imports must remain importable under the same name.

Proposed package `scripts/v2_035_support/`, entry point unchanged:

| Contents (source lines) | Destination | ~Lines |
| --- | --- | ---: |
| `sha256_*`, `read_json`, `write_json`, `load_state`, journal helpers (62–219) | `evidence.py` | 160 |
| `deterministic_bytes`, `gzip_member`, `exact_gzip_payload`, `small_payload` (115–156) | `payloads.py` | 50 |
| `DeviceApi`, `parse_success`, `blob_ids`, `snapshot`, `verify_snapshot` (281–396) | `device_api.py` | 120 |
| manifest/ELF/diagnostics provenance (78–114, 226–280, 403–431) | `provenance.py` | 130 |
| pending-creation and reconciliation logic (502–660) | `reconciliation.py` | 160 |
| `command_*` handlers, `argparse` wiring, `main` | `run-v2-035-hardware.py` | 150 |

- [ ] **T9-001** **Before moving anything**, confirm exactly which names
      `run-h5-055-hardware.py` and `tests/scripts/test-v2-035-hardware.py`
      reach into. Write the list into this task. Those names must remain
      accessible from the entry-point module after the split — re-export if
      needed.
- [ ] **T9-002** Create the package and move `payloads.py` **only**. Run both
      dependents. This proves the `sys.path` approach on the smallest possible
      move; if it fails, nothing else has been disturbed.
- [ ] **T9-003** Move `evidence.py`, then `device_api.py`, then
      `provenance.py`, then `reconciliation.py` — re-running
      `python3 tests/scripts/test-v2-035-hardware.py` after each.
- [ ] **T9-004** Confirm the entry point is under 800 and that
      `scripts/run-h5-055-hardware.py` still runs.
- [ ] **T9-005** Confirm **no evidence document's cited path is invalidated**.
      Grep `docs/implementation-v2/` for `run-v2-035-hardware` and check each
      citation still resolves.

**Cannot be verified on hardware as part of this task.** This is a hardware
harness; `check-scripts.sh` runs its *regression tests*, not the harness against
a board. Splitting it is behaviour-preserving by construction, but **do not
record hardware validation from a green `check-all.sh`** — say explicitly that
the harness was not re-run against the device, or re-run it against the board
and cite that separately.
**Verify:** `./scripts/check-scripts.sh`, then `./scripts/check-all.sh`.

---

## T10 — `run-browser-tests.mjs` (2401 → ~80 entry + modules) <a id="t10"></a>

Largest file, but low risk: it is a standalone Node driver with no importers,
run by `npm --prefix webapp run test:browser`. Sequenced last because it is the
biggest single move, not because it is dangerous.

| Contents (source lines) | Destination | ~Lines |
| --- | --- | ---: |
| send-model state machine: `nextSendId`, `advanceSend`, `isTerminal` (40–47, 188–234, 548–567) | `lib/sendModel.mjs` | 110 |
| HTTP helpers: `assert`, `contentType`, `sendJson`, `sendError`, `requestBody`, `rawRequestBody` (129–187) | `lib/http.mjs` | 60 |
| `startApplicationServer` (235–547) | `fixtures/applicationServer.mjs` | 315 |
| `startStartupFixtureServer` (568–888) | `fixtures/startupFixtureServer.mjs` | 320 |
| `evaluate`, `waitFor`, `clickButton`, `clickButtonByAriaLabel` (889–938) | `lib/page.mjs` | 50 |
| touch targets, overflow, viewport, responsive (939–1121) | `lib/layout.mjs` | 185 |
| `runAccessibilityScan` (2298–2321) | `lib/accessibility.mjs` | 25 |
| `runBrowserWorkflows` (1122–1392) | `workflows/browser.mjs` | 270 |
| `runSnapshotsWorkflows` (1393–1584) | `workflows/snapshots.mjs` | 190 |
| `runSettingsWorkflows` + `runUsbUnavailableWorkflow` (1585–1698) | `workflows/settings.mjs` | 115 |
| the six `runStartup*` scenarios (1699–2028) | `workflows/startup.mjs` | 330 |
| `runMacroEditingWorkflows` (2029–2297) | `workflows/macroEditing.mjs` | 270 |
| `main` and wiring | `run-browser-tests.mjs` | 80 |

- [ ] **T10-001** Extract `lib/` first (`http`, `sendModel`, `page`, `layout`,
      `accessibility`) — the fixtures and workflows depend on them, so leaf-first
      avoids circular imports.
- [ ] **T10-002** Extract `fixtures/` (both servers).
- [ ] **T10-003** Extract `workflows/` (six modules).
- [ ] **T10-004** Reduce `run-browser-tests.mjs` to `main` plus wiring; confirm
      every new file is under 800.

**Optional follow-up, not required:** `run-h4-recovery-tests.mjs` (435) and
`run-h5-storage-reconciliation-tests.mjs` (453) likely duplicate some of the
helpers extracted in T10-001. Once `lib/` exists they *could* reuse it. That is
a separate deduplication task with its own risk, and it is **out of scope here**
— neither file is over 800 lines, so nothing in Phil's instruction asks for it.

**Verify:** `npm --prefix webapp run test:browser` (runs all three drivers),
then `./scripts/check-all.sh`. Browser tests need Playwright Chromium installed.

---

## 4. Sequence

T0 → T1 → T2 → T3 → T4 → T5 → T6 → T7 → T8 → T9 → T10

Ordered lowest-risk-first within each class, so the pattern is proven on a small
file before it is applied to a large one: one component split (T1), then the two
harder components (T2, T3), then test files smallest-first (T4→T6), then the C
fragments (T7, T8) once T0 has closed the format gap, then the two
infrastructure files (T9, T10).

T0 blocks only T7 and T8. Everything else is independent — if a task stalls,
skip it and record why rather than blocking the queue.

## 5. Open decisions for Phil

1. **R4-040 / R4-041 ordering.** T2 and T3 touch the two files with open Round 2
   findings. Recommendation is fix-first for T2, either order for T3. Neither
   this document nor the refactor should be treated as closing those findings —
   a split does not fix a lifecycle bug.
2. **Is 800 a target or a floor to stop at?** Several tasks land well under it
   (T7 → ~120, T8 → ~90, T10 → ~80). That is a consequence of splitting at real
   seams, not padding. If you would rather have fewer, larger files, T4–T6 are
   the ones to merge back.
3. **T0-004** proposes a new regression test for `check-format.sh`. That adds a
   gate. Say if you would rather just extend the glob without the test.

## 7. Corrections to this document, found while executing it

Recorded rather than silently edited, because a plan that quietly rewrites
itself is not evidence of anything.

1. **Test counts were measured wrong.** The per-file counts in T4–T6 came from
   `grep -cE '^\s*(it|test)\('`, which misses `test.each` blocks. Authoritative
   counts from vitest: `v2-macros-page` **44** (not 41), `v2-app-v2` **14** ✓,
   `v2-snapshots-page` **41** ✓. Use vitest's own count as the before/after
   check, never a grep.
2. **T0-002's formatter instruction was wrong** — see §3.1a. It named apt
   clang-format 18 on the stated grounds that CI uses it. CI uses esp-clang 19.
3. **T2's table put `MoveAction` in the wrong destination file.** It is shared
   between `MacroRowProps` and `MacrosPage`, so it belongs in
   `macroSendStatus.ts`, not `MacroOverflowMenu.tsx`.

## 6. Out of scope

- Any behaviour change, bug fix, or API change (§2.2).
- Deduplicating the three browser drivers (see T10's follow-up note).
- The 26 existing `.inc` fragments beyond the 2 that T0-002 reformats.
- Any file not in the top-10 list, including
  `webapp/src/features/startup/v2/RepositoryStartupScreen.tsx` at 698 lines —
  closest to the line, still under it.
- Adding an enforced line-count gate. Phil asked for a target, not a ratchet.
