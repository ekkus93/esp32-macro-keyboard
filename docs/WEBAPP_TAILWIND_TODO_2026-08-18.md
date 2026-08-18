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
- [x] **T2-3** A test that fails on a class token that generates no CSS,
      allowlisting exactly the three hooks in `SPEC` §7. This is the direct
      guard against §4.1's silent-failure mode. Needs the built stylesheet, so
      it belongs with the browser tests or as a build-time script.
      *Evidence:* `3689c69`, prepared by `706fe18` (see below).
      `checkNoOrphanClasses.mjs`, wired into `check-webapp.sh` as
      `npm run check:no-orphan-classes` right after `test:visual`. Reuses
      `visual/scenarios.mjs` (T1-1's own scenario set) to collect the real
      class vocabulary through a real browser, one viewport per scenario —
      **deliberately not** a source-text regex over `className="..."` JSX
      attributes: a first draft used exactly that and missed all six SPEC
      §4.2 variant-map components entirely, since none of them assigns a
      literal string straight to a JSX `className` — they all reference a
      computed `CARD_CLASS[variant]`-style expression a regex can't see.
      Checks both directions: a zero-CSS token absent from the three-entry
      allowlist fails, and an allowlisted hook that unexpectedly *does* have
      CSS also fails (a hook is supposed to be bare; if it isn't, the
      allowlist itself is stale). **Verified both directions with real
      injected defects**: added a runtime-interpolated class to `Eyebrow`
      (the exact §8.2 failure mode) and confirmed it was named and failed;
      reverted; added a real CSS rule for the `landscape-block` hook in
      `styles.css` and confirmed that failed too, by name, in the other
      direction; reverted both.
      **Found and fixed two real bugs in T1's own harness while building
      this** (`706fe18`, its own commit): `capture.mjs` used
      `String(element.className)`, which on an SVG element (`className` is
      an `SVGAnimatedString` there, not a string) produced the literal text
      `"[object SVGAnimatedString]"` — this script's token collector choked
      on exactly that as an unrecognized class, which is how it was found.
      Fixed with `element.getAttribute("class") ?? ""`; diffed old vs new
      captures directly and confirmed the *only* differences anywhere are
      three SVG/path/circle `cls` fields, garbage → empty, zero change to
      any computed-style value — T1's actual regression-detection power was
      never affected, only a diagnostic label was wrong. Separately, five
      Snapshots-page scenarios raced the page's async list load (observed:
      `snapshots-storage-summary-boundary` capturing 37 elements one run, 55
      the next, both deterministic on repeat — a real race, not flakiness in
      the traditional sense); fixed by waiting for the storage-summary's own
      loaded-state label instead, case-insensitively (`dt`'s `uppercase`
      base-layer styling means `innerText` renders `"STORED"`, not the
      source text `"Stored"` — caught immediately by rerunning after the
      first fix). Regenerated all 96 T1-2 baselines with both fixes and
      re-ran them clean **5 times in a row**, plus 5 more isolated reruns of
      the previously-flaky Snapshots scenarios.
      Also required correct Tailwind selector escaping, verified against
      real compiled CSS rather than assumed: hyphen and underscore are
      **not** escaped in Tailwind's own selectors (an early guess escaped
      both and matched nothing).
      Getting escaping right also needed abandoning a first
      regex-lookahead boundary check that double-escaped an already-escaped
      selector string into a broken pattern — replaced with plain substring
      search plus a manual next-character check, which sidesteps regex
      entirely. `npm run test`: 569/569. Full `check-webapp.sh`: EXIT=0,
      ~2m17s end to end with this step included.
