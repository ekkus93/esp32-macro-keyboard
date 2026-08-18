# Webapp — Tailwind Utility-First Migration TODO

**Document status:** Implementation sequence — LIVE (not retired, not superseded)
**Date:** 2026-08-17
**Scope:** `webapp/` only. No firmware, no contracts, no API surface.
**Baseline commit:** `7132c95f`
**Governing specs (unchanged by this work):** `docs/SPEC_V2.md`, `docs/UI_UX_SPEC_V2.md`

> **This document is not a specification and introduces no requirements.**
> It is a refactor plan. Every visual and accessibility requirement it
> references already exists in `SPEC_V2.md` / `UI_UX_SPEC_V2.md`; where this
> document appears to state a requirement, those documents are the authority
> and this one is wrong. Do not cite this file as "the spec", do not add
> normative language to it, and do not treat a task description here as
> permission to change behaviour that the frozen specs define. Per
> `CLAUDE.md`, both specs are frozen: this refactor **must not** require
> editing either one. If you find a genuine conflict, stop and report it.

---

## 0. How to use this document

### 0.1 Task discipline

Work one task at a time, in phase order. A task is not complete until all
four hold:

1. The change is made and matches the task's stated outcome exactly.
2. **The rendered result is unchanged.** This refactor is behaviour-preserving
   by definition — see §7 for the required before/after evidence. A visual
   difference is a defect, not an improvement, even if it looks better.
3. `./scripts/check-webapp.sh` passes clean (exit 0) on the resulting tree.
4. The checkbox is updated with a one-line evidence citation: commit SHA, the
   exact command run, and the result. Never check a box from "should work".

One phase per commit minimum; larger phases may be split per-file. Do not
batch phases together — the point of the ordering is that each phase is
independently revertible.

### 0.2 The rule that matters most

**If you cannot prove the rendering is identical, revert the change.**

This migration buys idiom and colocation. It buys *nothing* functional. There
is no user-visible payoff to trade against a regression, so the acceptable
regression count is zero. When a rule is awkward to express as utilities, the
correct answer is frequently "leave it in CSS" (§3), not "approximate it".

### 0.3 Working agreement

Per `CLAUDE.md`: work directly on `master`, forward commits only, never
force-push or rewrite history. `docs/SPEC.md`, `docs/SPEC_V2.md` and
`docs/UI_UX_SPEC_V2.md` are frozen — do not edit them.

---

## 1. Goal and honest rationale

`webapp/src/styles.css` currently defines ~54 semantic component classes
(`.card`, `.app-header`, `.status-badge`, …) as `@apply` compositions, and
every `className` in the app is a semantic name. Zero utility classes appear
in markup.

That is a legitimate, working setup. Tailwind's tokens genuinely drive it.
But it is **not utility-first**, which is the methodology Tailwind is built
around and its maintainers recommend; `@apply` is documented primarily for
styling markup you don't control. The costs of the current shape are real but
modest:

- Changing one component's layout means editing a file far from the component.
- A class's blast radius is invisible at the call site (`.card` is used 17
  times across 5 files; nothing in `MacroRow.tsx` says so).
- Dead rules accumulate silently — this has already happened three times
  (`management.css`'s 7 dead groups, `.conflict-message`, `.save-message`).

**Goal:** move genuinely component-scoped styling into markup as utilities, so
a component's appearance lives in the component. Reduce `styles.css` to the
element defaults, global rules, and genuinely-shared design-system pieces that
utilities cannot or should not express.

**Explicit anti-goal:** deleting `styles.css`, or "0 lines of CSS" as a score.
A correct end state still has a substantial base layer (§2).

---

## 2. Non-goals — what stays in CSS, permanently

Do not migrate any of the following. These are not leftovers; they are the
correct place for this styling.

### 2.1 The entire `@layer base` element-default block

`styles.css` styles **unclassed elements**. Inlining these means adding an
identical utility string to every instance, forever, and losing the property
that a plain `<button>` is correctly styled by default.

Measured in the current tree:

| Element | Instances in `src/**/*.tsx` |
| --- | --- |
| `<button>` | 36 |
| `<label>` | 33 |
| `<h1>` / `<h2>` / `<h3>` | 16 / 15 / 20 |
| `<dt>` / `<dd>` | 16 / 16 |
| `<code>` | 4 |

Inlining the keycap treatment onto 36 buttons is 36 copies of a 12-utility
string with no single source of truth. **Keep the base layer as it is.**

### 2.2 Specific rules that must not move

- `button:active:not(:disabled)` — its position in the cascade is load-bearing
  (it must beat `.primary`/`.danger` regardless of which a button carries).
  The current file documents this inline. Leave it.
- The `@media (prefers-reduced-motion: reduce)` block — applies to `*`,
  `*::before`, `*::after` with `!important`. Required by `UI_UX_SPEC_V2` §14
  / `TODO_V2` V2-133. Has no utility form. Leave it.
- `body`, `#main-content`, and the `a, button, input, select, textarea {
  font: inherit }` reset.
- The `@theme` block. This is Tailwind's configuration and the single source
  of truth for the palette. It is not "hand-written CSS to be eliminated".

### 2.3 `.primary` and `.danger`

23 and 19 usages respectively, applied to bare `<button>` elements as
variants of the base keycap treatment. These are correct as classes. Do not
inline them; do not convert them to a React component in this migration
(that is a larger API change, out of scope).

---

## 3. The decision rule

For each component class, pick exactly one disposition:

**A — Inline into markup.** Used 1–2 times, and the rule is self-contained
(no descendant selectors styling children you'd have to hunt down, no
pseudo-element, no dynamic construction). This is the default for one-off
layout classes.

**B — Extract a React component.** Used ≥3 times *and* represents one
coherent thing. Colocate the utilities inside the new component. This is the
Tailwind-recommended answer to repetition — not `@apply`. Prefer this over
leaving a widely-used class in CSS.

**C — Leave in CSS.** Any of:

- It styles unclassed elements (§2.1).
- It is a global/`*` rule, or its cascade position is load-bearing (§2.2).
- A test selects on it (§8.1) and the hook is worth keeping.
- Expressing it as utilities produces something materially harder to read
  than the CSS, with no colocation benefit.

**When A and C are close, choose C.** An unnecessary migration that renders
identically is still churn with regression risk and zero payoff.

---

## 4. Phase 0 — vocabulary prerequisites (do this first)

The current CSS uses layout thresholds that Tailwind's defaults cannot
express. **These must exist before any markup changes**, or Phase 1+ will
silently drop responsive behaviour.

### 4.1 Breakpoints

Thresholds in use, none of which match Tailwind's defaults (`sm` 40rem,
`md` 48rem, `lg` 64rem, `xl` 80rem):

| Threshold | Rule | Direction |
| --- | --- | --- |
| 26rem | `.storage-summary` 2-col → 4-col | both (`>=` and `<`) |
| 32rem | `.header-actions`, `.timing-grid`, `.reorder-actions`, `.management-actions`/`.card-actions` | max |
| 34rem | `.card` → two-column grid | min |
| 40rem | `.dialog-heading` stacks | max |
| 42rem | `.toolbar-columns` side-by-side | min |
| 60rem | `.standalone`/`.app-shell` width 48rem → 64rem | min |
| 38rem **height** | editor fixed/scroll split fallback | max, **height** |

**Decision: use arbitrary variants, not named breakpoints.** Write
`min-[34rem]:` and `max-[32rem]:` at the call site rather than adding six
`--breakpoint-*` names to `@theme`.

