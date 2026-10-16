# ESP32 Macro Keyboard — Material 3 Expressive UI/UX Redesign Specification

**Status:** Product-owner review draft
**Scope:** Web application visual system, information architecture, responsive layouts, and user-facing interaction design
**Basis:** Product-owner direction given 2026-10-15; repository snapshot `esp32-macro-keyboard-master-2610152104.zip`

## 0. Purpose and authority

This document defines a proposed redesign of the ESP32 Macro Keyboard web application. It is intended to replace the current visual treatment and to refine the screen organization where that improves task clarity. It does not authorize changes to firmware behavior, API contracts, repository format, authentication policy, macro language, persistence semantics, or USB safety rules.

The existing `docs/SPEC_V2.md` remains authoritative for product and technical behavior. The existing `docs/UI_UX_SPEC_V2.md` describes the shipped v2 workflow and remains the behavioral baseline until the product owner accepts this redesign. On acceptance, this document supersedes the older UI specification only for presentation, navigation, screen composition, and interaction details explicitly changed here. Any unresolved behavior stays governed by the existing v2 specifications.

This is a design specification, not implementation evidence. The TODO companion defines implementation and qualification work. Neither document claims that the redesign is already implemented or approved for release.

## 1. Product direction

The web application is a consumer-facing control surface for configuring and using a physical macro keyboard. It must feel finished, calm, confident, and approachable. It must not resemble an engineering console, diagnostic dashboard, or developer demo.

The design system is Google Material 3 Expressive (M3 Expressive), adapted to this device's focused utility. Use expressive color, shape, typography, and motion with restraint: hierarchy and task clarity always come first. Adopt Material roles and components consistently instead of copying a few decorative details.

The application must work in a desktop browser and on a cellphone, including narrow portrait viewports and safe-area displays. It is served by the ESP32 and must continue to work without an internet connection.

## 2. Design principles

1. **Make the next action obvious.** Each screen has one clear title, one primary action where appropriate, and a visible path to secondary tasks.
2. **Teach through the interface.** Prefer familiar labels, icons with accessible names, examples in the control itself, previews, and immediate feedback. Avoid paragraphs that explain what a screen or obvious control is for.
3. **No scroll-to-discover controls.** A user must not need to scroll to discover a page's primary action, a task's next step, essential status, or a feature entry point. Keep screen-level controls visible in the initial viewport or move the workflow to a dedicated screen/subscreen. Do not hide important functions in long pages.
4. **Keep content navigation predictable.** Scroll may be used for genuinely variable content such as a long macro list, long editor content, or diagnostic output. Repeated content must make its actions obvious, and global/task-level controls remain visible. Avoid nested scroll containers and clipped actions.
5. **Show only what is useful now.** Put routine controls in the main task surface. Put infrequent, related actions in a clearly signaled menu or focused subpage. Do not expose technical detail until it helps a decision or recovery.
6. **Use short, plain language.** Labels name the object or action; status text says what happened and what to do next. Avoid jargon, redundant headings, instructional essays, and unexplained abbreviations.
7. **Preserve user work.** Unsaved repository edits remain visible and protected through navigation, reload, sign-out, network loss, and recoverable errors, as required by the v2 product specification.
8. **Make device, repository, and send state distinct.** A person can tell whether the device is ready, their edits are saved, and a macro is running without interpreting color alone.
9. **Design for touch and pointer.** Mobile and desktop share the same information architecture and capabilities, with layouts and navigation adapted to available space.
10. **Accessibility is part of the design.** Keyboard operation, screen reader names, contrast, focus, text resizing, reduced motion, and adequate touch targets are release requirements.

## 3. Material 3 Expressive design system

### 3.1 Foundations

Define a small, documented token set for:

- M3 semantic color roles, including surface, on-surface, primary, secondary, tertiary, error, outline, and their container/on-container roles;
- typography roles for display, headline, title, body, and label, using a locally available/system font stack unless a bundled font is proven to fit the firmware web-asset budget;
- shape scale and expressive shapes, with consistent geometry for cards, menus, dialogs, fields, and buttons;
- elevation/surface hierarchy, spacing, component density, focus, disabled, pressed, selected, error, and success states;
- motion durations/easing and reduced-motion behavior.

Use expressive accents to create product personality, not decoration on every surface. Maintain strong contrast and keep state communication semantic. Avoid the current keycap-like pressed-button metaphor, faux hardware legends, heavy outlines, and all-caps text as the dominant visual style.

