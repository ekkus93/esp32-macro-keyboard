# UXR-011 — Material 3 implementation approach

- **Date:** 2026-10-15
- **Task:** UXR-011 in `docs/UI_UX_REDESIGN_TODO_2026-10-15.md`
- **Decision:** build an **in-repo Material 3 Expressive token and component
  layer** on the existing React 19 / Tailwind 4 stack. Add no component-library
  dependency.
- **Status:** design record for product-owner review; not normative.

## Options compared

| Criterion | In-repo token/component layer | `@material/web` 2.5.0 (Google, Apache-2.0) | `@mui/material` 9.5.0 (MIT) |
| --- | --- | --- | --- |
| Components needed by the spec | Built as needed: app bar, navigation bar and rail, buttons, fields, lists, cards, menus, dialogs, snackbar, progress | Has buttons, fields, dialog, list, menu, progress, chips, tabs, select. **No navigation bar, navigation rail, top app bar, snackbar or toolbar** (checked in the installed package) | Large set, Material You styling, not an Expressive implementation |
| Measured size | Tokens and primitives are plain CSS and small React; measured at UXR-081 | 9 components bundled and minified: **175,074 bytes raw, 38,643 gzip** (before any missing component is hand-built) | 8.5 MB unpacked; tree-shaken cost not measured because the dependency tree (emotion, styled engine) is the larger concern |
| React 19 and strict lint | Plain function components under the existing `strictTypeChecked` ESLint | Lit web components; needs custom-element JSX typings and event wiring under the same lint rules | Supports React 19 |
| Tailwind coexistence | Native: tokens live in `@theme` | Own CSS custom properties and shadow DOM; two token systems | Own styling engine; two systems |
| Offline and no CDN | Trivially local | Local; ships no fonts by default | Local |
| Lockfile and pin policy | No change | Adds `@material/web`, `lit`, `@lit/context`, `tslib`; every addition needs an exact pin and audit (`audit:ci` already reports 22 findings on the current tree) | Adds a large tree |
| Expressive fidelity | Full control of shape, type and motion roles | Baseline M3; no Expressive-specific components | Not Expressive |

## Why in-repo

1. The two libraries leave the highest-value shell components (navigation bar
   and rail, app bar, snackbar) to be built by hand anyway, so a library would
   add a second system without removing the work.
2. A single token source (`@theme` in `webapp/src/styles.css`) keeps the
   contrast test, the Tailwind utilities, and the components in agreement.
3. No new runtime dependency means no new audit surface, no lockfile change,
   and no web-component typing exceptions in lint.
4. The webfs budget has ample headroom (UXR-000: 466,944 of 891,289 bytes), so
   size is not the deciding factor; coupling and missing components are.

## Constraints the layer follows

- Colors are M3 roles with a fixed branded palette (primary `#6750A4` family)
  and a dark scheme selected by `prefers-color-scheme`. `light-dark()` is not
  used: overriding custom properties in a media query works in every supported
  browser, and the app already requires `CompressionStream` (Safari 16.4+).
- Fonts are the system stack; no font files ship (`verify-no-remote-assets.sh`).
- Icons are inline SVG in one local component with a single stroke style; every
  icon-only control needs an accessible name.
- Shape and type scales are exposed as `rounded-shape-*` and `text-<role>`
  utilities so components never hard-code geometry.
- Status keeps its non-color shape cues (the four `StatusBadge` shapes).

## Limitations

- `@material/web` coverage was judged from the installed package's directory
  listing and a bundle measurement, not from a prototype of every component.
- Supported-browser evidence for the in-repo layer is gathered at UXR-071
  (axe, real Chrome) and UXR-081; other engines are not exercised by this
  repository's browser tests.
