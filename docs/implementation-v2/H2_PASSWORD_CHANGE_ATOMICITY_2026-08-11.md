# H2 — Password-change atomicity and authentication coherence

**Date:** 2026-08-11  
**Phase:** `H2 — Password-change atomicity and authentication coherence`  
**Baseline audited:** `5285f67f08b3889eb37d05ba1e6cb3decba9eea5`  
**Software implementation SHA:** `c9351d3ba2c862d50dd46c0b6f1827a3b3f40d92` (`fix: make password change transaction coherent`)

## 1. Audit result

H2 confirmed the failure described by the post-v2 review and found two additional concurrency hazards that had to be fixed before the password-change path could be considered coherent.

The pre-H2 path durably replaced the password settings record, invalidated sessions, returned success, and then performed a best-effort `refresh_password_record_cache()` that re-read NVS into the in-RAM login verifier. If that final re-read failed, the HTTP operation could already have returned `204` while login still used the old in-RAM verifier. The re-read also made a second storage operation part of a correctness invariant without exposing its failure.

The review's session-invalidation concern was also real but narrower than a silent-success bug: if the durable password write succeeded and `logout_all` then failed, the route returned a generic backend error even though the new password had already become the durable credential. The client could not tell whether nothing changed or whether the new password was authoritative while old sessions might still work.

The Ralph loop additionally found:

1. **Login race during credential activation.** A durable-write-first implementation still allowed a login request on the HTTP task to snapshot the old RAM verifier in the interval between durable commit and RAM activation.
2. **Concurrent password-change stale-authority race.** If a transaction gate were acquired only near the durable write, two change-password requests could both verify the same old password and derive different candidates. The second could later commit a candidate based on stale credential authority after the first transaction completed.
3. **Sensitive-structure lifetime.** The `current` and `candidate` settings structs contain password salt/verifier bytes and were not uniformly zeroed on every return path.
4. **Cookie/status ambiguity.** A pre-commit transaction-busy conflict and a post-commit session-invalidation failure could both have been represented as HTTP 409. Clearing the browser cookie on every 409 would incorrectly sign out a caller whose request made no durable change.

## 2. Final transaction invariant

The final transaction order is:

1. parse the exact request body;
2. acquire the password-transition gate;
3. read the authoritative durable credential/settings record;
4. validate and verify the current password;
5. derive the new credential candidate;
6. wipe the parsed plaintext JSON tree;
7. durably replace the settings/credential record;
8. activate the in-RAM verifier directly from the **same exact committed candidate**;
9. invalidate all sessions;
10. release the transition gate and securely wipe transient credential structures.

The transition gate is acquired before the durable credential is even read. Therefore two password-change operations cannot both establish candidates from the same stale authority. Login uses a transition-aware password-record snapshot and returns a visible `503` while the gate is active rather than authenticating against an old or changing verifier.

The portMUX protecting the transition flag/password-record copy is held only for bounded flag/copy operations. PBKDF2, NVS I/O, confirmation waiting, and session invalidation run outside that critical section.

## 3. Best-effort verifier refresh removed

`refresh_password_record_cache()` and its post-success NVS re-read were deleted.

After the durable settings replacement succeeds, production converts the exact committed `app_v2_device_settings_t` candidate into an `auth_password_record_t` and installs that record directly in RAM. This activation seam is deliberately infallible: it is an in-memory bounded copy, not another storage operation. Therefore H2 has no remaining RAM-activation failure step that could split durable and runtime credential authority.

A returned `204` now means:

- the new credential is durably stored;
- the same exact credential is active in RAM;
- all pre-existing sessions were invalidated;
- the current response clears the session cookie.

## 4. Explicit post-commit partial state

If session invalidation fails **after** durable commit and RAM activation, the operation returns a stable, machine-readable partial-commit result:

- HTTP `409 Conflict`;
- error code `auth_state_incomplete`;
- message: `password changed; session invalidation incomplete; sign in with the new password`;
- current browser cookie cleared.

This response has one unambiguous meaning: **the new password is authoritative**. Old sessions may still be valid because invalidation did not complete. Retrying the original old→new request is therefore correctly rejected; recovery must authenticate with the new authoritative password.

A pre-commit transaction-busy condition is intentionally different:

- HTTP `503 Service Unavailable`;
- error code `conflict`;
- message `password change already in progress`;
- no cookie clearing;
- no durable credential change.

This distinction prevents the client from treating "nothing changed" as if a password had committed.

## 5. Login fail-closed behavior

`web_server_password_record_snapshot_for_login()` checks the password-transition flag under the same short critical section used for record copies. While a password change is active, login does not snapshot any verifier and returns `APP_ERROR_AUTH_STATE_INCOMPLETE`; the HTTP login handler maps that to `503 Service Unavailable` and wipes its plaintext password buffer before returning.

