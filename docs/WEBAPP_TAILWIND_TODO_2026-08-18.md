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
- [ ] **T1-2** Commit baselines for the scenario set in `SPEC` §10.4, at the
      viewports in §10.3. Decide and document where baselines live (in-repo
      PNG + JSON, or JSON only with screenshots on demand) — in-repo binaries
      have a real cost in a firmware repo that also ships a flash image, so
      justify the choice in the evidence line rather than defaulting.
      *Evidence:*
- [ ] **T1-3** Wire it into `check-webapp.sh` after the browser workflows.
      Must be deterministic: fixed viewports, no animation timing races, no
      wall-clock in any fixture. If a scenario proves flaky, quarantine that
      scenario explicitly — do not add a retry that hides it.
      *Evidence:*
- [ ] **T1-4** Document baseline updates in `docs/DEVELOPMENT.md`: how to
      regenerate, and the rule that a baseline change must be reviewed as a
      deliberate visual change, never as a merge artefact.
      *Evidence:*
- [ ] **T1-5** Add the boundary-width scenarios: every threshold in `SPEC`
      §5.3 at exactly `X` and `X±1px`. This is the check that would have
      caught the `max-[X]:` defect on the day it was written.
      *Evidence:*

---

## 2. Phase 2 — Cover what nothing covers

- [ ] **T2-1** Unit tests for the six variant maps in `SPEC` §4.2. Assert that
      each variant resolves to a distinct, non-empty class string and that no
      variant sets the same property twice. These maps are branching logic
      with no test today; a wrong key is currently caught only by a visual
      diff nobody runs.
      *Evidence:*
- [ ] **T2-2** A test that fails if any element in a rendered page carries two
      utilities setting the same CSS property — the `SPEC` §3 invariant, as an
      executable check rather than a one-off scan. jsdom is enough: this is a
      `className` property, not a rendering one.
      *Evidence:*
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
