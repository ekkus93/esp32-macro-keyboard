# ESP32 Macro Keyboard — UI/UX Redesign Review Fixes Specification

**Status:** Proposed follow-up specification requested by the product owner;
implementation and acceptance remain open
**Review baseline:** `82eb7d1` on 2026-10-17
**Companion plan:** `docs/UI_UX_REDESIGN_REVIEW_FIXES_TODO_2026-10-17.md`
**Scope:** Webapp correctness and accessibility defects found while auditing
`docs/UI_UX_REDESIGN_TODO_2026-10-15.md`

## 1. Authority and boundaries

This document specifies corrective outcomes for the findings below. It does not
change the frozen `docs/SPEC_V2.md` or `docs/UI_UX_SPEC_V2.md`, the HTTP API, the
repository format, firmware behavior, or the accepted visual design. If an
implementation cannot satisfy an outcome without changing a frozen requirement,
record the conflict and obtain explicit per-change product-owner approval before
editing that requirement. Do not treat this proposed follow-up as evidence that
the fixes have shipped.

The requirements below preserve in-memory repository ownership, explicit
snapshot persistence, source privacy, bounded send polling, and the existing
physical release-all behavior. Browser tests cannot establish spoken
screen-reader or physical ESP32-S3 behavior.

## 2. Findings and required outcomes

### UXRF-001 — A snapshot response can overwrite edits made during upload

`webapp/src/v2/snapshotClient.ts` captures the repository before asynchronous
encoding, listing, and upload, then calls `store.markSaved(validated.value)`
after a successful `201`. `webapp/src/v2/repositoryWorkingCopy.ts` implements
`markSaved` by replacing both baseline and live working copy. Editing and route
navigation remain available while the header disables only its Save button.

**Required behavior:** A snapshot represents the repository version captured
for that upload. On confirmed success, that version may become the saved
baseline only if the same working-copy lineage is still active. Any edits made
after capture remain in the live working copy and remain marked unsaved. If the
working copy was deliberately replaced or discarded while the request was in
flight, the late response must not replace that new working copy or its
baseline. A late response must not set the current snapshot identity to an
unrelated or superseded repository. Failed or commit-uncertain uploads must
never clear dirty state or discard edits.

The UI must prevent overlapping snapshot commits for one working copy. It may
allow editing during upload if it implements the version/lineage behavior
above; otherwise it must block every content-changing and replacement path
consistently until the result is reconciled. No browser-persistent repository
storage may be added.

**Acceptance cases:**

1. Save version A; edit to B before `201`; after success, the live copy is B,
   dirty is true, and the saved baseline is A.
2. Save A; replace or discard the working copy before the response; the late
   response does not undo the replacement or its clean/dirty status.
3. Save A; receive a failure or uncertain result after editing to B; B remains
   available and dirty, including through the existing reconciliation flow.
4. Repeated Save actions while one commit is pending produce at most one upload.
5. The visible saved/unsaved state and snapshot identity describe the live
   working copy accurately after each case.

This closes the gap in UXR-042, UXR-051, and UXR-082. Relevant existing tests
are `webapp/tests/v2-repository-working-copy.test.ts` and
`webapp/tests/v2-snapshot-client.test.ts`.

### UXRF-002 — Query-only navigation can drop focus to the document body

`AppShellV2.tsx` focuses main content only when its top-level `route` prop
changes. `AppV2.tsx` rerenders for hash query changes, but Settings groups and
sections and Snapshots subviews retain the same top-level route. The focused
navigation control can unmount without a new focus target.

**Required behavior:** Every screen/subpage navigation, including browser Back
and Forward, must leave keyboard focus at a predictable visible target in the
new view. Direct entry must not steal focus during initial render. Focus should
move to the main region or the new screen heading after a user-initiated
subpage change, with `preventScroll` or equivalent behavior that preserves
the visible start of the page. Closing a transient dialog/menu should continue
to restore focus to its invoker where that invoker still exists. The active
navigation indication must remain correct.

**Acceptance cases:** Settings root → group → section → Back, Snapshots list
↔ files, and browser Back/Forward all leave focus in the new view rather than
on `BODY` or a removed element. Keyboard focus is visible and the screen's
heading can be reached in reading order. Initial page load does not move focus
unexpectedly. Test both narrow and wide layouts.

This closes the gap in UXR-020 and contributes to UXR-071. The current focus
effect is in `webapp/src/features/shell/v2/AppShellV2.tsx`.

### UXRF-003 — Cancel is missing during the initial send request

`MacrosPage.tsx` renders a starting status without Cancel and publishes a
cross-route summary with `onCancel: null`. The phone-landscape surface therefore
also hides the safety control during this interval. A delayed POST response can
leave the user with no visible way to request cancellation even if the device
has already accepted the send.

