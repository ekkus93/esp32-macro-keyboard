# H9 — Ralph regression-hardening correction evidence

**Date:** 2026-08-11  
**Phase:** `H9 — Cross-cutting secret, fallback, and regression audit`  
**Historical H9 evidence:** `docs/implementation-v2/H9_CROSS_CUTTING_SECRET_FALLBACK_AUDIT_2026-08-11.md`  
**Primary correction SHA:** `1cc8553229e5cccfe23474b56b0fde9ec98d8a7d` (`fix: harden H9 secret regression guards`)  
**Follow-up sink-hardening SHA:** `ccedef8965cd249a03e212d1ed02ffed0860ff12` (`fix: cover low-level H9 logging sinks`)

## Purpose

This file is an additive correction to the original H9 closeout evidence. The original file accurately records the H9 implementation and CI runs that existed at the time, including the then-current 9-case credential-log regression. A subsequent Ralph-loop review found two residual defects in the **regression controls**. This correction records those findings and the stricter current validation without rewriting the historical run record.

## Additional findings

### 1. Credential-output guard had bypassable sink/data shapes

Production did not contain a corresponding secret leak at the post-close baseline, but `scripts/check-credential-logging.sh` could be bypassed by several straightforward future regressions:

- a secret passed to an ESP log call through a dynamic format string;
- a simple local alias of a secret passed to a log call;
- a secret passed to `puts`/related stdio output functions outside the original sink pattern;
- secret verifier/salt material passed to an ESP buffer-log macro;
- secret material passed through lower-level ESP-IDF/ROM logging functions (`esp_log_write`, `esp_log_buffer_*`, `esp_rom_printf`/`ets_printf`).

Primary correction `1cc8553229e5cccfe23474b56b0fde9ec98d8a7d` expands the guarded macro/stdio sink family, checks direct sensitive identifiers independent of the format literal, and propagates simple secret aliases within the enclosing C/C++ scope. Follow-up `ccedef8965cd249a03e212d1ed02ffed0860ff12` adds the lower-level ESP-IDF/ROM sinks (`esp_log_write*`, `esp_log_buffer_*`, `esp_rom_printf`, `ets_printf`). The committed negative regression suite increases from 9 to **16 cases** and covers all of these bypass classes.

### 2. Generic `TEST_CHECK(...)` still disclosed caller expressions

The earlier H9 change removed compared values from specialized string/integer/buffer assertion helpers, but generic `TEST_CHECK(expression)` still passed `#expression` to the failure reporter. A failing host test could therefore disclose a password/passphrase/token literal or secret-bearing identifier embedded in the assertion expression.

The correction replaces generic expression stringification with a fixed `condition` label. `scripts/check-h9-architecture.py` now rejects reintroduction of `#expression`, and `tests/scripts/test-test-assert-redaction.sh` dynamically compiles/runs failing probes to verify that a secret sentinel and a distinctive compared integer never appear in emitted failure text. `scripts/check-scripts.sh` runs this regression permanently.

## Current local validation

The correction was validated from the user-provided `master` archive without monitoring GitHub Actions:

- `python3 scripts/check-h9-architecture.py` — passed;
- `python3 scripts/check-v2-phase2-architecture.py` — passed;
- `bash scripts/check-credential-logging.sh firmware` — passed;
- `bash tests/scripts/test-check-credential-logging.sh` — **16/16 passed**;
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

## Second Ralph pass — post-H3 regression-control audit (2026-08-12)

A second Ralph-loop pass re-audited H9 after subsequent hardening work had landed on `master`. The user-provided local archive corresponded to `d11089e141490356ce93c369c73e1c07fc9820e8`. During local validation, `master` advanced to `dd8030e7c2f9dbbdf5098f06eab1ebf0433d5803` (`fix: clarify reset-settings partial completion`). That delta did not overlap the H9 guard/test files, but it did change production reset behavior, so its new immediate-reboot fallback was explicitly reviewed before publishing the second H9 correction at `48fe586f1c384d45fca65feb89f54bd41509ca13` (`fix: harden H9 regression audit guards`).

### Credential-output laundering bypasses

The existing credential-output scanner rejected direct/dynamic/aliased secret output but could still be bypassed by copying or formatting a secret into a neutral-named buffer before sending that buffer to a log sink. Two concrete probes passed the old checker: `snprintf(message, ..., password)` followed by `ESP_LOGI(..., message)`, and `memcpy(message, session_token, ...)` followed by the same sink. No corresponding production leak was found.

`48fe586f1c384d45fca65feb89f54bd41509ca13` propagates secret taint through `memcpy`, `memmove`, `strcpy`, `strncpy`, `strlcpy`, `strcat`, `strncat`, `snprintf`, and `sprintf`. The committed credential-output regression suite grows from 16 to **18 cases**, adding formatted-buffer and copied-buffer laundering probes. `scripts/check-scripts.sh` also now applies the credential checker directly to `tests/host` and `firmware/test_app` so test-output surfaces cannot quietly escape the same policy.

