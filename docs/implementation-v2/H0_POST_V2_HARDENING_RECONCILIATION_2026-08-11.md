# H0 — Post-v2 hardening reconciliation closeout

**Date:** 2026-08-11  
**Phase:** `H0 — Baseline, evidence reconciliation, and failure inventory`  
**Historical H0 evidence:** `docs/implementation-v2/H0_POST_V2_HARDENING_BASELINE_AND_FAILURE_MATRIX_2026-08-10.md`  
**Current-master baseline before this documentation closeout:** `9632431fd08c26a2107cbff190cc4f3e09dde64b`  
**Reference board carried forward from the original H0 evidence:** ESP32-S3 QFN56 rev v0.2, 8 MiB embedded PSRAM, MAC `9c:13:9e:a8:77:38`

## 1. Purpose

This is an additive reconciliation of H0, not a rewrite of the historical H0 record. The 2026-08-10 H0 file already records the original hardening starting SHA (`78c356f35db252f15e37a7508a859b20517e45e0`), clean-checkout CI context, the required failure matrix, and the original `TODO_V2.md` reconciliation decisions. It explicitly left three things open: exact baseline-command results, the literal `TODO_V2.md` corrections, and final H0 checkbox reconciliation.

Since that file was written, later hardening phases changed some of the states that H0 originally inventoried. This document therefore records both the current baseline attempt and the current disposition of each historical failure row without pretending those defects were absent at H0 start.

No GitHub Actions job was monitored for this closeout; validation below is local unless explicitly identified as previously committed historical evidence.

## 2. Current source baseline and sandbox provenance

The user-provided archive was a GitHub `master` snapshot created after the original H9 closeout. During the preceding H9 Ralph loop, `master` advanced by four commits to `9632431fd08c26a2107cbff190cc4f3e09dde64b`. The current sandbox working tree is the uploaded archive plus the exact seven H9 follow-up files from that four-commit delta. The sandbox archive contains no `.git` directory, so local `git status` cannot truthfully be used as a cleanliness signal. The original H0 evidence's GitHub Actions baseline was a clean checkout; current remote `master` was independently verified at `9632431fd08c26a2107cbff190cc4f3e09dde64b` before these H0 documentation edits.

## 3. Tool/platform record

### Permanent/repository-pinned hardening environment

The repository and permanent workflows pin or install:

- Node.js **24.18.0** via `.nvmrc`;
- ESP-IDF **v5.5.5**, target `esp32s3`;
- Playwright **1.62.1** from `webapp/package-lock.json`, with the browser workflow installing its bundled Chromium/Chrome-for-Testing build;
- `cmakelang==0.6.13`;
- `yamllint==1.38.0`;
- `actionlint@v1.7.12`;
- `shfmt@v3.11.0`;
- `littlefs-python==0.15.0`;
- `gcovr==8.6` in the host coverage workflow;
- distro `clang-format`, `clang-tidy`, `shellcheck`, `jq`, and `libcjson-dev`.

The original H0 evidence also records Python 3.10.10 and the reference ESP32-S3 above from the immediately preceding V2 evidence. Older real-browser evidence recorded system Chromium 150.0.7871.128 before the project migrated the harness to Playwright's pinned bundled browser. The exact bundled browser build used by later Playwright CI was not written into the checked-in evidence, so this closeout does not invent one.

### Current sandbox

- Node.js **22.16.0** (does not satisfy `.nvmrc`);
- npm **10.9.2**;
- Python **3.13.5**;
- GCC/G++ **14.2.0**;
- Clang/Clang++ **17.0.0**;
- CMake/CTest **3.31.6**;
- Ninja **1.12.1**;
- runtime cJSON **1.7.18** is installed, but `libcjson-dev`/pkg-config metadata is absent;
- ESP-IDF/`idf.py`, `clang-tidy`, `clang-format`, `shellcheck`, `shfmt`, `actionlint`, `gcovr`, and `littlefs-python==0.15.0` are absent.

## 4. Exact H0 baseline-command attempts

All three commands required by H0-001 were invoked against the current-master sandbox before H0 documentation edits:

1. `./scripts/check-all.sh` — **exit 1 before product validation** because `IDF_PATH` is unset and ESP-IDF v5.5.5 is not installed. This is a sandbox toolchain blocker, not a repository check failure.
2. `./scripts/run-tests.sh --sanitizers` — the first invocation stopped at CMake configure because `pkg-config` could not resolve `libcjson`. A temporary, uncommitted sandbox-only `cJSON.h`/`libcjson.pc` shim was then pointed at the installed `/lib/x86_64-linux-gnu/libcjson.so.1.7.18`; with that development shim, the complete ASan+UBSan host suite passed **60/60**.
3. `./scripts/generate-native-coverage.sh` — **exit 1 before coverage generation** because `gcovr` is not installed.

The environment failures above are recorded separately rather than being converted into product failures or hidden with `|| true`/warning suppression.

## 5. Additional locally runnable baseline checks

With the same temporary cJSON development shim where required:

