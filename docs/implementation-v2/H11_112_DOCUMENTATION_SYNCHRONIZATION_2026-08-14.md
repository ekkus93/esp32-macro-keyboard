# H11-112 — Documentation synchronization — 2026-08-14

## Scope

This report records the H11-112 documentation synchronization pass from
`docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_2026-08-10.md`.

Documentation basis before this pass:
`e840624dfe5b0c101cdfe338cd78db2405789bbc`.

H11-112 is documentation-only. It does not change firmware, frontend runtime
behavior, authoritative v2 contracts, or any hardware claim. Historical reports
remain historical evidence at their recorded SHAs and are intentionally not
rewritten to make old behavior appear current.

## Current behavior document

This pass adds `docs/CURRENT_V2_HARDENED_BEHAVIOR.md` as the concise current-state
reference for post-v2 semantics that span multiple historical implementation
reports.

The page is explicitly subordinate to `docs/SPEC_V2.md` and
`docs/UI_UX_SPEC_V2.md`. It separates implemented software semantics from
hardware/release evidence that remains open.

## H11-112 checklist mapping

### Current implementation/status documentation

The new current-state page consolidates the hardened semantics that were
previously distributed across H1-H5 and H9 evidence. It identifies the relevant
machine codes, retry boundaries, authority boundaries, and still-open proof
boundaries without promoting implementation history into product authority.

### Factory-reset recovery behavior

The page documents the durable `PENDING` journal boundary, `202 Accepted`
semantics after durable acceptance, fail-closed `reset_recovery_required`
status/diagnostics behavior, idempotent replay, marker-clear ordering, and the
fact that post-H3 interruption/reprovisioning hardware evidence remains open.

It separately documents reset-settings because that path intentionally does not
use the factory-reset journal and has different `202` / `409
reset_settings_incomplete` semantics.

Source evidence:

- `H3_030_DURABLE_FACTORY_RESET_STATE_2026-08-11.md`;
- `H3_031_IDEMPOTENT_FACTORY_RESET_STAGES_2026-08-12.md`;
- `H3_032_FACTORY_RESET_ACCEPTED_ERROR_SEMANTICS_2026-08-12.md`;
- `H3_033_FACTORY_RESET_FAILURE_INJECTION_MATRIX_2026-08-12.md`;
- `H3_034_RESET_SETTINGS_SEMANTICS_2026-08-12.md`.

### Password-change guarantees

The page documents the transaction order and exact externally meaningful
outcomes:

- `204` means the new credential is durable, active in RAM, all previous
  sessions were invalidated, and the current cookie is cleared;
- post-commit invalidation failure is `409 auth_state_incomplete`, with the new
  password explicitly authoritative; and
- pre-commit concurrent change is `503 conflict`, with no durable credential
  change.

It also records that login is fail-closed during the transition and that the old
best-effort NVS verifier reread was removed.

Source evidence:
`H2_PASSWORD_CHANGE_ATOMICITY_2026-08-11.md`.

### Confirmation-required send behavior

The page documents authoritative setting capture, fail-closed settings-read
failure, `awaiting_confirmation`, UART `confirm` routing, no HID key-down before
confirmation, cancellation/timeout behavior, and the distinction between
software/browser evidence and still-open exact-SHA physical HID/UART evidence.

Source evidence:
`H1_END_TO_END_PHYSICAL_CONFIRMATION_2026-08-11.md` and
`H10_101_FRONTEND_VALIDATION_2026-08-13.md`.

### Active-send degraded recovery

The page documents `none` / `known` / `unavailable` recovery meanings,
last-known-state retention, the global Retry/Cancel surface, GET-only Retry,
DELETE-only Cancel, the no-new-POST latch, transient-versus-persistent degradation,
and reload behavior when startup recovery fails.

It explicitly states that the strengthened H4 reload/dirty-state assertions at
`1ab7993cf460917ae89320628f16df6c08db2770` and
`e840624dfe5b0c101cdfe338cd78db2405789bbc` still require the pinned Node.js
24.18.0 / real-Chrome pass. The documentation therefore does not silently close
H4 while that proof is pending.

Source evidence:
`H4_ACTIVE_SEND_RECOVERY_DEGRADED_STATE_2026-08-13.md`.

### Storage commit-uncertain behavior

Because `commit_uncertain` is externally observable, the page documents the
rename activation point, parent-sync durability acknowledgement, HTTP `503
commit_uncertain`, retention of the activated final blob, and React's mandatory
reconcile-before-retry algorithm.

The document also records the distinct server-side settings reconciliation path,
where secret-bearing canonical state cannot be delegated to the browser.

Source evidence:

- `H5_051_STRUCTURED_OPERATION_RESULTS_2026-08-13.md`;
- `H5_052_ATOMIC_WRITE_ERROR_PROVENANCE_2026-08-13.md`;
- `H5_053_COMMIT_UNCERTAIN_RECONCILIATION_2026-08-13.md`;
- `H5_054_STORAGE_FAILURE_INJECTION_BROWSER_2026-08-14.md`.

### Stale `best-effort` wording

The production-wording audit found no stale current `best-effort` behavior to
remove. The obsolete password verifier cache refresh itself was deleted during
H2.

The single current production `best-effort` occurrence is the negative comment
in `web_settings.c` stating that the committed credential must never be re-read
from NVS as a best-effort cache refresh. H9 already classifies that exact phrase
as a forbidden-fallback statement rather than an implemented fallback. Removing
or rewriting it merely to obtain zero textual matches would weaken the source
explanation and would also bypass the intent of the count-bounded H9 production
audit.

Historical reports that say a best-effort refresh existed at their baseline are
retained as history. They are not current implementation comments.

Source evidence:
`H9_CROSS_CUTTING_SECRET_FALLBACK_AUDIT_RALPH_CORRECTION_2026-08-11.md`.

## Validation performed

Before publication, the documentation content was checked for:

- one trailing LF at EOF;
- no CRLF line endings;
- balanced fenced-code delimiters;
- referenced current evidence paths found on `master` through the GitHub
  connector; and
- no modification to `docs/SPEC_V2.md` or `docs/UI_UX_SPEC_V2.md`.

The sandbox does not have the repository-pinned `markdownlint-cli2` dependency
available independently of the frontend install, so the authoritative Markdown
lint result remains the normal `./scripts/check-all.sh` / Quality gate.

## Disposition

The H11-112 content work is implemented by the documentation commit containing
this report and `docs/CURRENT_V2_HARDENED_BEHAVIOR.md`.

The H11-112 tracker checkboxes should be closed only after that exact descendant
passes the repository Markdown/Quality gate. H11's phase exit gate remains a
separate reconciliation step because H4 strengthened browser validation and the
remaining hardware/release gates are still open.
