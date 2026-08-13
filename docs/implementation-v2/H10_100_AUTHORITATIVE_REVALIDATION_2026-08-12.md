# H10-100 Authoritative Revalidation — 2026-08-12

## Purpose

This supplemental evidence record exists to obtain the two authoritative H10-100 results that could not be produced in the local sandbox: the repository's real ESP-IDF/clang `run-clang-tidy` gate and the pinned `gcovr` native coverage gate.

## Permanent authoritative gates

The permanent repository workflows are the source of truth:

- `.github/workflows/quality.yml` runs `./scripts/check-all.sh`, which includes format/policy checks, native v2 contracts, setup/route synchronization, firmware GCC builds, ESP-IDF clang compile databases, first-party `run-clang-tidy` with warnings fatal, frontend/script/doc checks, and the complete normal host suite.
- `.github/workflows/host-tests.yml` runs the complete normal host suite, the complete ASan+UBSan suite, and `scripts/generate-native-coverage.sh` using pinned `gcovr==8.6`. The coverage script enforces at least 90% line and 80% branch coverage on the pure-policy set.

## First authoritative candidate

Candidate `a58e9812ecd804a8e6b8b14f8a185138b2e587ed` revalidated the code state through `9f6f1d9abadc4d51f7ad943236130d828e724605`.

Host Tests workflow run `31669874246` passed its H10-100 jobs:

- complete normal host suite: PASS;
- complete ASan+UBSan host suite: PASS;
- pinned `gcovr==8.6` Native Coverage: PASS;
- pure-policy coverage: 96.2% lines (2566/2668), 98.1% functions (550/561), 82.8% branches (1870/2259), above the required 90% line / 80% branch thresholds.

Quality workflow run `31669874359` did not reach clang-tidy. Its uploaded authoritative failure log showed only clang-format violations in the newly extracted `change_password_verify_current()` helper and its call site in `firmware/components/web_server/web_settings.c`.

The format defect was repaired by `9ee6f21804eae6357a723c2e7800598cf3f4fdbc` using the same Ubuntu `clang-format` package used by Quality. The one-shot formatter workflow asserted the pre-repair source blob, ran `clang-format -i`, required `clang-format --dry-run --Werror` to pass afterward, ran `git diff --check`, committed the formatted source, and removed itself.

## Final candidate now under validation

This evidence-only checkpoint sits directly on top of formatted product/code commit:

`9ee6f21804eae6357a723c2e7800598cf3f4fdbc`

It changes no firmware, host test, build, contract, coverage, analyzer, or permanent workflow implementation. The permanent Quality and Host Tests workflows must both succeed on this exact checkpoint before H10-100 is closed.

## Status

Authoritative revalidation is **PENDING** on this exact evidence candidate. No H10-100 checkbox is claimed complete by this file until the corresponding permanent workflow/jobs complete successfully and the exact final results are recorded in this file and the primary H10-100 evidence report.
