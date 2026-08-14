# H12-121 — Complete authoritative gate

- **Date:** 2026-08-14
- **Task:** H12-121 — Run complete authoritative gate
- **Starting master SHA:** `bb83e9b73dd337d91633d1fddc7fea9d3de635fb`
- **Status:** in progress; exact-SHA authoritative rerun required after the H12-121 gate repair

## Required closure commands

H12-121 closes only when one exact candidate SHA passes all three repository commands:

```text
./scripts/check-all.sh
./scripts/run-tests.sh --sanitizers
./scripts/generate-native-coverage.sh
```

The final evidence must also record timings, test counts, coverage/budget margins, and confirm that
warnings or failures were not ignored or downgraded.

## Clean-sandbox preflight

The independently supplied clean source archive is the exact executable-source basis of starting
SHA `bb83e9b73dd337d91633d1fddc7fea9d3de635fb`: the two commits after the archive basis changed
only documentation. The local sandbox does not contain the repository's full Ubuntu 24.04 toolchain,
so its results below are preflight evidence rather than authoritative closure evidence.

### ASan and UBSan

The literal sanitizer command initially stopped during CMake configuration because the sandbox has
cJSON 1.7.18's runtime library but not its development header/pkg-config metadata. An external,
uncommitted sandbox-only header/pkg-config shim was pointed at the installed runtime solely to let
project code execute. No repository file or dependency pin was changed for that workaround.

With that diagnostic shim:

- `./scripts/run-tests.sh --sanitizers` passed **66/66** host test executables;
- CTest real test time was **1.94 seconds**;
- configure/build/test elapsed time was **14.25 seconds**;
- maximum resident set size reported by `/usr/bin/time` was **331832 KiB**; and
- the captured build/test log contained zero `warning:` and zero `error:` lines.

This is useful regression evidence but is not accepted as the final H12-121 dependency environment.

### Coverage preflight

The sandbox does not have the pinned `gcovr==8.6`, so the literal
`./scripts/generate-native-coverage.sh` correctly failed before generation with:

```text
gcovr is required for native coverage generation
```

The underlying `./scripts/run-tests.sh --coverage` build/test path was still exercised with the same
external cJSON development shim and passed **66/66** tests. CTest real test time was **1.49 seconds**
and configure/build/test elapsed time was **6.26 seconds**.

GCC 14's native JSON gcov data was independently aggregated as a diagnostic cross-check. Before the
gate repair, the script's effective policy set reproduced the previously recorded result closely:
**96.18% line / 82.78% branch**. That inspection exposed a fail-open coverage-list defect described
below.

## H12-121 defect: stale pure-policy coverage filters

`./scripts/generate-native-coverage.sh` contained two policy filters that matched no current source
file:

- `firmware/components/auth/auth_core.c` — auth was split into
  `auth_core_common.c`, `auth_core_password.c`, `auth_core_session.c`, and
  `auth_core_rate_limit.c`;
- `firmware/components/web_server/web_origin.c` — no current source file exists at that path.

The stale auth filter had already been noted in
`docs/implementation-v2/V2_044_WIFI_AND_RESET_SEMANTICS_2026-08-08.md` and deliberately left for a
later task. A gcovr filter that matches nothing is not itself an error, so retaining stale entries
silently narrowed the enforced policy set. That behavior is not acceptable for the final fail-closed
release gate.

The H12-121 repair:

1. replaces the obsolete auth aggregate entry with all four current auth-core translation units;
2. removes the obsolete `web_origin.c` entry;
3. centralizes the intended policy files in `pure_policy_files`;
4. fails before coverage work if any listed policy source does not exist;
5. fails after the coverage build if any listed source produced no `.gcno` instrumentation; and
6. generates the gcovr filter arguments from that validated list instead of maintaining a second
   independent list.

`tests/scripts/test-native-coverage-policy.sh` proves all three important states: missing source
fails, existing-but-uninstrumented source fails, and a complete source/instrumentation set proceeds.
The regression is wired into `scripts/check-scripts.sh`.

After widening the policy set to all **20** intended current source files, the same raw GCC coverage
data reports:

- lines: **2930 / 3065 = 95.60%**, margin **+5.60 percentage points** over the 90% gate;
- branches: **2106 / 2549 = 82.62%**, margin **+2.62 percentage points** over the 80% gate; and
- missing listed policy sources: **0**.

The authoritative candidate must reproduce passing margins with the repository-pinned gcovr 8.6
command; these raw-GCC numbers are diagnostic evidence only.

## Documentation discrepancy found during the gate audit

The normative `docs/SPEC_V2.md` development-appliance authentication model disables CORS, uses an
`HttpOnly` / `SameSite=Strict` session cookie, has no separate CSRF token, and performs no
Host/Origin check. Four current-product documents still described some part of the retired
security model:

- `docs/API.md` said mutating v2 routes required a matching CSRF token and accepted Host/Origin;
- `firmware/components/web_server/README.md` claimed same-origin checks and also listed many current
  v2 routes as unimplemented;
- `CLAUDE.md`, the live developer guide, still summarized `web_server` as owning same-origin checks;
  and
- `docs/PROVISIONING_SECURITY.md` said setup submission required matching Host and Origin values.

All four current-facing documents are synchronized by the H12-121 candidate.
`scripts/check-v2-auth-policy.py` now asserts that the API documentation, web-server README, live
developer guide, and provisioning-security guide remain aligned with the normative v2
development-appliance policy. The same live-guide audit also corrected stale `CLAUDE.md` status
wording ("mid v1→v2 rebuild" despite final release hardening) and refreshed its host-test/vitest
file counts to the current **64** `test_*.c` files / **26** `.inc` fragments and **49** frontend
Vitest files. Historical v1/FIX1 snapshots that explicitly identify themselves as retired are left
intact rather than rewriting their historical record.

This is also a correction to the breadth of the prior H11 documentation closure: H11's narrative
reconciliation was valid for the documents it inspected, but these four stale current-product
statements survived that pass. They are corrected before final release rather than being silently
ignored.

## Other offline quality preflight

The following checks passed from the clean source tree during this H12-121 pass:

- static-analysis exception policy;
- partition table policy;
- v2 device-settings persistence policy;
- production NVS/HMAC configuration policy;
- credential logging policy;
- mount policy;
- layer boundaries;
- removed-feature policy;
- v2 phase-2 architecture policy;
- USB identity policy;
- frontend persisted-state policy;
- setup-route isolation policy;
- web route/dispatch synchronization;
- v2 authentication policy; and
- native v2 contracts, **6/6**.

The local V2-034 capacity check could not execute because `littlefs-python==0.15.0` is not installed
in this network-isolated sandbox. `./scripts/check-all.sh` also cannot become authoritative locally
because ESP-IDF 5.5.5 is not installed/exported and the sandbox lacks the repository-pinned Node and
lint toolchain. Those environment gaps are not bypassed or downgraded.

## Closure boundary

H12-121 remains open after this repair. The replacement exact master SHA must pass all three literal
closure commands in the documented Ubuntu 24.04/pinned-toolchain environment. The local cJSON shim,
raw-gcov parser, or partial policy checks must not be cited as substitutes for that result.

H12-122 and H12-123 remain open and receive no inferred acceptance from this preflight.
