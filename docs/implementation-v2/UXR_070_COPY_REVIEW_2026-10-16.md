# UXR-070 — User-facing copy review

**Date:** 2026-10-16
**Scope:** screen-by-screen review of the Material 3 v2 app. Repository and
macro-language text entered by the user is data, not interface copy.

| Screen | Copy kept because it helps a decision or task | Review result |
| --- | --- | --- |
| Setup and sign-in | Field labels, validation, setup consequences, authentication failures and retry guidance | Plain language; labels are not repeated in helper text |
| Startup and recovery | What is unavailable, what a selected recovery action replaces, and what Retry will reattempt | Consequences remain attached to their action |
| Macros and Quick Send | Package name/count, Send/Edit labels, readiness, execution state and cancellation/release guidance | No navigation tutorial; source and macro names remain user data |
| Editor and key picker | Contextual syntax, generated-key preview, validation location and save/cancel actions | Technical wording is limited to macro syntax the user edits |
| Packages | Name constraints, selection state, and the named object/consequence in delete confirmation | No repeated explanation of opening the Packages destination |
| Snapshots and file transfer | Storage/retention state, replacement consequences, local unsaved-work state, import validation and recovery | Explicitly says imported work needs a snapshot to persist |
| Settings | Goal-based group names, field validation, reconnect consequences, fixed USB identity, and exact reset impact | Destructive effects remain beside their controls |
| Diagnostics | What export contains and excludes; technical field names beside their values | Privacy explanation is adjacent to Copy/Download actions |
| Shell, navigation, dialogs and transient feedback | Destination/action names, current status, object-specific confirmation and success/error result | Icons remain decorative where visible labels identify the action |

The scan found no remaining paragraph that merely explains obvious navigation
or repeats a control label. Concise decision, validation, consequence, privacy,
and recovery copy remains. API error codes are no longer displayed; network,
timeout, full-storage, and committed partial-failure cases provide user-facing
recovery guidance in `webapp/src/features/auth/v2/v2ErrorText.ts`. The complete frontend
format, type, lint, unit, browser/axe, and visual checks are the reproducible
copy/rendering evidence for this review.
