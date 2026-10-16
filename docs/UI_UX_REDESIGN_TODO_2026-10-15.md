# ESP32 Macro Keyboard — Material 3 Expressive UI/UX Redesign TODO

**Status:** Implementation plan for `docs/UI_UX_SPEC_MATERIAL_3_EXPRESSIVE.md`
**Source snapshot:** `esp32-macro-keyboard-master-2610152104.zip`
**Scope:** Webapp UX and visual redesign; no firmware or API redesign implied

## 0. Authority and working rules

This checklist converts the product-owner review draft in
`docs/UI_UX_SPEC_MATERIAL_3_EXPRESSIVE.md` into an implementation and evidence
plan. Until the product owner accepts that specification, implementation must
not reinterpret the existing `docs/SPEC_V2.md` or
`docs/UI_UX_SPEC_V2.md`. After acceptance, use the new UI specification for the
presentation and workflow changes it explicitly covers; preserve all other v2
product behavior.

- [ ] Read repository `CLAUDE.md`, `AGENTS.md`, `SPEC_V2.md`, `UI_UX_SPEC_V2.md`,
      current webapp architecture, and current release/bundle budgets before
      changing code.
- [ ] Work directly on `master`, following repository policy. Do not create a
      branch or PR for each task.
- [ ] Keep existing auth, API, persistence, repository schema, USB HID,
      execution, privacy, device-setting, and recovery contracts intact.
- [ ] Do not modify the frozen existing spec files as part of implementation.
      Any proposed change to the new specification requires product-owner
      review before it becomes normative.
- [ ] Do not claim a task complete without code plus reproducible evidence.
- [ ] Do not add a runtime network dependency, analytics, or browser-persistent
      repository storage.
- [ ] Maintain a compact before/after screenshot set and test/evidence log for
      each coherent phase.

## 0.1 Completion rules

- [ ] Implement in dependency order; keep the app usable at each phase boundary.
- [ ] Add/update tests alongside behavior changes. Avoid tests that only restate
      implementation details.
- [ ] Run the narrowest relevant checks first, then the webapp gate, then the
      applicable broader gates.
- [ ] Test desktop and phone layouts, keyboard and touch operation, and offline
      asset loading.
- [ ] Keep uncompleted hardware-dependent checks open; browser work does not
      imply hardware qualification.
- [ ] Reconcile this TODO and record exact commit SHA, commands, pass/fail
      counts, screenshot locations, bundle measurements, and limitations.

## Phase 0 — Baseline and workflow inventory

### UXR-000 — Record repository baseline

- [x] Record current master SHA and working-tree state.
- [x] Run existing frontend build/test/check scripts and record the baseline.
- [x] Capture current first-run, sign-in, macro list, editor, package, snapshot,
      settings, diagnostics, active-send, and error/recovery screens.
- [x] Capture screenshots at 320×568, 360×800, tablet portrait/landscape, and
      desktop 1440×900.
- [x] Record current initial JS/CSS/font/image bundle sizes and firmware webfs
      budget headroom.

### UXR-001 — Map current routes and behavior

- [x] Enumerate every route, screen, dialog, menu, loading state, error state,
      empty state, and destructive confirmation.
- [x] Map each user task from entry to completion and identify where controls
      appear below the fold or inside unclear menus.
- [x] Record all copy blocks that explain UI instead of helping a real decision.
- [x] Record current breakpoints, scroll containers, fixed/sticky elements,
      safe-area handling, and virtual-keyboard interactions.
- [ ] Map each UI flow to the existing v2 behavior and tests so presentation
      changes cannot silently alter product semantics.

**Phase 0 evidence** (2026-10-15, baseline `5bed6a6`): see
`docs/implementation-v2/UXR_000_001_BASELINE_AND_INVENTORY_2026-10-15.md`.
Gate run: 570 unit tests, browser and axe pass; `test:visual` fails on the
unmodified tree (host-font-specific baselines, recorded there). Screenshots:
`docs/ui-redesign/baseline-screenshots/` (120 PNGs, regenerate with
`webapp/tests/browser/visual/screenshot-set.mjs`). The flow-to-behavior mapping
is done at test-file level only, so that one UXR-001 item stays open until each
screen is mapped to `SPEC_V2.md` sections during its own task.