Rationale: these are one-off layout thresholds, not a shared scale. Each one
currently carries a comment explaining *why* that specific number (e.g. 34rem
is where `.card` has room for a second column). Naming them `xs`/`narrow`/…
hides the number and invites reuse of a threshold that was only ever right
for one component. Keeping the measurement at the call site preserves the
rationale.

**Verified working in this project (Tailwind 4.1.11):** `min-[34rem]:grid`
and `max-[32rem]:hidden` both compile from markup.

> **Correction (2026-08-18, `05528b8`) — "compiles" was the wrong test for
> the `max-` side, and this cost a real regression.** `min-[X]:` emits
> `@media(min-width:X)`, which is exactly `width >= X`, so every `min-`
> conversion is sound. But `max-[X]:` emits
> `@media not all and (min-width:X)`, which is `width < X` — **strictly**
> less than. Five of the six thresholds above whose direction is "max" came
> from source rules written `@media (width <= X)`, which includes the
> boundary. Converting those to `max-[X]:` silently drops the rule at
> exactly that width: 512px for the 32rem rules, 640px for the 40rem one.
> Measured against a pre-migration build at 511/512/513: **70 computed
> property values and 83 bounding boxes differed at 512px and nowhere
> else.**
>
> Use `[@media(width<=32rem)]:` — a bracketed at-rule variant — wherever the
> source said `<=`. It compiles to `@media(max-width:32rem)`, the original
> condition. `max-[26rem]:` is correct as written, because that one source
> rule really did say `width < 26rem`.
>
> The lesson generalises past this table: for a media-query conversion,
> "it compiles" and "it emits the same condition" are different claims, and
> only the second one is the thing being relied on.

### 4.2 The height query needs a custom variant

`@media (height <= 38rem)` has **no** built-in Tailwind form. Add exactly one
custom variant to `styles.css`:

```css
/* The macro editor's short-viewport fallback. Height-based media queries
   have no built-in Tailwind variant, and this one is load-bearing: below
   ~38rem of viewport height the fixed region can exceed the screen, which
   clamps .editor-scroll's flex height to zero -- and a zero-height
   overflow:auto container clips its content away with no scroll path back,
   making the directive toolbar and Save footer permanently unreachable. */
@custom-variant short (@media (height <= 38rem));
```

Then `short:h-auto`, `short:flex-none`, `short:overflow-y-visible`.

**Verified working:** compiles and emits the correct media block.

### 4.3 Tasks

- [x] **T0-1** Add the `@custom-variant short` declaration to `styles.css`
      with the comment above. No other change. Verify it compiles and that
      `dist` is byte-identical otherwise.
      *Evidence:* `2e315812`. Added the declaration; `dist/assets/*.css`
      diffed byte-identical against the `ee818060` baseline build (an unused
      `@custom-variant` emits nothing). Probe (`short:h-auto short:flex-none`
      on a throwaway class, reverted) compiled to
      `@media (max-height:38rem){...}`. `npm run test`: 544/544. `stylelint`:
      clean. `check-webapp.sh`: EXIT=0.
- [x] **T0-2** Confirm the arbitrary-variant vocabulary compiles in this
      tree by temporarily adding `min-[34rem]:grid max-[32rem]:hidden` to one
      `className`, building, grepping `dist/assets/*.css` for both rules, then
      reverting. Record the grep output.
      *Evidence:* No code change (probe reverted). Added
      `min-[34rem]:grid max-[32rem]:hidden` to `ExecutionRecoveryOverlay.tsx`'s
      `<aside>`, built, and confirmed both rules present in
      `dist/assets/*.css`:
      `@media not all and (min-width:32rem){.max-\[32rem\]\:hidden{display:none}}`
      and `@media(min-width:34rem){.min-\[34rem\]\:grid{display:grid}}`.
      Reverted via `git checkout`; rebuilt and confirmed 0 matches for the
      probe classes in the output.

> **Grep note:** Tailwind escapes brackets in emitted class names
> (`.min-\[34rem\]\:grid`). Grepping for the unescaped form returns nothing
> and looks like a failure. Search for the *declaration* (`grid-template`,
> `display:grid`) or the raw value instead. This exact mistake produced a
> false "content detection is broken" finding during planning.

---

## 5. Inventory and disposition

Usage counts are from the baseline commit. `(dyn)` = the class name is
constructed at runtime, never written literally in markup.

### 5.1 Disposition A — inline (one/two call sites, self-contained)

| Class | Uses | File(s) |
| --- | --- | --- |
| `app-shell`, `app-header`, `app-header-title`, `bottom-nav`, `header-button` | 1 each | `AppShellV2.tsx` |
| `editor-frame`, `editor-scroll`, `macro-source-pinned`, `editor-footer`, `directive-grid`, `timing-grid`, `toolbar-columns`, `field-label` | 1 each | `MacroEditorPage.tsx` |
| `validation-card`, `validation-good`, `validation-bad` | 1–2 | `MacroEditorPage.tsx` |
| `password-field`, `password-toggle` | 1 each | `SignInPage.tsx` |
| `empty-state` | 1 | `MacrosPage.tsx` |
| `overflow-menu`, `overflow-panel`, `confirmation-panel` | 1 each | `MacroOverflowMenu.tsx` |
| `recovery-overlay` | 1 | `ExecutionRecoveryOverlay.tsx` |
| `storage-summary` | 1 | `SnapshotsPage.tsx` — **but see §8.1** |
| `landscape-block` | 1 | `LandscapeBlockSurface.tsx` — **but see §8.1** |
| `metadata`, `management-list`, `management-card`, `reorder-actions`, `management-actions`, `card-actions`, `page-heading-title`, `directive-toolbar` | 1–2 | various |

### 5.2 Disposition B — extract a React component

| Class | Uses | Files | Proposed component |
| --- | --- | --- | --- |
| `standalone` | 19 | 5 files | `<StandaloneScreen>` |
| `card` | 17 | 5 files | `<Card>` |
| `form-stack` | 15 | 10 files | `<FormStack>` |
| `field-help` | 14 | 6 files | `<FieldHelp>` |
| `form-actions` | 11 | 9 files | `<FormActions>` |
| `danger-zone` | 6 | 5 files | `<DangerZone>` |
| `header-actions` | 6 | 5 files | `<HeaderActions>` |
| `page-heading` | 6 | 6 files | `<PageHeading>` |
| `checkbox-row` | 5 | 2 files | `<CheckboxRow>` |
| `send-status` | 5 | 3 files | `<SendStatus>` |
| `dialog-backdrop` + `dialog-panel` + `dialog-heading` | 3 each | 3 files | `<Dialog>` (one component, all three) |
| `eyebrow` (+ `.eyebrow.dark`) | 3 | 3 files | `<Eyebrow tone>` |
| `status-badge` + 4 state variants | 2 lit, 4 `(dyn)` | `StatusBadge.tsx` | already a component — see §8.2 |
| `error-message` | 1 | `ErrorBanner.tsx` | already a component |

`.card`, `.form-stack` and `.standalone` are the three highest-value
extractions; do them first within their phase.

### 5.3 Disposition C — leave in CSS

Entire `@layer base` (§2.1), `.primary`, `.danger`, the reduced-motion block,
`body`, `#main-content`, `@theme`, and `main` (bare element selector shared
with `.standalone`).

---

## 6. Phases

Ordered lowest-risk first. Each phase is one commit minimum and independently
revertible.