- [x] **T2-4** Make `DeviceReconnectScreen` reachable: add `POST
      /api/v1/restart` to `startupFixtureServer.mjs` so the reconnect screen
      can be rendered and diffed. It is the one `StandaloneScreen` call site
      that has never been visually verified, on the component whose padding
      regression went unnoticed for nine tasks.
      *Evidence:* `84dca26`. The real route is `POST
      /api/v1/device/restart` (this task's own text abbreviated it).
      Implemented in `startStartupFixtureServer.mjs` — not
      `applicationServer.mjs` — because it already supports the fully
      authenticated app state and shares its repository fixture data with
      `applicationServer.mjs` (both reach "Lab bench workflow"), so no
      second fixture module was needed. Two simulated-down 503 polls (the
      closest an HTTP fixture gets to a dropped connection —
      `useDeviceReconnect.ts` treats `5xx` the same as a network failure),
      then the response drops `state.authenticated`, letting the *existing*
      auth gate answer the next poll with 401 — no special-cased 401
      response needed.
      New `reconnect-waiting` scenario: Settings → Restart → confirm →
      capture. **Verified this is real coverage, not a stub**:
      `StandaloneScreen`'s `padding-top` measured `59.08px` with the correct
      `env(safe-area-inset-top)` `calc()` intact — exactly the property this
      call site could never previously prove.
      **Did not add a `reconnect-needs-reauth` scenario, and said why rather
      than forcing one.** `AppV2.tsx`'s own comment explains: a 401 on the
      reconnect poll *also* fires the shared `subscribeUnauthorized`
      mechanism, which drops the whole app to Sign In directly ("this shell
      unmounts on its own once that happens") — confirmed empirically, the
      fixture reaches Sign In every time, never
      `DeviceReconnectScreen`'s own needs-reauth copy. Whether that render
      branch is reachable by *any* real sequence is a genuine open question
      about the app's own control flow, not something to paper over by
      racing fixture timings until one scenario happens to pass.
      Regenerated all 98 baselines (was 96) and ran clean 5 times in a row.
      `npm run test`: 569/569. `check-webapp.sh`: EXIT=0.
- [x] **T2-5** Assert the four `StatusBadge` `::before` shapes are mutually
      distinct in geometry, not only in colour — `UI_UX_SPEC_V2` §14. Compare
      width/height/border-radius/border-width/background across the four
      states and fail if any two are identical.
      *Evidence:* `134a6ff`. **Deliberately excludes `background` from the
      comparison, unlike this task's own wording** — the whole point of "not
      only in colour" is that the four states must be told apart by shape
      *alone*; including `background-color` in the signature would let two
      states with identical geometry but different fill pass, which is
      exactly the failure `UI_UX_SPEC_V2` §14 forbids. Compares width,
      height, all four `border-radius` corners, all four `border-width`
      sides, and `box-shadow` (the "good" state's halo is a shadow, not a
      border, so its presence is part of the shape). Needs real Chrome —
      `::before` computed style is what `SPEC` §10.1 says jsdom cannot
      produce — so it reuses the visual harness's own `usb-badge-*`
      scenarios rather than a separate fixture drive. The shell header
      renders two `StatusBadge`s (USB, then Saved/Unsaved, itself always
      `neutral`); DOM order reliably puts the USB one first, asserted
      explicitly (`>= 2` found) rather than silently indexed.
      **Verified it has teeth**: collapsed the `bad`/error state's shape
      into `warning`'s (square → hollow ring, colour left alone), confirmed
      the check failed naming exactly that pair, reverted, confirmed clean.
      Printed signatures confirm all four documented shapes are real: ready
      = filled disc with a halo (`box-shadow` present), suspended = hollow
      ring (2px border), error = square (1px radius, filled), disconnected
      = smaller hollow dot (7.19px vs 9.59px, 1px border). `npm run test`:
      569/569. `check-webapp.sh`: EXIT=0.

**Phase 2 complete.** Every scenario `check-webapp.sh` now runs — the six
`SPEC` §4.2 variant maps (T2-1), every element on four real pages (T2-2), the
SPEC §7 bare-hook allowlist (T2-3), `DeviceReconnectScreen`'s previously
unreachable `StandaloneScreen` call site (T2-4), and `StatusBadge`'s
accessibility-load-bearing shape distinctness (T2-5) — is now something CI
checks on every run, not something that depends on a human remembering to run
a hand-built diff.

---

## 3. Phase 3 — Close the latent cascade traps

None of these is a live defect. Each is a loaded gun that fires the first time
someone nests two components.

- [x] **T3-1** Resolve the `[&_h2]` precedence inversion (`SPEC` §6.1):
      `PageHeading` inside `Card` would now resolve backwards relative to the
      pre-migration stylesheet. Either move the heading utilities onto the
      `<h2>` elements at the six `PageHeading` call sites, where specificity
      is unambiguous, or add a guard that fails if the nesting appears.
      Prefer the former; record which and why.
      *Evidence:* `a25fbc2`, prepared by `646ca7c` (see below). Took the
      preferred path: moved the six `[&_h2]:` utilities off `PageHeading`
      onto the `<h2>` at each of its six call sites (exported
      `PAGE_HEADING_TITLE_CLASS` so they share one definition rather than
      repeating the literal string — confirmed this doesn't trip
      `react-refresh/only-export-components`; a plain string constant is
      covered by `allowConstantExport`, unlike the object export `CARD_CLASS`
      attempt earlier in the migration, which did trip it).
      **Found a second, higher-impact defect while verifying this one**
      (`646ca7c`, its own commit, done first): after the move, the compiled
      CSS kept emitting `PageHeading`'s old `[&_h2]:` rules — not stale
      caching (confirmed with `rm -rf dist node_modules/.vite` and a fully
      clean rebuild, twice) but Tailwind's default content scan picking up
      `tests/browser/visual/baselines/*.json`, which are git-tracked and
      store, as literal text, every className the visual harness has ever
      captured — including from commits before this one. Any class that
      ever appeared in any baseline was permanently keeping its CSS rule
      alive, growing the bundle with dead utilities forever and, as found
      here, silently defeating exactly the kind of "did this rule actually
      disappear" verification this task needed. Fixed with Tailwind v4's
      `@source not` directive, excluding the baseline directory — the path
      is relative to `styles.css` itself, not the project root, and a first
      attempt using `./tests/…` resolved to nonexistent `src/tests/` and had
      no effect; only checking the compiled output (not trusting the
      directive syntax) caught that. Compiled CSS shrank 29.41kB → 29.02kB
      from the exclusion alone, before any source change. Verified
      render-neutral: all 98 visual-harness captures matched their existing
      baselines unchanged (confirming every removed rule was genuinely
      unused by any live markup), and zero baseline files themselves
      changed. `SPEC` §2.1/§6.1 updated in the same session (`b38a9a9`).
      **After both fixes, re-verified the actual T3-1 goal directly against
      the compiled CSS**: only `Card`'s and `Dialog`'s `[&_h2]:` rules
      remain; `PageHeading`'s six are gone. `npm run test`: 569/569.
      `check-webapp.sh`: EXIT=0.
- [x] **T3-2** Same treatment for `[&_p]` across `Card`, `SendStatus` and
      `Dialog`. Today's order happens to match the old stylesheet, by luck
      rather than design — an added or reordered utility anywhere could flip
      it with no visible cause.
      *Evidence:* `664557d`. **Not "the same treatment" as T3-1** — moving
      `Card`'s `[&_p]:` out to its call sites is infeasible (it scopes every
      paragraph across all of Diagnostics, every form's field help, every
      row's summary line), so this took T3-1's own named alternative: a
      guard, not a move. `descendantVariantNesting.ts` is deliberately
      general rather than `[&_p]:`-specific — for every element it walks
      ancestors counting how many own a descendant-variant scope matching
      that element's own tag, and fails on two or more. Covers `Card`,
      `Dialog` and `SendStatus`'s `[&_p]:` in one pass, plus `[&_h2]:`/
      `[&_h3]:`/`[&_button]:` for defence in depth against a *future*
      instance of this same trap, not just the one T3-1 already fixed.
      Extracted `representativePages.tsx` from T2-2's
      `no-rendered-property-collisions.test.tsx` so this reuses the same
      four real page renders (behavior-preserving: that file's own 4 tests
      still pass unchanged) rather than re-deriving fixture setup.
      **Verified the detector has teeth**, not merely that it runs clean on
      pages with no nesting today: a deliberately constructed `Card` nested
      inside a `SendStatus` (no real call site does this) is caught —
      exactly one violation, the doubly-scoped `<p>`, both ancestors named —
      and a sibling `Card`/`SendStatus` pair (not nested) correctly reports
      zero violations, proving the detector does not over-fire on ordinary
      composition. `SPEC` §6.1 updated (`8859608`) — the `[&_p]` case is
      recorded as guarded, not resolved, since the underlying hazard (two
      components able to nest and collide) still exists; only its
      consequence is now caught automatically. `npm run test`: 575/575.
      `check-webapp.sh`: EXIT=0.
- [x] **T3-3** Guard `StandaloneScreen`'s `*:` child rule (`SPEC` §6.2)
      against a `fixed`-positioned direct child, which would be given a 27rem
      width and stop covering the viewport. A comment on the component plus a
      test asserting no direct child is `position: fixed` is probably enough;
      do not restructure the rule without a diff.
      *Evidence:* `c0c0f2e`. Did exactly the scope this task named — a
      comment on `StandaloneScreen.tsx` pointing at the guard and the
      workaround (wrap a genuinely fixed child in one more `<div>`, since
      `*:` cannot exempt one child from a rule it applies to all of them),
      plus `no-fixed-standalone-child.test.tsx`. Did **not** restructure the
      rule. Checks the `fixed` class *token*, not the computed `position`
      property — as this task's own wording assumed it might be asserted —
      because jsdom applies no stylesheet at all (`SPEC` §10.1), so
      `getComputedStyle` would report every element's `position` as the
      browser default `static` regardless of which Tailwind classes it
      carries; the token is the only signal jsdom can see, and also the
      only thing a real call site could actually write. **Verified the
      detector has teeth**: a deliberately constructed `fixed` direct child
      is caught, and — the false-positive check this task didn't ask for
      but the detector needed — a `fixed` element nested one level *deeper*
      (a grandchild) is correctly **not** flagged, since `*:` is Tailwind's
      child combinator (`> *`), not a descendant selector. `SPEC` §6.2
      updated (`f2faf5c`). `npm run test`: 578/578. `check-webapp.sh`:
      EXIT=0.

**Phase 3 complete.** Every latent cascade trap `SPEC` §6 named now has
either no live instance (`[&_h2]`, T3-1) or an automated guard against one
ever forming (`[&_p]` across `Card`/`Dialog`/`SendStatus`, T3-2;
`StandaloneScreen`'s `*:` child rule, T3-3) — none of these was a live
defect, and none is any longer a silent one either.

---

## 4. Phase 4 — Consistency and rot

- [x] **T4-1** Sweep the 32 comment references to deleted classes. Most are
      legitimate history ("Replaces the `.card` rule") and should stay. The
      ones to fix are those written in the present tense as if the class still
      governs behaviour — notably `Card.tsx:14`, whose cascade argument is
      stated about two classes that no longer exist, and `Card.tsx:25-26`,
      which grounds its safety claim in three stylesheet rules that are now
      utilities on sibling components. Also `ExecutionRecoveryOverlay.tsx:61`
      and `tests/browser/workflows/settings.mjs:20`.
      *Evidence:* Commit `aaa39a0`. Fixed the 4 present-tense/dangling
      references: `Card.tsx`'s header comment rewritten to past-tense framing
      (`.card`/`.card danger-zone`/`.card p`/`.dialog-heading p`/
      `.page-heading h2` are no longer live stylesheet rules — the `[&_p]:`
      scope is now owned by `Card`, `Dialog`, and `SendStatus` independently,
      guarded by `tests/descendantVariantNesting.ts` per T3-2, not by this
      comment); `ExecutionRecoveryOverlay.tsx:61` (`.bottom-nav` →
      "AppShellV2's bottom nav"); `tests/browser/workflows/settings.mjs:20`
      (`.eyebrow` → `Eyebrow.tsx`). Re-swept all comment references to
      pre-migration selector syntax (`grep -rnE '\.[a-z][a-z-]*\b' --
      '*.tsx' '*.ts' '*.mjs'` restricted to comment lines, by hand) after the
      fix: 29 remain, all reviewed individually and confirmed legitimate
      past-tense history ("Replaces the `.card` rule", "used to be
      `.danger-zone`", etc.) that the task's own text says should stay,
      including `Eyebrow.tsx:9`'s specificity-comparison sentence (states a
      timeless structural fact bounded by surrounding past-tense context, not
      a claim that a deleted class still governs behaviour). Full gate:
      `./scripts/check-webapp.sh` — EXIT=0, 578/578 vitest tests passed,
      Playwright visual suite green.
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
