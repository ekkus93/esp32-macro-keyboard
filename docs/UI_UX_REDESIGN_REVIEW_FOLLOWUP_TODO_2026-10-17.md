# ESP32 Macro Keyboard — UI/UX Review Follow-up TODO

**Status:** Complete for the agreed automated scope; manual/device qualifications remain open
**Spec:** `docs/UI_UX_REDESIGN_REVIEW_FOLLOWUP_SPEC_2026-10-17.md`
**Review baseline:** `e7f5366` on 2026-10-17
**Scope:** UXRF2-001 through UXRF2-005; follow-up to
`docs/UI_UX_REDESIGN_REVIEW_FIXES_TODO_2026-10-17.md`

## 0. Working rules

- [x] Confirm current code paths and baseline SHA before editing; record any
      behavior that changed since this review.
- [x] Keep `docs/SPEC_V2.md` and `docs/UI_UX_SPEC_V2.md` frozen. Record any
      genuine conflict and obtain explicit per-change approval before editing
      normative content.
- [x] Preserve the API, firmware release-all behavior, in-memory repository,
      source privacy, and local-only assets.
- [x] Complete a checkbox only with exact implementation SHA, reproducible
      command and result, and manual evidence where its claim requires it.

## 1. Terminal send reconciliation — UXRF2-001

- [x] Add focused regressions for `409` and uncertain POST followed by a
      terminal GET with `releaseError`; cover failed, cancelled, timed-out,
      and completed outcomes.
- [x] Preserve and display the terminal outcome and separate, persistent
      key-release warning without an automatic DELETE or second POST.
- [x] Verify active GET recovery retains explicit Cancel and no-send GET gives
      accurate guidance; cover navigation and warning dismissal.
- [x] Run focused send/recovery tests and browser workflow; record command,
      result, and implementation SHA.

## 2. Recovery overlay and dialogs — UXRF2-002

- [x] Define and document one focus and priority policy for a dialog and
      execution-recovery warning that coexist, including which Cancel control
      remains available and what happens when either surface closes.
- [x] Add automated DOM regressions for both modal mount orders and a real
      Chrome regression for recovery appearing over an open dialog, including
      Cancel availability and focus restoration. The recovery-first case is
      DOM-tested because the active recovery modal correctly blocks user input
      to the background application.
- [x] Implement dynamic background isolation and focus ownership without
      stale `inert` attributes or two interactive modal surfaces.
- [x] Recheck confirmation, unsaved-work, and editor dialogs for names,
      keyboard trapping, restoration, Escape behavior, and dirty-work safety.
- [x] Run focused dialog/recovery/browser checks and record commands, results,
      and implementation SHA. Do not claim a spoken screen-reader pass.

## 3. Snapshot identity copy — UXRF2-003

- [x] Add a regression for Save A, edit to B before `201`, and assert the
      snapshot row, accessible name, and Unsaved changes status accurately
      describe B and its saved baseline A.
- [x] Make the snapshot identity wording or presentation distinguish a clean
      loaded copy from a dirty copy based on the indicated snapshot.
- [x] Cover discard, load, import, replace, delete, failed upload, and
      commit-uncertain states; verify no unrelated blob is marked current.
- [x] Run focused store/snapshot/AppV2 and browser checks; record commands,
      results, and implementation SHA.

## 4. Export privacy and retry — UXRF2-004

- [x] Add Settings tests that inject source and disposable-credential sentinels
      in both encoding and file-save exception messages; assert neither
      appears in rendered text or logs.
- [x] Bound user-facing export errors while keeping them actionable; apply
      the same protection to the equivalent Snapshots export path.
- [x] Drive a successful retry after each failure and prove busy/error state,
      dirty copy, open prompt, and pending destructive action remain correct.
- [x] Verify Cancel after failure and that successful export alone does not
      authorize Save, Discard, sign-out, reset, or factory reset.
- [x] Run focused Settings/Snapshots/prompt tests; record commands, results,
      and implementation SHA.

## 5. Evidence and checklist reconciliation — UXRF2-005

- [x] Correct the previous TODO's contradictory gate summary by separating
      the earlier Chrome crash from the later successful run; retain dates,
      SHA, exact commands, exits, and counts.
