# UXR-010 — Redesign screen map

- **Date:** 2026-10-15
- **Task:** UXR-010 in `docs/UI_UX_REDESIGN_TODO_2026-10-15.md`
- **Status:** Design record for product-owner review. **Not normative.** The
  normative sources are `docs/SPEC_V2.md`, `docs/UI_UX_SPEC_V2.md`, and, once
  accepted, `docs/UI_UX_SPEC_MATERIAL_3_EXPRESSIVE.md`. Nothing here adds a
  requirement; every row restates a rule from those documents or records a
  design choice that the product owner can change.

## Routing approach

`ScreenV2` and `routeFromHashV2` stay unchanged. Sub-screens use a query
parameter on an existing route, so every current hash keeps its meaning and
unknown or malformed queries fall back to the parent screen:

| Hash | Meaning |
| --- | --- |
| `#/settings` | Settings group list |
| `#/settings?group=device\|network\|security\|usb\|reset` | One settings group |
| `#/diagnostics` | Unchanged; highlights Settings |
| `#/snapshots` | Snapshot list |
| `#/snapshots?view=files` | Import and export |
| `#/packages` | Package list: choose, and manage through each row's menu and focused dialogs (no separate manage route) |
| `#/macro-editor[?macroId=]` | Unchanged editor route |

The key and combo picker is **not** a route. It is a view inside the editor
component, because the draft name and source live in editor state and a route
change would unmount it and lose them. It has its own title and Back control.

## Navigation by width

| Width | Pattern | Notes |
| --- | --- | --- |
| Under 600 px | Navigation bar at the bottom, four destinations | Labels never clipped at 320 px; top app bar is one row |
| 600-1199 px | Navigation rail at the left | Content column capped at a readable width |
| 1200 px and up | Same rail, wider content, editor uses two panes | Panes only where it helps (editor plus key picker) |

Destinations: Macros, Packages, Snapshots, Settings. Diagnostics and the editor
keep their current highlighting rules. Phone portrait-required behavior and the
landscape block are preserved; the block surface keeps the active-send status
and Cancel.

## Persistent app bar

One compact row on every authenticated route: page title (or back arrow plus
title on focused screens), device readiness (icon plus short text), and a single
saved/unsaved indicator. Package name appears on Macros and the editor only. A
**Save snapshot** action appears in the app bar whenever the working copy is
dirty. Active-send state and Cancel appear in a banner directly under the app
bar on every route while a send is active or its state is unknown.

## Screens

Columns: purpose, primary action (always visible), visible state, next step,
what may scroll, and whether it needs a subpage.

| Screen | Purpose | Primary action | Visible state | Next step | Scrolls | Subpage |
| --- | --- | --- | --- | --- | --- | --- |
| Checking, device unreachable | Say what is happening | Retry (unreachable) | Short status text | Retry | no | no |
| First-run setup | Provision the device | Continue / Apply setup | Step n of 3 | Next step, Back | no | three steps: name and code, Wi-Fi and password, review |
| Sign in | Authenticate | Sign in | Field error inline | Reveal password | no | no |
| Repository startup states | Open or create the repository | Retry, Create package, or Load snapshot | Loading, or the failure and what it affects | Start over | candidate list only | no |
| Package chooser | Pick a package | Tap a package | Macro counts | none | package list | no |
| Reconnect | Wait for the device | none (automatic); Reload on error | Elapsed state | Reload | no | no |
| Landscape block | Ask to rotate | Cancel send (only while sending) | Send progress | none | no | no |
| Macros | Run and manage macros | Send (per row); Add macro | Package, USB readiness, send state | Edit | macro list | no |
| Macro editor | Write one macro | Save | Name and source validity, byte counts | Insert keys, Cancel | source field | key picker view |
| Key picker | Insert special keys, modifiers, combos, delay, escapes | Insert | Generated syntax preview | Back to editor | key groups | in-editor view |
| Packages | Choose the open package; create, rename, reorder, duplicate, delete | Open (per row); New package | Current package marked | Row menu | package list | dialogs for naming and deleting |
| Snapshots | Save and restore versions | Save snapshot | Dirty state, loaded snapshot, storage | Load, Delete | snapshot list | `?view=files` |
| Import and export | Move a repository file in or out | Export / choose file | Validation result, replacement consequence | Back | no | yes |
| Settings | Reach a group | Tap a group | Group summaries | none | no | five groups |
| Settings group | Edit one group | Save (or the group's action) | Validation and saved state | Back | no | yes |
| Reset and reconnect group | High-impact actions | none highlighted | Consequence per action | Confirm dialog | no | yes |
| Diagnostics | Inspect and export device state | Download diagnostics | Privacy note beside export | Copy | output cards | no |

## Dialogs and confirmations

| Dialog | Kept behavior | Redesign note |
| --- | --- | --- |
| Unsaved changes | Cancel, Export, Save snapshot, Discard | Names the action being guarded |
| Delete macro / package / snapshot | Same confirm semantics; snapshot keeps type-the-ID | Names the object and consequence; modal M3 dialog instead of inline panels |
| Replace snapshot, import replace | Same consequences | Focused steps, not nested disclosures |
| Restart, reset settings, factory reset | Same phrases and password rule | Shown inside the Reset group |
| Remove station network | Currently no confirmation | Decision deferred to UXR-060 after reading `SPEC_V2.md` |

## Existing-behavior checks for this map

- Package selection stays a device preference that does not dirty the
  repository.
- The editor's Cancel still discards without a prompt unless the existing
  specification says otherwise; verified at UXR-042, not assumed here.
- Source visibility on the Macros list follows the shipped rule (V2-092) until
  the product owner decides otherwise; the redesign may shorten it to one
  line, not hide it.
- Settings controls that do not exist today (sending behavior, source preview)
  are not added by the redesign.

## Changes made while implementing (2026-10-15)

- Packages are one screen. Management is each row's "More actions" menu plus
  focused dialogs (New package, Rename, Delete), so `#/packages?view=manage`
  was not added.
- The macro editor hides the phone bottom bar and shows a Back control in the
  app bar. Import and export does the same, via `#/snapshots?view=files`.
- The app bar title is the route name; the package name is shown by the
  Macros page itself, once.
