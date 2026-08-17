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
      *Evidence:* `0947dfd1`. Added the declaration; `dist/assets/*.css`
      diffed byte-identical against the `ee818060` baseline build (an unused
      `@custom-variant` emits nothing). Probe (`short:h-auto short:flex-none`
      on a throwaway class, reverted) compiled to
      `@media (max-height:38rem){...}`. `npm run test`: 544/544. `stylelint`:
      clean. `check-webapp.sh`: EXIT=0.
- [ ] **T0-2** Confirm the arbitrary-variant vocabulary compiles in this
      tree by temporarily adding `min-[34rem]:grid max-[32rem]:hidden` to one
      `className`, building, grepping `dist/assets/*.css` for both rules, then
      reverting. Record the grep output.
      *Evidence:*

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

- [ ] **T1-1** `ExecutionRecoveryOverlay.tsx` — inline `.recovery-overlay`.
      Note it carries `bottom: calc(1rem + env(safe-area-inset-bottom))`,
      which overrides the `bottom-4` utility; express as
      `bottom-[calc(1rem+env(safe-area-inset-bottom))]` and drop `bottom-4`.
      *Evidence:*
- [ ] **T1-2** `SignInPage.tsx` — inline `.password-field`, `.password-toggle`.
      `.password-field input { @apply pr-12 }` becomes `[&_input]:pr-12` on
      the wrapper, or `pr-12` directly on the input (prefer the latter).
      *Evidence:*
- [ ] **T1-3** `MacroOverflowMenu.tsx` — inline `.overflow-menu`,
      `.overflow-panel`, `.confirmation-panel`.
      *Evidence:*
- [ ] **T1-4** `MacrosPage.tsx` — inline `.empty-state` (note the two
      descendant rules for `h3` and `p`: put those utilities on the children).
      *Evidence:*

### Phase 2 — The macro editor

Highest single-file concentration, and the one page with a documented
layout-collapse bug. Do it as its own commit.

- [ ] **T2-1** `MacroEditorPage.tsx` — inline `.editor-frame`,
      `.editor-scroll`, `.macro-source-pinned`, `.editor-footer`, using the
      `short:` variant from T0-1 for the `height <= 38rem` fallback.
      **Read the existing comment on that media query before touching it.**
      *Evidence:*
- [ ] **T2-2** `MacroEditorPage.tsx` — inline `.directive-grid`,
      `.directive-toolbar`, `.timing-grid`, `.toolbar-columns`,
      `.field-label`, `.validation-*`. `.directive-grid button` becomes
      utilities on the buttons themselves.
      *Evidence:*
- [ ] **T2-3** `.chord-modifier` / `.chord-modifier.active` — currently built
      as `pressed ? "chord-modifier active" : "chord-modifier"`
      (`MacroEditorPage.tsx:389`). Replace with a full literal string per
      branch (§8.2), not interpolation.
      *Evidence:*

### Phase 3 — Shell chrome

- [ ] **T3-1** `AppShellV2.tsx` — inline `.app-shell`, `.app-header`,
      `.app-header-title`, `.header-button`, `.bottom-nav`. Note
      `.app-header h1` and `.bottom-nav button` are descendant rules; move
      those utilities onto the elements.
      *Evidence:*
- [ ] **T3-2** `.bottom-nav button.active` — same literal-string treatment as
      T2-3.
      *Evidence:*
- [ ] **T3-3** `LandscapeBlockSurface.tsx` — inline `.landscape-block` and its
      descendants, **keeping the `landscape-block` class as a test hook**
      (§8.1). The class stays on the element; its rule leaves CSS.
      *Evidence:*

### Phase 4 — Shared component extraction (largest phase, split freely)

Order within the phase: `Card`, `FormStack`, `StandaloneScreen` first.

- [ ] **T4-1** `<Card>` — replaces 17 `.card` usages. Must preserve the
      `min-[34rem]:` two-column behaviour and the `h2`/`h3`/`p` descendant
      rules (pass through `children`; put the descendant utilities on a
      wrapper via `[&_h2]:…` or restyle call sites — prefer the latter where
      the call sites are few).
      *Evidence:*
- [ ] **T4-2** `<FormStack>` — 15 usages.
      *Evidence:*
- [ ] **T4-3** `<StandaloneScreen>` — 19 usages. Carries the four-sided
      `env(safe-area-inset-*)` padding and the `*:` child rule
      (`.standalone > *`). Express as `*:mx-auto *:w-[min(100%,27rem)]`
      (verified working) or keep that one rule in CSS if it reads badly.
      *Evidence:*
- [ ] **T4-4** `<FieldHelp>` (14), `<FormActions>` (11).
      *Evidence:*
- [ ] **T4-5** `<PageHeading>` (6), `<HeaderActions>` (6), `<DangerZone>` (6).
      *Evidence:*
- [ ] **T4-6** `<CheckboxRow>` (5), `<SendStatus>` (5, note the
      `[role="alert"]` variant — verified as `[&[role=alert]]:` or a prop).
      *Evidence:*
- [ ] **T4-7** `<Dialog>` — one component covering `.dialog-backdrop`,
      `.dialog-panel`, `.dialog-heading` across 3 call sites. Preserve
      `max-h-[calc(100dvh-2rem)]` exactly; **`100vh` here is a known bug**
      (fixed in `70aa7b65`) — do not "simplify" it back.
      *Evidence:*
- [ ] **T4-8** `<Eyebrow>` (3, with the `.dark` tone variant).
      *Evidence:*

### Phase 5 — Status badges

Isolated because it is the only pseudo-element cluster and the only
fully-dynamic class construction.

- [ ] **T5-1** `StatusBadge.tsx` — `.status-badge` + `::before` + four state
      variants, each with its own `::before` override. Use a **literal lookup
      map** (§8.2). Verified: `before:content-['']` and friends compile.
      The four states differ by shape as well as colour — this is a
      `UI_UX_SPEC_V2` §14 requirement (never colour alone); preserve all four
      distinct `::before` treatments exactly.
      *Evidence:*

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

### 8.1 Two tests select on class names — do not remove these classes

| Class | Selector site |
| --- | --- |
| `storage-summary` | `webapp/tests/browser/workflows/snapshots.mjs:24` — `document.querySelector(".storage-summary")` |
| `landscape-block` | `webapp/tests/v2-app-v2-orientation.test.tsx:128` — `requiredElement(".landscape-block", HTMLElement)` |

Keep both class names on their elements as test hooks even after their CSS
rules move to utilities. A class with no rule is free. Changing the tests
instead is allowed but must be deliberate and called out in the commit — do
not silently weaken an assertion to make a refactor pass.

This is the complete list; no other test in `tests/` depends on a class name.

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
