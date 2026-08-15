# H12-121 — Complete authoritative gate evidence

- **Date:** 2026-08-14
- **Task:** H12-121 — Run complete authoritative gate
- **Validated candidate SHA:** `dd8bf489dc4f07bcaf3974bea181bd3bd589182e`
- **Validated candidate tree:** `07ac121d80a40fe170595a0cdf91e3cff2cb49a6`
- **Runner:** GitHub-hosted Ubuntu 24.04
- **Disposition:** complete

## Scope and result

H12-121 requires the complete authoritative software gate to run on one exact
release candidate, with all required commands exiting zero, no first-party
warning hidden or downgraded, and durable timing/count/margin evidence. Those
requirements are satisfied for
`dd8bf489dc4f07bcaf3974bea181bd3bd589182e`.

The exact candidate passed all three required authoritative commands:

```text
./scripts/check-all.sh
./scripts/run-tests.sh --sanitizers
./scripts/generate-native-coverage.sh
```

H12-122 (exact-SHA physical-device confirmation) and H12-123 (final post-v2
acceptance) remain open. This evidence does not infer either hardware acceptance
or final release acceptance from software-gate success.

## Exact CI evidence

### Quality / `./scripts/check-all.sh`

- Workflow: `Quality`
- Run: `31849836471`
- Job: `94923407015`
- Head SHA: `dd8bf489dc4f07bcaf3974bea181bd3bd589182e`
- Job conclusion: `success`
- `Run authoritative checks` conclusion: `success`
- Authoritative-check step start: `2026-08-14T23:22:45Z`
- Authoritative-check step completion: `2026-08-14T23:30:03Z`
- Authoritative-check step duration: **7m18s (438s)**

The Quality job checked out the exact SHA, installed the repository-pinned Node
and frontend dependencies, Playwright Chromium, host lint tooling, exact
ESP-IDF v5.5.5, and pinned LittleFS dependency before running
`./scripts/check-all.sh`. The authoritative step completed successfully.

### Host ASan + UBSan / `./scripts/run-tests.sh --sanitizers`

- Workflow: `Host Tests`
- Run: `31849836512`
- Job: `94923407111`
- Head SHA: `dd8bf489dc4f07bcaf3974bea181bd3bd589182e`
- Job conclusion: `success`
- `Run host tests with sanitizers` conclusion: `success`
- Command-step start: `2026-08-14T23:17:49Z`
- Command-step completion: `2026-08-14T23:18:36Z`
- Command-step duration: **47s**
- CTest result: **66/66 passed; 0 failed**
- CTest real test time: **1.99s**

The sanitizer build used the repository's normal warning-as-error host compiler
flags. The command exited zero and produced no sanitizer defect.

### Native coverage / `./scripts/generate-native-coverage.sh`

- Workflow: `Host Tests`
- Run: `31849836512`
- Job: `94923407154`
- Head SHA: `dd8bf489dc4f07bcaf3974bea181bd3bd589182e`
- Job conclusion: `success`
- `Generate native coverage and enforce gates` conclusion: `success`
- Command-step start: `2026-08-14T23:18:00Z`
- Command-step completion: `2026-08-14T23:18:37Z`
- Command-step duration: **37s**
- Coverage host tests: **66/66 passed; 0 failed**
- Pure-policy line coverage: **2930 / 3065 = 95.60%**
- Required line threshold: **90%**
- Line margin to failure: **+5.60 percentage points**
- Pure-policy branch coverage: **2106 / 2549 = 82.62%**
- Required branch threshold: **80%**
- Branch margin to failure: **+2.62 percentage points**
- Intended policy sources: **20/20 present and instrumented**

The strengthened coverage gate fails closed if an intended policy source is
missing or if its coverage instrumentation is absent, so these percentages
cannot silently pass by dropping one of the enumerated policy translation
units.

## Release-gate defects fixed before the final candidate

H12-121 did not simply reuse an older green run. The Ralph Loop found and fixed
two release-closure problems before accepting this exact candidate.

### Silent coverage narrowing

The prior pure-policy gcovr filter list still named retired
`firmware/components/auth/auth_core.c` and
`firmware/components/web_server/web_origin.c`. An unmatched gcovr filter does not
by itself fail, so intended policy coverage could be silently narrowed. Commit
`e22b10a3e70bdfb4da245fe685e1309fc5f933f0` replaced that fail-open behavior
with an explicit 20-source manifest, existence checks, instrumentation checks,
and a permanent regression test.

### Current-facing security/documentation drift

Current documentation still contained retired CSRF/Host/Origin/same-origin
claims and stale rebuild-stage wording. Commits
`e22b10a3e70bdfb4da245fe685e1309fc5f933f0` and
`dd8bf489dc4f07bcaf3974bea181bd3bd589182e` synchronized the live API, web
server, provisioning-security, and developer guidance with the normative v2
contract. `scripts/check-v2-auth-policy.py` now fails closed if those
current-facing claims drift back. Historical retired-v1 evidence was left intact.

Neither repair changed production firmware runtime behavior.

## Warning disposition

No first-party compiler, lint, analyzer, test, coverage, or project-policy warning
was ignored, suppressed, or downgraded to obtain these passes. The repository's
warning-as-error/static-analysis policies remained active.

The GitHub-hosted runner emitted a platform warning that `actions/checkout` still
targets the deprecated Node.js 20 action runtime and is being forced to Node.js
24. This is external GitHub Actions platform noise rather than a first-party
source warning. It was visible in the log, was not suppressed, and was not used
to relax or bypass any project gate.

## H12-121 disposition

H12-121 is complete for exact candidate
`dd8bf489dc4f07bcaf3974bea181bd3bd589182e`:

1. all three authoritative commands completed successfully on the exact SHA;
2. 66/66 sanitizer tests passed;
3. 66/66 coverage tests passed;
4. policy coverage cleared the 90% line and 80% branch gates with explicit
   +5.60/+2.62 percentage-point margins;
5. command-step timings are recorded for regression comparison; and
6. no first-party warning was hidden or downgraded to achieve the result.

The tracker/report closure commit that records this evidence is documentation
only. It does not replace the exact-SHA physical-device work required by H12-122
and does not pre-close H12-123.
