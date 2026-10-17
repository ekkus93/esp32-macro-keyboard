# ESP32 Macro Keyboard — UI/UX Review Follow-up Specification

**Status:** Accepted follow-up; implemented and verified for the automated
scope. Remaining manual and device qualifications are listed in the companion
TODO.
**Review baseline:** `e7f5366` on 2026-10-17
**Companion plan:** `docs/UI_UX_REDESIGN_REVIEW_FOLLOWUP_TODO_2026-10-17.md`
**Scope:** Findings from the review of
`docs/UI_UX_REDESIGN_REVIEW_FIXES_TODO_2026-10-17.md`

## 1. Authority and boundaries

This document defines acceptance outcomes for the review findings below. It
does not change the frozen `docs/SPEC_V2.md` or `docs/UI_UX_SPEC_V2.md`, the v2
HTTP API, firmware execution or key-release behavior, repository format,
in-memory working-copy model, or local-only asset policy. If an outcome
conflicts with a frozen requirement, record the conflict and request explicit
per-change product-owner approval before editing that requirement.

The previous review-fixes TODO is a historical implementation record. Its
checked boxes are not evidence for this follow-up. The owner-waived spoken
screen-reader pass remains waived for that review; no spoken or physical-device
qualification is implied here.

## 2. Corrective outcomes

### UXRF2-001 — Preserve terminal release failures during GET reconciliation

`MacrosPage.tsx` currently treats a terminal response from
`recoverActiveSend()` as the absence of an active send. That path can discard
`releaseError` and the terminal outcome after a conflicting or uncertain POST.

**Required behavior:** A canonical terminal GET must end the pending-start or
recovery state without starting another POST or automatically cancelling an
unidentified send. It must retain a nonempty `releaseError` in the separate,
persistent key-release warning used by ordinary send completion. It must show
the actual terminal outcome where the existing UI has a corresponding
acknowledgement, rather than claiming that no send was confirmed. Recovery of
an active send must keep an explicit Cancel action available. A response with
no send may use the existing no-send guidance.

**Acceptance cases:**

1. After `409` or an uncertain POST, GET returns terminal `failed`,
   `cancelled`, or `timed_out` with a nonempty `releaseError`: the terminal
   outcome and separate release warning remain visible until dismissed.
2. A terminal `completed` response with `releaseError` still shows the
   release warning. No DELETE or second POST is issued by reconciliation.
3. An active GET response restores tracking and explicit cancellation; a
   no-send GET response gives accurate no-send guidance.
4. The warning remains available after route navigation, subject to the
   existing send-state ownership rules. Browser and unit tests distinguish
   client behavior from physical key-release evidence.

Relevant paths: `webapp/src/features/macros/v2/MacrosPage.tsx`,
`webapp/src/v2/sendClient.ts`, and their send/recovery tests.

### UXRF2-002 — Resolve execution recovery and modal ownership

`Dialog.tsx` marks siblings inert when it mounts. The global
`ExecutionRecoveryOverlay` is a sibling of `AppV2` and can appear before or
after a dialog. Depending on that timing, the recovery control can become
inert or remain interactive above an `aria-modal` dialog.

**Required behavior:** Exactly one keyboard and assistive-technology interaction
surface owns focus at a time. If execution recovery appears while a modal is
open, the application must present the recovery warning and its Cancel and
release all keys action through a coherent priority or modal policy. The
cancel action must remain operable, the page behind the active surface must
remain unavailable, and focus must have a predictable destination when the
recovery state clears. A dialog opened while recovery is already visible must
follow the same policy. Dynamic mount/unmount must not leave stale `inert`
attributes or expose two simultaneously interactive modal surfaces. The
unsaved-work prompt must not authorize a destructive action through background
interaction.

**Acceptance cases:**

1. Dialog first, then recovery overlay: recovery warning and Cancel are
   available by keyboard; the modal/background isolation rule remains valid.
2. Recovery first, then dialog: the same actions and focus policy hold.
3. Clearing either surface restores focus to an appropriate surviving control
   and restores background interactivity only when no modal owner remains.
4. Confirmation, unsaved-work, and editor dialogs retain their current names,
   keyboard trapping, and dismissal behavior. Automated DOM and real-browser
   checks cover the dynamic sibling case; they do not claim a spoken
   screen-reader qualification.

Relevant paths: `webapp/src/components/Dialog.tsx`,
`webapp/src/features/shell/v2/ExecutionRecoveryOverlay.tsx`,
`webapp/src/features/shell/v2/useFocusTrap.ts`, and `webapp/src/main.tsx`.

### UXRF2-003 — Identify the saved baseline without mislabeling newer edits

After version A is saved while version B is edited, the working copy correctly
remains B and dirty. The snapshot list can nevertheless label A simply
`Loaded`, which can suggest that the live contents equal A.

