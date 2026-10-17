# ESP32 Macro Keyboard — UI/UX Redesign Review Fixes TODO

**Status:** Closed for the agreed scope; spoken screen-reader pass waived
**Spec:** `docs/UI_UX_REDESIGN_REVIEW_FIXES_SPEC_2026-10-17.md`
**Review baseline:** `82eb7d1` on 2026-10-17
**Scope:** UXRF-001 through UXRF-005; follow-up to
`docs/UI_UX_REDESIGN_TODO_2026-10-15.md`

## 0. Working rules

- [x] Confirm the baseline SHA and current code paths before editing; record
      any behavior that changed since the review.
- [x] Keep `docs/SPEC_V2.md` and `docs/UI_UX_SPEC_V2.md` frozen. Escalate any
      genuine conflict for explicit per-change product-owner approval.
- [x] Preserve the current API, firmware send/release semantics, in-memory
      repository storage, source privacy, and local-only assets.
- [ ] Do not mark a checkbox complete without the exact implementation SHA,
      reproducible command/results, and manual evidence where required.

## 1. Repository save integrity — UXRF-001

- [x] Add a regression that pauses snapshot POST after capturing version A,
      edits the working copy to B, then returns `201`; assert B survives, dirty
      remains true, and A is the saved baseline.
- [x] Add regressions for a late response after deliberate replacement/discard,
      upload failure, commit uncertainty, and overlapping Save attempts.
- [x] Implement version/lineage-aware save completion across
      `snapshotClient.ts`, `repositoryWorkingCopy.ts`, and `AppV2.tsx` so a late
      response never overwrites newer edits or an unrelated working copy.
- [x] Ensure loaded snapshot identity and Saved/Unsaved changes indicators
      describe the live copy after each race; verify dirty-work prompts do not
      proceed while a newer edit remains unsaved.
- [x] Run focused repository/snapshot/AppV2 tests and browser workflows; record
      the command, result, and code SHA in the evidence section below.

## 2. Subpage focus — UXRF-002

- [x] Add browser regressions for Settings root/group/section, Snapshots
      list/files, browser Back/Forward, and initial load at phone and desktop
      widths; assert a visible, appropriate focus target after navigation.
- [x] Make focus management respond to the actual subpage location, not only
      the top-level `ScreenV2` route; preserve dialog/menu focus restoration.
- [x] Verify focus order, scroll position, and active navigation indication
      with keyboard-only operation and automated accessibility checks.
- [x] Run focused routing/shell/browser tests and record command, result, and
      code SHA in the evidence section below.

## 3. Cancel during send startup — UXRF-003

- [x] Add a delayed-POST regression showing Cancel on Macros, another route,
      and the phone-landscape surface throughout the starting state.
- [x] Add sequencing regressions: early Cancel + `202` sends one DELETE after
      acceptance; early Cancel + failure sends none; repeated Cancel is
      idempotent; `409`/uncertain recovery does not auto-cancel an unidentified
      send.
- [x] Implement a pending-cancel intent in the send lifecycle and keep its
      status visible until accepted cancellation or a clear failure/recovery
      outcome. Preserve bounded polling and separate release-error reporting.
- [x] Verify navigation and accepted-send recovery behavior; pending-start
      reload/reconnect remains covered by the existing send recovery contract
      and is not claimed as a new browser qualification. Record evidence below.

## 4. Settings export errors — UXRF-004

- [x] Add tests for `exportRepository` rejection and `saveAsFile` exception in
      the Settings unsaved-work prompt; assert visible actionable error, intact
      dirty copy, open prompt, cleared busy state, and retry/Cancel.
- [x] Handle both error paths in `SettingsPage.tsx` without unhandled promise
      rejection or sensitive text disclosure; keep successful export separate
      from the Save/Discard decision.
- [x] Run focused Settings and dirty-work prompt tests; record command,
      result, and code SHA in the evidence section below.

## 5. Dialog accessibility — UXRF-005

- [x] Audit every shared `Dialog` caller to distinguish modal from nonmodal
      behavior and record the expected name, focus target, dismissal, and
      background interaction for each.
- [x] Add modal semantics and background isolation where appropriate; preserve
      keyboard focus trapping/restoration and the unsaved-work prompt's
      protection against accidental dismissal.
- [x] Add automated DOM/accessibility-tree and keyboard regressions for a
      confirmation dialog, an unsaved-work prompt, and the editor dialog.
- [x] The product owner waived the human spoken screen-reader pass for this
      review on 2026-10-17. It was not performed; no spoken announcement or
      assistive-technology support qualification is claimed.

## 6. Final regression and reconciliation

- [x] Run the focused checks above and `./scripts/check-webapp.sh`; record
      exact commands, counts/results, and implementation SHA. Review screenshots
      for unintended visual changes.
- [x] Re-audit UXR-020, UXR-041, UXR-042, UXR-051, UXR-062, UXR-071, and
      UXR-082 against the fixed code and tests. Update the original redesign
      TODO only where exact evidence supports a changed status.
- [x] Keep physical-device, device-served offline, constrained-performance,
      full manual keyboard/screen-reader, and deferred Safari checks open until
      their own qualification evidence exists.
- [x] Close this TODO only when UXRF-001 through UXRF-005 meet every remaining
      acceptance case, the full regression gate completes without failure, and
      required manual evidence is recorded. The owner-waived spoken
      screen-reader pass is excluded; document any residual limitation.

## 7. Implementation and verification evidence

- Implementation commit: `f547d69` (`fix(webapp): address UI review
  findings`), based on review baseline `82eb7d1`. It is committed on `master`.
  The full regression gate remains open because the Chrome browser phase crashed.
- Focused unit tests passed for snapshot save races, send startup cancellation,
  Settings export failures, dialog isolation, macro editor dialogs, and
  phone-landscape active-send cancellation. The full webapp unit and coverage
  runs passed: `npm run test` and `npm run test:coverage`, 719 tests across 67
  files; coverage was statements 89.09%, branches 84.88%, functions 91.51%, and
  lines 89.15%.
- `./scripts/check-webapp.sh` passed end to end on 2026-10-17: Prettier,
  typecheck, ESLint, Stylelint, 719 unit tests across 67 files, coverage
  thresholds (89.09% statements, 84.88% branches, 91.51% functions, 89.15%
  lines), production builds, Chrome workflows including subpage focus and H4/H5
  recovery, all 97 visual captures, orphan-class checks, status-badge shape
  checks, and the local-asset check.
- `bash ./scripts/check-docs.sh` passed with zero Markdown issues; existing
  yamllint warnings remain. `git diff --check` passed before the final evidence
  update and will be rerun before the documentation follow-up commit.
- The spoken screen-reader pass was waived by the product owner on 2026-10-17;
  it was not performed and no support qualification is claimed. This follow-up
  TODO is closed for its agreed automated scope. Full manual keyboard and
  screen-reader qualification, Safari (deferred by the owner), and physical
  ESP32-S3/device-served offline/constrained-performance qualification remain
  open in the original redesign tracker.