### 3.2 Components and implementation

Use M3 Expressive component patterns and interaction behavior for app bars/toolbars, navigation, buttons, icon buttons, lists, cards, text fields, selection controls, menus, dialogs, snackbars/status messages, and progress indicators. The implementation may use the existing React/TypeScript/Tailwind stack with a coherent M3 token/component layer, or a compatible locally bundled component implementation selected during the TODO. The selection must be documented and validated against React 19, accessibility, offline operation, firmware bundle budgets, and supported browsers before adoption.

Do not load fonts, scripts, icons, stylesheets, or images from a network CDN at runtime. Icons must be locally bundled/vector or use a consistent text-safe icon set; every icon-only control needs an accessible name and a visible or discoverable tooltip/label appropriate to its context.

### 3.3 Color and theme

Use a cohesive, modern palette with a recognizable primary color and restrained supporting colors. If dynamic color is offered, it must be optional, deterministic offline, contrast-checked, and must not cause device status, saved state, errors, or active-send state to lose their semantic meaning. A fixed branded palette is acceptable where device/browser dynamic color is not reliable.

Do not use color as the only state indicator. Pair status color with concise text, iconography, shape, or progress treatment.

### 3.4 Motion and expressive styling

Use short, purposeful transitions for navigation, selection, expansion, send progress, and confirmation. Motion must clarify state changes and never delay a task. Honor `prefers-reduced-motion`; all essential state changes remain understandable without animation. Avoid spring/bounce effects on safety-critical send, cancel, release, authentication, destructive, and error flows.

## 4. Layout and navigation model

### 4.1 Responsive behavior

- Minimum supported viewport width remains 320 CSS pixels unless existing supported-browser evidence requires a higher value.
- Use phone-first single-column screens. Use tablet/desktop space for clearer grouping, wider content, and side navigation or split panes where that genuinely improves a workflow.
- Do not merely stretch phone cards across a desktop viewport. Set readable maximum widths and align related content.
- Keep primary navigation, device readiness, save/dirty state, and active-send safety visible or immediately reachable without scrolling.
- Respect safe-area insets, browser chrome, keyboard appearance, zoom, and text resizing.
- Do not block desktop/tablet landscape. Preserve the existing portrait-required phone behavior unless the product owner later approves changing it. Active-send progress and cancellation must remain available on every orientation surface.

### 4.2 Screen composition

Every screen defines: purpose, title, visible primary action, visible task state, available next step, and what content may scroll. Screen-level controls that are needed to complete the current task must fit without scrolling at the supported minimum viewport. If they cannot, split the task into screens or progressive steps.

Avoid nested scroll areas. A single content region may scroll for long variable data, while the app bar/navigation or task action stays available. For forms, use focused sections or subpages rather than stacking every setting into one long page. Do not use sticky/fixed controls that cover content or keyboard focus.

### 4.3 Navigation

Keep the product's core destinations easy to identify: **Macros**, **Packages**, **Snapshots**, and **Settings**. On phones, use M3 navigation-bar patterns where space permits. On larger screens, use a navigation rail or drawer when it materially improves orientation and leaves more room for the task. Preserve route semantics and active-route indication.

Use a clear back affordance and page title on focused task screens. Do not make browser Back the only way to exit a workflow. Confirm only when the user could lose work or trigger a consequential action.

### 4.4 Persistent shell

The authenticated shell communicates device identity/readiness, selected package when relevant, repository saved/unsaved state, and active send state. Keep the shell compact. Do not render a row of redundant badges or repeat the same status in the page body. Dirty-state save action remains easy to find on every authenticated operational route. USB status is concise and understandable, with details available when needed.

## 5. Core user workflows

Functional rules below preserve the behavior already specified in `docs/SPEC_V2.md` and `docs/UI_UX_SPEC_V2.md`. This document changes how those workflows are presented, not their security or persistence semantics.

### 5.1 Startup, setup, and sign-in

- Keep first-run setup focused and divided into a small number of understandable steps. Group fields by the decision being made. Show password requirements at the field and reveal/copy controls only when useful.
- Use a clear review/submit step for device changes that restart or disconnect the device. Explain the consequence in one concise sentence immediately before the action.
- Sign-in has one clear password field, submit action, and inline recovery/error state. Do not surround it with paragraphs about the authentication implementation.
- After sign-in, show concise progress while the repository loads. Make recovery options visible when the newest snapshot is absent or invalid; never silently load a different snapshot against existing rules.