### Phase 1 — Leaf components, single call site

Lowest blast radius. Establishes the pattern before anything shared moves.

- [x] **T1-1** `ExecutionRecoveryOverlay.tsx` — inline `.recovery-overlay`.
      Note it carries `bottom: calc(1rem + env(safe-area-inset-bottom))`,
      which overrides the `bottom-4` utility; express as
      `bottom-[calc(1rem+env(safe-area-inset-bottom))]` and drop `bottom-4`.
      *Evidence:* `1b186e96`. Computed styles captured via the H4 recovery
      fixture before and after: identical (position, bottom/left/right,
      z-index, max-width, radius, border, background, padding, box-shadow).
      `npm run test`: 544/544. `check-webapp.sh`: EXIT=0, H4 workflow passed.
- [x] **T1-2** `SignInPage.tsx` — inline `.password-field`, `.password-toggle`.
      `.password-field input { @apply pr-12 }` becomes `[&_input]:pr-12` on
      the wrapper, or `pr-12` directly on the input (prefer the latter).
      *Evidence:* `abe43d1e`. Computed styles verified identical against a
      fresh baseline (position, padding-right, every toggle-button property
      including translate-based centering, and :hover color). `npm run
      test`: 544/544. `check-webapp.sh`: EXIT=0.
- [x] **T1-3** `MacroOverflowMenu.tsx` — inline `.overflow-menu`,
      `.overflow-panel`, `.confirmation-panel`.
      *Evidence:* `bc59bd73`. Confirmed no other `.tsx` referenced any of
      the three before deleting their CSS. Computed styles identical for
      all three (position, margin/display/min-width/gap/radius/border/
      background/padding/shadow). `npm run test`: 544/544. `check-webapp.sh`:
      EXIT=0.
- [x] **T1-4** `MacrosPage.tsx` — inline `.empty-state` (note the two
      descendant rules for `h3` and `p`: put those utilities on the children).
      *Evidence:* `43e69515`. This state had never been exercised by any
      existing test; built a one-off fixture (zero-macro package) to reach
      it and capture a real baseline. Computed styles identical. `npm run
      test`: 544/544. `check-webapp.sh`: EXIT=0. **Phase 1 complete.**

**Gap found during execution (after T2-1, before T2-2):** §5.1's disposition-A
inventory table lists `metadata`, `management-list`, `management-card`,
`reorder-actions`, `management-actions`, `card-actions`, `page-heading-title`,
and `storage-summary` but none was ever assigned to a phase task above. Adding
them here rather than skipping them, per §0.1's "do not skip a task or
subtask, even if it looks trivial."

- [x] **T1-5** `.page-heading-title` — `MacroEditorPage.tsx`, `MacrosPage.tsx`.
      Disposition A.
      *Evidence:* `2758cecf`. Computed style identical. `check-webapp.sh`:
      EXIT=0.
