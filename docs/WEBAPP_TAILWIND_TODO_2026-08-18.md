# Webapp Tailwind Styling — Hardening TODO

**Status: live, not started.** Written 2026-08-18. Governing document:
`docs/WEBAPP_TAILWIND_SPEC_2026-08-18.md`. Predecessor:
`docs/WEBAPP_TAILWIND_UTILITY_MIGRATION_TODO_2026-08-17.md` — complete, all 29
tasks closed, do not reopen it.

Every task here comes from the post-migration code review, not from
speculation. Each one names the defect it prevents and the evidence that would
close it.

---

## 0. How to use this document

### 0.1 Task discipline

- Work tasks in order within a phase; phases in order.
- One task per commit. Do not combine unrelated changes.
- Do not tick a box without a commit SHA and reproducible commands.
- If a task turns out to be wrong or unnecessary, say so in its evidence line
  and leave it ticked with that finding. Do not silently drop it.
- If you hit a defect this document did not anticipate, fix it forward in its
  own commit and record it here.

### 0.2 The rule that matters most

The migration this follows produced three defects that reached `master` inside
tasks whose evidence line said "byte-identical". All three were found by a
check the task itself did not run. **Assume your verification has a blind spot
and name it in the evidence line.**

### 0.3 What "done" means for a styling change

`./scripts/check-webapp.sh` exiting 0 is necessary and not sufficient — it
asserts nothing about spacing, colour or pseudo-element shape
(`SPEC` §10.1). Until T1-1 lands, a cross-tree visual diff by the method in
`SPEC` §10.2 is also required.

---

## 1. Phase 1 — Make the verification reproducible

The whole migration's correctness rests on diffs that were run by hand and
then deleted. Nothing in CI can catch their recurrence. This phase is the
highest-value work in the document and everything else depends on it.