### 5.2 Macros (primary operating screen)

- The Macros destination is the default operating screen after a package is resolved.
- Make the selected package, USB readiness, macro list, Add macro, and Quick Send purpose immediately clear.
- Each macro entry presents its name and a prominent Send action. Edit and secondary actions remain identifiable and touch-accessible. Do not require opening an unexplained overflow menu to discover common operations.
- Macro source remains visible/editable only where the existing product behavior requires it. Protect source from logs, diagnostics, telemetry, notifications, and send acknowledgements. Avoid showing long source text in a list if it harms scanability; provide a clear preview/edit path while preserving any approved privacy/visibility rules.
- Disable Send when USB is not ready and explain the state next to the control with concise actionable text.
- Send progress, cancellation, completion, failure, timeout, and release errors are shown in context. During active send, Cancel and required release-all controls remain visible and reachable.
- Empty state uses a short prompt and an obvious Add macro action.
- Long macro collections may scroll as data; Add macro, package context, active-send safety, and essential state must not be discoverable only below the fold.

### 5.3 Macro editor

- Use a focused edit screen with a clear name field and macro source editor, compact contextual syntax/help affordance, and validation tied to the relevant field/line.
- Preserve the full macro authoring language and make its insertion tools discoverable from the editor: named special keys including Tab and function/navigation keys; Ctrl, Alt, Shift, and GUI modifiers; simultaneous key groups; escaped brace/bracket symbols; and delay directives.
- Provide an explicit, easy-to-find key-combo builder or equivalent controls that produce valid simultaneous-key syntax. Show the generated source form (for example, `[{CTRL}{SHIFT}t]`) before insertion. Modifier selection and special-key insertion must not be reduced to ordinary text entry.
- Keep Save/Done, Cancel/Back, and the entry point for key/directive insertion visible without scrolling. The full palette may live on a focused picker subpage so it remains usable on a phone.
- Do not remove or silently alter parser-supported symbols, directives, key combinations, or cursor-aware insertion behavior. Keep syntax help concise and contextual.
- A preview or test action must be clearly distinct from sending a macro to the physical host computer.

### 5.4 Packages

- Package selection is a direct, scannable choice. Clearly mark the current package and make switching behavior predictable.
- Package creation, rename, deletion, and ordering are grouped as management tasks. Use concise confirmations for destructive actions and explain impact, not implementation details.
- Preserve the existing behavior that package selection is a device UI preference and does not itself dirty repository content.

### 5.5 Snapshots and repository

- Present snapshots as a clear list with useful metadata and distinct actions to load, save, export, or delete as permitted by existing behavior.
- Save snapshot remains an explicit action. Show unsaved changes as a persistent, concise state and avoid implying edits were saved automatically.
- Make retention guidance an advisory, not an automatic deletion mechanism. Preserve manual creation/deletion and recovery semantics.
- Separate import/export from routine snapshot operations when combining them makes the screen confusing. Show file selection, validation, replacement consequences, and progress as focused steps.
- Protect dirty work before loading/replacing/deleting where the existing spec requires it. A failure retains the working copy whenever possible.

### 5.6 Settings and diagnostics

- Settings is organized into named task groups with short labels and clear entry points: device identity, network/access point, password/security, USB identity/confirmation, reset/reconnect, and diagnostics, matching available product functions.
- Put each setting group on its own page or subpage when the complete group cannot fit in the initial viewport. Avoid a single long settings page.
- Separate ordinary settings from high-impact actions. Show a concise consequence and require confirmation for destructive/reset operations.
- Diagnostics are secondary and technical by nature. Keep them out of the primary workflow; provide a clear export/share/download action and concise privacy note near export.

## 6. Status, errors, and confirmation

- Use inline status for immediate field or row results; snackbar/toast for transient confirmation that needs no follow-up; dialog for decisions that must be made before proceeding; dedicated recovery screen when a workflow cannot continue safely.
- Every error states what happened in plain language and provides the next action when one exists. Keep underlying technical diagnostics available without putting them in the main user message.
- Do not display raw server responses, stack traces, HTTP codes, or unexplained identifiers as primary user-facing copy.
- Authentication expiry, device disconnection, failed snapshot save/load, invalid import, USB not ready, send rejection, cancellation, timeout, and release failure all receive designed states.
- Destructive confirmation names the object and consequence. Avoid generic “Are you sure?” prompts.
- Keep progress feedback stable; polling must not cause repeated screen-reader announcements or visual jitter.