- `./scripts/run-tests.sh` — **60/60 passed**;
- `./scripts/run-tests.sh --sanitizers` — **60/60 passed** under ASan+UBSan;
- `python3 scripts/check-h9-architecture.py` — passed;
- `python3 scripts/check-v2-phase2-architecture.py` — passed;
- `bash scripts/check-production-config.sh` — passed;
- `bash scripts/check-credential-logging.sh` — passed;
- `bash tests/scripts/test-check-credential-logging.sh` — **16/16 passed**;
- `bash tests/scripts/test-test-assert-redaction.sh` — **3/3 passed**;
- mount, layer-boundary, removed-feature, frontend-persisted-state, setup-route-isolation, route/dispatch synchronization, auth-policy, USB-identity, partition, and device-settings-policy checks — passed;
- `bash scripts/check-v2-contracts.sh --native-only` — native contract CTest passed **6/6**;
- `python3 scripts/check-v2-034-capacity.py` — blocked before product validation because `littlefs-python==0.15.0` is unavailable.

The pinned-Node frontend gate, ESP-IDF firmware/clang-tidy gate, script lint/format gate, docs yaml gate, image/capacity gate, and gcovr coverage gate cannot be reproduced completely in this sandbox. H0 records those limitations; it does not downgrade their permanent requirements.

## 6. `TODO_V2.md` reconciliation applied

The literal ledger was updated instead of only documenting intended corrections:

- V2-154's compound login/logout/idle/absolute/lockout claim is split. Login/logout/lockout remain checked from hardware evidence; idle and absolute expiry remain explicitly open for independent hardware evidence while deterministic host coverage is retained.
- V2-154's historical no-secret hardware evidence remains identified as a spot-check, but the item can now truthfully remain checked because post-v2 H9 subsequently completed the full cross-cutting no-secret audit and permanent regression guards. The Phase-4 no-secret exit item is likewise updated to cite H9 rather than remaining stale-open.
- V2-061/Phase-6 evidence no longer says real HTTP sends categorically omit `requireSerialConfirmation`. H9 now reads authoritative device settings, binds `require_confirmation`, and fails closed on settings-read failure. H1 remains open for the complete real-API/browser/hardware acceptance matrix.
- V2-055/V2-154 password-change wording now distinguishes valid route/happy-path evidence from H2's still-open partial-commit/verifier/session-invalidation failure semantics.
- V2-153 factory-reset happy-path evidence now explicitly does not claim H3's interruption-safe/resumable destructive-reset semantics.

Historical implementation reports were not edited to retroactively claim coverage they did not have.

## 7. Historical failure-matrix disposition on current master

The original matrix remains the authoritative description of what was found at H0 start. Current status is:

| Historical ID | Current disposition at H0 reconciliation |
| --- | --- |
| F-H1-01 | **Implementation gap narrowed/fixed by H9:** real send construction reads authoritative confirmation settings and fails closed on read failure. **H1 remains open** for complete API/browser/hardware acceptance evidence. |
| F-H2-01 | **Open — H2.** Durable password vs RAM verifier coherence under refresh/activation failure is not closed by H0. |
| F-H2-02 | **Open — H2.** Durable password plus session-invalidation partial commit needs explicit transaction semantics and failure injection. |
| F-H3-01 | **Open — H3.** Reset after session-invalidation failure requires durable recovery semantics. |
| F-H3-02 | **Open — H3.** Blob-deletion failure during reset requires resumable cleanup/barrier semantics. |
| F-H3-03 | **Open — H3.** Power loss between destructive stages requires a durable reset state machine. |
| F-H4-01 | **Open — H4.** Recovery failure must not collapse to confirmed no-send. |
| F-H4-02 | **Open — H4.** Poll freshness/degradation needs explicit UI state. |
| F-H5-01 | **Open — H5.** Primary and cleanup storage errors still require generalized provenance treatment. |
| F-H5-02 | **Open — H5.** Post-rename parent-sync failure still requires explicit commit-uncertain/reconciliation semantics. |
| F-H6-01 | **Resolved by H6.** Worker-unavailable confirmation paths fail closed rather than synchronously blocking the server. |
| F-H6-02 | **Resolved by H7.** Safety-relevant release-all results are observed and release failure can fault-latch the executor. |
| F-H7-01 | **Resolved by H8** (historical matrix ID retained). Package-selection persistence failure is visible/retryable without falsely dirtying repository state. |
| F-H7-02 | **Resolved by H8** (historical matrix ID retained). Snapshot export failure is visible and does not mutate repository/selection/snapshot association. |

## 8. H0 disposition

H0 is complete as a **baseline/reconciliation phase**:

- the original starting SHA and clean-checkout context remain preserved;
- all three mandated baseline commands have a current-master result recorded, including environment-only blockers;
- the complete local host suite passes normally and under ASan+UBSan;
- the original failure matrix remains committed and has a current disposition table;
- every `TODO_V2.md` contradiction specifically identified by H0 has been reconciled literally against current evidence;
- no historical report was rewritten to manufacture evidence.

This H0 closeout does **not** claim the missing sandbox tools passed, does not substitute local host tests for hardware, and does not close H1-H5. The next implementation phase remains H1.
