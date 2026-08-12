# H3-033 — Factory-reset failure injection matrix

Date: 2026-08-12

Starting `master` SHA:

`e47d8be16937e2de21f606b87ed33523b3f09b20`

## Scope

H3-033 turns the H3-030/H3-031/H3-032 reset design into permanent failure-injection evidence. It does not add hardware interruption evidence; that remains H3-035.

The audited sequence is:

1. commit durable `PENDING`,
2. erase settings/credentials,
3. invalidate RAM sessions,
4. delete repository blobs,
5. remove canonical temporary upload debris,
6. clear `PENDING` only after every required destructive effect succeeds,
7. restart, with immediate reboot as the fail-safe if the delayed timer cannot be armed.

Boot recovery repeats the destructive stages idempotently before ordinary settings, storage, USB, Wi-Fi, HTTP, or setup operation is allowed.

## Correctness defect found during the matrix audit

H3-032 made status and diagnostics report reset recovery while `PENDING`, but that was a readiness signal rather than a complete authority gate. After the marker committed, a later cleanup failure could leave a short pre-reboot window in which the provisioned HTTP server was still alive. Dedicated login/blob/send handlers and the generic API policy did not all consult the reset journal.

That was insufficient for H3-033's stronger invariants: once reset is accepted, old credentials and sessions must not become usable again, and surviving blobs must not be exposed while cleanup is incomplete.

H3-033 therefore puts a fail-closed guard around every provisioned `/api/v1` registration in `web_server_lifecycle.c`. While `PENDING`, every normal API request returns `503 reset_recovery_required`; if the journal itself cannot be read, every normal API request returns 503 with the underlying journal error. Static UI assets are not an authority-bearing API and remain readable. Setup-mode routes are unchanged; app-core already completes pending recovery before setup-mode startup.

## Injection matrix

| Cut/fault | Permanent proof | Required result |
| --- | --- | --- |
| before marker commit | integrated matrix, `LIVE_FAIL_MARK` | reset is not accepted; no destructive stage or restart occurs |
| marker committed / before settings erase | simulated durable cut | normal API authority blocked; reboot recovery erases credentials/settings |
| settings erased / before session invalidation | simulated durable cut | old session cannot regain API authority; reboot discards RAM session and recovery continues |
| session invalidation failure | live reset injected failure | accepted + `PENDING`; reboot recovery converges |
| first blob deletion failure | storage matrix, blob ID 3 | other blobs are still attempted; survivor is removed on retry |
| middle blob deletion failure | storage matrix, blob ID 2 | other blobs are still attempted; survivor is removed on retry |
| final blob deletion failure | storage matrix, blob ID 1 | prior blobs stay deleted; survivor is removed on retry |
| temporary cleanup failure | live + boot recovery matrix | marker remains pending and cleanup is retried |
| before marker clear / clear failure | live + boot recovery matrix | marker remains pending until every destructive effect is complete |
| delayed restart scheduling failure | integrated restart fail-safe + architecture guard | immediate restart is requested; reset is never silently left accepted without reboot |
| reboot between destructive stages | durable-cut table | recovery restarts from the beginning safely and converges |
| failure at each boot-recovery stage | recovery-stage matrix | first run fails with `PENDING`; second run succeeds after fault removal |

Blob IDs 3, 2, and 1 are deterministic first/middle/final attempts because `storage_blob_scan_with_ops()` sorts valid entries newest-first before invoking the delete observer.

## Authority and final-state assertions

The integrated matrix models the durable reset state separately from volatile session state. Every accepted-but-incomplete case asserts `PENDING`, which corresponds to the production normal-API guard. A simulated reboot clears the RAM session table before boot recovery.

Every successful recovery asserts all of the following:

- reset marker is `NONE`;
- credentials are absent;
- noncredential settings are absent;
- no old RAM session remains;
- repository blob count is zero;
- canonical temporary debris is absent;
- setup authority becomes available only after that complete state is reached.

Repeated recovery after marker clear is a successful no-op, and repeated blob delete-all after the survivor is removed deletes zero additional blobs.

## Permanent guards

`scripts/check-h3-architecture.py` now fails closed if:

- any provisioned `/api/v1` route bypasses a `reset_guarded_*` handler;
- any wrapper loses its intended delegate binding;
- the reset guard stops reading the durable journal or returning recovery-required;
- the production restart adapter loses its `esp_timer_create` / `esp_timer_start_once` -> `esp_restart()` fail-safe;
- the integrated failure matrix, route-gate regressions, or first/middle/final blob test disappears;
- H3-033 is left partially open or H3-034 is accidentally closed.

## Validation

The exact-base publication gate runs formatting/diff checks, H2/H3/H9 architecture guards, the credential logging guard, focused controls/storage/web/startup host tests, the same focused targets under ASan+UBSan, and the full host suite before it can publish the product commit to `master`.

The current sandbox cannot resolve GitHub for a local clone, so executable validation is performed by the exact-base publication gate before the product commit can reach `master`. No test result is claimed until that gate succeeds.

## Explicitly still open

- H3-034 reset-settings semantics.
- H3-035 real hardware interruption/power-cycle evidence.
- The broader H1/H2 hardware/browser evidence items already left open by their phases.
