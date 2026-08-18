# Webapp Tailwind Styling Specification

**Status: live.** Written 2026-08-18, immediately after the utility-first
migration closed (`docs/WEBAPP_TAILWIND_UTILITY_MIGRATION_TODO_2026-08-17.md`,
all 29 tasks complete). Supersedes nothing; it is the first document to
describe the styling architecture that migration produced.

**This file is not a product specification and must never be cited as one.**
It constrains *code architecture* — where styling lives, how it is written, how
a change to it is proven safe. It states no user-visible requirement of its
own. `docs/SPEC_V2.md` and `docs/UI_UX_SPEC_V2.md` remain the only authority
for what the interface must do, and they are frozen. Where this file mentions
a requirement (safe-area insets, touch targets, colour-never-alone) it is
quoting one of those two and says so; if this file and a frozen spec ever
disagree, the frozen spec wins and this file is the bug.

Every measurement below was taken on `3380776` and is reproducible by the
commands in §10. Nothing here is estimated.

---

## 1. Scope

Applies to `webapp/src/**`: the single stylesheet, the shared components in
`webapp/src/components/`, and the `className` strings in every feature
component. Does not apply to firmware, to the host tests, or to any tooling
outside `webapp/`.

---

## 2. Where styling lives

### 2.1 `webapp/src/styles.css` holds exactly four things

333 lines, 26 `@apply` compositions, and nothing else:

1. **`@theme`** — 25 tokens; the single source of truth for palette, radius,
   tracking and type stack. It generates the utility classes the components
   use (`bg-shell`, `text-legend`, `rounded-keycap`, `tracking-legend`, …) as
   well as the custom properties. There are no hardcoded colour values
   anywhere else in the webapp.
2. **`@layer base`** — element defaults for unclassed HTML: the type scale,
   the keycap treatment every bare `<button>` gets, form controls, focus
   rings, `dl`/`dt`/`dd`, `details`/`summary`. `.primary` and `.danger` live
   here as variants of the base keycap treatment applied to bare `<button>`s,
   and `button:active:not(:disabled)` must keep beating both regardless of
   which one a button carries. The reduced-motion block applies to `*`,
   `*::before` and `*::after` with `!important` (`UI_UX_SPEC_V2` §14,
   "Reduced-motion preferences disable nonessential animation") and has no
   utility form.
3. **Two globals and one variant** — `body`, the `main` / `#main-content`
   pair whose selectors are shared between the shell and the standalone
   screens, and the `@custom-variant short` declaration.
