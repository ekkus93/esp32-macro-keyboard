# H3-032 — Factory-reset accepted/error semantics

## Scope

This evidence covers H3-032 only. H3-033's full cut-point/failure-injection matrix, H3-034 reset-settings semantics, H3-035 hardware interruption evidence, and the Phase H3 exit gate remain open.

Starting `master` SHA: `4cf3c06e85f65f9af73d032326f8adf23aff84ef`.

## Audited ambiguity

Before H3-032, `device_controls_factory_reset()` returned only `app_error_code_t`. That erased the transaction boundary established by H3-030: failure to commit the durable `PENDING` marker meant the reset was not accepted, while any failure after marker commit meant reset ownership was already durable and H3-031 would resume cleanup on reboot. The web layer mapped both to an ordinary backend failure.

A second ambiguity existed after backend return: the handler allocated and serialized its fixed `202` JSON only after destructive reset work. Allocation/encoding failure could therefore produce an ordinary `500` after durable acceptance.

During the brief interval between durable acceptance and reboot, concurrent status/diagnostics requests also did not consult `PENDING`.

## Structured transaction result

Device controls now returns:

- `durably_accepted`
- `recovery_required`
- `primary_error`

Pre-marker failure is `{false, false, exact_error}`. Complete cleanup is `{true, false, APP_ERROR_NONE}`. Any post-marker failure is `{true, true, exact_first_error}`.

The first post-marker failure remains identifiable instead of being replaced by a generic recovery code.

## HTTP semantics

`POST /api/v1/device/factory-reset` returns `202 Accepted` for both durable accepted states: cleanup already completed, or cleanup remains owned by the reset journal and will resume on reboot. Only a result that never durably established `PENDING` is mapped as an ordinary failure.

The authoritative `SPEC_V2.md` response remains unchanged:

```json
{
  "accepted": true,
  "connectionWillClose": true,
  "reprovisioningRequired": true,
  "repositoryBlobsPreserved": false
}
```

The complete accepted response is now allocated and serialized before the destructive backend call. If preparation fails, the backend is never called. After durable acceptance, no cJSON allocation remains necessary to return the already-prepared response.

## Readiness visibility

H3-031 prevents ordinary boot services from starting until pending cleanup succeeds. H3-032 additionally closes the pre-reboot visibility window:

- `GET /api/v1/status` returns `503` with `reset_recovery_required` while `PENDING`;
- `GET /api/v1/diagnostics` returns the same recovery condition instead of a normal snapshot.

The existing React factory-reset flow already enters the full-screen reconnect surface after `202`, polls status, and reloads when the device is genuinely reachable. The new status gate prevents that poll from mistaking an accepted-but-still-pending reset for readiness. After H3-031 clears the marker, reload re-runs provisioning detection and enters real unprovisioned setup.

## Marker-clear invariant

The established order remains: settings/credential erase, session invalidation, blob deletion, temporary-debris cleanup, then marker clear. A prior failure skips marker clear. H3-032 does not weaken or reorder that invariant.

## Regression evidence

Permanent host coverage proves:

- reset-engine success, pre-marker rejection, and post-marker recovery-required outcomes;
- web-device-action classification preserves the primary failure;
- direct administration handler maps precommit failure to non-202 and postcommit cleanup failure to 202;
- the live HTTP route preserves the checked-in `factoryResetAccepted` object on postcommit cleanup failure;
- a live precommit failure does not trigger the post-response restart;
- status and diagnostics return explicit reset recovery while `PENDING`.

The H3 architecture guard now enforces these semantics and verifies H3-033 remains open. Hardware evidence is not claimed.