### Browser-console guard bypasses

The earlier browser-console prohibition only recognized bare `console.log/info/warn/error/debug(...)` shapes. It could miss qualified, bracket, or aliased access such as `window.console.error(...)`, `globalThis["console"]["warn"](...)`, and `const logger = console; logger.info(...)`. A broad text regex was intentionally rejected during this pass because it would falsely classify legitimate user-visible strings such as “serial console.”

The new `webapp/tests/v2-browser-console-prohibition.test.ts` uses the TypeScript AST instead. It scans `AppV2.tsx`, all `src/v2/**/*.{ts,tsx}`, and every `src/features/**/v2/**/*.{ts,tsx}`, rejects direct/qualified/bracket/aliased `console` references, and contains a positive regression proving ordinary string content containing the word `console` is allowed. A sandbox-side AST scan of **51 production V2 TS/TSX sources** found no offender. The actual Vitest suite could not be executed locally because the archive has no frontend dependencies and the sandbox Node version is 22.16.0 rather than the pinned 24.18.0.

### Permanent production classification guard

The earlier H9 mechanical audit was a point-in-time classification rather than a complete fail-closed regression gate. `48fe586f1c384d45fca65feb89f54bd41509ca13` adds `scripts/check-h9-production-audit.py` and wires it into `scripts/check-scripts.sh`. The guard rejects new or changed:

- empty `catch {}` blocks;
- empty Promise rejection handlers;
- `best-effort` markers;
- fallback variants including `fallback`, `fallbacks`, `fall back`, `falls back`, `fell back`, and `falling back`;
- explicit C `(void)callee(...);` result discards.

Every currently reviewed occurrence is an exact, count-bounded allowlist entry, so copy/paste duplication or wording changes force a new H9 classification. The guard’s own negative regression suite passes **7/7**.

The current reviewed inventory is **1 best-effort occurrence, 13 fallback occurrences, and 18 explicit discarded C calls**, with zero empty catches and zero empty Promise catches. Newly surfaced wording was classified rather than hidden:

- provisioning station failure intentionally leaves AP service available; the failure path is warning-logged and the bounded station engine retains `WIFI_STATION_FAILED`, so this is not silent success;
- two snapshot-client fallback references explicitly state that unsafe persistence fallbacks are forbidden;
- the reset scheduler fallback introduced by `dd8030e7c2f9dbbdf5098f06eab1ebf0433d5803` immediately reboots when delayed restart ownership cannot be established after durable state may already have changed, then returns an internal error if `esp_restart()` unexpectedly returns; this is fail-safe behavior, not a swallowed failure;
- the single `best-effort` wording is likewise a negative statement that login must not re-read NVS as an unsafe cache refresh.

### Local validation for the second pass

On the user-provided `d11089e141490356ce93c369c73e1c07fc9820e8` archive plus the H9 patch:

- complete native host suite — **66/66 passed**;
- complete native host suite under ASan+UBSan — **66/66 passed**;
- `python3 scripts/check-h9-architecture.py` — passed;
- `python3 scripts/check-v2-phase2-architecture.py` — passed;
- `python3 scripts/check-h2-architecture.py` — passed;
- `python3 scripts/check-h3-architecture.py` — passed;
- `python3 scripts/check-h9-production-audit.py` — passed on the archive with its 12 then-present fallback occurrences;
- `bash tests/scripts/test-check-h9-production-audit.sh` — **7/7 passed**;
- firmware, host-test, and device-test credential-output checks — passed;
- `bash tests/scripts/test-check-credential-logging.sh` — **18/18 passed**;
- `bash tests/scripts/test-test-assert-redaction.sh` — **3/3 passed**;
- browser-console TypeScript AST scan — **51 production sources clean**;
- `bash -n` on changed shell scripts, Python byte-compilation, and diff whitespace checks — passed.

The exact final `48fe586f1c384d45fca65feb89f54bd41509ca13` product tree was not rebuilt in the sandbox because `master` advanced after the native validation run. The intervening H3 delta was reviewed for H9 scope and did not overlap the six H9 code/test files. Frontend npm/Vitest/build checks were not locally runnable for the Node/dependency reasons above. `actionlint`, `shellcheck`, and `shfmt` remain unavailable in the sandbox. GitHub Actions was intentionally not monitored, per the user’s Ralph-loop workflow.

### Disposition

No new production credential disclosure or critical silent failure was found. The second pass did find real weaknesses in the **regression controls**, and those are now fail-closed: secret laundering through common copy/format operations, browser-console alias forms, and newly introduced fallback/discard patterns all have committed negative tests. H9 remains complete on the reviewed scope; H10 and the remaining hardware/release gates remain separate and are not claimed here.
