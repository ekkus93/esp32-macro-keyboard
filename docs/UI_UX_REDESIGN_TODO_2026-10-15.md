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

- [x] Read repository `CLAUDE.md`, `AGENTS.md`, `SPEC_V2.md`, `UI_UX_SPEC_V2.md`,
      current webapp architecture, and current release/bundle budgets before
      changing code.
- [x] Work on the product-owner-authorized review branch and keep it pushed for
      review; merge only after the owner accepts the completed experience.
- [x] Keep existing auth, API, persistence, repository schema, USB HID,
      execution, privacy, device-setting, and recovery contracts intact.
- [x] Do not modify the frozen existing spec files as part of implementation.
      Any proposed change to the new specification requires product-owner
      review before it becomes normative.
- [x] Do not claim a task complete without code plus reproducible evidence.
- [x] Do not add a runtime network dependency, analytics, or browser-persistent
      repository storage.
- [x] Maintain a compact before/after screenshot set and test/evidence log for
      each coherent phase.

## 0.1 Completion rules

- [x] Implement in dependency order; keep the app usable at each phase boundary.
- [x] Add/update tests alongside behavior changes. Avoid tests that only restate
      implementation details.
- [x] Run the narrowest relevant checks first, then the webapp gate, then the
      applicable broader gates.
- [x] Test desktop and phone layouts, keyboard and touch operation, and offline
      asset loading. (Local asset policy passes; device-served offline operation
      remains open.)
- [x] Keep uncompleted hardware-dependent checks open; browser work does not
      imply hardware qualification.
- [x] Reconcile this TODO and record exact code/test commit SHA, commands, pass/fail
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
- [x] Map each UI flow to the existing v2 behavior and tests so presentation
      changes cannot silently alter product semantics.

**Phase 0 evidence** (2026-10-15, baseline `5bed6a6`): see
`docs/implementation-v2/UXR_000_001_BASELINE_AND_INVENTORY_2026-10-15.md`.
Gate run: 570 unit tests, browser and axe pass; `test:visual` fails on the
unmodified tree (host-font-specific baselines, recorded there). Screenshots:
`docs/ui-redesign/baseline-screenshots/` (120 PNGs, regenerate with
`webapp/tests/browser/visual/screenshot-set.mjs`). The flow-to-behavior mapping
was initially recorded at test-file level; it was completed in
`docs/implementation-v2/UXR_001_BEHAVIOR_TRACEABILITY_2026-10-16.md`, which
maps flows to normative sections and regression suites.

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
- [x] Review proposed screen map against all existing behavior requirements.
      (`UXR_001_BEHAVIOR_TRACEABILITY_2026-10-16.md` maps each flow to
      `SPEC_V2.md` sections and executable tests.)

### UXR-011 — Select and document implementation approach

- [x] Evaluate an M3 Expressive implementation compatible with React 19 and the
      existing webapp stack. Compare a locally bundled component library with
      an in-repo M3 token/component layer.
- [x] Measure package size, build/runtime compatibility, accessibility support,
      license, supported-browser compatibility, and firmware web-asset impact.
      (Measured candidate library size, license, runtime fit, documented
      accessibility support and browser matrix; the selected in-repo layer
      passed Chrome/Firefox checks, axe, and the webfs budget. Actual assistive
      technology and Safari remain UXR-071/081 qualification items.)
- [x] Choose one approach and record rationale. Do not add multiple overlapping
      component systems.
- [x] Confirm every dependency and asset is available offline after device
      flash.

### UXR-012 — Build design tokens and primitives

- [x] Define semantic M3 color roles, typography scale, shape, spacing,
      elevation, state, focus, and motion tokens.
- [x] Define button, icon-button, text-field, list, card, navigation,
      menu, dialog, snackbar, status, and progress primitives.
      (`Button`, `IconButton`, `Icon`, `Card`, `Dialog`, `StatusBadge`,
      `ErrorBanner`, `Snackbar`, `Progress`, `ListItem`, and `Menu` are reusable;
      text fields use the shared M3 global input style.)
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
- [x] Split screens where task controls cannot fit; do not solve by shrinking
      text or hiding controls below the fold.
