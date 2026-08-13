# H10-100 Authoritative Revalidation — 2026-08-12

## Purpose

This supplemental evidence record exists to obtain the authoritative H10-100 results that could not be produced in the local sandbox: the repository's real ESP-IDF/clang `run-clang-tidy` gate and the pinned `gcovr` native coverage gate.

## Permanent authoritative gates

The permanent repository workflows are the source of truth:

- `.github/workflows/quality.yml` runs `./scripts/check-all.sh`, including formatting/policy checks, native v2 contracts, setup/route synchronization, firmware GCC builds, ESP-IDF clang compile databases, first-party `run-clang-tidy` with warnings fatal, and the complete normal host suite.
- `.github/workflows/host-tests.yml` runs the complete normal host suite, the complete ASan+UBSan suite, and `scripts/generate-native-coverage.sh` using pinned `gcovr==8.6`. The coverage script enforces at least 90% line and 80% branch coverage on the pure-policy set.

## Authoritative iterations

Candidate `a58e9812ecd804a8e6b8b14f8a185138b2e587ed` established that the permanent Host Tests workflow was green, including pinned `gcovr==8.6`, while Quality exposed formatting defects before clang-tidy. Commit `9ee6f21804eae6357a723c2e7800598cf3f4fdbc` applied the authoritative formatter output.

Candidate `24c9e5d24f66f7174871194c61ca0963574ed634` then produced exact-SHA H10-100 evidence:

- Host Tests workflow run `31670534890`: normal host PASS, ASan+UBSan PASS, Native Coverage PASS;
- coverage-instrumented host tests: 66/66 PASS;
- pure-policy coverage: **96.2% lines (2566/2668)**, **100.0% functions (227/227)**, **82.8% branches (1870/2259)**, above the required 90% line / 80% branch thresholds;
- Quality workflow run `31670534732` passed formatting and reached the complete firmware clang-tidy scan. It analyzed all **97/97** firmware translation units and reported exactly one first-party diagnostic: `bugprone-easily-swappable-parameters` on the adjacent `app_v2_string_view_t` parameters of `change_password_verify_current()` in `web_settings.c`.

Commit `c6ac972badbbea0ac673ef0373b6fd9ef5c5d07f` fixes that diagnostic without suppression by separating the two same-typed parameters in the helper signature and updating its sole call site. The one-shot repair gate required the exact pre-fix source blob, authoritative `clang-format`, `git diff --check`, the normal web suite, and the ASan+UBSan web suite before committing; it removed itself in the same commit.

## Final candidate now under validation

This evidence-only checkpoint sits directly on top of product/code commit:

`c6ac972badbbea0ac673ef0373b6fd9ef5c5d07f`

It changes no firmware, host test, build, contract, coverage, analyzer, or permanent workflow implementation. The permanent Quality and Host Tests workflows must succeed on this exact checkpoint before H10-100 is closed.

## Final authoritative result

**H10-100 PASS / COMPLETE** on exact validation checkpoint
`910a8fd461fc8f9079cc99ffa450c1c4f76589eb`.

Permanent Host Tests run `31671332886` passed normal host, ASan+UBSan, and pinned
`gcovr==8.6` Native Coverage. Coverage-instrumented tests passed **66/66** and
the policy-enforced set measured **96.2% lines (2566/2668)**, **100.0%
functions (227/227)**, and **82.8% branches (1870/2259)**.

Permanent Quality run `31671332867` passed the H10-100 segment: static-analysis
policy, setup isolation, route synchronization, native contracts (**6/6**),
production and device-test firmware builds, and both fatal first-party
clang-tidy gates. `check-all.sh` then progressed through stack usage, release
budgets, frontend checks, and real-Chrome scenarios before failing later on
`shfmt` diffs in `tests/scripts/test-check-credential-logging.sh` and
`tests/scripts/test-test-assert-redaction.sh`.

That later script-format failure is recorded explicitly rather than hidden. It
is outside H10-100's six native/contract acceptance items, so H10-100 is closed
while the broader Phase H10 exit gate remains open.
