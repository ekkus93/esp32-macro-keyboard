# H11-110 V2 Checkbox Re-audit — 2026-08-13

## Scope and audit basis

This report closes only **H11-110 — Re-audit every affected v2 checkbox** from
`docs/ESP32_MACRO_KEYBOARD_POST_V2_CORRECTNESS_HARDENING_TODO_2026-08-10.md`.

The exact product/document state inspected before this docs-only reconciliation
was:

`cc9e05727a2767f070dc79e9e699146e10509b34`

H11-111 (literal evidence audit), H11-112 (documentation synchronization), the
Phase H11 exit gate, H10 physical-device work, and H12 release validation remain
separate and are **not** claimed complete here.

The product owner explicitly deferred current physical ESP32-S3R8 work to a
later Claude Code session. This audit therefore reopens stale hardware claims
where behavior materially changed after the historical run, rather than
substituting host/browser/build evidence.

## Re-audit disposition

| Item | Disposition | Reason |
| --- | --- | --- |
| V2-055 settings/password/device actions | Keep core route/contract checks checked | Routes remain implemented; H2/H3 now own hardened password/reset failure semantics. Current board validation remains open. |
| V2-061 confirmation | Keep executor/state-machine checks checked | Authoritative confirmation setting is wired fail-closed; H10-101 supplies executable browser confirmation evidence. Physical post-hardening HID confirmation remains open. |
| V2-062 release-all | Keep checked | H7 strengthened the invariant by observing release errors, preserving provenance, and latching unsafe release fail-closed. |
| V2-074 selection persistence | Keep checked | H8 adds visible persistence failure, Retry, non-dirty local continuation, and last-successfully-persisted selection semantics. |
| V2-075 send helper | Keep named V2 requirements checked | Normal/reload send helper behavior remains valid. Current code now also models `none`/`known`/`unavailable`, but H4's full degraded cancellation/freshness/browser matrix remains open. |
| V2-082 startup send recovery | Keep checked | Normal startup recovery remains implemented; unavailable recovery is visible and retryable without discarding the working copy. H4 remains broader than this checkbox. |
| V2-116 advanced replace/import-export | Keep checked | The explicit advanced flow is still intentionally delete-then-add. H5 storage commit/durability uncertainty is a distinct lower-level open requirement and is not claimed by V2-116. |
| V2-153 reset/power matrix | Reopen current factory-reset/reprovisioning item | H3 materially changed reset ownership with a durable journal, replayable cleanup, and fail-closed recovery. The old happy-path board run does not validate the new state machine. Other unchanged historical storage evidence remains recorded. |
| V2-154 auth/network/no-secret matrix | Reopen current password-change/PBKDF2 compound item; retain other supported items | H2 materially changed password transaction semantics after the old board run. PBKDF2 timing remains valid/frozen, but the current password/session path needs board revalidation. Idle/absolute expiry remains open; H9 supports the no-secret checkbox. |
| Phase 4/5/6/7/8/11/15 affected exit gates | Preserve literal truth; do not over-close | Phase 5 already keeps incomplete contract/security coverage open. Phase 4/6/7/8/11 named historical/core evidence remains supported by newer H7-H10 software evidence, but none is promoted into current hardware proof. Phase 15 remains open. |
| Final sign-off checklist | Reopen stale aggregate clean-checkout claim; update stale rationale | Current H10 software segments are proven, but the aggregate Quality gate is still red later in `check-scripts.sh`; H4/H5, current hardware, H11-111/112, and H12 remain open. |

## Specific stale-completion corrections

### V2-150 aggregate gate

The historical `de47eee` `./scripts/check-all.sh` exit-0 result is retained as
history. It is no longer a current release-gate result after post-v2 hardening.

H10-100/H10-101 re-prove the current native, sanitizer, coverage, analyzer,
firmware-build, frontend, production-build, and real-Chrome segments on exact
post-hardening candidates. The latest aggregate Quality run, however, proceeds
through those segments and then fails in `check-scripts.sh` on `shfmt`
differences. Therefore only the aggregate V2-150 `check-all.sh` checkbox is
reopened here; H12-121 owns the next clean aggregate proof.

### V2-151 device Unity execution

The old physical run executed the then-complete **10-case** device suite. H10-102
subsequently added two `[device][executor][hardening]` cases, so the current suite
contains **12 cases**. Build-only CI is not execution. All current V2-151 device
execution items are therefore reopened until a real board produces the complete
12-case result.

### V2-153 factory-reset hardware evidence

The 2026-08-10 board run remains useful historical happy-path evidence, but H3
later introduced a durable factory-reset journal, idempotent/replayable cleanup,
reset-pending authority gating, and changed reset-settings failure semantics.
The historical run predates those changes, so the current factory-reset/
reprovisioning checkbox is reopened. H3-035/H10-103 own the board rerun.

### V2-154 password-change hardware evidence

H2 later changed password change into the hardened durable-commit -> direct RAM
activation -> session-invalidation transaction and made post-commit incomplete
session invalidation explicit. The old happy-path password test predates that
path. Because the V2-154 checkbox combines successful password change with
PBKDF2 timing, the compound item is reopened: the PBKDF2 benchmark remains
valid/frozen, while the current password/session path requires H2-024/H10-103.

## H1 browser reconciliation

H1-014 had remained open only for executable browser evidence. H10-101 now
provides that evidence on exact frontend candidate
`d440be6c26174a26b5b62748161f59d8aa5c18c1` (Quality run `31675479517`, job
`94369022215`): the current real-Chrome suite exercises awaiting confirmation,
keeps Cancel reachable, verifies confirmation does not duplicate send POSTs,
and distinguishes timeout from ordinary failure. H1-014 is therefore closed.
H1-015 and the hardware-dependent H1 exit items remain open.

## H4 tracker finding

The H11 re-audit found that the H4 tracker is conservative relative to current
source: `startup.ts` already models send recovery as `none`, `known`, or
`unavailable`; non-404 recovery errors are not collapsed to `null`; `AppV2.tsx`
renders an explicit execution-state-unavailable warning and Retry while retaining
the working copy; and permanent Vitest covers that recovery path plus stale USB
status.

This does **not** justify closing H4 wholesale. The full H4 acceptance language
also requires complete cancellation-availability/freshness behavior and a real
Chrome scenario that intentionally fails recovery. Those remain open and should
be handled as their own software task rather than being silently inferred here.

## H5 boundary

V2-116's user-visible advanced delete-then-add operation remains deliberately
non-atomic and correctly warns about delete-success/add-failure. H5 asks a
different question: whether lower-level storage errors preserve initiating and
cleanup provenance, represent post-rename parent-sync uncertainty explicitly,
and reconcile before retry so an uncertain commit cannot silently duplicate a
snapshot. H5 remains open.

## Hardware deferral boundary

No current board-only requirement is checked by this audit. In particular the
following remain open/deferred:

- H1-015 physical confirmation/HID evidence;
- H2-024 hardened password-change board validation;
- H3-035 reset interruption/recovery board validation;
- H10-102 current 12-case Unity execution;
- H10-103 affected hardware matrix refresh;
- H12-122 final exact-release-SHA hardware confirmation;
- Android/manual/optional-host items that still lack their required real-device
  evidence.

## Disposition

**H11-110 COMPLETE** as a checkbox/evidence reconciliation task on audit basis
`cc9e05727a2767f070dc79e9e699146e10509b34`.

H11-111 and H11-112 remain open. No Phase H11 or final release completion is
claimed.