- [x] **T1-1** Check in the visual-regression harness. Promote the throwaway
      probe into `webapp/tests/browser/visual/`: the element walk (the fixed
      property list from `SPEC` §10.2), the comparator that comments as sorted
      key→value maps, and a scenario driver. It must accept a baseline
      directory so it can run tree-vs-tree, and must fail loudly rather than
      silently skip a scenario it could not reach.
      *Evidence:* `3d77a1f`. `props.mjs` (the curated property list, plus a
      separate pseudo-element list for `StatusBadge`'s `::before` shapes),
      `capture.mjs` (the element walk + screenshot), `compare.mjs`
      (sorted key→value diff), `scenarios.mjs` (35 independent scenarios —
      every ordinary page, every dialog/danger-zone state, all four USB
      badge states, both landscape-overlay states, the three send banners,
      and every pre-auth/pre-provisioning screen reachable by a fixture),
      and `run-visual-tests.mjs` (the driver). `--baseline-dir` verified
      tree-vs-tree: wrote a baseline to a scratch directory, compared
      against it successfully, independent of the checked-in baseline path.
      "Fail loudly" verified: with no baseline present, every scenario
      reports `no baseline at … -- run with --update-baselines` and the run
      exits nonzero — a missing baseline is a failure, not a skip.
      **Verified the harness actually catches a regression**, not just that
      it runs: injected `Card`'s `default` margin `my-3` → `my-6`, rebuilt,
      and the diff named the exact element, `margin-top: "12px" -> "24px"`,
      `margin-bottom` likewise, plus the correct downstream position shift
      on every element below it in the document — then reverted and
      confirmed clean again. The full 77-capture set (two boundary-width
      scenarios included, proving the pattern T1-5 extends) ran clean three
      times in a row, ~11s each. One genuine timing race was found and
      fixed during that stabilization: `send-in-flight` originally waited
      for the "starting" lifecycle text, which exists only until the first
      1000ms status poll resolves (`sendClient.ts`'s `pollIntervalMs`) and
      is inherently racy to capture; it now waits for the "active" state's
      Cancel button, which holds for a full poll interval. `npm run
      format:check` / `lint` / `typecheck`: clean (required a
      `.prettierignore` entry for the not-yet-committed baseline JSON,
      included in this commit). Baselines themselves are not committed
      here — that is T1-2.
- [x] **T1-2** Commit baselines for the scenario set in `SPEC` §10.4, at the
      viewports in §10.3. Decide and document where baselines live (in-repo
      PNG + JSON, or JSON only with screenshots on demand) — in-repo binaries
      have a real cost in a firmware repo that also ships a flash image, so
      justify the choice in the evidence line rather than defaulting.
      *Evidence:* `2864bd4` (prep: extracted the H4 recovery fixture so the
      execution-recovery-overlay scenario could share it rather than
      duplicate a second copy of that server — verified behavior-preserving
      by running `run-h4-recovery-tests.mjs` unchanged against the extracted
      version) and `d67c7e7` (the baselines). **Decision: JSON only, no
      PNG**, recorded in full in `baselines/README.md`: the property walk
      already caught everything a screenshot did during the review that
      produced this harness, plus differences a screenshot structurally
      cannot see (inside a scroll container, behind a modal overlay); PNG
      baselines at two viewports per scenario would roughly double this
      directory's cost in a repo that also ships a flash image; and a
      changed PNG shows nothing in a `git diff`, while the JSON shows the
      exact property that moved. Screenshots are still captured every run
      and written to `--diff-dir` on failure, for human debugging, never
      committed as truth. Every screen `SPEC` §10.4 names is covered — the
      execution recovery overlay was the one entry still missing after
      T1-1, added here using the newly-extracted fixture. 79 baseline files
      (one per scenario/viewport pair), ~14MB, at the viewports `SPEC` §10.3
      requires. Verified: the full 79-capture set regenerated from a clean
      slate (`rm -rf baselines && … --update-baselines`, so no leftover
      state from T1-1's smaller subset) and then ran clean three times in a
      row. `npm run test`: 544/544. `check-webapp.sh`: EXIT=0.
      `check-docs.sh`: EXIT=0 — `markdownlint-cli2` scans `**/*.md` from the
      repo root, so it reaches `baselines/README.md` too.
- [x] **T1-3** Wire it into `check-webapp.sh` after the browser workflows.
      Must be deterministic: fixed viewports, no animation timing races, no
      wall-clock in any fixture. If a scenario proves flaky, quarantine that
      scenario explicitly — do not add a retry that hides it.
      *Evidence:* `0671ffa`. `npm run test:visual` added to `package.json`
      and called from `check-webapp.sh` immediately after
      `npm run test:browser`. Verified the wiring actually fails the gate,
      not merely that it runs: reused the T1-1 injected regression (Card's
      `default` margin, `my-3` → `my-6`), ran `npm run test:visual`
      standalone — exit 1, 30 differences reported across every `Card` call
      site the scenario set touches — reverted, and confirmed exit 0. Ran
      the full `check-webapp.sh` end to end afterward: **EXIT=0, ~1m47s
      total**, the visual step contributing roughly 11–15s of that.
      **No scenario proved flaky against this wiring; none needed
      quarantining.** (One genuine race was found and fixed while building
      the scenario set itself, at T1-1 — `send-in-flight` — and is not
      counted again here since it was fixed before this task, not
      discovered by it.)
- [x] **T1-4** Document baseline updates in `docs/DEVELOPMENT.md`: how to
      regenerate, and the rule that a baseline change must be reviewed as a
      deliberate visual change, never as a merge artefact.
      *Evidence:* `45f1bfd`. New "Frontend visual-regression baselines"
      section: what `test:visual` checks and why the rest of the gate
      cannot (cites `SPEC` §10.1), the rule to read the diff before doing
      anything else, the regenerate command, and — the "never as a merge
      artefact" requirement — that the resulting `git diff` on the baseline
      JSON must be reviewed like any other change and committed as its own
      commit, separate from whatever caused it. Points to
      `baselines/README.md` for the full workflow rather than duplicating
      it, matching this document's existing terse style (53 lines before
      this addition). `check-docs.sh`: EXIT=0.
- [x] **T1-5** Add the boundary-width scenarios: every threshold in `SPEC`
      §5.3 at exactly `X` and `X±1px`. This is the check that would have
      caught the `max-[X]:` defect on the day it was written.
      *Evidence:* `46a9186`. Six new scenarios cover the six thresholds T1-1
      hadn't reached (`macros-32rem-boundary`, `macros-34rem-boundary`,
      `macro-editor-42rem-boundary`, `macros-60rem-boundary` and
      `signin-60rem-boundary` for the shell/standalone width pair, and
      `macro-editor-short-viewport-boundary` for the one height-based
      threshold — replacing the single arbitrary 600px point T1-1 shipped
      with a real 607/608/609px sweep). Combined with T1-1's two
      (`snapshots-storage-summary-boundary` at 26rem,
      `dialog-restart-heading-boundary` at 40rem), **every threshold in
      `SPEC` §5.3 now has a boundary scenario.** Verified each is real, not
      a vacuous three-times-identical capture: diffed each threshold's
      `-1px` capture directly against its `+1px` capture (not against
      baseline) and confirmed every pair differs — 23 to 196
      property/geometry changes each, none zero. The full 96-capture set
      (was 79) regenerated from a clean slate and ran clean three times in
      a row. `npm run test`: 544/544. `check-webapp.sh`: EXIT=0.

**Phase 1 complete.** The visual-regression gap `SPEC` §10.1 named — the
gate asserting nothing about spacing, colour or geometry — is closed: 96
scenario/viewport captures, every `SPEC` §10.4 screen, every `SPEC` §5.3
boundary, wired into `check-webapp.sh`, documented for maintainers. Proven
twice over on two separate injected regressions (T1-1, T1-3) that it
actually fails the gate, not merely that it runs.

---

## 2. Phase 2 — Cover what nothing covers

- [x] **T2-1** Unit tests for the six variant maps in `SPEC` §4.2. Assert that
      each variant resolves to a distinct, non-empty class string and that no
      variant sets the same property twice. These maps are branching logic
      with no test today; a wrong key is currently caught only by a visual
      diff nobody runs.
      *Evidence:* `0546a49`. **Found and fixed a real, previously-unnoticed
      defect while building this** (`1b4fda5`, its own commit, done first):
      `border` (bare) sets `border-width` on all four sides and
      `border-l-[3px]` sets only `border-left-width` — they collide on the
      left side, and only worked by luck of Tailwind's emission order,
      exactly the `SPEC` §3 rule-4 hazard this task exists to catch. Five
      occurrences across the app (`Card`'s `danger` tone, `Dialog`'s
      `danger` tone, `DangerZone`, `MacroOverflowMenu`, `MacroEditorPage`'s
      validation panel — the last also colliding on border *colour*, not
      just width). Fixed by making every side explicit
      (`border-y border-r border-l-[3px]`) instead of a shorthand plus an
      override, verified render-neutral against the full 96-capture visual
      baseline. `tailwindClassCollisions.ts` is the classifier this task
      needed to find that: deliberately non-general (recognizes exactly the
      vocabulary these six maps use, throws on anything else — §4.1's
      "fail loudly on the unaudited" discipline applied to the checker
      itself), scopes tokens by prefix (`before:`, a descendant selector, a
      breakpoint — different scopes never collide) and models border
      width/colour as four independent per-side slots rather than one
      property each. `component-variant-maps.test.tsx` renders every real
      variant through each component's own public props — exporting the
      module-private class-string maps directly was tried first and
      rejected: it broke Vite Fast Refresh
      (`react-refresh/only-export-components`). **Verified the test has
      teeth**: reintroduced the (by-then-fixed) border collision in Card's
      danger variant, confirmed the test failed naming the exact property
      and the two colliding classes, reverted, confirmed it passed again.
      19 new tests (12 direct classifier tests, 7 component tests).
      `npm run test`: 563/563. `check-webapp.sh`: EXIT=0.
- [x] **T2-2** A test that fails if any element in a rendered page carries two
      utilities setting the same CSS property — the `SPEC` §3 invariant, as an
      executable check rather than a one-off scan. jsdom is enough: this is a
      `className` property, not a rendering one.
      *Evidence:* `71e5b43`. Generalizes T2-1's classifier from the six known
      variant maps to every element on a real, populated page.
      `no-rendered-property-collisions.test.tsx` renders four structurally
      distinct pages — `MacrosPage` (via the existing `macrosPageHarness`),
      `DiagnosticsPage`, `SettingsPage`, `PackageManagementPage` — each
      through its own existing dependency-injection pattern (no fetch
      mocking, no `dist/` dependency: React renders straight from source in
      jsdom, so nothing here needs a prior `npm run build`), and walks
      `querySelectorAll("*")` on each, asserting zero collisions across the
      whole tree.
      **Getting there required expanding `tailwindClassCollisions.ts` well
      past the six components' vocabulary**: checked all 160 distinct class
      tokens used anywhere in `src/**/*.tsx` against it and fixed every
      throw, adding per-side modelling for padding/margin/position (the same
      fix border needed), display/flex/grid/z-index/overflow families, two
      recognized non-utility classes (`.primary`/`.danger`, which live in
      `@layer base` and are therefore beaten by any utility deterministically
      by layer order — not the fragile same-layer rule 4 is about — so they
      correctly claim no property slots), and a generic
      `[property-name:value]` arbitrary-property handler so a future one-off
      utility doesn't need a hand-written table entry.
      **Found and fixed a real false positive in the classifier itself**:
      `gap-x-4 gap-y-3` (`PageHeading`, appears on every page tested) was
      flagged as a collision on one "gap" family — wrong, `column-gap` and
      `row-gap` are independent CSS properties. Split into a proper
      column-gap/row-gap model (bare `gap` claims both axes, `gap-x-`/
      `gap-y-` claim one each) with its own regression test — the inverse
      finding from T2-1's real collision, and worth recording for the same
      reason: the checker itself needs the same "verify, don't assume"
      discipline as the code it checks.
      **Verified the page-level test has teeth**: reintroduced T2-1's
      already-fixed `Card` border collision (`1b4fda5`) and confirmed
      *exactly* the one page rendering `Card`'s `danger` variant
      (`SettingsPage`) failed — `MacrosPage`, `DiagnosticsPage`,
      `PackageManagementPage` correctly stayed green, proving the check is
      precise per-element, not a blunt whole-suite trip-wire. Reverted,
      confirmed clean. `npm run test`: 569/569. `check-webapp.sh`: EXIT=0.
- [ ] **T2-3** A test that fails on a class token that generates no CSS,
      allowlisting exactly the three hooks in `SPEC` §7. This is the direct
      guard against §4.1's silent-failure mode. Needs the built stylesheet, so
      it belongs with the browser tests or as a build-time script.
      *Evidence:*
- [ ] **T2-4** Make `DeviceReconnectScreen` reachable: add `POST
      /api/v1/restart` to `startupFixtureServer.mjs` so the reconnect screen
      can be rendered and diffed. It is the one `StandaloneScreen` call site
      that has never been visually verified, on the component whose padding
      regression went unnoticed for nine tasks.
      *Evidence:*
- [ ] **T2-5** Assert the four `StatusBadge` `::before` shapes are mutually
      distinct in geometry, not only in colour — `UI_UX_SPEC_V2` §14. Compare
      width/height/border-radius/border-width/background across the four
      states and fail if any two are identical.
      *Evidence:*

---

## 3. Phase 3 — Close the latent cascade traps

None of these is a live defect. Each is a loaded gun that fires the first time
someone nests two components.

- [ ] **T3-1** Resolve the `[&_h2]` precedence inversion (`SPEC` §6.1):
      `PageHeading` inside `Card` would now resolve backwards relative to the
      pre-migration stylesheet. Either move the heading utilities onto the
      `<h2>` elements at the six `PageHeading` call sites, where specificity
      is unambiguous, or add a guard that fails if the nesting appears.
      Prefer the former; record which and why.
      *Evidence:*
- [ ] **T3-2** Same treatment for `[&_p]` across `Card`, `SendStatus` and
      `Dialog`. Today's order happens to match the old stylesheet, by luck
      rather than design — an added or reordered utility anywhere could flip
      it with no visible cause.
      *Evidence:*
- [ ] **T3-3** Guard `StandaloneScreen`'s `*:` child rule (`SPEC` §6.2)
      against a `fixed`-positioned direct child, which would be given a 27rem
      width and stop covering the viewport. A comment on the component plus a
      test asserting no direct child is `position: fixed` is probably enough;
      do not restructure the rule without a diff.
      *Evidence:*

---

## 4. Phase 4 — Consistency and rot

- [ ] **T4-1** Sweep the 32 comment references to deleted classes. Most are
      legitimate history ("Replaces the `.card` rule") and should stay. The
      ones to fix are those written in the present tense as if the class still
      governs behaviour — notably `Card.tsx:14`, whose cascade argument is
      stated about two classes that no longer exist, and `Card.tsx:25-26`,
      which grounds its safety claim in three stylesheet rules that are now
      utilities on sibling components. Also `ExecutionRecoveryOverlay.tsx:61`
      and `tests/browser/workflows/settings.mjs:20`.
      *Evidence:*
- [ ] **T4-2** Decide on `prettier-plugin-tailwindcss`. Class ordering is
      unenforced and already inconsistent (`MacroEditorPage.tsx:322` has base
      utilities after variants). Adopting it reorders nearly every class
      string in one commit, which is noisy but mechanical and provably
      render-neutral — order does not affect the cascade (`SPEC` §3 rule 3).
      **This is a judgement call with a real cost; put the options and a
      recommendation to the user rather than deciding unilaterally.** If
      adopted, land the reformat as its own commit with a visual diff.
      *Evidence:*
- [ ] **T4-3** Review the two sharp component APIs: `Dialog` hardcodes
      `role="alertdialog"` (correct for all three call sites today, wrong the
      first time a non-alert dialog is needed) and `DangerZone` derives
      `tabIndex` from `role`. Either is fine to keep; the task is to decide
      deliberately and document the decision, not necessarily to change code.
      *Evidence:*

---

## 5. Verification protocol

### 5.1 Per task

```bash
export NVM_DIR="$HOME/.nvm" && . "$NVM_DIR/nvm.sh" && nvm use 24.18.0
npm --prefix webapp run test          # fast inner loop
./scripts/check-webapp.sh             # before committing
```

### 5.2 Per phase

`./scripts/check-all.sh` must exit 0. Capture the output and check the exit
code; a wall of suppressed third-party clang-tidy warnings is normal.

### 5.3 Any task that touches a `className`

A cross-tree visual diff per `SPEC` §10.2–§10.4, naming which screens were
rendered and which were not. "I did not render X" is an acceptable evidence
line; implying full coverage is not.

---

## 6. Definition of done

- [ ] Every task above ticked with a commit SHA and command evidence.
- [ ] A visual regression in any covered scenario fails `check-webapp.sh`.
- [ ] The `SPEC` §3 no-conflict invariant and the §7 orphan-token rule are
      executable checks, not one-off scans.
- [ ] `docs/SPEC_V2.md` and `docs/UI_UX_SPEC_V2.md` unmodified.
- [ ] No `*.tmp.mjs` left in `webapp/tests/browser/`.
- [ ] `docs/WEBAPP_TAILWIND_SPEC_2026-08-18.md` still accurate — if a task
      changed the architecture, the spec was updated in the same commit.

---

## 7. Explicitly out of scope

- Any change to user-visible behaviour. This document hardens how styling is
  written and proven, not what the interface does. A task that appears to need
  a `SPEC_V2` or `UI_UX_SPEC_V2` change means something is wrong: stop and
  report it (both specs are frozen).
- Redesign. No colour, spacing or layout value changes for aesthetic reasons.
- The two post-v2 hardening trackers (H0–H12, R1–R8). Unrelated; do not fold
  work from them in here.
