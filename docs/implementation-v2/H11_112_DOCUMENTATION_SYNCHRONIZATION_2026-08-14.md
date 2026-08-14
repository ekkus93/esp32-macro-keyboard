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

## Documentation changes

The synchronization is implemented by two documentation commits:

- `7a6acc9c5bbfd60b3ca94293b2a1486f318f00cf` — adds the current hardened
  behavior reference and this H11-112 evidence report; and
- `5ee90282266b2b652ab1387011b4d71e8ae1e260` — exposes the hardened recovery
  semantics from the current README and human-readable API reference.

The resulting current documentation surface is:

- `docs/CURRENT_V2_HARDENED_BEHAVIOR.md` — cross-subsystem current behavior and
  proof boundaries;
- `docs/API.md` — current HTTP route index plus externally observable
  partial-commit/recovery meanings;
- `README.md` — current repository-status entry point linking the hardened
  behavior reference; and
- this report — the literal H11-112 mapping and validation boundary.

The current-behavior page is explicitly subordinate to `docs/SPEC_V2.md` and
`docs/UI_UX_SPEC_V2.md`. No authoritative v2 requirement was silently changed by
this documentation pass.

## H11-112 checklist mapping

### Current implementation/status documentation

`docs/CURRENT_V2_HARDENED_BEHAVIOR.md` consolidates hardened semantics that were
previously distributed across H1-H5 and H9 evidence. It identifies relevant
machine codes, retry boundaries, authority boundaries, and still-open proof
boundaries without promoting implementation history into product authority.

`README.md` now links that current-state reference directly from Repository
status so readers are not forced to infer present behavior from chronological
implementation reports.

`docs/API.md`, which identifies itself as the current human-readable v2 API
reference, now has a **Hardened mutation and recovery semantics** section. It
warns explicitly that several non-2xx outcomes cannot safely be interpreted as
"nothing changed."

### Factory-reset recovery behavior

The current behavior page and API reference document the durable `PENDING`
journal boundary, `202 Accepted` semantics after durable acceptance, fail-closed
`reset_recovery_required` status/diagnostics behavior, idempotent replay,
marker-clear ordering, and the fact that post-H3 interruption/reprovisioning
hardware evidence remains open.

They separately document reset settings because that path intentionally does not
use the factory-reset journal and has different `202` and
`409 reset_settings_incomplete` semantics.

Source evidence:

- `H3_030_DURABLE_FACTORY_RESET_STATE_2026-08-11.md`;
- `H3_031_IDEMPOTENT_FACTORY_RESET_STAGES_2026-08-12.md`;
- `H3_032_FACTORY_RESET_ACCEPTED_ERROR_SEMANTICS_2026-08-12.md`;
- `H3_033_FACTORY_RESET_FAILURE_INJECTION_MATRIX_2026-08-12.md`;
- `H3_034_RESET_SETTINGS_SEMANTICS_2026-08-12.md`.

### Password-change guarantees

The current behavior page and API reference document the transaction order and
exact externally meaningful outcomes:

- `204` means the new credential is durable, active in RAM, all previous
  sessions were invalidated, and the current cookie is cleared;
- post-commit invalidation failure is `409 auth_state_incomplete`, with the new
  password explicitly authoritative; and
- pre-commit concurrent change is `503 conflict`, with no durable credential
  change.

They also record that login is fail-closed during the transition and that the old
best-effort NVS verifier reread was removed.

Source evidence:
`H2_PASSWORD_CHANGE_ATOMICITY_2026-08-11.md`.

### Confirmation-required send behavior

The current behavior page and API reference document authoritative setting
capture, fail-closed settings-read failure, `awaiting_confirmation`, UART
`confirm` routing, no HID key-down before confirmation, cancellation/timeout
behavior, and the distinction between software/browser evidence and still-open
exact-SHA physical HID/UART evidence.

Source evidence:
`H1_END_TO_END_PHYSICAL_CONFIRMATION_2026-08-11.md` and
`H10_101_FRONTEND_VALIDATION_2026-08-13.md`.

### Active-send degraded recovery

The current behavior page and API reference document `none` / `known` /
`unavailable` recovery meanings, last-known-state retention, the global
Retry/Cancel surface, GET-only Retry, DELETE-only Cancel, the no-new-POST latch,
transient-versus-persistent degradation, and reload behavior when startup
recovery fails.

The current behavior page explicitly states that the strengthened H4
reload/dirty-state assertions at
`1ab7993cf460917ae89320628f16df6c08db2770` and
`e840624dfe5b0c101cdfe338cd78db2405789bbc` still require the pinned Node.js
24.18.0 / real-Chrome pass. The documentation therefore does not silently close
H4 while that proof is pending.

Source evidence:
`H4_ACTIVE_SEND_RECOVERY_DEGRADED_STATE_2026-08-13.md`.

### Storage commit-uncertain behavior

Because `commit_uncertain` is externally observable, both current references
document the rename activation point, parent-sync durability acknowledgement,
HTTP `503 commit_uncertain`, retention of the activated final blob, and React's
mandatory reconcile-before-retry algorithm.

The current behavior page also records the distinct server-side settings
reconciliation path, where secret-bearing canonical state cannot be delegated to
the browser.

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

The documentation was checked for:

- one trailing LF at EOF on the changed current documents;
- no CRLF line endings;
- balanced fenced-code delimiters where applicable;
- referenced current evidence paths present on `master` through the GitHub
  connector;
- `README.md` and `docs/API.md` edited from source blobs that exactly matched the
  pre-edit `master` blobs, preventing stale-copy overwrite; and
- no modification to `docs/SPEC_V2.md` or `docs/UI_UX_SPEC_V2.md`.

The sandbox has Node.js 22.16.0 and no installed `markdownlint-cli2` or frontend
`node_modules`; the repository requires pinned Node.js 24.18.0 for its frontend
chain. The authoritative Markdown lint result therefore remains the normal
`./scripts/check-all.sh` / Quality gate rather than being simulated with a
different toolchain.

## Disposition

The H11-112 documentation content is implemented through
`5ee90282266b2b652ab1387011b4d71e8ae1e260`, with this evidence update following
that implementation commit. No runtime or authoritative-spec file changed.

The H11-112 tracker checkboxes remain open until an exact descendant containing
these documentation changes passes the repository Markdown/Quality gate. The
Phase H11 exit gate remains a separate reconciliation step because H4's
strengthened browser validation and the remaining hardware/release gates are
still open.
