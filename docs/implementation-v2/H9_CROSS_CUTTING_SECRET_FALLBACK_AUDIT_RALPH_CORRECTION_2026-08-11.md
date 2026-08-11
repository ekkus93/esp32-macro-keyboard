# H9 — Ralph regression-hardening correction evidence

**Date:** 2026-08-11  
**Phase:** `H9 — Cross-cutting secret, fallback, and regression audit`  
**Historical H9 evidence:** `docs/implementation-v2/H9_CROSS_CUTTING_SECRET_FALLBACK_AUDIT_2026-08-11.md`  
**Correction SHA:** `1cc8553229e5cccfe23474b56b0fde9ec98d8a7d` (`fix: harden H9 secret regression guards`)

## Purpose

This file is an additive correction to the original H9 closeout evidence. The original file accurately records the H9 implementation and CI runs that existed at the time, including the then-current 9-case credential-log regression. A subsequent Ralph-loop review found two residual defects in the **regression controls**. This correction records those findings and the stricter current validation without rewriting the historical run record.

## Additional findings

### 1. Credential-output guard had bypassable sink/data shapes

Production did not contain a corresponding secret leak at the post-close baseline, but `scripts/check-credential-logging.sh` could be bypassed by several straightforward future regressions:

- a secret passed to an ESP log call through a dynamic format string;
- a simple local alias of a secret passed to a log call;
- a secret passed to `puts`/related stdio output functions outside the original sink pattern;
- secret verifier/salt material passed to an ESP buffer-log macro.

Correction `1cc8553229e5cccfe23474b56b0fde9ec98d8a7d` expands the guarded sink family, checks direct sensitive identifiers independent of the format literal, and propagates simple secret aliases within the enclosing C/C++ scope. The committed negative regression suite increases from 9 to **13 cases** and explicitly covers the four bypass classes above.

### 2. Generic `TEST_CHECK(...)` still disclosed caller expressions

The earlier H9 change removed compared values from specialized string/integer/buffer assertion helpers, but generic `TEST_CHECK(expression)` still passed `#expression` to the failure reporter. A failing host test could therefore disclose a password/passphrase/token literal or secret-bearing identifier embedded in the assertion expression.

The correction replaces generic expression stringification with a fixed `condition` label. `scripts/check-h9-architecture.py` now rejects reintroduction of `#expression`, and `tests/scripts/test-test-assert-redaction.sh` dynamically compiles/runs failing probes to verify that a secret sentinel and a distinctive compared integer never appear in emitted failure text. `scripts/check-scripts.sh` runs this regression permanently.

## Current local validation

The correction was validated from the user-provided `master` archive without monitoring GitHub Actions:

- `python3 scripts/check-h9-architecture.py` — passed;
- `python3 scripts/check-v2-phase2-architecture.py` — passed;
- `bash scripts/check-credential-logging.sh firmware` — passed;
- `bash tests/scripts/test-check-credential-logging.sh` — **13/13 passed**;
- `bash tests/scripts/test-test-assert-redaction.sh` — **3/3 passed**;
- `./scripts/run-tests.sh web` — **29/29 passed**;
- `./scripts/run-tests.sh startup` — **2/2 passed** for the current local startup CTest label;
- `./scripts/run-tests.sh --sanitizers web` — **29/29 passed** under ASan+UBSan;
- `./scripts/run-tests.sh --sanitizers startup` — **2/2 passed** under ASan+UBSan;
- `bash -n` on every changed shell script — passed;
- `python3 -m py_compile scripts/check-h9-architecture.py` — passed;
- production mechanical inventory — zero empty catches, zero empty Promise catches, zero production best-effort markers; the four production `fallback` text hits remain the already-classified gzip/routing/host-portability cases from the original H9 evidence.

## Sandbox limitations

The container has Node **22.16.0**, while the frontend is pinned with `engine-strict=true` to Node **24.18.0**. The local archive also has no installed frontend dependencies. Frontend npm/lint/Vitest/build checks therefore were not rerun locally; the earlier H9 frontend CI results remain historical evidence only.

`actionlint`, `shellcheck`, and `shfmt` are not installed in the sandbox, so `scripts/check-scripts.sh` could not be executed end-to-end even though the changed shell regressions and shell syntax checks passed individually.

The runtime `libcjson.so.1` 1.7.18 library is installed but its development package/pkg-config metadata is absent. Native host validation used a temporary sandbox-only header/pkg-config shim targeting that installed runtime. No shim or dependency workaround is committed to the repository.

## H9 disposition after correction

The original production fallback/ignored-result classifications remain valid. The Ralph correction closes the two newly identified regression-control gaps. No new production secret leak was found, and no known critical silent failure from H9 remains after the stricter guards and executable redaction regression.

This correction does not claim completion of work explicitly assigned elsewhere, including H5 storage provenance, H1 remaining browser/hardware confirmation evidence, or H10 final real-browser/device release-candidate gates.