- [x] **T1-6** `.metadata` — `FirstRunSetupPage.tsx`, `SnapshotsPage.tsx`.
      Disposition A.
      *Evidence:* `3eaae2e5`. **Split during execution**:
      `FirstRunSetupPage.tsx`'s standalone usage inlined and verified
      identical (margin-top, font-size) against a fresh baseline via the
      real setup→review flow. `SnapshotsPage.tsx`'s usage is
      `className="metadata storage-summary"` — probed inlining `.metadata`'s
      `mt-3` there in isolation and it's a real regression: a literal markup
      utility (`@layer utilities`) always beats an `@apply`'d component class
      (`@layer components`) regardless of source order, flipping that dl's
      margin-top from 8px (`.storage-summary`'s `mt-2`, currently winning) to
      12px. Deferred to T1-7, which owns `.storage-summary` and migrates both
      classes on that element together. `check-webapp.sh`: EXIT=0.
- [ ] **T1-7** `.storage-summary` — `SnapshotsPage.tsx`, **now also carrying
      the `.metadata` half of that same `dl` deferred from T1-6** (see its
      evidence for why). **Keep the class name on the element** (§8.1 test
      hook, `webapp/tests/browser/workflows/snapshots.mjs:24`); only the CSS
      rule moves. Has two `nth-of-type` descendant rules and two width media
      queries (`>= 26rem`, `< 26rem`) — the most structurally complex
      "disposition A" class in the inventory. When inlining both classes
      together, verify the resulting margin-top matches the *current*
      combined behaviour (8px, `.storage-summary`'s `mt-2` winning), not
      `.metadata`'s `mt-3` — order the utilities in the className string (or
      use `!` if needed) so the intended one wins now that both are in the
      same layer.
      *Evidence:* `88eccbda`. Also removed `.metadata` (T1-6's deferred call
      site) -- migrated together onto the same `dl` so the margin-top
      cascade resolves correctly, not by layer-order luck. Verified the
      `min-[26rem]:`/`max-[26rem]:` boundary matches the source's exact
      `>=`/`<` semantics before using them. `storage-summary` kept as a
      class with no CSS rule, per §8.1. Computed styles byte-identical at
      390px and 1280px, all 4 dt/dd pairs. `npm run test`: 544/544.
      `check-webapp.sh`: EXIT=0.
- [x] **T1-8** The management cluster — `.management-list`,
      `.management-card`, `.management-actions`, `.card-actions`,
      `.reorder-actions` across `PackageManagementPage.tsx`, `SnapshotRow.tsx`,
      `MacroRow.tsx`, and the leftover `.card-actions` div in
      `ExecutionRecoveryOverlay.tsx` (T1-1 only touched the outer
      `.recovery-overlay`, not this inner class). `.management-actions,
      .card-actions` and `.reorder-actions` each have a `@media (width <=
      32rem)` override that currently shares a block with `.header-actions`
      (Phase 4, disposition B) and `.timing-grid` (Phase 2 T2-2, not yet
      done) — split the block, do not inline `.header-actions`'s share of
      it early.
      *Evidence:* `c057712c`. Found a second `.management-list` call site
      (`DiagnosticsPage.tsx`) not in the plan's inventory, caught by
      grepping for leftover references before finishing — migrated in the
      same commit. Caught and fixed a real selector-narrowing mistake
      before verifying: the original `.management-actions button,
      .card-actions button` is a descendant selector (any depth, including
      inside nested `.reorder-actions`/`.danger-zone`), and the first draft
      used `[&>button]` (direct child only), silently narrowing it — fixed
      to `[&_button]`. Computed styles byte-identical at 1280px and 390px
      across all five classes' properties, all four files, plus the
      recovery overlay via the H4 fixture. `npm run test`: 544/544.
      `check-webapp.sh`: EXIT=0. **Phase 1 (including the gap-fill) is now
      fully complete.**

### Phase 2 — The macro editor

Highest single-file concentration, and the one page with a documented
layout-collapse bug. Do it as its own commit.

- [x] **T2-1** `MacroEditorPage.tsx` — inline `.editor-frame`,
      `.editor-scroll`, `.macro-source-pinned`, `.editor-footer`, using the
      `short:` variant from T0-1 for the `height <= 38rem` fallback.
      **Read the existing comment on that media query before touching it.**
      *Evidence:* `dcf7e539`. Fresh baselines at 844px and 500px (well under
      the 608px threshold) via `startStartupFixtureServer` + Add-macro,
      before and after: byte-identical at both heights including the
      fallback state itself (`overflow-y: visible`, `flex: 0 0 auto`) —
      confirms `short:` correctly overrides the unprefixed utilities (§8.6),
      not merely that it compiles. Rationale comments moved to the TSX, not
      dropped. `npm run test`: 544/544. `check-webapp.sh`: EXIT=0.
- [x] **T2-2** `MacroEditorPage.tsx` — inline `.directive-grid`,
      `.directive-toolbar`, `.timing-grid`, `.toolbar-columns`,
      `.field-label`, `.validation-*`. `.directive-grid button` becomes
      utilities on the buttons themselves.
      *Evidence:* `db1e2162`. `.form-actions` split back out and left in
      CSS (Phase 4). `.directive-grid button` → `[&_button]:...` (descendant,
      not `[&>button]:`, per T1-8's fidelity fix). Comprehensive baseline at
      1280px/390px across all 7 classes, plus both validation states
      (typed a bad token, then a valid macro, into the real textarea) — all
      byte-identical. `npm run test`: 544/544. `check-webapp.sh`: EXIT=0.
- [x] **T2-3** `.chord-modifier` / `.chord-modifier.active` — currently built
      as `pressed ? "chord-modifier active" : "chord-modifier"`
      (`MacroEditorPage.tsx:389`). Replace with a full literal string per
      branch (§8.2), not interpolation.
      *Evidence:* `e3beb01c`. The ternary was already safe (two full literal
      strings, not `` `template-${x}` `` interpolation) — §8.2's actual
      failure mode doesn't apply here; this was inlining the CSS, not fixing
      a scanner miss. Both states (unpressed, and pressed via a real click)
      verified byte-identical against a fresh baseline. `npm run test`:
      544/544. `check-webapp.sh`: EXIT=0. **Phase 2 complete.**

### Phase 3 — Shell chrome

- [x] **T3-1** `AppShellV2.tsx` — inline `.app-shell`, `.app-header`,
      `.app-header-title`, `.header-button`, `.bottom-nav`. Note
      `.app-header h1` and `.bottom-nav button` are descendant rules; move
      those utilities onto the elements.
      *Evidence:* `32aabe50`. **Forced together with T3-2** — same coupling
      shape as T1-7: `.bottom-nav button.active` only worked via the
      ancestor's literal `bottom-nav` class, so inlining one without the
      other would have silently detached the active-tab styling. `.app-shell`
      kept as a bare class (no CSS rule) — a third test dependency
      (`tests/v2-app-v2-orientation.test.tsx:56`) that §8.1's audit missed,
      caught by the full `npm run test` run, not by re-reading the plan.
      `.app-shell`'s width formula extracted from its 3-way-combined
      selector with `.standalone` (Phase 4, untouched). Merging the two
      leftover `.standalone` rule blocks (now textually identical) was
      required — stylelint's `no-duplicate-selectors` caught it before it
      could hide as a false pass. Verified via structural locators at
      1280px/800px (the `>=60rem` boundary), the dirty-state Save-snapshot
      button, and the active nav button (clicked live) — all byte-identical.
      `npm run test`: 544/544. `check-webapp.sh`: EXIT=0.
- [x] **T3-2** `.bottom-nav button.active` — same literal-string treatment as
      T2-3.
      *Evidence:* Done together with T3-1 — see its evidence. Not a separate
      commit; the coupling made splitting them unsafe.
- [x] **T3-3** `LandscapeBlockSurface.tsx` — inline `.landscape-block` and its
      descendants, **keeping the `landscape-block` class as a test hook**
      (§8.1). The class stays on the element; its rule leaves CSS.
      *Evidence:* `ef9f7f6a`. No coupling this time — the split held.
      `.send-status`'s own rule stays in CSS (Phase 4); only this surface's
      two overrides of it moved, and both are additive (the base rule sets
      neither colour nor width), so no cascade race. Verified in real Chrome
      at 844×390 with `hasTouch` (`pointer: coarse`, one of the three
      conditions in `landscapePhoneMediaQuery`), in **both** the idle and the
      awaiting-confirmation states: **4736 computed properties across 9
      elements, zero value differences**, identical bounding boxes, identical
      overlay `innerText`, and **byte-identical PNG screenshots** of both
      states (`cmp` on the full-page captures). The four
      `env(safe-area-inset-*)` `calc()` paddings were checked separately in
      the compiled CSS — headless Chrome resolves every inset to `0px`, so
      computed style alone cannot prove they survived; `grep` confirmed all
      four emitted with correct `calc()` spacing. `landscape-block` now
      appears **0 times** in the compiled CSS, confirming it is a bare hook.
      `npm run test`: 544/544. `check-webapp.sh`: EXIT=0.

**Phase 3 complete.**

### Phase 4 — Shared component extraction (largest phase, split freely)

Order within the phase: `Card`, `FormStack`, `StandaloneScreen` first.

- [x] **T4-1** `<Card>` — replaces 17 `.card` usages. Must preserve the
      `min-[34rem]:` two-column behaviour and the `h2`/`h3`/`p` descendant
      rules (pass through `children`; put the descendant utilities on a
      wrapper via `[&_h2]:…` or restyle call sites — prefer the latter where
      the call sites are few).
      *Evidence:* `c4a9029`. `src/components/Card.tsx`, three variants as
      **complete literal class strings** (§8.2): `default` (`my-3`), `flush`
      (the two `card m-0` list rows, whose list owns the spacing), `danger`
      (the one `card danger-zone`). The variant form is what the fourth
      coupling instance forced — `SettingsPage.tsx:371` renders correctly
      today only because `.danger-zone` sits *later* in `styles.css` than
      `.card`; once `.card` became markup utilities they would have beaten
      the components layer (§8.6) and flipped that card back to
      `bg-panel`/`border-cap-edge`. Selecting a whole variant means no two
      conflicting utilities ever land on one element, so nothing races and
      **T4-5's `<DangerZone>` did not have to be pulled into this commit** —
      `.danger-zone` stays in CSS for its five other call sites. The
      descendant rules moved as `[&_h2]:`/`[&_h3]:`/`[&_p]:` variants, not to
      call sites: the cards hold far too many paragraphs (all of Diagnostics,
      every form's field help, every row's summary) for restyling each to be
      safe. They keep the same `(0,1,1)` specificity `.card p` had, and the
      only later same-specificity rules on those elements — `.send-status p`,
      `.dialog-heading p`, `.page-heading h2` — were checked and never appear
      inside a card. Verified in real Chrome against the fixture server on all
      five card-bearing pages (Macros, Packages, Snapshots, Settings,
      Diagnostics) at **390 / 544 / 1280 px** — 544 is the `min-[34rem]`
      boundary exactly — selecting on the class-independent `article` hook:
      **118 338 computed properties across 15 page/viewport captures, zero
      value differences**, zero bounding-box differences, zero `innerText`
      differences, and **all 15 full-page PNGs byte-identical** (`cmp`).
      Compiled CSS confirms `.card` now appears **0 times**, the four
      `[&_…]` descendant rules and `@media(min-width:34rem)` are emitted, and
      `danger-zone` still has its one rule. `npm run test`: 544/544.
      `check-webapp.sh`: EXIT=0 (its real-Chrome browser suites included).
      The stale `.card` mention in the `styles.css` header comment is left
      for T6-3, which owns that rewrite.
- [x] **T4-2** ~~`<FormStack>`~~ — inlined at all 15 usages instead.
      *Evidence:* `2551d9d`. **Deliberate deviation from this task's proposed
      disposition, recorded here rather than applied silently.** The 15 call
      sites are 10 `<form>`s (each with its own `onSubmit`, one also with an
      `id`), 3 `<div>`s and 2 `<ul>`s. A component covering those needs a
      three-branch discriminated union re-spreading heterogeneous props, to
      replace *two layout utilities* — which is §3's own C criterion
      ("materially harder to read than the CSS, with no colocation benefit")
      pointing away from the extraction. `<Card>` earned its component (three
      variants, a real cascade race to defuse, descendant rules to carry);
      this one earns nothing. Inlining still achieves §1's actual goal —
      the `@apply` rule is gone — with no indirection and no cascade risk:
      nothing else on any of those elements sets `display` or `gap`, and the
      one site that already mixed utilities (`MacroEditorPage.tsx:325`) sets
      only `flex`/overflow shorthands, not `display`. Verified in real Chrome
      across **12 captures** — Settings (4 forms), Packages, package rename,
      the macro editor, and four screens the signed-in fixture cannot reach
      (sign-in, first-run setup, no-stored-snapshots, and snapshot recovery,
      which is the one reachable `ul` site) — at 390 px and 1280 px, selecting
      on the class-independent `form` / `ul` / `form > *` / `ul > *` hooks:
      **64 663 computed properties, zero value differences**, zero
      bounding-box differences, zero `innerText` differences, and **all 12
      full-page PNGs byte-identical**. Compiled CSS: `form-stack` now appears
      **0 times**, `.gap-[0.85rem]{gap:.85rem}` is emitted. Three sites are
      not browser-reachable in any fixture (`AppV2`'s two error-state `div`s,
      `StationForm`'s connected-network `div`) and one `ul` needs a
      two-package repository — all four receive the identical textual
      substitution on the same element with no other class present.
      `npm run test`: 544/544. `check-webapp.sh`: EXIT=0. Also removed the
      `/* --- Keycap surfaces --- */` section header, left empty once `.card`
      (T4-1) and `.form-stack` were gone — stylelint's
      `comment-empty-line-before` caught the dangling header; the rebuild
      after that edit produced a **byte-identical** CSS bundle (same content
      hash `index-Cn3DbjP1.css`), so the visual proof above still stands.
- [x] **T4-3** `<StandaloneScreen>` — 19 usages. Carries the four-sided
      `env(safe-area-inset-*)` padding and the `*:` child rule
      (`.standalone > *`). Express as `*:mx-auto *:w-[min(100%,27rem)]`
      (verified working) or keep that one rule in CSS if it reads badly.
      *Evidence:* `1231e61`. `src/components/StandaloneScreen.tsx`, always a
      `<main>` so the bare `main` rule (which stays in CSS, §5.3) keeps
      supplying `px-4 py-5 [flex:1_1_auto]`; the component carries only
      `mx-auto flex min-h-dvh w-[min(100%,48rem)] flex-col bg-shell
      min-[60rem]:w-[min(100%,64rem)]` plus the child rule as
      `*:mx-auto *:w-[min(100%,27rem)]`, which compiles to
      `:is(.*\:mx-auto>*)` — the same `(0,1,0)` specificity `.standalone > *`
      had. Every direct child was checked first: they are `h1`/`p`/`section`/
      `button`/`form`/`ul`/`ErrorBanner`, and none carries a margin or width
      utility, so nothing races the `*:` rules inside the utilities layer.
      Verified in real Chrome across **18 captures** — sign-in, first-run
      setup, the first-run review step, no-stored-snapshots, snapshot
      recovery, and the ordinary shell (to prove narrowing `.standalone, main`
      to `main` left the shell's own `<main>` untouched) — at **390 / 800 /
      1280 px**, where 800 exercises the `48rem` cap and 1280 the `60rem`
      step (confirmed: `width: 1024px` there): **71 001 computed properties,
      zero value differences**, zero bounding-box differences, zero
      `innerText` differences, **all 18 full-page PNGs byte-identical**.
      `npm run test`: 544/544. `check-webapp.sh`: EXIT=0.
      **Finding — `.standalone`'s safe-area padding was dead code, and §7.5's
      "count 5 sites" check was measuring presence, not effect.** `.standalone`
      declared `padding: calc(max(2rem,7vh) + env(safe-area-inset-top)) …` for
      all four sides, but the `.standalone, main` rule *after* it set
      `padding-inline` / `padding-block` at equal specificity, overriding every
      one of them. Measured on the pre-change build at 390×844: computed
      padding `20px 16px 20px 16px`, i.e. `py-5 px-4` — not the `≈59px` top
      the declaration asks for. Re-expressing it as utilities would have
      **revived** it (utilities beat the components layer), a rendering change
      this refactor is not allowed to make, so it was dropped instead. Two
      consequences. (1) `env(safe-area-inset` now appears **7 times across 4
      elements** in the compiled CSS (`.app-header` 1, `.bottom-nav` 1,
      `landscape-block` 4, `.recovery-overlay` 1), not 5 elements — §7.5's row
      should read 4, and the check should verify *effect*, not presence; this
      task is the proof that a present `env()` declaration can be entirely
      overridden. (2) **`UI_UX_SPEC_V2` §13 "Safe-area insets are respected on
      devices with display cutouts or gesture navigation" is not actually
      satisfied on the single-task screens** (Sign In, First-Run Setup, the
      reconnect screens, every repository-startup screen). This is
      **pre-existing**, not introduced here. Per §7.5 it is reported rather
      than fixed inside a refactor commit: the fix is a one-line change to
      `StandaloneScreen.tsx`'s class string
      (`pt-[calc(max(2rem,7vh)+env(safe-area-inset-top))]` and its three
      siblings, which now *would* take effect), but it changes rendering on
      notched hardware, cannot be validated on this bench, and so needs its
      own commit and its own decision.
- [x] **T4-4** `<FieldHelp>` (14), `<FormActions>` (11).
      *Evidence:* `6cb10e7`. `<FormActions>` covers all 11 `.form-actions`
      divs — every one was bare, so there is no variant and nothing to
      override. `<FieldHelp>` covers **17** `.field-help` sites, not 14:
      §5.1's inventory undercounts again, the same defect class it hit before
      (MacroEditor 2, PackageManagement 1, Station 2, Password 2, AccessPoint
      2, Identity 2, FirstRunSetup 5, RepositoryStartup 1). It also absorbs
      **`.limit-exceeded`**, which turned out to have no call site outside
      `field-help limit-exceeded`, so that rule left CSS too. Its two states
      disagree on `font-weight` *and* `color`, so `exceeded` selects a
      complete literal class string (§8.2): the class pair worked only
      because `.limit-exceeded` sat later in `styles.css`, and as two
      utilities on one element the winner would have been Tailwind's
      stylesheet sort order rather than `className` order. `as` is kept
      explicit because the tag is load-bearing twice over — `<p>` is invalid
      inside a `<label>` (six sites), and `<Card>`'s `[&_p]:` rules from T4-1
      apply to the paragraph form and not the span form. Verified with a
      **full-document walk** this time rather than selector guesses: every
      element in the page, 68 computed properties each, over **16 captures**
      at 390 px and 1280 px — Settings, the Restart `alertdialog`, Packages,
      the macro editor, first-run setup, and the no-blobs startup screen,
      with the byte counters driven **both under and over their limits**
      (65/64 bytes and 4097/4096 bytes) so the `exceeded` branch is proven in
      its own right. **1112 elements, 75 616 computed properties, zero value
      differences**, zero bounding-box differences, zero structural
      differences, and **all 16 full-page PNGs byte-identical**.
      `npm run test`: 544/544. `check-webapp.sh`: EXIT=0.
- [x] **T4-5** `<PageHeading>` (6), `<HeaderActions>` (6), ~~`<DangerZone>`~~
      (moved to T4-7).
      *Evidence:* `1b6e1fd`. `<PageHeading>` covers all 6 `.page-heading`
      divs and carries `.page-heading h2` as `[&_h2]:` variants rather than
      pushing it to call sites: that rule is what makes an arbitrary-length
      title (a package name, up to 64 UTF-8 bytes) truncate instead of
      wrapping to a second row, so it belongs to the component. All six
      `<h2>`s were checked — none carries a utility of its own, so nothing
      races the `(0,1,1)` `[&_h2]:` rules inside the utilities layer.
      `<HeaderActions>` covers all 6 `.header-actions` divs and folds in the
      `@media (width <= 32rem)` rule. **This originally shipped as
      `max-[32rem]:justify-start`, and the evidence line here originally
      claimed that compiles to `@media(max-width:32rem)`. That claim was
      wrong** — the `@media(max-width:32rem)` seen in the build came from the
      source `@media (width <= 32rem)` blocks still present at the time, not
      from the variant, which emits `@media not all and (min-width:32rem)`
      (`width < 32rem`). The rule therefore stopped applying at exactly
      512 px. Fixed in `05528b8` along with the seven Phase-1/Phase-2
      occurrences of the same mistake; see §4.1's correction note. The
      390 px captures below exercise the variant but not its boundary, which
      is precisely why the error survived this task's verification. Verified with the
      full-document walk over **14 captures** at 390 px and 1280 px — Macros,
      the macro preview, the macro editor, Packages, Snapshots, Settings and
      Diagnostics, which between them render every `page-heading` and
      `header-actions` site including the shell header's: **1052 elements,
      71 536 computed properties, zero value differences**, zero
      bounding-box differences, zero structural differences, **all 14
      full-page PNGs byte-identical**. `npm run test`: 544/544.
      `check-webapp.sh`: EXIT=0.
      **`<DangerZone>` is deliberately deferred to T4-7.** Its last call site
      is `ConfirmPhraseDialog.tsx:35`'s `dialog-panel danger-zone` — the same
      cascade coupling `<Card>` hit at `SettingsPage.tsx:371`, where the pair
      renders correctly only because `.danger-zone` sits later in
      `styles.css` than `.dialog-panel`. Splitting it would either leave that
      coupling half-migrated or force a `<Dialog>` variant to be designed
      here, so per the handoff's rule for coupled tasks it moves whole into
      the commit that owns `.dialog-panel`. `.danger-zone` stays in CSS until
      then, unchanged and serving its five remaining call sites.
- [x] **T4-6** `<CheckboxRow>` (5), `<SendStatus>` (5, note the
      `[role="alert"]` variant — verified as `[&[role=alert]]:` or a prop).
      *Evidence:* `a2faefb`. Resolved as a **prop**, not `[&[role=alert]]:`,
      and it had to be: `DismissibleBanner` passes `role` through as a runtime
      value, so an interpolated class name would compile to nothing (§8.2),
      and the two branches disagree on both border colour and background.
      `role` therefore selects one of two complete literal strings.
      `overlay` — the landscape surface's `w-[min(100%,24rem)] text-legend`,
      inlined there by T3-3 — *appends* instead, which is safe here for the
      reason T3-3 recorded: both are properties the base string never sets,
      so there is no property for them to race over. `<CheckboxRow>` covers
      all 5 `.checkbox-row` labels; its `min-h-[44px]` is `UI_UX_SPEC_V2`
      §13's touch target measured on the label rather than the input, so it
      now lives in the component instead of in a class a call site can
      forget. Verified with the full-document walk over **12 captures**:
      390 px and 1280 px for the in-flight `role="status"` banner, the
      completed banner, the `role="alert"` failure banner (driven for real
      via "Send Trigger failure"), Settings (four checkbox rows) and
      first-run setup (the fifth); plus the landscape overlay at 844×390
      with `hasTouch`, in **both** the idle and the awaiting-confirmation
      states, which is the only place the `overlay` variant renders.
      **1178 elements, 80 104 computed properties, zero value differences**,
      zero bounding-box differences, zero structural differences, **all 12
      full-page PNGs byte-identical**. `npm run test`: 544/544.
      `check-webapp.sh`: EXIT=0.
- [x] **T4-7** `<Dialog>` — **also carries `<DangerZone>` (6 → 5 sites),
      moved here from T4-5 because `dialog-panel danger-zone` couples them.**
      *Evidence:* `8a8b56a`. One `<Dialog>` covers `.dialog-backdrop`,
      `.dialog-panel`, `.dialog-heading` and its `h2`/`p` descendant rule
      across all three modal surfaces; `<DangerZone>` covers `.danger-zone`
      at its four standalone sites, and the fifth —
      `ConfirmPhraseDialog.tsx`'s `dialog-panel danger-zone` — becomes
      `<Dialog tone="danger">`, a complete literal string holding the merged
      result. That pair rendered correctly only because `.danger-zone` sat
      later in `styles.css` than `.dialog-panel`; they disagree on border
      colour, left border width and background, so as markup utilities the
      winner would have been Tailwind's sort order instead. `<DangerZone>`
      couples `role="alertdialog"` with `tabIndex={-1}` because
      `useFocusTrap` moves focus to the container, which a `<div>` can only
      accept when it is programmatically focusable — all three call sites
      that need one need the other. Verified with the full-document walk
      over **21 captures** at 390 / 700 / 1280 px, driving every dialog and
      danger-zone state for real: the restart dialog, the reset-settings
      `alertdialog` (the coupled tone), the package-delete and
      snapshot-delete confirmations, the snapshot Advanced panel, the
      import-ready panel (through a genuine file selection), and the
      unsaved-changes prompt. **1551 elements, 105 468 computed properties,
      zero value differences**, zero bounding-box differences, zero
      structural differences, **all 21 full-page PNGs byte-identical**.
      The heading's narrow-screen stack is written
      `[@media(width<=40rem)]:`, **not** `max-[40rem]:` — see §4.1's
      correction note; verified separately against a clean worktree of the
      previous commit at **511 / 512 / 513 / 639 / 640 / 641 px**
      (3234 elements, 219 912 properties, zero differences, 36/36
      screenshots byte-identical), which is the boundary this task's other
      viewports cannot see. `npm run test`: 544/544. `check-webapp.sh`:
      EXIT=0. — one component covering `.dialog-backdrop`,
      `.dialog-panel`, `.dialog-heading` across 3 call sites. Preserve
      `max-h-[calc(100dvh-2rem)]` exactly; **`100vh` here is a known bug**
      (fixed in `70aa7b65`) — do not "simplify" it back.
      *Evidence:*
- [x] **T4-8** `<Eyebrow>` (3, with the `.dark` tone variant).
      *Evidence:* `4fa01b2`. All three `.eyebrow` sites, with
      `.eyebrow.dark` as a `tone` prop. The two tones are complete literal
      strings (§8.2): they differ only in `color`, and as a class pair that
      resolved by specificity — `.eyebrow.dark` is `(0,2,0)` against
      `.eyebrow`'s `(0,1,0)` — whereas two colour utilities on one element
      have no such tiebreak and would fall to Tailwind's emission order.
      Verified with the full-document walk over **6 captures** at 390 px and
      1280 px (Macros and the macro editor, which carry the two `dark`
      eyebrows, plus Settings; the shell header's plain eyebrow is on all
      three): **606 elements, 41 208 computed properties, zero value
      differences**, zero bounding-box differences, zero structural
      differences, **all 6 full-page PNGs byte-identical**. `npm run test`:
      544/544. `check-webapp.sh`: EXIT=0.

**Phase 4 complete.**

### Phase 5 — Status badges

Isolated because it is the only pseudo-element cluster and the only
fully-dynamic class construction.

- [x] **T5-1** `StatusBadge.tsx` — `.status-badge` + `::before` + four state
      variants, each with its own `::before` override. Use a **literal lookup
      map** (§8.2). Verified: `before:content-['']` and friends compile.
      The four states differ by shape as well as colour — this is a
      `UI_UX_SPEC_V2` §14 requirement (never colour alone); preserve all four
      distinct `::before` treatments exactly.
      *Evidence:* `c7e7d1f`. Four complete literal strings. This was the one
      site where §8.2's failure mode was not hypothetical: the component built
      `` `status-${state}` ``, so once those rules left `styles.css` the four
      state classes would never have been emitted at all, and the badge would
      have rendered unstyled **with no build error**. Each entry also spells
      out only the *winning* `::before` declarations, because the per-state
      treatments override the base rather than adding to it — `warning`
      replaces `bg-current` with `bg-transparent`, `bad` replaces
      `rounded-full` with `rounded-[1px]`, `neutral` replaces both the size
      and the fill. Verified by capturing **`::before` computed style for
      every element in the page**, which an ordinary element walk cannot see
      and which is exactly where the §14 requirement lives, in all four states
      driven for real through the fixture's `usbState`
      (`ready`→good, `suspended`→warning, `error`→bad,
      `disconnected`→neutral), on both surfaces that render the badge (the
      shell header and the macro preview page): **12 160 pseudo-element
      properties, zero differences**, alongside **640 elements / 43 520
      element properties, zero differences**, zero bounding-box differences,
      and **all 8 full-page PNGs byte-identical**. The four shapes measured
      distinct in the baseline and unchanged after: 9.59px filled disc plus
      halo, 9.59px hollow 2px ring, 9.59px square (1px radius), 7.19px hollow
      dot. Compiled CSS: `status-badge`/`status-good`/`status-warning`/
      `status-bad`/`status-neutral` all appear **0 times**, and
      `before:content`, `before:shadow`, `before:rounded-[1px]` and
      `before:border-[1.5px]` are all emitted. Also converted the shell's
      unsaved-changes indicator, which had been using the raw classes rather
      than the component. `npm run test`: 544/544. `check-webapp.sh`: EXIT=0.

**Phase 5 complete.**

### Phase 6 — Sweep and close

- [ ] **T6-1** Delete every now-unused rule from `styles.css`. Re-run the
      defined-vs-used audit (§7.4) and confirm zero unused component rules
      and zero used-but-undefined classes.
      *Evidence:*
- [ ] **T6-2** Re-run the unused-`@theme`-token audit. Any token orphaned by
      the migration is either a bug (a style got dropped) or genuinely dead —
      determine which before deleting.
      *Evidence:*
- [ ] **T6-3** Update the `styles.css` header comment. It currently describes
      the all-`@apply` architecture and will be wrong. State the new split:
      base layer + globals in CSS, component styling in markup.
      *Evidence:*
- [ ] **T6-4** Final full-gate run and a complete visual diff across every
      page at both viewports (§7.2).
      *Evidence:*

---

## 7. Verification protocol

### 7.1 Per task

```bash
export NVM_DIR="$HOME/.nvm" && . "$NVM_DIR/nvm.sh" && nvm use 24.18.0
npm --prefix webapp run test        # fast inner loop
```

### 7.2 Per phase — the visual diff is mandatory

`check-webapp.sh` passing does **not** prove this refactor correct; almost
every possible regression here is visual and the test suite asserts on text
and roles, not appearance.

Before each phase, capture baseline screenshots; after, capture again and
compare. Drive the app with the existing fixture server — it needs no
hardware and no device:

```js
// webapp/tests/browser/<name>.tmp.mjs   (temporary; delete before committing)
import { chromium } from "playwright";
import { startApplicationServer } from "./fixtures/applicationServer.mjs";
const app = await startApplicationServer();
// viewports: 1280x900 (desktop) and 390x844 (phone)
// pages: Sign In, first-repo onboarding, Macros, Macro Editor (dirty),
//        Packages, Snapshots, Settings, Diagnostics, a dialog, a danger-zone
```

Pages that need a specific state:

- **Macro Editor dirty** — reorder a macro, then check "Unsaved changes".
- **A dialog** — Settings → Restart device.
- **A `.danger-zone`** — Snapshots → Delete.
- **The recovery overlay** — copy `tests/browser/run-h4-recovery-tests.mjs`
  and inject a screenshot after the "Execution state unavailable" wait.
- **Short-viewport editor fallback** — set viewport height ≤ 608px and
  confirm the directive toolbar and Save footer are reachable.

Also assert computed styles on at least one element per migrated class rather
than eyeballing alone — `getComputedStyle` values must match the baseline
exactly.

**Delete every `*.tmp.mjs` before committing.** Stray temp scripts have
broken `format:check` in this repo before.

### 7.3 Per phase — full gate

```bash
./scripts/check-webapp.sh > /tmp/gate.log 2>&1; echo "EXIT=$?"
```

Only `EXIT=0` is a pass. This includes axe-core; contrast must not shift.

### 7.4 The audit scripts

Re-run these at T6-1/T6-2. Both were used to find real defects during
planning (`.conflict-message`/`.save-message` dead; `.card-actions`
used-but-undefined):

- Defined-vs-used: parse `.class` names out of `@layer components`, compare
  against every literal `className` token in `src/**/*.tsx`. Remember that
  dynamically-built names (§8.2) will show as false "unused" — check each by
  hand before deleting.
- Unused tokens: for each `--color-*` in `@theme`, search the stylesheet body
  and all markup for `bg-*`/`text-*`/`border-*`/`var(--color-*)`.

### 7.5 Spec clauses this refactor must preserve

**No spec change is required for this work, and needing one is a bug signal.**
Both specs are outcome-based: they constrain the rendered result, never the CSS
architecture. `SPEC_V2.md` mentions Tailwind exactly once (§5.2, in the
technology-stack list) and says nothing about how it is used. `UI_UX_SPEC_V2.md`
contains no reference to CSS, stylesheets, class names, or `@apply` at all.

So if a task here appears to require editing either spec, the refactor has
changed user-visible behaviour and the change is wrong. Stop and report it
rather than reaching for a spec edit.

These are the clauses a CSS refactor can plausibly break. Verify each survives:

| Clause | Mechanism today | How it is checked |
| --- | --- | --- |
| UI_UX §13 — touch targets ≥ 44×44 | `min-h-[44px]` / `min-h-[48px]` on buttons, `select`, `.password-toggle` (`h-11 w-11`), checkbox/radio min sizes | **Automated:** `assertTouchTargets()`, `tests/browser/workflows/browser.mjs:22` |
| UI_UX §13 — single column, fluid to 320px | `min-w-[320px]` on `body`; fluid grids | **Automated:** `assertResponsiveLayout()`, same file line 23 |
| UI_UX §13 — safe-area insets respected | `env(safe-area-inset-*)` in `.standalone`, `.app-header`, `.bottom-nav`, `.landscape-block`, `.recovery-overlay` | Manual — grep the built CSS for `env(safe-area-inset` and count 5 sites |
| UI_UX §14 — colour never the only indicator | Four distinct `::before` **shapes** on the status badges; inset top rule on the active nav tab and pressed chord modifier | Manual — §5 T5-1; all four shapes must differ after migration |
| UI_UX §14 — reduced motion disables animation | The `@media (prefers-reduced-motion: reduce)` block on `*` | Stays in CSS (§2.2); confirm it still exists |
| UI_UX §14 — focus always visible | `:focus-visible` outline rules on `a`/`button`/`input`/`select`/`textarea`/`[tabindex="-1"]` | **Automated:** axe-core, in `check-webapp.sh` |
| UI_UX §14 — dialogs trap focus | `useFocusTrap` (JavaScript, untouched by this work) | **Automated:** browser workflows |
| SPEC_V2 §5.2 — no remote assets | System font stack in `@theme`; no `@import` of a remote sheet | **Automated:** `verify-no-remote-assets.sh` |
| SPEC_V2 §5.2 — Tailwind is the CSS tool | Unchanged — this refactor uses *more* of Tailwind, not less | n/a |

Contrast is not in the table because no colour value may change (§8.5); axe-core
gates it regardless.

---

## 8. Risk register

### 8.1 Tests that select on class names — do not remove these classes

| Class | Selector site |
| --- | --- |
| `storage-summary` | `webapp/tests/browser/workflows/snapshots.mjs:24` — `document.querySelector(".storage-summary")` |
| `landscape-block` | `webapp/tests/v2-app-v2-orientation.test.tsx:128` — `requiredElement(".landscape-block", HTMLElement)` |
| `app-shell` | `webapp/tests/v2-app-v2-orientation.test.tsx:56` — `document.querySelector(".app-shell")`, proving the shell stays mounted (not reloaded) behind the landscape-block overlay |

Keep these class names on their elements as test hooks even after their CSS
rules move to utilities. A class with no rule is free. Changing the tests
instead is allowed but must be deliberate and called out in the commit — do
not silently weaken an assertion to make a refactor pass.

**This list was wrong once already** — `app-shell` was found only by running
the full `npm run test` suite after T3-1's edit, not by re-reading this
section. Treat this table as a starting point, not a guarantee: run the full
suite after every phase (§7.1/§7.3) rather than trusting this list is
exhaustive. If you find another one, add it here.

### 8.2 Dynamically-constructed class names will silently break

Tailwind scans source files for **literal** class strings. It cannot see
`` `status-${state}` ``. Three sites build names at runtime:

| Site | Current |
| --- | --- |
| `src/components/StatusBadge.tsx:10` | `` className={`status-badge status-${state}`} `` |
| `MacroEditorPage.tsx:389` | `pressed ? "chord-modifier active" : "chord-modifier"` |
| bottom-nav active tab | `.bottom-nav button.active` |

Correct pattern — a literal map, never interpolation:

```tsx
const TONE = {
  good:    "bg-good-tint text-good before:shadow-[0_0_0_3px_rgb(11_92_51_/_22%)]",
  warning: "bg-warning-tint text-warning-ink before:border-2 before:border-current before:bg-transparent",
  bad:     "bg-bad-tint text-alert before:rounded-[1px]",
  neutral: "bg-neutral-tint text-legend-soft before:h-[0.45rem] before:w-[0.45rem] before:border-[1.5px] before:border-current before:bg-transparent",
} as const;
```

Every value must be a complete literal string. `` `bg-${x}-tint` `` produces
nothing and fails **silently at build time** — the page renders unstyled with
no error. Grep the built CSS for one class from each branch to prove they
were emitted.

### 8.3 Load-bearing rules that look like they could be simplified

- **`@media (height <= 38rem)` on the editor.** Below this height the fixed
  region can exceed the viewport, clamping `.editor-scroll`'s flex height to
  zero; a zero-height `overflow: auto` container clips content away with *no
  scroll path back*, making the directive toolbar and Save footer permanently
  unreachable. The fallback abandons the fixed/scroll split entirely. Preserve
  it exactly.
- **`100dvh`, never `100vh`** — in `.standalone` (`min-h-dvh`) and
  `.dialog-panel` (`max-h-[calc(100dvh-2rem)]`). Both were real, separately
  diagnosed mobile bugs (`ae7f5eea`, `75f454d8`, `70aa7b65`). `vh` measures
  the largest-possible viewport, leaving content behind the mobile address bar.
- **`minmax(0, 1fr)` in grids** (`grid-cols-4` on `.bottom-nav`,
  `grid-cols-2` on `.reorder-actions`). A bare `1fr`'s implied minimum is its
  content's min-content width, which pushes the row wider than the viewport.
  Tailwind's `grid-cols-N` already emits `minmax(0, 1fr)` — verify it stayed.
- **`overscroll-y-contain`** on `#main-content` and `.editor-scroll` — stops
  inner scroll from chaining into page rubber-banding.
- **`.status-badge` is deliberately not uppercased.** `text-transform`
  rewrites `innerText` and previously broke a real-browser assertion matching
  "Unsaved changes".

### 8.4 Readability regression

Some rules become long utility strings (`.app-header` is ~12 utilities plus a
`calc()+env()` padding). If a `className` becomes unreadable, that is a
signal for Disposition B (extract a component) or C (leave it), not a reason
to push through. Do not introduce `clsx`/`cn` helpers as part of this
migration — that is a separate decision.

### 8.5 Contrast

Do not change any colour value while moving it. Every pairing in the palette
was contrast-checked (worst case 8.14:1 against a 4.5:1 floor) and axe-core
gates it. A "tidy-up" of a hex value during a mechanical move is out of scope.

### 8.6 The base layer will start losing to utilities

Once utilities appear in markup, specificity interactions change: a base-layer
`button { … }` rule and a markup `px-2` are both single-class-equivalent, and
`@layer` ordering decides. Tailwind puts `utilities` after `components` and
`base`, so markup utilities win — which is what you want. But verify it on the
first button you touch rather than assuming, and record the result in T1-1.

---

## 9. Rollback

Each phase is one commit. To abandon mid-migration, revert the phase commits
in reverse order; there is no cross-phase state. Do not force-push. If a
phase is partially done and wrong, `git revert` it and re-plan rather than
patching forward.

---

## 10. Definition of done

- [ ] Every task above checked with commit SHA and command evidence.
- [ ] `styles.css` contains: `@theme`, the full `@layer base` block, the
      reduced-motion block, `body`/`#main-content`/`main`, `.primary`,
      `.danger`, the `@custom-variant short` declaration, and any rule
      deliberately retained under Disposition C — and nothing else.
- [ ] Zero unused component rules; zero used-but-undefined classes.
- [ ] Every `@theme` token still referenced, or deliberately deleted with a
      note saying why.
- [ ] `./scripts/check-webapp.sh` exits 0.
- [ ] Visual diff across all pages at 1280×900 and 390×844 shows no
      difference from the `7132c95f` baseline.
- [ ] No `*.tmp.mjs` left in `webapp/tests/browser/`.
- [ ] `docs/SPEC_V2.md` and `docs/UI_UX_SPEC_V2.md` unmodified
      (`git diff 7132c95f -- docs/SPEC_V2.md docs/UI_UX_SPEC_V2.md` empty).