## 7. Content and copy rules

- Use concise action labels: verb + object where useful (for example, “Add macro”, “Save snapshot”, “Change package”).
- Use sentence case. Keep headings short and avoid acronym-heavy language.
- Remove paragraphs whose only purpose is to explain an obvious control, page, or navigation item.
- Keep help available at the point of need as a short tooltip, expandable “More” detail, example, or contextual link. Do not make essential instructions hidden behind help.
- Prefer visible examples and constraints in a field over separate instruction blocks. Use concrete error messages that say how to recover.
- Keep technical terms that are part of the user's task (“USB”, “macro”, “snapshot”) consistent across the app.

## 8. Accessibility and input

- All controls work with keyboard, touch, and pointer. Focus order follows the visual task sequence; focus is clearly visible and not covered by fixed chrome.
- Meet WCAG 2.2 AA contrast for text and interactive components. Status is never color-only.
- Use semantic HTML, associated labels, accessible names, and correct roles/states. Icon-only actions have accessible labels; status updates use live regions judiciously.
- Touch targets are at least 44 by 44 CSS pixels where practical and never overlap.
- Support browser zoom and text enlargement without clipping controls or losing actions. Use responsive reflow rather than horizontal scrolling for ordinary workflows.
- Dialogs trap focus, close appropriately, and restore focus. Menus support keyboard navigation and Escape.
- Reduced-motion preferences disable nonessential transitions. No essential information is conveyed only by animation.

## 9. Performance, offline operation, and privacy

- The app and all UI assets work offline from the device. No runtime CDN or external network dependency is introduced.
- Keep bundled web assets within the repository's existing release budgets. Measure before and after; document any new dependency and its compiled/gzipped cost.
- Preserve the prohibition on browser persistent storage for repository data.
- Never send macro source or credentials into logs, analytics, diagnostic exports, error banners, accessibility announcements, or notifications unless an existing approved flow explicitly requires the content and protects it.
- Do not add analytics or third-party tracking.

## 10. Screen/viewport acceptance rules

1. At 320×568 CSS px and common phone dimensions, every screen's title, primary action, task state, and next-step control are visible without scrolling; if not, redesign/split that workflow.
2. No major feature or primary control is discoverable only by scrolling through explanatory content.
3. Long variable data can scroll, but screen-level actions and active-send safety remain visible or immediately reachable.
4. No horizontal overflow, clipped control, obscured focus ring, hidden submit action, or nested-scroll trap at supported sizes.
5. At tablet and desktop widths, layouts use space deliberately and all functions remain available.
6. Browser resize, orientation changes, safe areas, and virtual keyboard appearance do not hide the focused field's action or active-send cancellation.
7. Current phone-landscape policy and desktop/tablet landscape behavior remain correct unless explicitly revised in a separate product decision.

## 11. Acceptance criteria

- The experience visibly follows one coherent Material 3 Expressive system across setup, sign-in, macros, editor, packages, snapshots, settings, diagnostics, dialogs, and error states.
- Visual review describes it as a polished consumer product, not a developer/engineering demo; the existing keycap/petrol visual metaphor is removed.
- Core workflows are understandable from concise labels and visible affordances; explanatory copy is limited to consequential decisions, recovery, and genuinely non-obvious concepts.
- Primary actions, essential state, and task completion controls are visible in the initial viewport or the workflow is split into a focused subpage.
- Desktop and cellphone workflows offer the same capabilities, with appropriate responsive navigation and layout.
- Existing repository, authentication, persistence, device settings, macro execution, USB, recovery, and privacy behavior remains compliant with `SPEC_V2.md` unless separately approved.
- All screen states have keyboard, screen reader, contrast, touch, zoom, and reduced-motion coverage.
- Existing offline operation and firmware web-asset budgets are preserved.
- Automated responsive, interaction, accessibility, and visual regression tests pass; representative real-browser screenshots are reviewed at phone, tablet, and desktop widths.

## 12. Reference material

- Material Design 3 / Expressive overview: <https://m3.material.io/get-started>
- Material design foundations and tokens: <https://m3.material.io/foundations/>
- Material typography: <https://m3.material.io/styles/typography/applying-type>
- Material color roles: <https://m3.material.io/styles/color/the-color-system>
- Existing product behavior: `docs/SPEC_V2.md`, `docs/UI_UX_SPEC_V2.md`
