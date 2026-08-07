# V2-042 — Sessions and rate limiting

**Phase:** 4 — Authentication, provisioning, and device settings  
**Task:** V2-042 — Sessions and rate limiting  
**Status:** Software implementation candidate; final exact-SHA CI pending  
**Hardware:** None required for this task's software semantics; Phase 4 hardware exit remains blocked elsewhere

## Scope

This change replaces the legacy session lifetime and global login-throttle behavior with the V2 session and rate-limit policy. It is intentionally limited to authentication/session behavior and its production login/logout integration. It does not claim V2-040 provisioning completion, V2-041 PBKDF2 benchmark completion, V2-043 settings completion, V2-044 Wi-Fi/reset completion, or the Phase 4 exit gate.

No compatibility fallback, warning suppression, ignored failure, unbounded allocation, or alternate global throttle path was added.

## Implemented behavior

### Session tokens and bounded RAM

- Session tokens continue to contain 32 bytes of random entropy encoded as 64 lowercase hexadecimal characters.
- At most eight sessions are stored in fixed RAM.
- No session token is written to NVS.
- Session creation remains fail-closed on entropy failure.
- A failed ninth-session replacement is transactional: if token generation or lifetime initialization fails, the prior session table is restored rather than silently losing the LRU session.

### Idle and absolute expiry

Each session now has both:

- a 24-hour / 86,400-second idle lifetime;
- a seven-day / 604,800-second absolute lifetime.

Successful validation refreshes the idle deadline but never extends it past the absolute deadline. Expired entries are cleared from the bounded table. A newly initialized auth core contains no sessions, proving reboot/reinitialization invalidates all RAM-only sessions.

### Deterministic ninth-session behavior

When all eight session slots are active, a successful ninth session replaces the least-recently-used session. Last-use activity is updated only on successful validation. The tie behavior is deterministic because the first minimum-index entry is retained as the LRU candidate.

The previous full-table `APP_ERROR_CONFLICT` behavior is removed from the session core.

### Per-source rolling login throttling

The old global fixed-window failure counter is replaced by a fixed eight-entry per-IPv4 table. Each entry stores at most five failure timestamps.

Policy:

- five failures within a rolling 60-second window trigger lockout;
- lockout lasts 300 seconds;
- successful login clears only that source's failure state;
- one source's failures do not throttle another source;
- active lockouts are never evicted merely to make room for a new source;
- the table remains fixed-size and cannot become an attacker-controlled memory leak;
- if all bounded entries are actively locked, recording another source fails explicitly rather than silently falling back to a global or untracked bucket.

`Retry-After` is calculated from the remaining lockout duration with ceiling-to-seconds semantics.

### Production HTTP integration

The production login handler now obtains the peer IPv4 address from the request socket and uses it for all rate-limit operations. If the source address cannot be resolved or is not IPv4, login fails explicitly; there is no global fallback bucket.

The production login cookie remains:

`HttpOnly; SameSite=Strict; Path=/`

The device serves plain HTTP, so the cookie deliberately does not claim `Secure`.

Logout invalidates the server-side session and clears the cookie with:

`HttpOnly; SameSite=Strict; Path=/; Max-Age=0`

## Fail-closed drift guard

`scripts/check-v2-auth-policy.py` is now part of both the authoritative `scripts/check-all.sh` gate and `scripts/check-scripts.sh`.

It verifies that production code and host coverage retain:

- exactly eight active sessions;
- exactly 32 random bytes of session-token entropy;
- 86,400-second idle lifetime;
- 604,800-second absolute lifetime;
- five failures in a rolling 60-second window;
- 300-second lockout;
- bounded per-source throttle state;
- deterministic LRU session replacement;
- transactional replacement failure handling;
- source-aware production login throttling;
- fail-closed peer-address handling;
- required login/logout cookie attributes;
- reboot/reinitialization session invalidation coverage;
- per-source isolation and bounded-capacity coverage.

The guard also compares the auth-core lifetime constants to the centralized V2 limits contract so a future numeric drift fails CI.

## Regression coverage