**Phase 0 exit gate**

- [x] Baseline record is reproducible; route/state inventory is complete.
- [x] Every known viewport, discoverability, copy, and layout problem has a
      specific target screen and acceptance condition.

## Phase 1 — UX architecture and Material 3 system

### UXR-010 — Establish the redesign screen map

- [x] Define the navigation and screen map for startup, sign-in, macros,
      macro editing, package selection/management, snapshots, import/export,
      settings, diagnostics, recovery, and active send.
- [x] For each screen, state purpose, primary action, required visible state,
      next action, scrollable data, and whether a dedicated subpage is needed.
- [x] Make the phone, tablet, and desktop navigation patterns explicit.
- [x] Preserve phone portrait-required behavior and desktop/tablet landscape use;
      document active-send access during phone-landscape blocking.
- [ ] Review proposed screen map against all existing behavior requirements.
      (Partial: `UXR_010_SCREEN_MAP_2026-10-15.md` lists the behavior checks it
      relies on; a section-by-section `SPEC_V2.md` review is still open.)

### UXR-011 — Select and document implementation approach

- [x] Evaluate an M3 Expressive implementation compatible with React 19 and the
      existing webapp stack. Compare a locally bundled component library with
      an in-repo M3 token/component layer.
- [ ] Measure package size, build/runtime compatibility, accessibility support,
      license, supported-browser compatibility, and firmware web-asset impact.
      (Partial: size, license and web-asset impact measured in
      `UXR_011_M3_IMPLEMENTATION_APPROACH_2026-10-15.md`; accessibility and
      cross-browser support were not measured for the library options.)
- [x] Choose one approach and record rationale. Do not add multiple overlapping
      component systems.
- [x] Confirm every dependency and asset is available offline after device
      flash.

### UXR-012 — Build design tokens and primitives

- [x] Define semantic M3 color roles, typography scale, shape, spacing,
      elevation, state, focus, and motion tokens.
- [ ] Define button, icon-button, text-field, list, card, navigation,
      menu, dialog, snackbar, status, and progress primitives.
      (Partial: `Button`, `IconButton`, `Icon`, `Card`, `Dialog`,
      `StatusBadge`, `ErrorBanner`, and the shell navigation exist; snackbar,
      progress, list item, text field, and menu are not yet extracted.)
- [x] Establish sentence-case copy and icon labeling rules.
- [x] Remove the PBT/keycap/uppercase-legend visual language from shared
      components and global styles.
- [x] Add theme/contrast checks and reduced-motion behavior.

**Phase 1 evidence** (2026-10-15): `UXR_010_SCREEN_MAP_2026-10-15.md`,
`UXR_011_M3_IMPLEMENTATION_APPROACH_2026-10-15.md`,
`UXR_012_TOKENS_AND_PRIMITIVES_2026-10-15.md` in `docs/implementation-v2/`;
prototype screenshots in `docs/ui-redesign/phase1-prototype/`.

**Phase 1 exit gate**

- [x] A representative screen prototype uses the selected M3 system and reads
      clearly at phone and desktop sizes.
- [x] No remote resources are loaded; bundle headroom remains acceptable.
- [x] Product owner accepts the overall design direction before broad screen
      conversion. (Accepted by the product owner in the 2026-10-15 session, after
      commit `eb7df61`.)

## Phase 2 — Application shell and responsive navigation

### UXR-020 — Redesign authenticated shell

- [x] Replace the current header with a compact M3 app bar/toolbar suited to
      phone and large-screen widths.
- [x] Show device readiness, selected package context, saved/unsaved state, and
      save action without redundant status badges.
- [x] Keep active-send status and cancellation visible/reachable across routes.
      (Macros stays mounted while hidden on other routes; the shell shows the send banner with Cancel. Real Chrome workflow cancels a send from Packages.)
- [x] Implement phone navigation bar and suitable larger-screen navigation.
- [x] Keep route names, active indication, focus behavior, and browser history
      behavior accessible and predictable.

### UXR-021 — Enforce visible task controls