- [x] Review the previous TODO's unchecked evidence rule and every completion
      claim affected by UXRF2-001 through UXRF2-004; reopen or annotate an
      overclaim rather than silently treating a checked box as proof.
- [x] Assert focus scroll position and active navigation indication in a
      keyboard/browser regression, or narrow the earlier claim explicitly.
- [x] Test delayed-POST reload/reconnect if claiming that exact behavior; if
      relying only on the established recovery contract, state its limit.
- [x] Record screenshot artifact paths, comparison method, and visual review
      outcome separately from the capture count.
- [x] Reconcile the original redesign TODO's affected UXR items and retain
      the waived spoken pass and outstanding manual/device/Safari items.

## 6. Final qualification

- [x] Run all focused checks with recorded commands and results at the final
      implementation SHA.
- [x] Run `./scripts/check-webapp.sh` and applicable broader gates; record
      exit statuses, counts, and any failures followed by later passing runs.
- [x] Review the final screenshots for unintended visual changes and record
      the artifacts and conclusion.
- [x] Close UXRF2-001 through UXRF2-005 only when every acceptance case in the
      companion spec has implementation and reproducible evidence. Keep the
      waived spoken pass and physical-device, device-served offline,
      constrained-performance, full manual keyboard, and deferred Safari
      qualifications separate.

## 7. Implementation and verification evidence

### Implementation

- Baseline: `e7f5366a3a05edbc65f866345091c95ba1580e44` on 2026-10-17.
- Implementation commit: `66276c02e9dcf66972522ef967f78f5d82d02e4f`
  (`fix(webapp): close UI review follow-up findings`), committed on `master`.
- Frozen normative files `docs/SPEC_V2.md` and `docs/UI_UX_SPEC_V2.md` were
  unchanged. No API or firmware behavior was changed.

### Focused checks

- `npm --prefix webapp run test -- --run tests/v2-execution-recovery.test.tsx tests/v2-macros-page-send.test.tsx tests/v2-snapshots-page-management.test.tsx tests/v2-snapshots-page-transfer.test.tsx tests/v2-settings-page.test.tsx` — passed; 5 files, 87 tests.
- `npm --prefix webapp run typecheck` — passed.
- `npm --prefix webapp run lint` — passed with zero warnings.
- `npm --prefix webapp run test:browser` — passed on real Chrome. Macros,
  Snapshots/import-export, Settings/Diagnostics, axe, reduced motion, USB
  unavailable, startup/recovery, macro/package editing, subpage focus, visible
  controls, H4 degraded-send recovery, and H5 uncertain-commit/no-duplicate-POST
  workflows passed. The release-error banner remained visible after navigating
  Macros → Packages → Macros.
- Focus scroll and active navigation assertions are in
  `webapp/tests/browser/workflows/visibleControls.mjs`: subpage navigation
  requires `#main-content.scrollTop === 0` and the matching
  `aria-current="page"` navigation item.
- Delayed-POST reload/reconnect remains a stated limitation. The existing
  recovery contract and H4 reload case cover recovery of a known accepted send;
  this work does not claim that a reload during an unresolved POST is covered.

### Full gates and failures retained

- `./scripts/check-webapp.sh` — passed, exit 0, on the implementation contents
  in commit `66276c02e9dcf66972522ef967f78f5d82d02e4f`. Unit and coverage runs:
  727 tests across 67 files; coverage 89.20% statements, 85.05% branches,
  91.41% functions, 89.25% lines. Browser workflows passed and all 97 visual
  scenario/viewport captures matched. Orphan-class, StatusBadge-shape, and
  local-asset checks passed. Full output: `/tmp/uxrf2-check-webapp-final.log`.
- `. "$HOME/esp/esp-idf-v5.5.5/export.sh"; PATH="$(go env GOPATH)/bin:$PATH" ./scripts/check-all.sh` — passed, exit 0, on the same implementation
  commit. Firmware and device-test-app builds passed; stack policy analyzed 688
  first-party frames (largest 1536 bytes); all 63/63 host tests passed. Script,
  contract, release-budget, webapp, and documentation gates passed. Full
  output: `/tmp/uxrf2-check-all-implementation.log`.