- [x] Keep only genuinely variable content in a scroll region; remove nested
      scroll areas and prevent fixed chrome from covering content/focus.
- [x] Verify safe-area insets, browser chrome, text zoom, and virtual keyboard.
      (Android Chrome 133 on Android 16: with the emulator's 48 dp tall-cutout
      overlay enabled, the browser viewport was 412×766 CSS px on a 412×915 CSS
      screen, `env(safe-area-inset-*)` was zero, the header began at viewport
      y=0, and document width stayed 412 px. Chrome placed its browser/system
      chrome outside the page viewport, so no content overlapped the cutout in
      the supported browser mode. The overlay was restored afterward. The app
      is served in a browser and has no installed standalone/PWA mode.
      Firefox 153 was also tested at 200% full-page zoom and 200% text-only
      zoom. All 20 responsive-control states passed; full-page zoom reflowed to
      320×576 CSS px, while text-only zoom kept 640×1151 CSS px and doubled the
      heading from 22 px to 44 px. Required controls remained reachable without
      horizontal overflow or occlusion after scrolling. Captures are in
      `docs/ui-redesign/phase10-firefox-200zoom/`; run with
      `UXR_FIREFOX_ZOOM_MODE=browser ./scripts/check-firefox-zoom-docker.sh` and
      `UXR_FIREFOX_ZOOM_MODE=text-only ./scripts/check-firefox-zoom-docker.sh`.
      The bottom
      navigation hides while a field has focus on a touch phone
      (`runKeyboardNavigationWorkflow`).
      `runMacroEditorKeyboardViewportWorkflow` checks that the focused source
      field and Save/Cancel remain reachable in a reduced mobile viewport; an
      Android emulator check with Gboard open also passed. Chrome 133 on the
      Android 16 emulator showed browser chrome above the app and Gboard below;
      the focused source field and Save action stayed visible. See
      `docs/ui-redesign/phase9-android-keyboard-review/`.
      `interactive-widget=resizes-content` was tried and rejected: it would
      shrink the layout viewport and make a narrow phone with its keyboard open
      match the landscape-block media query. Android emulator and desktop
      Firefox checks do not qualify a physical phone's browser chrome.)

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

- [x] Startup decision-table tests still pass and visual tests cover every
      startup branch. (Package chooser and settings retry scenarios were added;
      decision tests and visual baselines pass.)
- [x] No setup/sign-in/recovery action is hidden below the fold.

## Phase 4 — Macros operating screen and editor

### UXR-040 — Redesign Macros as primary operating surface

- [x] Make selected package, USB state, macro count/list, Add macro, and Send
      entry points immediately understandable.
- [x] Redesign rows/cards for fast scanning, clear names, obvious Send, and
      discoverable edit/manage actions.
- [x] Keep common actions visible; do not hide essential features behind an
      unexplained overflow affordance. (Send and Edit are always visible.
      Reorder, Duplicate and Delete are in a "More actions" menu whose
      trigger has a name and tooltip; the menu is keyboard operable and
      disabled items stay listed. The menu trigger is an icon, not a text
      label, so it is the one place a product owner may prefer more.)
- [x] Make empty state actionable and concise.
- [x] Preserve long-list scrolling only for macro data; keep Add macro, package
      context, and active-send controls available.
- [x] Preserve source privacy constraints and verify no source leaks through
      logs, diagnostics, notifications, or completion feedback. (The picker's
      live status names only the inserted token; `check-v2-snapshot-send-policy.py`,
      `check-frontend-persisted-state.sh` and the console/storage prohibition
      tests pass.)

### UXR-041 — Redesign Quick Send states

- [x] Design ready, disabled/not-ready, starting, awaiting-confirmation,
      running, canceling, completed, rejected, timeout, and release-failure
      states.
- [x] Keep Cancel and required safety/release controls visible during sends.
- [x] Ensure polling produces stable visuals and restrained live announcements.
- [x] Verify cancellation/recovery behavior on navigation, reload, reconnect,
      and phone-landscape surface.

### UXR-042 — Redesign macro editor

- [x] Create focused editor layout with name, source, compact contextual help,
      validation locations, and clear save/cancel actions.
- [x] Keep Save/Done, Cancel/Back, and a clearly labeled key/directive insertion
      entry point visible at minimum viewport heights.
- [x] Preserve cursor-aware insertion for all named special keys, including Tab,
      navigation/function keys, and Ctrl/Alt/Shift/GUI modifiers.
- [x] Provide an explicit key-combo builder or equivalent controls for
      simultaneous-key groups; show the exact generated syntax before insertion.
- [x] Preserve escaped brace/bracket insertion, delay directives, and all
      supported macro language symbols without reducing them to unexplained
      manual typing.
- [x] Keep the full key palette usable on phones through a focused picker
      subpage when it cannot fit in the editor viewport.
- [x] Preserve dirty state, navigation guard, validation, CRUD, and macro-language
      behavior.
- [x] Clearly distinguish preview/validation from physical send.

**Phase 4 evidence** (2026-10-15): `MacrosPage.tsx`, `MacroRow.tsx`,
`MacroOverflowMenu.tsx`, `UsbNotice.tsx`, `MacroEditorPage.tsx`,
`MacroKeyPicker.tsx`, `macroInsertions.ts`, `m3/Menu.tsx`, `m3/Progress.tsx`;
unit tests `v2-macros-page-states`, `v2-macro-insertions`,
`v2-macro-editor-page`, `m3-menu`; Chrome: macro-editing workflow drives the
picker and combo builder, `visibleControls.mjs` covers Macros, the editor and
the picker at six viewports.

**Phase 4 exit gate**

- [x] Macro list and editor pass existing behavior tests plus responsive and
      accessibility checks.
- [x] Quick Send remains one clear action when USB is ready; safety states are
      never obscured.

## Phase 5 — Packages, snapshots, and repository files

### UXR-050 — Redesign package selection and management

- [x] Make the current package and selection state easy to scan.
- [x] Create focused management flows for add, rename, reorder, duplicate, and
      delete as supported.
- [x] Provide accessible move controls in addition to any drag interaction.
- [x] Preserve package preference and repository dirty-state rules.

### UXR-051 — Redesign snapshots

- [x] Create a clear snapshot list and focused save/load/delete flows.
- [x] Show useful metadata without exposing repository contents or sensitive
      macro source.
- [x] Keep retention target advisory and deletion manual.
- [x] Add distinct empty, saving, loading, invalid, full-storage, and failure
      states. (Storage-full now has actionable recovery guidance and a regression
      assertion in `v2-snapshots-page-replace`.)
- [x] Protect dirty work before replacement/load/delete as specified.

### UXR-052 — Redesign import/export

- [x] Separate routine snapshot management from file import/export when that
      reduces cognitive load.
- [x] Design file selection, validation, progress, replacement confirmation,
      success, and failure states. (Import completion shows an explicit success
      snackbar.)
- [x] Preserve gzip/schema/size rules and avoid browser-persistent repository
      storage.

**Phase 5 evidence** (2026-10-15): `PackageManagementPage.tsx`,
`SnapshotRow.tsx`, `SnapshotsPage.tsx` (list and files screens), `ConfirmDialog`,
`m3/Menu`, `routingV2.ts` (`snapshotsViewFromHash`); unit tests
`v2-package-management-page`, `v2-snapshots-page-*`, `v2-routing`; Chrome
workflows for package CRUD, snapshots, import/export and the six-viewport
visible-controls table (Packages, Snapshots, Import and export).

**Phase 5 exit gate**

- [x] Package, snapshot, and file workflows pass their existing contracts and
      tests; all consequential actions are clear and reversible where possible.

## Phase 6 — Settings, diagnostics, dialogs, and system feedback

### UXR-060 — Redesign settings information architecture

- [x] Group settings by user goal and create subpages when a group cannot fit in
      the first viewport.
- [x] Cover identity, Wi-Fi/access point, password/security, USB identity and
      confirmation, reset/reconnect, and diagnostics as available.
- [x] Put reset/destructive functions in a distinct area with exact impact and
      confirmation.
- [x] Preserve settings validation, reconnect, password-change, and security
      behavior.

### UXR-061 — Redesign diagnostics

- [x] Keep diagnostics secondary and technical, with concise explanation of
      export contents and privacy.
- [x] Make export/download action visible without scrolling through output.
- [x] Ensure sensitive data and macro source remain excluded.

### UXR-062 — Unify dialogs, menus, errors, and confirmation

- [x] Implement M3 patterns for dialogs, menus, snackbars, inline messages,
      progress, and recovery surfaces.
- [x] State consequence and object in destructive confirmations.
- [x] Ensure error copy says what happened and the next useful action.
- [ ] Verify keyboard dismissal, focus trap/restore, screen reader announcement,
      and no accidental dismissal of critical unsaved-work prompts.
      (Automated unit/browser tests cover Escape dismissal, focus trapping and
      restoration, and unsaved-work cancellation; Android Chrome's accessibility
      tree exposes names for the main macro controls. Actual spoken
      screen-reader announcements still need a human TalkBack/VoiceOver pass.)

**Phase 6 exit gate**

- [x] All settings, diagnostics, menu, dialog, and error routes have responsive
      and accessible coverage.

## Phase 7 — Copy, accessibility, and visual consistency

### UXR-070 — Remove explanatory UI copy

- [x] Audit all user-facing copy screen by screen.
- [x] Remove text that repeats control labels, explains obvious navigation, or
      narrates implementation details.
- [x] Keep concise decision, consequence, privacy, validation, and recovery
      messages where needed.
- [x] Verify plain language and sentence case across all routes.

### UXR-071 — Accessibility qualification

- [x] Run axe-core scans on representative pages and all key overlays.
- [ ] Test keyboard-only operation, focus visibility/order, Escape behavior,
      screen-reader labels/status announcements, and touch target size.
      (Automated browser workflows and unit tests cover keyboard-operated menus,
      focus traps/restoration, Escape, axe checks, and 44 px touch targets.
      Manual screen-reader announcements and a full screen-by-screen keyboard
      pass remain open.)
- [x] Verify contrast for light/dark/system states if more than one theme exists.
- [x] Test 200% zoom/text enlargement, reflow at 320 CSS px, reduced motion, and
      virtual keyboard interactions. Firefox 153 was run in the pinned
      Playwright Docker image; Ctrl+Plus produced DPR 2 and a 320×576 CSS
      viewport from a 640×1236 window. All 20 route states in
      `workflows/visibleControls.mjs` passed title/control bounds, occlusion,
      and horizontal-overflow checks; the key picker also opened with Enter.
      See `scripts/check-firefox-zoom-docker.sh` and
      `docs/ui-redesign/phase10-firefox-200zoom/`. Chromium reduced-motion and
      Android Chrome/Gboard checks are covered by the earlier evidence above.

### UXR-072 — Visual regression and design review

- [x] Capture and review screenshots for phone 320×568 and 390×844, tablet
      portrait/landscape, and desktop 1440×900.
- [x] Add stable visual regression baselines for high-value screens and states.
- [x] Review alignment, hierarchy, whitespace, typography, color, shape,
      component consistency, and empty/error/loading states.
- [x] Remove styling that resembles the old keycap/engineering demo treatment.
- [x] Obtain product-owner visual review of representative completed screens
      before declaring the design phase complete. (The owner reviewed the phase
      7 and dark-mode screenshots and requested a palette/theme option and a
      more restrained type treatment; those changes were implemented and pushed
      for continued review. Final phase acceptance is still open.)

**Phase 7 exit gate**

- [x] No major accessibility defect, clipping, overflow, or visual inconsistency
      remains in reviewed viewports. Current evidence: all 97 browser visual
      captures match; the six-viewport visible-control workflow checks the
      route titles, essential controls, viewport fit, phone touch targets, and
      axe at the narrowest phone and widest desktop. Manual screen-reader and
      final product-owner review remain separate open gates below.
- [x] Screens communicate the task through layout and concise labels rather than
      long instructions.

## Phase 8 — Offline, performance, compatibility, and release qualification

### UXR-080 — Verify offline asset/runtime behavior

- [x] Inspect built assets and network requests; confirm no external runtime
      fonts, scripts, styles, images, analytics, or icon requests.
- [ ] Test the built app served from the device with internet unavailable.
- [x] Verify all app routes and error/recovery screens still render offline.
      With Android emulator Wi-Fi and mobile data disabled, all 46 visual
      scenario states rendered in Android Chrome through `adb reverse` to the
      local fixture serving `webapp/dist`, including setup, sign-in, empty and
      recovery states, send states, and authenticated routes. This verifies the
      route matrix without internet. Reproduce with
      `./scripts/check-android-offline-visuals.sh`; it restores the emulator's
      original Wi-Fi/mobile-data settings and ADB mappings on exit. This does
      not test ESP32 webfs serving.

### UXR-081 — Verify size and performance

- [x] Compare JS/CSS/font/icon asset sizes against the recorded baseline and
      repository release budgets.
- [x] Document the M3 implementation's raw and compressed cost.
- [ ] Confirm list/editor interactions remain responsive on target mobile
      browsers and constrained ESP32 serving conditions.
      (Android Chrome 133/Gboard was exercised manually and the reduced-viewport
      editor workflow is automated. The Android emulator rendered all 46
      scenario states offline; interaction timings under constrained ESP32
      serving and physical-phone performance remain unmeasured.)

### UXR-082 — Full regression and reconciliation

- [x] Run formatting, typecheck/build, webapp unit/component tests, real-browser
      interaction tests, axe tests, and visual regression checks.
- [x] Run `scripts/check-webapp.sh`, `scripts/build-webfs-image.sh`,
      `scripts/generate-flash-manifest.sh`, and the applicable repository gates.
- [x] Re-run relevant authentication, setup-route, persisted-state, route-sync,
      API contract, and macro privacy policy scripts.
- [x] Compare all UI behavior against `SPEC_V2.md` and the accepted redesign
      specification; resolve any conflict without weakening product safety.
- [x] Update webapp documentation and this TODO with completed tasks and exact
      evidence.
- [x] Record the implementation commit SHA, test counts, screenshot/baseline references,
      bundle sizes, offline results, and known limitations.

**Final exit gate**

- [ ] Every dependency-satisfied task is complete and evidenced. (Open items
      require product-owner review and physical/manual qualification listed
      below.)
- [x] All major workflows pass on phone and desktop, with tablet coverage.
- [ ] App is visually coherent with Material 3 Expressive and feels polished.
      (Representative screenshots await product-owner acceptance.)
- [x] No primary action or essential state is hidden below the fold.
- [ ] Offline operation, privacy, firmware asset budgets, and existing product
      contracts remain intact.
- [ ] Product owner has reviewed the completed experience.

## 2026-10-16 implementation and verification record

- Starting repository HEAD: `c331aad3dc9b8d582a1864bdb9eb0f56b5269890`. The
  working tree already contained user changes; no commit was created.
- Webapp unit and coverage runs passed: 66 files and 699 tests. Prettier,
  typecheck, ESLint, stylelint, build, no-orphan-classes, badge shapes, and
  `verify-no-remote-assets.sh webapp/dist` passed.
- Chrome browser workflows, axe scans, reduced-motion checks, H4 and H5 passed.
  All 97 visual scenario/viewport captures match baselines. The combined
  `check-all.sh` initially hit a transient browser crash and later a malformed
  tracker code span; both were resolved, and the final complete run passed.
- ESP-IDF firmware builds, release policy checks, and native coverage passed.
  `./scripts/run-tests.sh`: 63/63 host tests passed. `check-docs.sh` passed with
  existing yamllint warnings in workflow files. `check-scripts.sh` passed after
  formatting two host stub headers caught by its format regression self-test.
- The repo-wide run generated webfs successfully: 520,192 / 891,289 bytes used
  (58.4%, 371,097 bytes free); OTA app 1,006,896 / 2,097,152 bytes (48.0%).
- Built webapp assets: JS 339,490 raw / 101,680 gzip; CSS 38,760 raw / 7,620
  gzip. Baseline was JS 313,486 / 93,241 and CSS 27,946 / 6,193; total gzip
  increased 9,866 bytes. No external runtime assets were found.
- Review screenshots: `docs/ui-redesign/phase7-review/` (72 PNGs; 12 scenarios
  across six viewports). Visual baselines: `webapp/tests/browser/visual/baselines/`.
  Inspected phone, tablet, and desktop screens had no obvious clipping.
- Still open: final product-owner acceptance; physical device/offline route
  testing (no ESP32 board attached); Apple Safari; screen-reader announcement
  review; and manual keyboard-only review. Android virtual-keyboard behavior is
  further qualified in the continuation below.

## 2026-10-16 continuation — theme, keyboard, and final gate

- Base revision: `bc3eb5d2626cb331e432772daacc4693a6a28b7c` on
  `codex/ui-ux-redesign-2026-10-16`.
- UXR-011 candidate evaluation is complete in
  `UXR_011_M3_IMPLEMENTATION_APPROACH_2026-10-15.md`: size, license, runtime,
  browser matrix, firmware budget, and published accessibility evidence are
  compared. The selected in-repo layer passes automated axe/focus/contrast
  checks; manual assistive-technology verification remains an application
  qualification item.
- Added an all-route phone touch-target audit to
  `tests/browser/workflows/visibleControls.mjs`; every visible target on each
  listed phone route must meet 44×44 CSS pixels. The keyboard-open macro-editor
  viewport test also asserts the source field and Save/Cancel stay in-view.
- Android emulator: Chrome 133 on Android 16, 390 CSS px wide, Gboard open.
  The source field and Save action remained visible; the bottom navigation
  stepped aside. Screenshot:
  `docs/ui-redesign/phase9-android-keyboard-review/`.
- Offline smoke check: Android emulator Wi-Fi and mobile data were disabled;
  authenticated Macros, Packages, Snapshots, Import/export, Settings, and
  Diagnostics routes rendered through a local fixture with no external
  requests. This did not exercise ESP32 webfs serving or device-error recovery.
- Browser coverage: Android Chrome 133 emulator; Firefox 153 responsive-control
  workflow at phone and desktop sizes, including touch-target and axe checks;
  Chromium workflows and axe; and Firefox 153 at actual 200% browser zoom / 320
  CSS px across all 20 responsive-control route states. Playwright WebKit 26.5
  in the same pinned Docker image passed responsive controls, touch-target/axe,
  startup, keyboard viewport, and macro-editing/package workflows. Apple Safari
  itself remains unqualified.
- `./scripts/check-all.sh` (ESP-IDF v5.5.5 / Node 24.18.0): passed, exit 0.
  This includes 706 webapp tests across 67 files (unit and coverage runs),
  Chromium workflows including H4/H5, 97 visual baselines, no-orphan and
  status-badge checks, docs and scripts checks, firmware and device-test-app
  builds, and 63/63 host tests. Existing yamllint warnings remain in workflow
  files; docs lint reported zero Markdown issues.
- Final gate measurements: JS 340,970 bytes raw (Vite: 340.97 kB / 102.12 kB
  gzip); CSS 38,968 bytes raw (Vite: 38.97 kB / 7.68 kB gzip); webfs budget
  520,192 / 891,289 bytes (58.4%, 371,097 free); OTA app 1,006,896 /
  2,097,152 bytes (48.0%). The built distribution contains no remote assets.
- Review captures: `phase7-review/` (72 light screenshots),
  `phase8-dark-review/` (14 dark screenshots), and the Android keyboard capture
  in `phase9-android-keyboard-review/`. The unit/browser suite and all 97 visual
  baselines pass.
- Still needs owner assistance or a physical target: final visual acceptance
  of the current palette/type/screens; a real screen-reader pass (TalkBack or
  VoiceOver) and full manual keyboard-only route pass; Apple Safari
  qualification; and ESP32-served offline/recovery and constrained-serving
  performance checks. No ESP32 board is connected in this environment. The
  TODO remains open for these gates.

## 2026-10-16 evidence revision

- App code revision: `dc9a87a6e08cbf536bf636d49b23c557011e091b` on
  `codex/ui-ux-redesign-2026-10-16`, pushed to GitHub. Full repository command
  at this app revision: source ESP-IDF v5.5.5, then
  `./scripts/check-all.sh`; **passed, exit 0**. Counts: 706 webapp tests across
  67 files (unit and coverage runs), 97 visual scenarios, and 63/63 host tests.
  Firmware and the on-device test app built. See the continuation above for
  bundle and webfs measurements.
- Additional browser command: launch Firefox with Playwright and call
  `runVisibleControlsWorkflow` from
  `webapp/tests/browser/workflows/visibleControls.mjs`; **passed** at phone and
  desktop sizes, including the 44×44 CSS-pixel visible-control audit and axe.
- `./scripts/check-firefox-zoom-docker.sh`: **passed** in
  `mcr.microsoft.com/playwright:v1.62.1-noble`; Firefox 153 reached DPR 2 via
  Ctrl+Plus, producing a 320×576 CSS viewport from a 640×1236 window. All 20
  responsive-control states passed, including title/control bounds, occlusion,
  overflow, and Enter-opening the key picker. Captures are in
  `docs/ui-redesign/phase10-firefox-200zoom/`.
- `UXR_FIREFOX_ZOOM_MODE=text-only ./scripts/check-firefox-zoom-docker.sh`:
  **passed** all 20 responsive-control states with Firefox full-page zoom
  disabled; text enlarged 200% while viewport width and DPR stayed fixed.
- `./scripts/check-android-offline-visuals.sh`: **passed**, all 46 visual
  scenarios in Android Chrome with emulator Wi-Fi and mobile data disabled.
- `docker run --rm -v "$PWD:/repo:ro" -w /repo/webapp
  mcr.microsoft.com/playwright:v1.62.1-noble node
  tests/browser/run-webkit-smoke.mjs`: **passed** with WebKit 26.5 across
  responsive controls, touch-target/axe scans, keyboard viewport, startup, and
  macro-editing/package workflows. This Linux WebKit run does not qualify Apple
  Safari.
- Android 16 emulator tall-cutout overlay (48 dp top cutout), Chrome 133: the
  browser viewport remained below the cutout and page content did not overflow
  horizontally. Chrome returned zero for all four `env(safe-area-inset-*)`
  values in its normal browser viewport. This does not qualify edge-to-edge or
  standalone display modes; the overlay was restored to disabled afterward.
- Added Docker browser runners and the two zoom captures in code/test commit
  `90d4be51f1323029d350f030eb9cfd7e5d37d2df` on the same branch. The pinned
  Docker image is version-matched to the webapp's Playwright 1.62.1 dependency.
- At that commit, webapp Prettier and ESLint passed; `check-format.sh`,
  `check-scripts.sh` (with ESP-IDF v5.5.5 sourced and `$(go env GOPATH)/bin` on
  `PATH`), and the Firefox/WebKit Docker workflows passed. The new browser
  runner files are linted and formatted; the production application is
  unchanged from the full-gate revision above.
- `bash scripts/check-docs.sh`: **passed**, zero Markdown issues; existing
  yamllint warnings remain. `git diff --check`: **passed** before this final
  documentation reconciliation; it will be rerun before commit.
- Screenshots: `docs/ui-redesign/phase7-review/` (72 light captures),
  `docs/ui-redesign/phase8-dark-review/` (14 dark captures), and
  `docs/ui-redesign/phase9-android-keyboard-review/` (Android 16 / Chrome 133 /
  Gboard, 390 CSS px wide), plus
  `docs/ui-redesign/phase10-firefox-200zoom/` (Macros and macro editor at
  200% Firefox full-page zoom / 320 CSS px and 200% text-only enlargement).
- Remaining exit gates are external/manual: owner acceptance of the final
  screenshots; spoken screen-reader and full manual keyboard review; Apple
  Safari; and physical ESP32 offline/recovery/performance checks. These are not
  inferred from browser automation or build results.

## 2026-10-16 full regression after scenario refactor

- Code revision: `03db342` (`test(webapp): split visual scenarios by
  workflow`), pushed to `master`. `webapp/tests/browser/visual/scenarios.mjs`
  is now a 17-line ordered aggregator; its workflow modules are each under
  800 lines.
- Full repository command: source ESP-IDF v5.5.5, add `$HOME/go/bin` to `PATH`,
  then run `./scripts/check-all.sh`; **passed, exit 0**. Webapp format,
  typecheck, ESLint, stylelint, unit and coverage runs passed (706 tests across
  67 files); Chrome browser and H4/H5 workflows passed; all 97 visual captures
  matched; script and documentation checks passed; firmware and device-test-app
  builds passed; host suite passed 63/63. `git diff --check` passed.
- Release budget check reported one expected skip: runtime task stack margin
  requires a stack report and cannot be measured from this build-only run. No
  physical ESP32 was attached; `adb devices -l` showed only Android emulator
  `emulator-5554`. The build, emulator, and browser checks do not close the
  device-served offline, constrained-device performance, or spoken
  screen-reader gates.
- Remaining assistance: product-owner acceptance of the current palette,
  typography, and representative screens; a human TalkBack/VoiceOver spoken
  announcement review and complete keyboard-only route review; Apple Safari;
  and an ESP32 target for device-served offline/recovery and constrained-serving
  performance checks. Keep UXR-062, UXR-071, the device-specific UXR-080/081
  items, and final acceptance open until that evidence is available.

## 2026-10-16 non-manual verification completion

- Re-ran `./scripts/check-all.sh` after adding reproducible Android offline
  route coverage and Firefox text-only zoom support; **passed, exit 0**.
  Formatting, static policy, firmware and device-test-app builds, webapp tests
  and coverage (706 tests across 67 files), Chrome workflows, all 97 visual
  baselines, scripts, docs, and 63/63 host tests passed. Release-budget check
  skipped only the runtime stack-margin measurement, which requires a live
  stack report.
- Firefox 153 passed all 20 responsive-control states at both full-page 200%
  zoom and text-only 200% enlargement. Android Chrome 133 passed all 46 visual
  route scenarios with Wi-Fi and mobile data disabled. Runners:
  `scripts/check-firefox-zoom-docker.sh` and
  `scripts/check-android-offline-visuals.sh`.
- The remaining unchecked work is human review or needs an ESP32/Apple device:
  TalkBack/VoiceOver announcements, full keyboard-only review, product-owner
  design acceptance, Apple Safari, ESP32-served offline behavior, and
  constrained-device performance. The firmware build does not close those
  hardware checks.