The host auth suite now covers:

- 24-hour idle expiry;
- seven-day absolute expiry;
- periodic activity preserving a session only until the absolute deadline;
- idle-deadline refresh capped at the absolute deadline;
- malformed and wrong tokens;
- LRU replacement when the ninth session is created;
- preservation of the replaced session table when entropy generation fails;
- reboot/reinitialization invalidating RAM-only sessions;
- five-failure lockout;
- rolling-window expiry rather than a fixed window;
- exact lockout expiry and `Retry-After` behavior;
- source isolation;
- success clearing only the matching source;
- bounded throttle-table saturation;
- lock/unlock failure rollback.

## Ralph-loop findings

The first V2-042 candidate at `81073534ba3ce3ac7ac5ae0d74405a4766d16e51` failed Host Tests because the newly added absolute-expiry regression jumped directly from session creation to seven days minus one microsecond. The implementation correctly rejected that session because its 24-hour idle deadline had already elapsed. The test was repaired to perform periodic validations inside the idle window before asserting the absolute cap.

That failure was not bypassed or weakened.

A subsequent source review found a separate production failure window: with all eight slots full, LRU replacement cleared the selected session before random token generation. If entropy acquisition then failed, the new login failed while the old session was lost. Session creation now restores the pre-operation session table on any creation failure, and a regression proves the invariant.

## Commit trail

Major implementation commits include:

- `45edada856357f0970d795fbc6a609ca8791f2b3` — source-aware public auth API and dual-expiry session view;
- `16b1b48c705861ab0c757b869478ac0409f54b6b` — bounded session/rate state;
- `ab9b1cab87ea0af842581c0ab0b30acaf96ccc2f` — bounded state snapshot/rollback support;
- `fe00a95646df47b73e2cdce00ca0308f63eb9d29` — V2 session lifetime and LRU behavior;
- `8e2dc72cbe0fb2cce75d363b4b8c4a57532fbcf6` — per-source rolling lockout;
- `e68088ee6760e36872f6a88743dd8957dcf008f8` — source-aware production login integration;
- `13ea72ed2f998bc224f0628894bea799444bc39e` and `aabffbc66e116d403cc56b80563d4fae0f0150be` — primary V2 auth regressions;
- `f2a3dc8a504cd32d7bcba581c2e34312c524c2ab` — corrected absolute-expiry regression;
- `3a7b66980ea6a507336f94beb69b53079142254a` — transactional failed replacement;
- `25221dfcd3e9072c24de5175d483d4e58c1a6a87` — reboot and replacement-failure regressions;
- `c610a81a034f9a23af47bd25a4fe5cdcb359c36d` — permanent V2 auth policy guard;
- `58dba47d30c001fece913c78a2c6775070998d6d` and `b101e4af7cbc8d20a89800a987d097f7d30a363a` — authoritative gate integration.

## Validation boundary

On code SHA `25221dfcd3e9072c24de5175d483d4e58c1a6a87`, the complete Host Tests workflow passed, including ordinary native tests, ASan/UBSan, native coverage, frontend tests, and frontend coverage. Browser Tests also passed on that SHA.

The commit containing this report and the permanent auth-policy guard must still pass all four permanent workflows on one exact final SHA:

- Browser Tests;
- Host Tests;
- Device Test Build;
- Quality, including `Run authoritative checks`.

The TODO must not be marked complete until that exact-SHA gate is green. GitHub's immutable workflow records are the final CI evidence; another documentation-only commit is not required merely to copy run IDs into this report.

## Hardware-deferred boundary

No physical ESP32-S3R8 was used. V2-042's session/rate-limit semantics are deterministic software behavior and are testable on host, but this report does not claim:

- the V2-041 PBKDF2 timing benchmark;
- physical provisioning/reconnect behavior;
- physical Wi-Fi behavior;
- any Phase 4 hardware exit evidence.

## Completion statement

This is a software-completion candidate for V2-042 only. Until the final exact-SHA permanent CI gate passes, **V2-042 remains unchecked**. All hardware-dependent tasks elsewhere remain explicitly deferred.