- Earlier attempts are retained accurately: `/tmp/uxrf2-check-all.log` exited
  1 because `useFocusTrap.ts` needed Prettier formatting; after formatting,
  `/tmp/uxrf2-check-all-final.log` exited 1 because the four intended visual
  changes had not yet been baseline-reviewed. Only the two recovery-overlay
  and two unsaved-changes-prompt baselines were then updated. The final
  `check-all.sh` and `check-webapp.sh` runs passed.
- A later standalone `npm --prefix webapp run test:browser` attempt exited 1
  because the H4 workflow did not observe the release-error banner after
  navigation to Packages. An immediate rerun passed the full browser workflow,
  including H4 and H5. The subsequent full webapp gate also passed at
  `ac6f4e8b32f33f99ba9fe533e4f395ade3774a58`; the transient failure was
  retained here rather than removed from the evidence record.
- `./scripts/check-docs.sh` — passed, zero Markdown issues. It reports existing
  yamllint warnings in workflow files; no new warning was introduced.
- `git diff --cached --check` passed for the implementation commit. The final
  docs-only ledger change is checked again before its commit.

### Screenshot review

- Captured with:
  `node tests/browser/visual/screenshot-set.mjs --out docs/ui-redesign/followup-review-2026-10-17 --scenarios execution-recovery-overlay,dialog-restart,unsaved-changes-prompt --viewports 390x844,1440x900`.
- Six reviewed PNGs are committed under
  `docs/ui-redesign/followup-review-2026-10-17/`. The phone and desktop
  recovery/dialog captures were inspected: the recovery warning and both
  actions are visible, the app behind the modal is dimmed, and the prompt
  remains readable and actionable at phone width. This is visual review of
  these captured states, not owner signoff or assistive-technology evidence.
- Changed visual-regression files are the JSON baselines for
  `execution-recovery-overlay` at 390×844 and 1280×900, and
  `unsaved-changes-prompt` at 390×844 and 1280×900. They were updated using
  `node tests/browser/visual/run-visual-tests.mjs --update-baselines --grep execution-recovery-overlay`
  and
  `node tests/browser/visual/run-visual-tests.mjs --update-baselines --grep unsaved-changes-prompt`;
  final verification matched all 97 captures.

### Scope limits

- The product-owner waiver of the spoken screen-reader pass still applies. No
  spoken screen-reader or full manual keyboard qualification is claimed.
- Manual ESP32-S3 key-release behavior, device-served offline operation,
  constrained-device performance, and Apple Safari remain outstanding. Safari
  is deferred because the owner has no Safari access. The original redesign
  TODO retains those qualification items as open.

### Supplemental keyboard route automation

- Test-only follow-up commit:
  `ac6f4e8b32f33f99ba9fe533e4f395ade3774a58` (`test(webapp): audit keyboard
  route focus in Android Chrome`). It strengthens route navigation to use Tab,
  Enter, and Escape, audits visible main-area tab stops and focus indicators,
  and runs the route check in the Android Chrome emulator.
- `./scripts/check-webapp.sh` — passed, exit 0 at that commit; 727 tests across
  67 files, coverage 89.20% statements, 85.05% branches, 91.41% functions,
  89.25% lines, browser workflows passed, and all 97 visual captures matched.
  Full output: `/tmp/uiux-followup-check-webapp-ac6f4e8.log`.
- `./scripts/check-android-offline-visuals.sh` — passed, 46 Android Chrome
  offline visual scenarios plus keyboard-only subpage navigation/focus checks.
  This uses an emulator and a local fixture; it does not qualify ESP32-served
  offline behavior or physical-device performance.
- `cd webapp && node --input-type=module -e 'import { chromium } from
  "playwright"; import { runSubpageFocusWorkflow } from
  "./tests/browser/workflows/visibleControls.mjs"; const browser = await
  chromium.launch({ headless: true }); try { await
  runSubpageFocusWorkflow(browser); } finally { await browser.close(); }'` —
  passed in desktop Chrome at 390×844 and 1440×900.
- This adds automated route and focus coverage. It does not claim the full
  manual keyboard qualification or a spoken screen-reader pass.