- [x] Add layout rules and tests ensuring screen title, primary action, essential
      state, and next-step control fit the initial viewport at supported sizes.
      (`tests/browser/workflows/visibleControls.mjs`: table-driven, six viewports; only controls that hold today are listed, later phases add theirs.)
- [ ] Split screens where task controls cannot fit; do not solve by shrinking
      text or hiding controls below the fold.
- [ ] Keep only genuinely variable content in a scroll region; remove nested
      scroll areas and prevent fixed chrome from covering content/focus.
- [ ] Verify safe-area insets, browser chrome, text zoom, and virtual keyboard.
      (Partial: safe-area insets are applied in the shell; the bottom
      navigation hides while a field has focus on a touch phone and
      `interactive-widget=resizes-content` is set, both checked in
      `runKeyboardNavigationWorkflow`. Text zoom and a real keyboard are not
      yet verified; the editor's keyboard case waits for UXR-042.)

**Phase 2 evidence** (2026-10-15): app bar, navigation bar/rail, send banner,
focus handling in `AppShellV2.tsx` and `AppV2.tsx`; browser checks in
`webapp/tests/browser/workflows/visibleControls.mjs` and the cross-route cancel
step in `workflows/browser.mjs`; unit tests in `tests/v2-app-shell.test.tsx`.

**Phase 2 exit gate**

- [x] Shell works at phone, tablet, desktop, portrait, and landscape sizes.
      (320x568 to 1440x900 including 1024x768 tablet landscape; phone landscape
      is the unchanged landscape block surface.)
- [x] No horizontal overflow, hidden primary action, trapped focus, or
      inaccessible navigation remains. (Scoped to the shell and the routes in
      `visibleControls.mjs`; per-screen splits are tracked in later phases.)

## Phase 3 — Startup, setup, and sign-in

### UXR-030 — Redesign first-run setup

- [x] Convert setup into concise, focused steps with clear grouping and
      progress/orientation.
- [x] Keep validation beside fields and show one concise explanation for
      consequential network/restart actions.
- [x] Preserve setup code, device identity, password, network, confirmation,
      restart, and reconnect semantics.
- [x] Test incomplete fields, invalid values, request failure, device restart,
      and reconnect. (Unit tests: incomplete, invalid, per-step gating, Back
      keeps entries, request failure keeps review; Chrome: full setup to Sign
      In. The restart/reconnect screen itself is unchanged here and is restyled
      with the other device-action screens in UXR-060.)

### UXR-031 — Redesign sign-in and startup recovery

- [x] Build a concise sign-in screen with visible field label, submit action,
      password reveal affordance, and inline errors.
- [x] Redesign loading, session recovery, first repository, no snapshots,
      unreadable snapshot, and repository failure screens.
- [x] Ensure recovery choices state the consequence in plain language and never
      silently switch snapshots.
- [x] Preserve a live in-memory working copy through recoverable auth/network
      failures.

**Phase 3 evidence** (2026-10-15): `FirstRunSetupPage.tsx` (four-stop
stepper), `SignInPage.tsx`, `RepositoryStartupScreen.tsx`, `StepIndicator`,
`m3/Progress`, `m3/ListItem`; unit tests in `v2-first-run-setup-page`,
`v2-repository-startup-screen`; Chrome checks in `visibleControls.mjs`
(`STARTUP_SCREENS`, six viewports, axe at 320 and 1440).

**Phase 3 exit gate**

- [ ] Startup decision-table tests still pass and visual tests cover every
      startup branch. (Decision-table tests pass. Visual scenarios exist for
      setup, review, sign-in error, no blobs, empty package, and snapshot
      recovery, but none for the package chooser or the retry screens, and the
      visual baselines are regenerated only at UXR-072.)
- [x] No setup/sign-in/recovery action is hidden below the fold.

## Phase 4 — Macros operating screen and editor

### UXR-040 — Redesign Macros as primary operating surface

- [ ] Make selected package, USB state, macro count/list, Add macro, and Send
      entry points immediately understandable.
- [ ] Redesign rows/cards for fast scanning, clear names, obvious Send, and
      discoverable edit/manage actions.
- [ ] Keep common actions visible; do not hide essential features behind an
      unexplained overflow affordance.
- [ ] Make empty state actionable and concise.
- [ ] Preserve long-list scrolling only for macro data; keep Add macro, package
      context, and active-send controls available.
- [ ] Preserve source privacy constraints and verify no source leaks through
      logs, diagnostics, notifications, or completion feedback.

### UXR-041 — Redesign Quick Send states

- [ ] Design ready, disabled/not-ready, starting, awaiting-confirmation,
      running, canceling, completed, rejected, timeout, and release-failure
      states.
- [ ] Keep Cancel and required safety/release controls visible during sends.
- [ ] Ensure polling produces stable visuals and restrained live announcements.
- [ ] Verify cancellation/recovery behavior on navigation, reload, reconnect,
      and phone-landscape surface.

### UXR-042 — Redesign macro editor

- [ ] Create focused editor layout with name, source, compact contextual help,
      validation locations, and clear save/cancel actions.
- [ ] Keep Save/Done, Cancel/Back, and a clearly labeled key/directive insertion
      entry point visible at minimum viewport heights.
- [ ] Preserve cursor-aware insertion for all named special keys, including Tab,
      navigation/function keys, and Ctrl/Alt/Shift/GUI modifiers.
- [ ] Provide an explicit key-combo builder or equivalent controls for
      simultaneous-key groups; show the exact generated syntax before insertion.
- [ ] Preserve escaped brace/bracket insertion, delay directives, and all
      supported macro language symbols without reducing them to unexplained
      manual typing.
- [ ] Keep the full key palette usable on phones through a focused picker
      subpage when it cannot fit in the editor viewport.
- [ ] Preserve dirty state, navigation guard, validation, CRUD, and macro-language
      behavior.
- [ ] Clearly distinguish preview/validation from physical send.

**Phase 4 exit gate**

- [ ] Macro list and editor pass existing behavior tests plus responsive and
      accessibility checks.
- [ ] Quick Send remains one clear action when USB is ready; safety states are
      never obscured.

## Phase 5 — Packages, snapshots, and repository files

### UXR-050 — Redesign package selection and management

- [ ] Make the current package and selection state easy to scan.
- [ ] Create focused management flows for add, rename, reorder, duplicate, and
      delete as supported.
- [ ] Provide accessible move controls in addition to any drag interaction.
- [ ] Preserve package preference and repository dirty-state rules.

### UXR-051 — Redesign snapshots

- [ ] Create a clear snapshot list and focused save/load/delete flows.
- [ ] Show useful metadata without exposing repository contents or sensitive
      macro source.
- [ ] Keep retention target advisory and deletion manual.
- [ ] Add distinct empty, saving, loading, invalid, full-storage, and failure
      states.
- [ ] Protect dirty work before replacement/load/delete as specified.

### UXR-052 — Redesign import/export

- [ ] Separate routine snapshot management from file import/export when that
      reduces cognitive load.
- [ ] Design file selection, validation, progress, replacement confirmation,
      success, and failure states.
- [ ] Preserve gzip/schema/size rules and avoid browser-persistent repository
      storage.

**Phase 5 exit gate**

- [ ] Package, snapshot, and file workflows pass their existing contracts and
      tests; all consequential actions are clear and reversible where possible.

## Phase 6 — Settings, diagnostics, dialogs, and system feedback

### UXR-060 — Redesign settings information architecture

- [ ] Group settings by user goal and create subpages when a group cannot fit in
      the first viewport.
- [ ] Cover identity, Wi-Fi/access point, password/security, USB identity and
      confirmation, reset/reconnect, and diagnostics as available.
- [ ] Put reset/destructive functions in a distinct area with exact impact and
      confirmation.
- [ ] Preserve settings validation, reconnect, password-change, and security
      behavior.

### UXR-061 — Redesign diagnostics

- [ ] Keep diagnostics secondary and technical, with concise explanation of
      export contents and privacy.
- [ ] Make export/download action visible without scrolling through output.
- [ ] Ensure sensitive data and macro source remain excluded.

### UXR-062 — Unify dialogs, menus, errors, and confirmation

- [ ] Implement M3 patterns for dialogs, menus, snackbars, inline messages,
      progress, and recovery surfaces.
- [ ] State consequence and object in destructive confirmations.
- [ ] Ensure error copy says what happened and the next useful action.
- [ ] Verify keyboard dismissal, focus trap/restore, screen reader announcement,
      and no accidental dismissal of critical unsaved-work prompts.

**Phase 6 exit gate**

- [ ] All settings, diagnostics, menu, dialog, and error routes have responsive
      and accessible coverage.

## Phase 7 — Copy, accessibility, and visual consistency

### UXR-070 — Remove explanatory UI copy

- [ ] Audit all user-facing copy screen by screen.
- [ ] Remove text that repeats control labels, explains obvious navigation, or
      narrates implementation details.
- [ ] Keep concise decision, consequence, privacy, validation, and recovery
      messages where needed.
- [ ] Verify plain language and sentence case across all routes.

### UXR-071 — Accessibility qualification

- [ ] Run axe-core scans on representative pages and all key overlays.
- [ ] Test keyboard-only operation, focus visibility/order, Escape behavior,
      screen-reader labels/status announcements, and touch target size.
- [ ] Verify contrast for light/dark/system states if more than one theme exists.
- [ ] Test 200% zoom/text enlargement, reflow at 320 CSS px, reduced motion, and
      virtual keyboard interactions.

### UXR-072 — Visual regression and design review

- [ ] Capture and review screenshots for phone 320×568 and 390×844, tablet
      portrait/landscape, and desktop 1440×900.
- [ ] Add stable visual regression baselines for high-value screens and states.
- [ ] Review alignment, hierarchy, whitespace, typography, color, shape,
      component consistency, and empty/error/loading states.
- [ ] Remove styling that resembles the old keycap/engineering demo treatment.
- [ ] Obtain product-owner visual review of representative completed screens
      before declaring the design phase complete.

**Phase 7 exit gate**

- [ ] No major accessibility defect, clipping, overflow, or visual inconsistency
      remains in reviewed viewports.
- [ ] Screens communicate the task through layout and concise labels rather than
      long instructions.

## Phase 8 — Offline, performance, compatibility, and release qualification

### UXR-080 — Verify offline asset/runtime behavior

- [ ] Inspect built assets and network requests; confirm no external runtime
      fonts, scripts, styles, images, analytics, or icon requests.
- [ ] Test the built app served from the device with internet unavailable.
- [ ] Verify all app routes and error/recovery screens still render offline.

### UXR-081 — Verify size and performance

- [ ] Compare JS/CSS/font/icon asset sizes against the recorded baseline and
      repository release budgets.
- [ ] Document the M3 implementation's raw and compressed cost.
- [ ] Confirm list/editor interactions remain responsive on target mobile
      browsers and constrained ESP32 serving conditions.

### UXR-082 — Full regression and reconciliation

- [ ] Run formatting, typecheck/build, webapp unit/component tests, real-browser
      interaction tests, axe tests, and visual regression checks.
- [ ] Run `scripts/check-webapp.sh`, `scripts/build-webfs-image.sh`,
      `scripts/generate-flash-manifest.sh`, and the applicable repository gates.
- [ ] Re-run relevant authentication, setup-route, persisted-state, route-sync,
      API contract, and macro privacy policy scripts.
- [ ] Compare all UI behavior against `SPEC_V2.md` and the accepted redesign
      specification; resolve any conflict without weakening product safety.
- [ ] Update webapp documentation and this TODO with completed tasks and exact
      evidence.
- [ ] Record final commit SHA, test counts, screenshot/baseline references,
      bundle sizes, offline results, and known limitations.

**Final exit gate**

- [ ] Every dependency-satisfied task is complete and evidenced.
- [ ] All major workflows pass on phone and desktop, with tablet coverage.
- [ ] App is visually coherent with Material 3 Expressive and feels polished.
- [ ] No primary action or essential state is hidden below the fold.
- [ ] Offline operation, privacy, firmware asset budgets, and existing product
      contracts remain intact.
- [ ] Product owner has reviewed the completed experience.