**Required behavior:** The UI must distinguish the saved or loaded baseline
from a dirty live copy. It must state plainly when the current working copy
contains changes beyond the indicated snapshot, while retaining the visible
Unsaved changes status. A clean copy may retain the existing Loaded label.
The label must remain accurate across successful saves, edits during upload,
discard, load, import, replacement, deletion of the associated blob, failed
uploads, and uncertain commits. No repository bytes may be persisted in the
browser or exposed in snapshot-list metadata.

**Acceptance cases:**

1. Save A, edit to B before `201`: B survives, dirty stays true, A becomes
   baseline, and the snapshot row does not imply that B equals A.
2. Discard B: the clean copy and snapshot label agree again.
3. Load/import/replace/delete/failure/uncertainty paths do not point at an
   unrelated blob or falsely claim the live copy was saved.
4. Unit and browser tests assert the text and accessible name of the
   snapshot identity, as well as the dirty indicator.

Relevant paths: `webapp/src/AppV2.tsx`,
`webapp/src/features/snapshots/v2/SnapshotRow.tsx`,
`webapp/src/features/snapshots/v2/SnapshotsPage.tsx`, and the working-copy
store.

### UXRF2-004 — Bound Settings export errors and prove retry

`SettingsPage.tsx` catches encoding and file-save failures, but sends the
underlying error through `v2ErrorText()`, which can display an arbitrary
`Error.message`. The existing test checks that Retry is enabled after a
failure but does not complete a retry.

**Required behavior:** Export failures must use safe, actionable UI text that
does not reproduce arbitrary dependency error messages, macro source, or
credentials. A failure must keep the dirty working copy and pending action,
keep the prompt open, clear the export busy state, and permit an actual retry
or Cancel. A successful retry downloads the file but must still require an
explicit Save snapshot or Discard changes decision before a destructive
action. If a failure is logged, its logged fields must be similarly bounded.

**Acceptance cases:**

1. An `exportRepository` rejection whose message contains sentinel macro
   source and a disposable credential produces a useful error without either
   sentinel appearing in the DOM or logs.
2. A `saveAsFile` exception containing the same sentinels has the same result.
3. After each failure, retry succeeds, clears the error and busy state, leaves
   the prompt open and copy dirty, and does not start the pending destructive
   action. Cancel remains usable.
4. The same privacy policy applies to the equivalent Snapshots export path if
   it uses the same arbitrary error rendering.

Relevant paths: `webapp/src/features/settings/v2/SettingsPage.tsx`,
`webapp/src/features/snapshots/v2/SnapshotsPage.tsx`, and
`webapp/src/features/auth/v2/v2ErrorText.ts`.

### UXRF2-005 — Make completion claims match reproducible evidence

The previous TODO contains an unchecked evidence rule, contradictory full-gate
claims, and completion assertions broader than their recorded checks.

**Required behavior:** Reconcile the prior record without erasing historical
failures. State clearly which run failed, which later run passed, with dates,
commit SHA, commands, exit status, and material counts. For each focused
check, record an exact command and result. Record how screenshot review was
performed and its outcome; do not infer visual review solely from 97 successful
capture operations. Directly test or explicitly narrow claims about focus
scroll position, active navigation indication, and delayed-POST
reload/reconnect. Mark checkboxes complete only when their stated acceptance
case is evidenced; reopen or annotate any historical overclaim. The follow-up
TODO must contain its own evidence ledger.

**Acceptance cases:**

1. No paragraph simultaneously calls the same gate open and passed.
2. An independent reader can reproduce every claimed focused check from an
   exact command and identify the implementation SHA and result.
3. Focus scroll position and active navigation indication have explicit
   assertions, or the claim is narrowed. Delayed-POST reload/reconnect is
   either tested or explicitly recorded as a limitation under the existing
   recovery contract.
4. Screenshot review has identifiable artifacts and a recorded visual
   conclusion, separate from capture count.
5. Prior and current TODO statuses align with verified evidence; the waived
   spoken pass and still-open physical-device, device-served offline,
   constrained-performance, full manual keyboard, and deferred Safari checks
   remain accurately described.

Relevant paths: both review TODOs, the original redesign TODO, focused test
files, browser workflows, and CI or local command logs.

## 3. Verification and completion

Write a regression that demonstrates each defect before accepting its fix.
Run focused unit and browser checks, then `./scripts/check-webapp.sh` and
applicable repository gates. Record exact commands, implementation SHA,
exit status, test counts, and screenshot review evidence in the companion
TODO. Compilation, host fakes, and browser fixtures do not qualify physical
key release or ESP32-S3 behavior. Do not close the follow-up while any
acceptance case above lacks implementation or reproducible evidence.
