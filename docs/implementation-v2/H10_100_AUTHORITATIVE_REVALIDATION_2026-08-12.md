# H10-100 Authoritative Revalidation — 2026-08-12

## Purpose

This supplemental evidence record exists to obtain the two authoritative H10-100 results that could not be produced in the local sandbox: the repository's real ESP-IDF/clang `run-clang-tidy` gate and the pinned `gcovr` native coverage gate.

The product/code state immediately before this evidence-only commit is:

`9f6f1d9abadc4d51f7ad943236130d828e724605`

That commit contains the last H10-100 clang-tidy repair (`web_change_password_handle()` complexity reduction). This evidence-only commit changes no firmware, host test, build, contract, coverage, analyzer, or workflow implementation.

## Authoritative gates

The permanent repository workflows are the source of truth:

- `.github/workflows/quality.yml` runs `./scripts/check-all.sh`, which includes format/policy checks, native v2 contracts, setup/route synchronization, firmware GCC builds, ESP-IDF clang compile databases, first-party `run-clang-tidy` with warnings fatal, frontend/script/doc checks, and the complete normal host suite.
- `.github/workflows/host-tests.yml` runs the complete normal host suite, the complete ASan+UBSan suite, and `scripts/generate-native-coverage.sh` using pinned `gcovr==8.6`. The coverage script enforces at least 90% line and 80% branch coverage on the pure-policy set.

## Status

Authoritative revalidation is **PENDING** on this exact evidence candidate. No H10-100 checkbox is claimed complete by this file until the corresponding workflow/job has completed successfully and the exact results are recorded here and in the primary H10-100 evidence report.
