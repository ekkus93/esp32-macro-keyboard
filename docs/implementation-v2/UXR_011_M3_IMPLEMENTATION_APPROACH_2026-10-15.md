# UXR-011 — Material 3 implementation approach

- **Date:** 2026-10-15; qualification updated 2026-10-16
- **Task:** UXR-011 in `docs/UI_UX_REDESIGN_TODO_2026-10-15.md`
- **Decision:** build an **in-repo Material 3 Expressive token and component
  layer** on the existing React 19 / Tailwind 4 stack. Add no component-library
  dependency.
- **Status:** design record for product-owner review; not normative.

## Options compared

| Criterion | In-repo token/component layer | `@material/web` 2.5.0 (Google, Apache-2.0) | `@mui/material` 9.5.0 (MIT) |
| --- | --- | --- | --- |
| Components needed by the spec | Built as needed: app bar, navigation bar and rail, buttons, fields, lists, cards, menus, dialogs, snackbar, progress | Has buttons, fields, dialog, list, menu, progress, chips, tabs, select. **No navigation bar, navigation rail, top app bar, snackbar or toolbar** (checked in the installed package) | Large set, Material You styling, not an Expressive implementation |
| Measured size | Tokens and primitives are plain CSS and small React; full app JS/CSS measured below | 9 components bundled and minified: **175,074 bytes raw, 38,643 gzip** (before any missing component is hand-built) | 8.5 MB unpacked; tree-shaken cost not measured because the dependency tree (emotion, styled engine) is the larger concern |
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

- Colors use a contrast-checked M3 palette with deep teal primary, warm ivory
  surfaces, and terracotta accents. The appearance setting supports System,
  Light, and Dark; System follows `prefers-color-scheme`. The explicit choice is
  stored as a UI preference only, never with repository data. `light-dark()` is
  not used, so the app controls all role overrides from its single token set.
- Fonts are the system stack; no font files ship (`verify-no-remote-assets.sh`).
- Icons are inline SVG in one local component with a single stroke style; every
  icon-only control needs an accessible name.
- Shape and type scales are exposed as `rounded-shape-*` and `text-<role>`
  utilities so components never hard-code geometry.
- Status keeps its non-color shape cues (the four `StatusBadge` shapes).

## Limitations

- `@material/web` coverage was judged from the installed package's directory
  listing and a bundle measurement, not from a prototype of every component.
- Accessibility comparison: Material Web describes its components as
  accessible, but its project site says the library is in maintenance mode
  ([project site](https://material-web.dev/)). MUI's current accessibility
  report is a draft self-assessment of 12 components: 27 criteria are marked
  supported and five partially supported, with no assistive-technology tests;
  it excludes several components this app needs, including Dialog, Menu,
  Select, and Snackbar ([MUI accessibility report](https://mui.com/material-ui/getting-started/accessibility/)).
  The in-repo layer has app-level axe scans and automated focus/contrast tests,
  while manual screen-reader qualification remains open under UXR-071.
- Supported-browser evidence for the selected implementation: Playwright
  Chromium workflows, axe, reduced-motion, six viewport sizes, and the Android
  Chrome 133 emulator all passed. Firefox 153 passed the responsive-control
  workflow at phone and desktop sizes, including touch-target and axe checks.
  MUI's published v9.5.0
  matrix lists Chrome >=117, Firefox/Edge >=121, and Safari >=17
  ([supported platforms](https://mui.com/material-ui/getting-started/supported-platforms/));
  this is candidate-library evidence, not a claim that this app has passed all
  those browsers. Playwright WebKit 26.5 launched in the pinned
  `mcr.microsoft.com/playwright:v1.62.1-noble` Docker image and passed responsive
  controls, touch-target and axe scans, startup layouts, keyboard viewport
  checks, and macro-editing/package workflows. Safari itself is not qualified
  by this Linux WebKit run. Actual screen-reader and physical-device
  performance checks remain open; 200% browser zoom is recorded in the TODO's
  Firefox Docker evidence.

## 2026-10-16 release measurements

Production webapp build: JS **340,970 bytes raw / 102,120 gzip**; CSS
**38,968 raw / 7,680 gzip**; no font or external runtime assets. Compared with
the recorded baseline (JS 313,486 / 93,241 and CSS 27,946 / 6,193), total gzip
increased by **9,866 bytes**. This is the whole redesign app delta, not an
isolated component-library cost.

`check-all.sh` produced a webfs image using **520,192 of 891,289 budget bytes**
(58.4%, 371,097 bytes free); the OTA app used 1,006,896 of 2,097,152 bytes
(48.0%). The 1 MiB generated LittleFS image is partition-sized; the manifest
budget counts packed webfs content.