4. **One `@source not` exclusion** — `webapp/tests/browser/visual/baselines/`
   (T1-2's checked-in visual-regression baselines) is excluded from
   Tailwind's content scan. Those JSON files store, as literal text, every
   className the visual harness has ever captured, including from past
   commits; without the exclusion a class that stops being used anywhere in
   real markup keeps generating a CSS rule forever, because the scanner
   cannot tell historical test data from live source (found verifying T3-1,
   fixed in `646ca7c` — CSS shrank 29.41kB → 29.02kB from this exclusion
   alone, no source changes). The path is relative to `styles.css` itself,
   not the project root — get this wrong (as the first attempt did, writing
   `./tests/…` instead of `../tests/…`) and Tailwind silently accepts the
   directive and excludes nothing; verify against the compiled output, not
   the directive syntax.

`@layer components` contains exactly two selectors: `main` and
`#main-content`. **Zero class rules.**

### 2.2 Component styling lives in markup

Thirteen shared components in `webapp/src/components/`:

`Card`, `CheckboxRow`, `DangerZone`, `Dialog`, `ErrorBanner`, `Eyebrow`,
`FieldHelp`, `FormActions`, `HeaderActions`, `PageHeading`, `SendStatus`,
`StandaloneScreen`, `StatusBadge`.

Everything else is inlined at its call site. A reader looking for how a card,
a dialog or a status badge is styled reads that component; there is no
indirection through a name defined in the stylesheet.

### 2.3 Rules for adding styling

- **Used once or twice, self-contained** → inline the utilities at the call
  site.
- **Used three or more times and it is one coherent thing** → extract a
  component under `webapp/src/components/` and colocate the utilities inside
  it. Not `@apply`.
- **Styles unclassed elements, or its cascade position is load-bearing** →
  it belongs in `@layer base` or the globals, not in a component.
- A component is not automatically the right answer at three call sites. If
  the sites differ in element type and carry heterogeneous props, a wrapper
  that re-spreads them can be materially harder to read than the utilities it
  replaces — `.form-stack` was inlined at 15 sites for exactly this reason.
  Prefer the option a reader can follow.

---

## 3. Cascade: the four rules that decide everything

Tailwind's layer order is **theme → base → components → utilities**.

1. **A utility in markup beats anything in `styles.css`**, regardless of
   specificity. This is what makes §2.2 work.
2. **Between two utilities, CSS specificity decides first.** A descendant
   variant such as `[&_p]:my-[0.2rem]` compiles to `.…\:my-\[0\.2rem\] p`,
   specificity `(0,1,1)`, and therefore beats a plain `(0,1,0)` utility on
   that same `<p>`.
3. **At equal specificity, Tailwind's emission order in the compiled
   stylesheet decides — NOT the order tokens appear in `className`.** This is
   the single most dangerous property of the system. Reordering a class string
   changes nothing; the winner is whatever Tailwind happened to emit later.
4. **Therefore: never put two utilities that set the same property on one
   element.** See §4.

Invariant, verified: **no element in the webapp carries two utilities that set
the same CSS property.** 156 distinct class tokens scanned.

---

## 4. The literal-variant rule

### 4.1 Class names must be literal

Tailwind scans source for **literal** class strings. `` `status-${state}` ``
produces no CSS and fails **silently at build time** — the element renders
unstyled with no error. This is not hypothetical: `StatusBadge.tsx` shipped
that construction until T5-1.

Interpolated class names are prohibited. A ternary of two complete literals is
correct; a template literal that assembles a name is not.

### 4.2 Conflicting variants select a whole string

When variants of a component disagree on any property, each variant is one
**complete literal class string** — never a base string with an override
appended. Rule 3 of §3 is why: appending does not win, it races.

Current variant maps:

| Component | Prop | Variants | They disagree on |
| --- | --- | --- | --- |
| `Card` | `variant` | `default` / `flush` / `danger` | margins, border colour, left border width, background |
| `Dialog` | `tone` | `panel` / `danger` | border colour, left border width, background, margin-top |
| `StatusBadge` | `state` | `good` / `warning` / `bad` / `neutral` | background, text colour, and four distinct `::before` treatments |
| `FieldHelp` | `exceeded` | `within` / `exceeded` | font-weight, colour |
| `SendStatus` | `role` | `status` / `alert` | border colour, background |
| `Eyebrow` | `tone` | `default` / `dark` | colour |

### 4.3 Appending is allowed only for disjoint properties

Concatenating a shared shape constant with a variant string is fine **when the
two sets touch no common property**. Two current cases, both deliberate:

- `Card`'s `CARD_SHAPE` + variant: the shape sets no margin, border colour or
  background.
- `SendStatus`'s `overlay`: adds a width and a text colour; the base sets
  neither.

Anything else appends at its peril and must be justified in a comment.

---

## 5. Breakpoints and media queries

### 5.1 Arbitrary variants, not named breakpoints

Thresholds are written at the call site so the number and its rationale stay
together. No `--breakpoint-*` names are added to `@theme`.

### 5.2 The exact-condition rule

**`min-[X]:` is safe. `max-[X]:` is not.**

| Written | Tailwind emits | Means |
| --- | --- | --- |
| `min-[34rem]:` | `@media(min-width:34rem)` | `width >= 34rem` |
| `max-[26rem]:` | `@media not all and (min-width:26rem)` | `width < 26rem` |
| `[@media(width<=32rem)]:` | `@media(max-width:32rem)` | `width <= 32rem` |
| `short:` | `@media(max-height:38rem)` | `height <= 38rem` |

`max-[X]:` is **exclusive** and does not match a viewport exactly `X` wide.
Use it only where the intended condition is genuinely `<`. Where the condition
is `<=`, write the bracketed at-rule form.

This cost a real regression. Eight utilities converted with `max-[32rem]:`
silently stopped applying at exactly 512px; measured against a pre-migration
build, 70 computed property values and 83 bounding boxes differed at 512px and
nowhere else. Fixed in `05528b8`.

**Verification requirement: "it compiles" is not the test.** A media-query
conversion is proven by grepping the built CSS for the emitted *condition* and
by diffing at the boundary width itself. Phase 0 of the migration recorded
`@media not all and (min-width:32rem)` in its own evidence and read it as a
pass, because the task only asked whether the class compiled.

### 5.3 Thresholds currently in use

| Threshold | Where | Direction | Written as |
| --- | --- | --- | --- |
| 26rem | storage summary grid rows | `>=` and `<` | `min-[26rem]:` / `max-[26rem]:` |
| 32rem | header actions, reorder/card actions, timing grid | `<=` | `[@media(width<=32rem)]:` |
| 34rem | card two-column grid | `>=` | `min-[34rem]:` |
| 40rem | dialog heading stack | `<=` | `[@media(width<=40rem)]:` |
| 42rem | editor toolbar columns | `>=` | `min-[42rem]:` |
| 60rem | shell / standalone column width | `>=` | `min-[60rem]:` |
| 38rem **height** | editor fixed/scroll fallback | `<=` | `short:` |

`short` is declared once in `styles.css` because height-based media queries
have no built-in Tailwind variant. It is load-bearing: below that height the
editor's fixed region can exceed the viewport, which clamps the scrolling
form's flex height to zero, and a zero-height `overflow:auto` container clips
its content away with no scroll path back.

---

## 6. Descendant and child variants

Some components style their children rather than pushing utilities to every
call site. This is correct where the children are numerous and generated, and
it is how `Card` handles the paragraphs across all of Diagnostics, every
form's field help and every row's summary line.

Current owners:

| Component | Variant | Sets |
| --- | --- | --- |
| `Card` | `[&_h2]` `[&_h3]` | `mb-[0.3rem] text-[1.05rem]` |
| `Card` | `[&_p]` | `my-[0.2rem] text-legend-soft` |
| `Dialog` (heading) | `[&_h2]` `[&_p]` | `mt-0` |
| `SendStatus` | `[&_p]` | `mb-2 font-bold` |
| `StandaloneScreen` | `*:` | `mx-auto w-[min(100%,27rem)]` |
| four action rows | `[&_button]` | `flex-initial` |
| `MacroEditorPage` toolbar | `[&_button]` | sizing and type |

### 6.1 The nesting hazard

Two components that own the same descendant variant produce two rules at
identical `(0,1,1)` specificity. If one is ever rendered inside the other,
§3 rule 3 applies and **emission order decides** — which no source file
states and no author controls.

**`[&_h2]` was one such case and is now resolved** (T3-1,
`WEBAPP_TAILWIND_TODO_2026-08-18.md`, `a25fbc2`): `PageHeading` and `Card`
both owned it at `(0,1,1)`, and the order that happened to result — `Card`
beating `PageHeading` — was backwards relative to the pre-migration
stylesheet (`.page-heading h2` came after `.card h2` there and won). Fixed
by moving the utilities off `PageHeading` onto the `<h2>` at each of its six
call sites, where specificity is unambiguous regardless of nesting. Verified
against the compiled CSS with a fully clean rebuild (`rm -rf dist
node_modules/.vite`; a stale build or a stale cache both read as "still
present" — see §2.1's `@source` entry for why the first verification attempt
was misleading) that only `Card`'s and `Dialog`'s `[&_h2]` rules remain.

**`[&_p]` is the same hazard, still open** — `Card`, `Dialog` and
`SendStatus` all own it at `(0,1,1)`. Measured order on `646ca7c` (byte
offset in the compiled stylesheet):

```text
Card         [&_p]:my-[0.2rem]
Dialog       [&_p]:mt-0
SendStatus   [&_p]:mb-2
SendStatus   [&_p]:font-bold
Card         [&_p]:text-legend-soft
```

`SendStatus` inside `Card` would resolve **correctly** today (`SendStatus`
later, matching the pre-migration source order) — but that is luck, not
design, exactly as the `[&_h2]` case was before it was fixed. No such
nesting occurs today. **Rule: do not nest two components that own the same
descendant variant.** If a change would create such a nesting, put the
utilities on the child element instead, where specificity is unambiguous.

### 6.2 `StandaloneScreen`'s child rule

`*:mx-auto *:w-[min(100%,27rem)]` sizes **every** direct child. A
`fixed inset-0` child — a dialog backdrop — would be given a 27rem width and
stop covering the viewport. No such child exists today. This is inherited
behaviour from the `.standalone > *` rule it replaced, not new.

---

## 7. Test hooks

Three class names carry **no CSS rule** and exist only so tests can select an
element structurally. They are the only class tokens in the webapp that
generate no CSS; every other one of the 156 does.

| Hook | Element | Used by |
| --- | --- | --- |
| `app-shell` | the shell wrapper | `tests/v2-app-v2-orientation.test.tsx:56` |
| `landscape-block` | the orientation overlay | `tests/v2-app-v2-orientation.test.tsx:128` |
| `storage-summary` | the snapshots `dl` | `tests/v2-snapshots-page-management.test.tsx:38,145`; `tests/browser/workflows/snapshots.mjs:24` |

Rules:

- A hook must carry a comment on its element saying it has no rule and naming
  the test that selects it.
- Do not delete a hook without deleting or rewriting that test.
- Do not add a hook where a role, label or `id` would serve. `landscape-block`
  exists because the overlay's "Cancel and release all keys" button is
  identical in name to one on the page behind it.

---

## 8. Safe-area insets

`UI_UX_SPEC_V2` §13: "Safe-area insets are respected on devices with display
cutouts or gesture navigation."

Eleven `env(safe-area-inset-*)` terms across **five elements**:

| Element | Sides |
| --- | --- |
| `AppShellV2` header | top |
| `AppShellV2` bottom nav | bottom |
| `ExecutionRecoveryOverlay` | bottom |
| `LandscapeBlockSurface` | all four |
| `StandaloneScreen` | all four |

The landscape surface takes all four because it fills the viewport in either
rotation, so the cutout can land on either side.

**These must be utilities on the element itself, never a class rule.** A
utility outranks the components layer by construction; a class rule wins only
by source order, and source order is exactly what broke. In the original
stylesheet `.standalone`'s padding was declared *after* `.standalone, main`
and won. T3-1 merged two `.standalone` rules into one placed *before* it,
which flipped the cascade and dropped the top padding from ~63px to 20px on
every single-task screen, taking all three insets with it. It survived nine
consecutive "byte-identical" evidence lines and was found only by the
migration's final full-page diff. Fixed in `8c707fd`.

**Presence is not effect.** Grepping the built CSS for `env(safe-area-inset`
proves a declaration exists, not that it applies. Headless Chrome resolves
every inset to `0px`, so computed style cannot distinguish them either. The
non-inset part of the expression is what makes the check possible: a
`StandaloneScreen` with live padding measures 63px top at a 900px-tall
viewport, 59.08px at 844px, against 20px when it is overridden.

---

## 9. Accessibility invariants carried by styling

Quoted from the frozen specs; listed here because a styling change can break
them silently.

| Requirement | Source | Mechanism |
| --- | --- | --- |
| Touch targets ≥ 44×44 | `UI_UX_SPEC_V2` §13 | `min-h-[44px]` / `min-h-[48px]` on buttons and `CheckboxRow`'s label, `h-11 w-11` on the password toggle |
| Single column, fluid to 320px | `UI_UX_SPEC_V2` §13 | `min-w-[320px]` on `body`; fluid grids |
| Safe-area insets respected | `UI_UX_SPEC_V2` §13 | §8 above |
| Colour is never the only indicator | `UI_UX_SPEC_V2` §14 | four distinct `::before` **shapes** on `StatusBadge`; inset top rule on the active nav tab and pressed chord modifier |
| Reduced motion disables animation | `UI_UX_SPEC_V2` §14 | the `@media (prefers-reduced-motion: reduce)` block on `*` |
| Focus always visible | `UI_UX_SPEC_V2` §14 | `:focus-visible` outline rules in `@layer base` |
| Dialogs trap focus | `UI_UX_SPEC_V2` §14 | `useFocusTrap`; `Dialog` and `DangerZone` supply the `tabIndex={-1}` it needs |

The four badge shapes are: filled disc with a halo (good), hollow 2px ring
(warning), square with a 1px radius (bad), smaller hollow dot (neutral).
Measured: 9.59px / 9.59px / 9.59px / 7.19px. Collapsing any of these into a
colour-only difference violates §14.

---

## 10. Verification

### 10.1 What the CI gate does and does not assert

`./scripts/check-webapp.sh` runs typecheck, ESLint, stylelint, 544 vitest
tests, the production build, the real-Chrome browser workflows, and axe-core.
Its only *style* assertions are:

- `assertTouchTargets` — real geometry, ≥ 44×44, at 360px and default width.
- `assertResponsiveLayout` — content fits the viewport and adapts between
  360px and 1280px.
- axe-core — contrast and focus.

**It asserts nothing about spacing, colour, or pseudo-element shape.** The
T3-1 padding regression passed all three: content still fit, targets stayed
≥44px, contrast never changed. Any styling change whose failure mode is
"looks wrong" is invisible to this gate.

### 10.2 The visual diff

Until the harness in the companion TODO is checked in, a styling change is
proven by a cross-tree diff run by hand:

1. `git worktree add <scratch>/base <commit-before>` — never `git checkout`
   on a dirty tree.
2. Build both trees; drive both with the same script against the fixture
   servers in `webapp/tests/browser/fixtures/`.
3. For each state, walk **every element in the document** (`querySelectorAll("*")`)
   capturing a fixed property list plus `getBoundingClientRect()`, and take a
   full-page screenshot.
4. Compare as sorted key→value maps. Custom-property *enumeration order*
   shifts when stylesheet rules are removed; only values matter.

The property walk is the stronger instrument. In the boundary diff, two
screenshots differed but five captures differed in computed values — the extra
three were inside an inner scroll container or behind a modal overlay, where a
full-page screenshot cannot see.

### 10.3 Viewports that must be covered

- 390×844 and 1280×900 for every affected screen.
- **The exact boundary width of any media query touched**, and one pixel
  either side. A two-viewport sweep cannot see a breakpoint edge.
- 844×390 with `hasTouch: true` for the landscape surface — `hasTouch`
  supplies `pointer: coarse`, one of the three conditions in
  `landscapePhoneMediaQuery`.
- Height ≤ 608px for anything touching the editor's `short:` fallback.

### 10.4 The coverage rule

**A diff proves only what it rendered.** Every regression this migration
produced was invisible to the task that caused it because that task never drew
the affected screen. When changing a shared component, enumerate its call
sites and render at least one screen per site.

Screens that need deliberate setup, with how to reach them:

| Screen | How |
| --- | --- |
| `.empty-state` | a fixture blob whose package has zero macros |
| overflow menu + delete confirmation | `More actions for <macro>` → `Delete <macro>` |
| execution recovery overlay | copy `tests/browser/run-h4-recovery-tests.mjs` |
| sign-in, first-run, no-blobs, snapshot recovery | `startStartupFixtureServer` options |
| four USB badge states | `startStartupFixtureServer({usbState})` |
| import-ready panel | `page.setInputFiles` with a gzipped fixture |
| unsaved-changes prompt | reorder a macro, then Load a snapshot |
| **reconnect screen** | **not reachable — no fixture implements `POST /api/v1/restart`** |

### 10.5 Reproducing this document's measurements

```bash
export NVM_DIR="$HOME/.nvm" && . "$NVM_DIR/nvm.sh" && nvm use 24.18.0
npm --prefix webapp run build
# class tokens that generate no CSS (expect exactly the three §7 hooks)
# emission order of descendant variants (§6.1)
# env(safe-area-inset-*) occurrences and owners (§8)
```

The scripts that produced §3's no-conflict invariant, §6.1's offsets and §7's
orphan count are one-off analyses; the companion TODO's first task is to check
them in so they stop being one-off.

---

## 11. Do not

- Do not build a class name by interpolation (§4.1).
- Do not append an override to a variant string when the two touch a common
  property (§4.2).
- Do not use `max-[X]:` where the source condition is `<=` (§5.2).
- Do not nest two components that own the same descendant variant (§6.1).
- Do not move safe-area padding into a class rule (§8).
- Do not delete a test hook without its test (§7).
- Do not add a rule to `@layer components` (§2.1). It holds `main` and
  `#main-content`, and that is the whole list.
- Do not treat "it compiles", "the tests pass" or "check-webapp.sh exits 0"
  as evidence that rendering is unchanged (§10.1).
- Do not run `git checkout -- .` to get a clean baseline; use a worktree.

---

## 12. Provenance

| Claim | Established by |
| --- | --- |
| Zero same-property utility collisions | scan of 156 class tokens across all `.tsx` |
| Three orphan class tokens, all hooks | escaped-selector lookup of every token against the built CSS |
| Emission offsets in §6.1 | regex offsets into the built stylesheet |
| `max-[X]:` semantics | built-CSS grep + 511/512/513px cross-tree diff |
| Padding regression figures | cross-tree diff against `7132c95f` at 390×844 and 1280×900 |
| Badge shape measurements | `getComputedStyle(el, "::before")` in all four states |
| CI's style assertions | reading `tests/browser/lib/page.mjs` |
| Whole-migration equivalence | 110 cross-tree captures, 607 784 computed properties, zero differences, 110/110 screenshots byte-identical |
