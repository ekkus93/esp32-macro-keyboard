# UXR-012 — Design tokens, primitives, and the Phase 1 prototype

- **Date:** 2026-10-15
- **Task:** UXR-012 in `docs/UI_UX_REDESIGN_TODO_2026-10-15.md`
- **Status:** implementation evidence for product-owner review of the design
  direction. Not normative; the Phase 1 exit gate (owner acceptance) is open.

## What changed

- `webapp/src/styles.css` defines Material 3 colour roles (warm ivory, deep teal
  and terracotta in light mode; deep green-black surfaces and sea-glass teal in
  dark mode), M3 type roles
  (`text-title-lg`, `text-body-md`, ...), a shape scale (`rounded-shape-*`),
  elevation, and motion easing, all in `@theme`. The keycap button treatment,
  uppercase legends and the "PBT and petrol" palette are removed.
- A bare `<button>` is the M3 tonal button; `.primary` is filled and `.danger`
  is the error button, so existing call sites changed colour and shape without
  markup edits. Inputs are M3 outlined fields. Focus ring is a 3 px primary
  outline with 2 px offset.
- `src/components/m3/Icon.tsx`: one local inline-SVG icon set (22 icons, one
  stroke style, `aria-hidden`).
- `src/components/m3/Button.tsx`: `Button` (filled, tonal, outlined, text,
  danger, optional icon) and `IconButton` (requires `label`, used as the
  accessible name and tooltip).
- Shared components restyled onto M3 roles: `Card`, `StatusBadge` (keeps four
  distinct shapes), `Dialog`, `DangerZone`, `ErrorBanner` (now with an icon),
  `SendStatus`, `Eyebrow`, `PageHeading`, `FieldHelp`, `StandaloneScreen`.
- `AppShellV2` is rebuilt: one compact top bar (device name, package, USB and
  saved indicators, Save snapshot when dirty) plus an M3 navigation bar at the
  bottom under 600 px and a navigation rail from 600 px up. Content is capped at
  52 rem. This is UXR-020 work pulled forward so the prototype reads as a whole;
  UXR-020 stays open until it is verified against its own checklist.
- `webapp/stylelint.config.mjs` allows Tailwind's `--role--sub-variable`
  custom-property names (one optional `--kebab-case` suffix). Other naming rules
  are unchanged.
- `webapp/tests/tailwindClassCollisions.ts` knows the type-role utilities set
  size, line height, and (where defined) weight and tracking together.

## Evidence

| Check | Result |
| --- | --- |
| `vitest run` | 61 files, 640 tests pass (was 60 files, 570; adds 70 contrast cases) |
| `tests/m3-contrast.test.ts` | 29 text pairs at 4.5:1 and 5 boundary or focus pairs at 3:1, each in light and dark (68 cases), plus 2 token-set checks |
| `eslint`, `stylelint`, `prettier --check`, `tsc -b` | clean |
| `test:browser` (workflows, axe-core, H4, H5) | pass |
| `check:status-badge-shapes` | all four states geometrically distinct |
| `check:no-orphan-classes` | pass |
| `verify-no-remote-assets.sh webapp/dist` | pass |
| `check-docs.sh` | pass |
| `test:visual` | still fails, as at baseline (host-font-specific baselines); not regenerated until UXR-072 |

Bundle after the change: JS 315,823 raw / 94,255 gzip (baseline 313,486 /
93,241); CSS 34,854 raw / 6,988 gzip (baseline 27,946 / 6,193). Cost of the
whole layer so far: +2.3 KB JS and +6.9 KB CSS raw, about +1.8 KB gzip combined.

Screenshots of the prototype at 320x568, 390x844 and 1440x900:
`docs/ui-redesign/phase1-prototype/` (Macros, macro editor, Settings with the
restart dialog).

## Rules established

**Copy.** Sentence case everywhere; no CSS uppercase. Labels are verb plus
object where useful. A paragraph is kept only if it states a consequence, a
privacy fact, or a recovery step.

**Icons.** Icons are decorative and `aria-hidden`; meaning comes from visible
text or an accessible name. An icon-only control must be an `IconButton`, whose
`label` is required and doubles as the tooltip. No icon font and no network
icon source.

**Variants.** A component with several visual variants selects one complete
literal class string per variant; layered overrides are not used.

## Known gaps (deliberately left for later tasks)

- Snackbar, progress, list-item and text-field components are not extracted;
  fields use the global base style. They are built where first needed.
- Macros still shows the package name twice and seven buttons per row; the row
  restructure is UXR-040. The prototype shows the direction, not the final
  Macros screen.
- The macro editor, Settings and Snapshots layouts are unchanged apart from
  colour, shape and type.
- Visual baselines are stale (see above).
- Dark palette contrast is tested; dark-mode axe coverage is in the Chrome
  browser suite. Dark review screenshots are collected for product-owner review.