**Required behavior:** From the first Send press until a terminal result, the
Macros page and any active-send surface must show a clear Cancel and release
all keys action. A Cancel press while POST is pending records a cancellation
intent and immediately acknowledges that intent. After a matching `202`
acceptance, request `DELETE /api/v1/send` once and continue tracking until the
device reports a terminal state and release outcome. Repeated presses must not
start duplicate DELETE requests.

The send API has no client request ID. A premature DELETE could target an
unrelated send or return `404` before the POST is accepted. Therefore a Cancel
press before acceptance must not issue an unqualified DELETE. If POST fails,
report that no send was confirmed. If the POST result is uncertain or returns
`409`, use the existing GET recovery path; do not silently cancel a recovered
send whose identity cannot be tied to the pending request. Keep the recovered
send's Cancel action visible so the user can explicitly cancel it. A pending
cancel must never start a second POST.

**Acceptance cases:** Delayed-success POST followed by early Cancel causes one
POST, one subsequent DELETE, and a visible cancellation state on Macros, another
route, and phone landscape. Delayed-failure POST does not issue DELETE. A `409`
or uncertain response recovers status without automatically cancelling an
unidentified send. Reload/reconnect continues to use the existing recovery
rules. Terminal release failures remain visible separately.

This closes the gap in UXR-020 and UXR-041. Relevant code is
`webapp/src/features/macros/v2/MacrosPage.tsx`,
`webapp/src/features/shell/v2/LandscapeBlockSurface.tsx`, and
`webapp/src/v2/sendClient.ts`.

### UXRF-004 — Settings export failure is swallowed by the action caller

`SettingsPage.tsx` offers Export working copy from the unsaved-changes prompt.
Its export handler has no `catch`, and the caller ignores the returned promise.
The Snapshots file screen already shows an explicit export error.

**Required behavior:** Any encoding or file-save failure leaves the dirty
working copy and pending action intact, keeps the prompt usable, and shows a
concise, actionable error associated with the export attempt. The export busy
state must clear. A successful export must not itself resolve the dirty-work
prompt or authorize a destructive action. No macro source or credentials may
appear in the error text or logs.

**Acceptance cases:** Inject an `exportRepository` rejection and a
`saveAsFile` exception separately; both show a visible error, keep the prompt
open and dirty state unchanged, and allow retry or Cancel. Success downloads
the file but still requires an explicit Save or Discard decision.

This closes the gap in UXR-062 and the Settings destructive-action flow.
Relevant tests are `webapp/tests/v2-settings-page.test.tsx`.

### UXRF-005 — Verify modal screen-reader isolation

The shared `Dialog.tsx` assigns `role="dialog"` or `role="alertdialog"` and
uses a focus trap, but does not expose `aria-modal="true"`. This was identified
as an accessibility risk, not a proven spoken screen-reader defect.

**Required behavior:** Dialogs that actually block interaction with the page
must expose modal semantics and keep background content unavailable to keyboard
and assistive-technology navigation while open. Focus starts inside, remains
inside, and returns to the invoker when appropriate. Dialog name, consequence,
validation/error status, and available actions must be announced in a useful
order. Do not label a nonmodal surface as modal. Critical unsaved-work prompts
must not dismiss accidentally through background interaction.

**Acceptance cases:** Automated DOM/accessibility-tree checks establish modal
semantics and focus behavior for confirmation, unsaved-work, and editor dialogs.
A human spoken screen-reader pass verifies announcements and background
isolation with the available target assistive technology. If the manual pass
finds a defect, fix it and repeat that pass before closing this item.

This contributes to the still-open UXR-062 and UXR-071 manual qualification
items. No screen-reader pass is claimed by this document.

**Caller inventory audited for this fix:** `ConfirmDialog` covers macro,
package, and snapshot destructive confirmations and the Settings restart prompt;
each is an `alertdialog` whose first focus target is Cancel. `ConfirmPhraseDialog`
covers settings reset and factory reset; it is an `alertdialog` whose first
focus target is the phrase field or, for factory reset, the admin-password
field. Package rename uses a `dialog` with the name field first. The Macro
Editor Timing panel uses a `dialog` with the key-press field first.
`UnsavedChangesPrompt` is an `alertdialog` whose first focus target is Cancel
and whose policy forbids dismissal through the background. These call sites
all use the shared focus-trap hook. Menus and snackbars are not `Dialog`
callers and retain their existing nonmodal behavior.

## 3. Qualification and completion

Each fix needs a regression that reproduces its original failure before the
implementation is accepted. Run the focused unit/browser checks, then
`./scripts/check-webapp.sh`, followed by applicable broader repository gates.
Record the exact code SHA, commands, results, and any manual evidence in the
companion TODO. Existing UI screenshots should be compared for unintended
visual changes; update baselines only for intentional changes.

The original redesign TODO's open items remain open: spoken screen-reader and
full keyboard qualification, device-served offline behavior, and constrained
ESP32-S3/mobile performance. Safari qualification remains deferred at the
product owner's request. Neither this spec nor a passing browser fixture may
be used to mark those physical or manual checks complete.
