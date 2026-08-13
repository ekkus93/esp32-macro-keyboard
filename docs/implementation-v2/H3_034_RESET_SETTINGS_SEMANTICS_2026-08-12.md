# H3-034 — Reset-settings semantics

**Date:** 2026-08-12  
**Starting master:** `be727d666347990abc097a9a23e202f85d02c988`  
**Task:** `H3-034 — Reset-settings semantics`

## Audit finding

The pre-H3-034 reset-settings path had the same class of partial-completion ambiguity previously corrected for password change and factory reset. `device_settings_reset_noncredential()` can atomically commit the §11.4 noncredential defaults before `auth_session_logout_all()` runs. If session invalidation then failed, `device_controls_reset_engine_reset_settings()` still scheduled reboot but returned the session error. The web layer collapsed that into ordinary `reset-settings unavailable`, which falsely suggested that nothing durable had changed.

A second boundary defect existed in the HTTP composition order: the accepted JSON body was allocated and serialized only after the destructive backend returned. A response-allocation failure could therefore produce an ordinary `500` after the settings reset had already committed.

Finally, restart scheduling was modeled as a `void` side effect. The reset engine could not distinguish “restart ownership established” from “durable reset committed but no restart can be guaranteed,” so the H3-034 restart-failure row was not representable honestly.

## Required semantics

Reset settings remains distinct from factory reset. It does **not** use the factory-reset NVS journal because after the noncredential settings record commits, durable configuration is already coherent. A reboot is needed to apply the restart boundary and to discard any RAM-only sessions that could not be invalidated synchronously.

The structured controls outcome records:

- whether the noncredential settings reset crossed its durable boundary;
- whether all RAM sessions were synchronously invalidated;
- whether restart ownership was established;
- the exact primary settings/session error; and
- a separate restart-ownership error.

The externally visible matrix is:

| Durable settings reset | Sessions invalidated | Restart owned | HTTP meaning |
| --- | --- | --- | --- |
| no | n/a | no | ordinary precommit backend error; never `202` |
| yes | yes | yes | exact SPEC `202` accepted response |
| yes | no | yes | exact SPEC `202`; reboot owns RAM-session recovery |
| yes | either | no | explicit `409 reset_settings_incomplete`; durable reset is not misrepresented as absent |

The exact successful response from `SPEC_V2.md` §13.12 is unchanged:

```json
{
  "accepted": true,
  "connectionWillClose": true,
  "reprovisioningRequired": false,
  "repositoryBlobsPreserved": true
}
```

That response is fully allocated before the reset-settings backend is invoked, so postcommit allocation failure cannot fabricate an ordinary failure.

## Pre-reboot authority boundary

After a successful durable noncredential reset, device-controls raises a RAM-only `reset_settings_restart_required` latch. The provisioned web route guard denies normal `/api/v1` authority with `503 reset_settings_incomplete` until reboot. This closes the brief interval in which a failed `logout_all()` could otherwise leave an old session usable before the scheduled reboot.

The latch is intentionally not durable. Reset settings preserves a coherent provisioned settings record; after any real reboot, RAM sessions and the latch are gone and no additional destructive recovery is required.

## Restart scheduling

The shared reset-engine scheduler callback now reports an `app_error_code_t` rather than being unobservable. The production adapter still preserves the prior fail-safe: failure to create/start the delayed ESP timer calls `esp_restart()` immediately. A non-`NONE` scheduler return is reachable only if restart ownership cannot be established (including the pathological case where the immediate reboot call unexpectedly returns).

For factory reset, restart ownership is now established **before** the durable `PENDING` marker is cleared. Therefore a scheduler failure leaves `PENDING` intact, and an immediate/power reboot before marker clear safely re-enters the already-idempotent H3 recovery path.

## Preservation semantics

The authoritative §11.4 reset remains noncredential-only. Existing permanent device-settings host coverage (`test_reset_preserves_credentials_and_blob_counter`) continues to prove that reset-settings preserves administrator credential material, access-point credentials, provisioning state, and the blob-ID counter while restoring the defined noncredential defaults. Repository blob bytes are not touched by this path.

The production reset adapter also securely wipes its local full settings output after `device_settings_reset_noncredential()` returns, because that structure contains the preserved AP/admin credential material even though those values are never exposed externally.

## Permanent regression coverage

H3-034 adds/strengthens permanent host coverage for:

- precommit settings-write failure;
- session invalidation failure with restart ownership;
- restart-ownership failure after durable settings commit;
- simultaneous session and restart failure with both errors retained separately;
- factory-reset restart-ownership failure retaining `PENDING` due the shared scheduler seam;
- web-device-action classification of reboot recovery vs committed restart failure;
- HTTP `202` for reboot-owned session cleanup and explicit `409 reset_settings_incomplete` for unowned restart;
- HTTP error-status mapping for the new stable app error;
- normal API denial while reset-settings reboot is required; and
- preservation of the existing H3-033 immediate-restart fail-safe matrix.

## Validation gate

The exact candidate must pass, before publication:

```text
./scripts/check-format.sh
git diff --check
python3 scripts/check-h2-architecture.py
python3 scripts/check-h3-architecture.py
python3 scripts/check-h9-architecture.py
bash scripts/check-credential-logging.sh firmware
./scripts/run-tests.sh controls
./scripts/run-tests.sh storage
./scripts/run-tests.sh web
./scripts/run-tests.sh startup
./scripts/run-tests.sh --sanitizers controls
./scripts/run-tests.sh --sanitizers storage
./scripts/run-tests.sh --sanitizers web
./scripts/run-tests.sh --sanitizers startup
./scripts/run-tests.sh
```

The publish gate must also prove that H3-034 alone is checked, H3-035 remains open, and no transport-only `.github/` files enter the product commit.

## Hardware scope

No hardware completion is claimed by H3-034. `H3-035 — Hardware interruption evidence` remains open.