This closes the durable-write/RAM-activation timing window and also prevents login from observing an intermediate credential state while session invalidation is pending.

## 6. Zeroization

`web_change_password_handle()` now uses a common cleanup path. On success, pre-commit failure, or post-commit partial failure it wipes:

- the parsed cJSON request tree containing current/new plaintext passwords;
- derived password material;
- the candidate settings struct containing salt/verifier bytes;
- the current settings struct containing salt/verifier bytes.

The production RAM-activation adapter also wipes its temporary `auth_password_record_t` after copying it into the synchronized server configuration.

The existing credential-output guard remains in force, and no H2 error response contains passwords, salts, verifiers, or session tokens.

## 7. Failure-injection and success regression matrix

H2 adds a real-auth-core integration target plus route/unit regressions.

Pre-commit injection covers:

- password-material creation failure;
- settings/durable replace failure;
- password-transition gate busy/conflict;
- cleanup/zeroization and retry behavior.

For those failures, tests prove the old password remains authoritative, the new password is not active, old sessions remain valid, and retry with the old password can proceed once the injected fault clears.

Post-commit injection covers session invalidation failure after durable replace and RAM activation. Tests prove the new password works immediately, the old password fails, pre-existing sessions can remain valid under the injected invalidation fault, the result is the explicit committed-partial outcome, a blind old-password retry fails, and recovery using the new authoritative password succeeds and then invalidates the old sessions.

The RAM activation step has no failure injection because H2 deliberately redesigned it to be an infallible in-memory copy from the committed candidate; there is no fallible storage re-read or allocation left in that step.

The success path uses real host auth-core password/session logic and proves immediately after the `204` transaction result:

- old password fails;
- new password succeeds;
- both pre-existing sessions fail validation;
- a newly created session works and retains the normal idle and absolute expiry values;
- immediately afterward the normal five-failure login threshold produces the expected 300-second lockout.

Real HTTP-handler regressions additionally prove the `409 auth_state_incomplete` envelope and cookie clearing for post-commit partial failure, and `503 conflict` with no cookie clearing for the pre-commit busy case.

## 8. Client behavior

The v2 settings client preserves `409 auth_state_incomplete` as a typed `V2ApiError` rather than collapsing it into a generic password-change failure.

The Settings page regression requires the explicit message telling the owner that the new password is authoritative. It intentionally does not run the ordinary success notification path for the partial result; the server has already cleared the cookie, and the next authenticated request will drive the normal unauthorized/sign-in transition.

These frontend regressions are committed but were not executed locally because this sandbox lacks the repository-pinned Node 24.18.0 frontend environment/dependencies. Permanent repository gates will execute them; no CI job was monitored during this Ralph loop.

## 9. Local validation

Using the same temporary, uncommitted cJSON development shim described by H0/H9 against the installed cJSON 1.7.18 runtime:

- `./scripts/run-tests.sh web` — **30/30 passed**;
- `./scripts/run-tests.sh auth` — **5/5 passed**;
- `./scripts/run-tests.sh --sanitizers web` — **30/30 passed** under ASan+UBSan;
- `./scripts/run-tests.sh --sanitizers auth` — **5/5 passed** under ASan+UBSan;
- local uploaded-snapshot full host suite — **61/61 passed**;
- same local uploaded-snapshot full suite under ASan+UBSan — **61/61 passed**;
- `python3 scripts/check-h2-architecture.py` — passed;
- `python3 scripts/check-h9-architecture.py` — passed against the locally available H9 guard;
- `bash scripts/check-credential-logging.sh firmware` — passed;
- `git diff --check` — passed.

The 61-test full-suite count is explicitly **not** claimed as the exact current-master count because the user-provided ZIP predates H1's additional serial-confirmation host target. The focused H2 web/auth results are valid for the H2-modified paths; those paths had not changed between the uploaded baseline and the H2 starting `master`.

## 10. H2 disposition

H2 software work is complete through H2-023:

- H2-020: complete — no best-effort credential-cache refresh remains;
- H2-021: complete — durable/RAM/session authority has explicit transaction semantics and a distinguishable committed-partial result;
- H2-022: complete — failure injection, retry semantics, zeroization, and exact API outcomes are covered; RAM activation is intentionally infallible;
- H2-023: complete — immediate success invariants are covered with real host auth-core logic;
- H2-024: **open** — reference-board password-change, power-cycle persistence, and PBKDF2 timing sanity still require the physical ESP32-S3R8.

Accordingly, the H2 phase exit gate remains partially open only for the required hardware evidence. Software does not claim hardware completion.
