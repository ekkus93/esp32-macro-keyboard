# Current v2 hardened behavior

**Status date:** 2026-08-14  
**Documentation basis:** `e840624dfe5b0c101cdfe338cd78db2405789bbc`

This page describes the current post-v2 hardened behavior that is easy to
misunderstand from the older implementation-history reports. It is a status and
operator/developer reference, not a replacement for the authoritative product
contracts in `docs/SPEC_V2.md` and `docs/UI_UX_SPEC_V2.md`.

When this page and either authoritative specification disagree, the specification
wins. Historical reports under `docs/implementation-v2/` remain evidence of what
was true at their recorded SHA; they are not rewritten when later hardening
changes behavior.

## Password change

A successful `POST /api/v1/settings/change-password` is a coherent transaction:

1. the password-transition gate is acquired before the authoritative credential
   record is read;
2. the new credential candidate is derived and the plaintext request tree is
   wiped;
3. the candidate is durably committed;
4. the in-RAM login verifier is activated directly from that exact committed
   candidate; and
5. all existing sessions are invalidated before success is returned.

A `204 No Content` therefore means the new password is both durable and active in
RAM and all pre-existing sessions were invalidated. The response clears the
current browser session cookie.

There is no correctness dependency on a post-success NVS cache refresh. The old
`refresh_password_record_cache()` best-effort reread was removed. Login is
fail-closed while the password transition is in progress, so a concurrent login
cannot authenticate against a stale verifier.

If durable commit and RAM activation succeeded but session invalidation failed,
the server returns:

- HTTP `409 Conflict`;
- machine code `auth_state_incomplete`; and
- a message stating that the password changed, session invalidation is
  incomplete, and the new password is authoritative.

That outcome must not be treated as "nothing changed." The current cookie is
cleared, the old password is no longer authoritative, and recovery uses the new
password. A pre-commit concurrent password-change conflict is different: it
returns `503 Service Unavailable` with code `conflict`, makes no durable
credential change, and does not clear the current cookie.

Evidence: `docs/implementation-v2/H2_PASSWORD_CHANGE_ATOMICITY_2026-08-11.md`.
Reference-board validation of the hardened password path remains open under
H2-024/H10-103/H12-122; host/software proof is not being called new hardware
proof.

## Confirmation-required sends

`requireSerialConfirmation` is read from the authoritative device settings at
send acceptance and captured by value into the accepted execution request. A
settings-read failure rejects the send; there is no fallback that silently turns
confirmation off.

When confirmation is required:

- accepted POST state is `awaiting_confirmation`;
- `GET /api/v1/send` reports the same explicit state;
- no HID key-down is permitted before confirmation;
- the UART `confirm` command is routed to a pending macro execution before any
  unrelated administrative confirmation domain;
- Cancel remains valid while confirmation is pending; and
- cancellation or confirmation timeout terminates without typing the macro.

A later settings change cannot mutate an already accepted send's confirmation
semantics. Status serialization remains redacted and does not expose macro source
or key content.

Evidence: `docs/implementation-v2/H1_END_TO_END_PHYSICAL_CONFIRMATION_2026-08-11.md`
and `docs/implementation-v2/H10_101_FRONTEND_VALIDATION_2026-08-13.md`.
Physical HID/UART confirmation evidence for the post-hardening candidate remains
open under H1-015/H10-103/H12-122.

## Active-send degraded recovery

Execution recovery distinguishes three meanings rather than using a nullable
"send or no send" value:

- `none` — the server authoritatively reported no send, including a true 404;
- `known` — current or most-recent execution state was recovered; and
- `unavailable` — execution state could not be established because status
  recovery failed.

Unknown execution state is never converted into confirmed "no send." After the
bounded send-status retry policy is exhausted, the client retains the last known
status when available and raises a global **Execution state unavailable** surface.
That surface survives route changes and provides:

- **Retry execution status**, which is GET-only;
- **Cancel and release all keys**, which is DELETE-only; and
- explicit cancellation-delivery success/failure text.

While execution recovery is unresolved, `sendMacro()` refuses to issue another
POST. A recovered nonterminal execution is handed back to the normal tracking
path; a recovered terminal execution completes through the original callback
path. A true 404 clears the recovery latch as authoritative no-send.

A single transient tracking failure stays quiet; persistent failure becomes
visible and a later successful refresh clears the degraded state. In-place
recovery does not discard the current dirty working copy. A browser reload
naturally reconstructs repository state from the canonical device snapshot, but
if the first send-status recovery after reload fails the UI still reports
execution state as unavailable rather than claiming there is no send.

The strengthened reload/dirty-state H4 assertions were added at
`1ab7993cf460917ae89320628f16df6c08db2770` and made deterministic at
`e840624dfe5b0c101cdfe338cd78db2405789bbc`. Their pinned Node.js 24.18.0 /
real-Chrome validation is still pending, so H4 remains formally open until that
exact descendant is proven by the repository frontend gate.

Evidence: `docs/implementation-v2/H4_ACTIVE_SEND_RECOVERY_DEGRADED_STATE_2026-08-13.md`.

## Factory reset and reset recovery

Factory reset has a durable ownership boundary. Before any destructive reset
stage, firmware commits a dedicated factory-reset journal marker with state
`PENDING`. If that marker cannot be committed, the reset was not durably accepted
and the request returns an ordinary precommit failure.

Once `PENDING` is durable, reset ownership belongs to recovery even if later
cleanup fails. `POST /api/v1/device/factory-reset` therefore returns the normal
`202 Accepted` response for both:

- cleanup completed before the response; and
- cleanup remains incomplete but is durably owned by the reset journal and will
  resume on reboot.

The accepted response is fully allocated before destructive work starts, so an
allocation/serialization failure cannot turn an already accepted destructive
reset into a misleading ordinary `500`.

While the journal remains `PENDING`:

- ordinary provisioned API authority is fail-closed;
- `GET /api/v1/status` returns `503 reset_recovery_required`;
- `GET /api/v1/diagnostics` reports the same recovery condition; and
- boot does not expose normal/setup operation until idempotent cleanup succeeds.

Reset cleanup is replay-safe: settings/credential erase, session invalidation,
blob deletion, upload-debris cleanup, marker clear, and restart/re-entry may be
repeated. The marker is not cleared until every required destructive effect has
succeeded and restart ownership has been established. A reboot or power cut
therefore re-enters recovery instead of guessing that partial work was a
rollback.

Evidence:

- `docs/implementation-v2/H3_030_DURABLE_FACTORY_RESET_STATE_2026-08-11.md`;
- `docs/implementation-v2/H3_031_IDEMPOTENT_FACTORY_RESET_STAGES_2026-08-12.md`;
- `docs/implementation-v2/H3_032_FACTORY_RESET_ACCEPTED_ERROR_SEMANTICS_2026-08-12.md`;
- `docs/implementation-v2/H3_033_FACTORY_RESET_FAILURE_INJECTION_MATRIX_2026-08-12.md`.

The post-H3 power-interruption/reprovisioning run remains open under
H3-035/H10-103/H12-122. Historical pre-H3 reset hardware evidence is not used as
proof of the new journal state machine.

### Reset settings is different from factory reset

Reset settings does not use the durable factory-reset journal because the
noncredential settings record is coherent after its own atomic commit. It
preserves administrator credentials, access-point credentials, provisioning
state, the blob-ID counter, and repository blobs.

After a durable noncredential reset, normal API authority is denied until reboot.
If session invalidation fails but restart ownership is established, the request
still returns the normal `202` because reboot owns RAM-session cleanup. If the
settings reset committed but restart ownership cannot be established, the route
returns explicit `409 reset_settings_incomplete` rather than pretending nothing
changed.

Evidence: `docs/implementation-v2/H3_034_RESET_SETTINGS_SEMANTICS_2026-08-12.md`.

## Storage commit uncertainty

Blob creation has a distinct activation and durability boundary:

- before final rename: not committed;
- final rename: canonical blob activation;
- successful parent-directory synchronization: final durability
  acknowledgement; and
- rename succeeded but required parent sync failed: commit state is uncertain.

For the last case, the server returns HTTP `503` with machine code
`commit_uncertain`. This is intentionally distinct from ordinary storage
unavailability because the canonical mutation may already exist. The activated
final blob is retained; it is not temporary cleanup debris.

The webapp must not automatically repeat the POST. Before a create it records the
canonical blob-ID set and the exact gzip bytes. After `commit_uncertain` it:

1. refreshes the canonical blob list;
2. considers only newly observed IDs;
3. downloads those candidates as raw gzip bytes; and
4. compares them byte-for-byte with the attempted payload.

Exactly one new exact match is recognized without a second POST and the working
copy remains dirty because no `201 Created` acknowledgement was received. An
authoritative no-match ends that save attempt and permits a later explicit user
Save. Unavailable or ambiguous reconciliation keeps the uncertainty latch and
blocks another POST until canonical state can be established or the user
replaces the working copy deliberately.

Device-settings NVS commit uncertainty is reconciled internally because the
browser never receives secret credential material needed for byte comparison.
An unresolved uncertain settings replace invalidates the cached settings record;
the next write must first reload canonical persistence state.

Evidence:

- `docs/implementation-v2/H5_051_STRUCTURED_OPERATION_RESULTS_2026-08-13.md`;
- `docs/implementation-v2/H5_052_ATOMIC_WRITE_ERROR_PROVENANCE_2026-08-13.md`;
- `docs/implementation-v2/H5_053_COMMIT_UNCERTAIN_RECONCILIATION_2026-08-13.md`;
- `docs/implementation-v2/H5_054_STORAGE_FAILURE_INJECTION_BROWSER_2026-08-14.md`.

H5-055 remains a physical durability sanity run and is not inferred from host or
browser evidence.

## "Best-effort" and fallback wording

H11-112 re-audited current production `best-effort` wording rather than deleting
historical evidence. The obsolete best-effort password verifier refresh is gone.
The one current production `best-effort` occurrence is the negative source
comment in `web_settings.c` stating that the committed credential must **never**
be re-read from NVS as a best-effort cache refresh. It describes a forbidden
fallback, not an implemented best-effort behavior, and therefore is not stale.

The H9 fail-closed production audit count-bounds that occurrence along with all
reviewed fallback wording and explicit discarded C return values. Historical
implementation reports may still describe defects that existed at their recorded
baseline; those reports are intentionally preserved rather than edited to make
history look current.

Evidence:
`docs/implementation-v2/H9_CROSS_CUTTING_SECRET_FALLBACK_AUDIT_RALPH_CORRECTION_2026-08-11.md`.

## Validation boundary

This page documents current implemented semantics; it does not close hardware or
release gates by itself. The remaining proof boundaries include the H1/H2/H3/H5
physical reruns, H10 device/hardware validation, H12 exact-release-SHA hardware
confirmation, and the strengthened H4 pinned frontend/real-Chrome run described
above. `docs/TODO_V2.md` and the post-v2 hardening TODO remain the authoritative
completion ledgers.
