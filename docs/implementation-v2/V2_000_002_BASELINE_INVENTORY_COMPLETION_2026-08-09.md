# V2-000/V2-001/V2-002 completion pass (2026-08-09)

**Scope:** Close out the remaining Phase 0 gaps identified in
[`V2_AUDIT_PHASE_0_1_2_2026-08-09.md`](V2_AUDIT_PHASE_0_1_2_2026-08-09.md):
V2-000 (starting-state record), V2-001 (code inventory), V2-002 (migration-map
completeness), and the Phase 0 exit gate. Builds on, and does not replace,
[`PHASE_0_BASELINE_2026-08-03.md`](PHASE_0_BASELINE_2026-08-03.md) and
[`V2_MIGRATION_MAP.md`](V2_MIGRATION_MAP.md), which remain the primary Phase 0
documents. This report only adds what those two did not already establish.

**Commit at time of this work:** `e703ff2d3709f51eb2b1a51d9ea56fac783e15f7`
(worktree branch `worktree-agent-a479c35119d79a4b6`).

**Method:** direct git/GitHub-history inspection for the historical items
(V2-000), direct `find`/`wc`/test-suite execution against the current tree for
the present-tense items (V2-001), and direct `grep`/source reading for the
sweep and traceability items (V2-002). No number below is inferred or copied
from a prior report without independent re-verification in this session.

---

## V2-000 — Record the starting state

### Starting commit SHA — confirmed

`ad75859f56986a81a2faf01832008b69b26a94e1` ("Add authoritative v2
implementation TODO"), already recorded in
`PHASE_0_BASELINE_2026-08-03.md:5` and `V2_MIGRATION_MAP.md:4`. Reconfirmed
this session:

```bash
git cat-file -t ad75859f56986a81a2faf01832008b69b26a94e1   # commit
git log --oneline --reverse ad75859f..HEAD | head -3
  # 1697731 Add v2 migration map
  # e2cb663 Record v2 Phase 0 baseline
  # (first two commits after the baseline SHA — exactly the two Phase 0 docs)
```

`ad75859f` has a single parent (`19db66b`, "Finalize synchronized v2 spec
authority") — an ordinary, non-merge commit, immediately followed by the two
Phase 0 doc-only commits. No box changed here; this just reconfirms the
existing `[x]`.

### Toolchain versions — now checked, with real evidence

Previously left unchecked because the toolchain was never independently
re-verified. Two independent facts close this out honestly:

1. **The toolchain pin has not changed since the starting commit.** Byte-for-byte
   diff across the baseline commit and current `HEAD`:

   ```bash
   git diff ad75859f HEAD -- scripts/verify-toolchain.sh   # empty
   git log --oneline ad75859f..HEAD -- scripts/verify-toolchain.sh   # empty
   git show ad75859f:.nvmrc            # 24.18.0  (matches current .nvmrc)
   git show ad75859f:webapp/package.json | grep -A2 engines
     # "node": "24.18.0"               (matches current webapp/package.json)
   ```

   So whatever was true about the pinned versions at the starting commit is
   still true today — the requirement itself (`v5.5.5`/`esp32s3`/`24.18.0`,
   enforced by `scripts/verify-toolchain.sh`) is unchanged.

2. **The pinned versions are actually present in this environment**, confirmed
   directly rather than assumed:

   ```bash
   git -C "$HOME/esp/esp-idf-v5.5.5" describe --tags        # v5.5.5
   "$HOME/.nvm/versions/node/v24.18.0/bin/node" --version   # v24.18.0
   ```

   `scripts/verify-toolchain.sh` itself (unchanged since the baseline, per
   above) could not be executed end-to-end in this sandboxed worktree — it
   requires `source`-ing ESP-IDF's `export.sh`, which this worktree-isolated
   agent's sandbox refuses to run regardless of redirection — but every value
   it checks was confirmed directly by the two commands above, which is the
   same information the script would have printed.

`IDF_TARGET=esp32s3` is a build-time environment variable requirement, not a
stateful fact to record; `scripts/verify-toolchain.sh` and every firmware
script in this repo hard-fail on any other target.

**Checkbox closed** — see `docs/TODO_V2.md`.

### `./scripts/check-all.sh` — still never executed, now with much stronger evidence of why

This cannot be honestly checked, and new evidence gathered this session makes
that conclusion firmer, not just repeated:

- Locally: `PHASE_0_BASELINE_2026-08-03.md` §5 already documented this was
  never run in the connector-only environment that produced that report.
- **In CI, at the exact starting commit**, this session pulled the real
  GitHub Actions history (this environment has network access and an
  authenticated `gh`, unlike the connector-only environment the original
  baseline doc and the 2026-08-09 audit ran in):

  ```bash
  gh api repos/ekkus93/esp32-macro-keyboard/commits/ad75859f.../check-runs
  ```

  Five workflows ran against `ad75859f` on 2026-08-03 (run IDs below). The
  **Quality** workflow (`.github/workflows/quality.yml`, run `30852290318`,
  job `91814925270`) is the one that invokes `./scripts/check-all.sh` — but
  it failed at an *earlier* step, **"Enforce reviewed npm audit policy"**
  (`npm run audit:ci`), before the `Run authoritative checks` step (the one
  that actually runs `check-all.sh`) ever started. Confirmed directly from
  the job log (`gh api .../actions/jobs/91814925270/logs`):

  ```text
  npm audit policy failed: npm audit contains unreviewed non-high findings: postcss
  ##[error]Process completed with exit code 1.
  ```

  and from the job step list itself: `Run authoritative checks` shows as
  skipped (`-`), never started. So `check-all.sh` was not executed **at that
  commit, in CI, at the time**, for a reason independent of anything
  `check-all.sh` itself checks — confirming, from a second independent
  source (GitHub's own historical CI record, not just the absence of a local
  run), that this box was never satisfiable retroactively and genuinely
  wasn't satisfied at the time either.

**Left unchecked** — correctly, and now for a fully evidenced reason rather
than an environment limitation.

### Record every current failure without weakening a gate — partially known, box stays unchecked

What is now known, from the same CI history (all times 2026-08-03, commit
`ad75859f`):

| Workflow | Run | Result | Detail |
| --- | --- | --- | --- |
| Quality (`quality.yml`) | 30852290318 | **Failed** | Failed at `npm run audit:ci`: "npm audit policy failed: npm audit contains unreviewed non-high findings: postcss". `check-all.sh` itself never started. |
| Host Tests / Native Coverage / Frontend Coverage / Host ASan+UBSan / Frontend Tests (`host-tests.yml`) | 30852290270 | All 5 jobs passed | See test-count table below. |
| Device Test Build (`device-tests-build.yml`) | 30852290258 | Passed | Builds `firmware/test_app` only, not `firmware/main`. |
| Real Chrome Workflows (browser tests) | 30852290256 | Passed | Log: "Real Chrome Phase 17.10 workflows passed." (no itemized count found in the log) |
| Publish latest workflow status | several | Mixed (status-badge publisher, re-triggered by the other runs; not a substantive check) | Not evidence of any gate result. |

So **the one substantive failure that is knowable for this commit** is the
npm audit policy failure above. But `check-all.sh` comprises roughly 21
constituent checks (toolchain, formatting, static analysis, partitions,
production config, credential logging, mount policy, layer boundaries,
removed-feature checks, USB identity, frontend persisted-state, setup route
isolation, firmware build + clang-tidy, stack usage, webfs image build, flash
manifest, release budgets, frontend checks, script checks, doc checks, native
host tests — per `PHASE_0_BASELINE_2026-08-03.md` §5) and **none of those ran
at this commit**, in CI or locally. Their pass/fail status at the starting
commit is genuinely unknown, not "assumed passing." Claiming "every current
failure" is recorded would be false — only one failure (npm audit,
pre-`check-all.sh`) is actually known; the rest of the gate's status is an
honest blank, not a zero.

**Left unchecked** — for a materially better-documented reason than before.

### Test counts, firmware build status, webfs size, app image size, userdata partition size — partially recovered, box stays unchecked

From the same CI logs (`91814925693` Host Tests, `91814925797` Frontend
Tests, `91814925650` Frontend Coverage, `91814925626` Native Coverage,
`91814925655` Host ASan/UBSan, `91814925161` Device Test Build), all for
commit `ad75859f`, 2026-08-03:

| Item | Value at starting commit | Source |
| --- | --- | --- |
| Host CTest suites | **52/52 passed, 0 failed** (`100% tests passed, 0 tests failed out of 52`) | Host Tests job log |
| Host CTest suites (ASan+UBSan) | **52/52 passed** | Host ASan/UBSan job log |
| Frontend Vitest | **18 files / 133 tests, all passed** | Frontend Tests + Frontend Coverage job logs (identical figures in both) |
| Frontend line/branch/function/statement coverage | 78.09% / 77.24% / 80.6% / 78.34% (`All files` row) | Frontend Coverage job log |
| Native host coverage (all files) | lines 87.8% (6360/7240), branches 70.7% (4294/6077) | Native Coverage job log |
| Native host coverage (policy files only) | lines 95.4% (1543/1618), branches 83.4% (1167/1400) | Native Coverage job log |
| `firmware/test_app` (on-device Unity test app) build | **Succeeded** — `esp32_macro_keyboard_device_tests.bin` 0x2f200 bytes (193,024 B), 82% of its app partition free | Device Test Build job log |
| `firmware/main` (production app) build | **Unknown** — never built at this commit; `check-firmware.sh` is inside `check-all.sh`, which never ran (see above); the Device Test Build workflow builds only `firmware/test_app` | — |
| webfs image size | **Unknown** — `scripts/build-webfs-image.sh` runs inside `check-all.sh`, which never ran at this commit | — |
| Production app image size | **Unknown** — same reason | — |
| `userdata` partition size | **512 KiB** (`0x80000`), static from `firmware/partitions.csv` at `ad75859f` — unchanged at current `HEAD` | `git show ad75859f:firmware/partitions.csv` |
| `webfs` partition size (declared, not built size) | 1 MiB (`0x100000`) | Same |

Two of the six required facts (webfs *generated* size, production app *built*
size) are genuinely unrecoverable — they were never produced at this commit,
by any process, at any time. The other four are now recovered with real
contemporaneous evidence where the original baseline report had none. Because
the composite checkbox requires all of the listed facts and two remain
unknown, **it stays unchecked**, but the gap is now precisely bounded instead
of blanket "not executed."

### Confirm no uncommitted user work is mixed into the rebuild baseline — investigated, stays unchecked

This item is inherently about the state of a person's local working directory
at the moment the baseline was established (2026-08-03) — not something git
history can prove after the fact, since an uncommitted diff by definition
leaves no trace once work continues. What *is* checkable from history was
checked:

- `ad75859f` is a normal, single-parent, non-merge commit — not a squash of a
  branch that could hide intermediate uncommitted state.
- `git log --merges 19db66b..ad75859f` — empty; the commit immediately before
  the baseline is a direct linear ancestor, not a merge.
- GitHub's CI checked out this exact commit standalone (no working-directory
  dependency) and 4 of 5 workflows completed fully using nothing but the
  committed tree — corroborating that the commit's tree was self-sufficient,
  though this is not equivalent to confirming the *implementer's local
  worktree* was clean at that moment, which remains unrecoverable.
- `git fsck --unreachable --no-reflogs` in the current clone shows dangling
  objects, but this is ordinary repository churn (amended commits, abandoned
  branches across the project's many active worktrees) with no date
  correlation to 2026-08-03 specifically; it is not evidence either way about
  the baseline moment and was not treated as such.

**Left unchecked** — genuinely not verifiable now, consistent with the
original baseline doc's own §7 deferral and the 2026-08-09 audit's finding.

### V2-000 summary

One of five previously-open sub-items closed (toolchain versions/confirmation).
The remaining four stay open, each now backed by concrete evidence of *why*
they cannot be closed rather than an unexplored gap — this is treated as the
correct, non-fabricated outcome per this task's brief and
`docs/TODO_V2.md` §0.1.

---

## V2-001 — Build a complete code inventory

All counts below were measured directly against commit `e703ff2` in this
worktree this session (`find`, `wc -l`, and the two real test-suite runs
below), not copied from an earlier report.

### Native host tests (`tests/host/`)

```bash
find tests/host -maxdepth 1 -name 'test_*.c' | wc -l   # 53
find tests/host -maxdepth 1 -name '*.inc' | wc -l       # 26
find tests/host/fakes -type f | wc -l                   # 27 (.c/.h backend fakes)
find tests/host/support -type f | wc -l                 # 12 (assert harness + test utilities)
./scripts/run-tests.sh                                  # 54/54 CTest suites, 100% passed, 2.41s
```

- 53 top-level `test_*.c` files register 54 CTest labels (one file,
  `test_device_settings_repository_isolation.c`, plus the rest, map to more
  CTest entries than files in a couple of cases via multiple `add_test`
  registrations per file — see `tests/host/CMakeLists.txt`).
- 26 `.inc` fragments, all included by a parent `test_*.c` rather than built
  standalone: 5 auth fragments (`auth_*_tests.inc`, `auth_test_fixture.inc`),
  6 executor fragments (`executor_*_tests.inc`, `executor_test_fixture.inc`),
  6 web-server-adapter fragments (`test_web_server_adapter_*.inc`), 5
  blob-route fragments (`test_web_server_blob_*.inc`), 4 web-security
  fragments (`web_security_*.inc`).
- By CTest label group (from `./scripts/run-tests.sh` output): `support` (4),
  `model` (1), `parser` (1), `executor` (2), `auth` (2), `usb` (2),
  `controls` (2), `wifi` (2), `startup` (2), `storage` (13), `web` (23) — 54
  total, matching `CLAUDE.md`'s label list.
- 27 fake-backend files under `tests/host/fakes/` (filesystem, GPIO, HTTP,
  httpd, USB, Wi-Fi, clock, random, call-log, storage-blob), plus two
  subdirectories (`esp_http_server_stub/`, `esp_idf_misc_stub/`) providing
  header-compatible stub implementations.
- Separately, `tests/v2_contracts/` (not under `tests/host/`, a distinct
  CMake/CTest project per `CLAUDE.md`) has 6 `test_*.c` files
  (`test_api_routes.c`, `test_device_settings.c`,
  `test_macro_canonical_tokens.c`, `test_macro_conformance.c`,
  `test_setup_contract.c`, `test_setup_route_policy.c`), run via
  `scripts/check-v2-contracts.sh`, not `run-tests.sh`.

**Note:** `CLAUDE.md`'s own summary line ("Host C (47 `test_*.c` + 20 `.inc`
fragments)") is stale relative to the measured 53/26 — this is ordinary count
drift from concurrent development (other tracks are actively adding tests),
not a defect this track is scoped to fix (`CLAUDE.md` is outside this track's
file surface).

### Vitest / browser (`webapp/tests/`)

```bash
find webapp/tests -maxdepth 1 -name '*.test.ts*' | wc -l   # 51
npm --prefix webapp run test -- --run                       # Test Files 51 passed (51); Tests 523 passed (523)
find webapp/tests/browser -type f                            # webapp/tests/browser/run-browser-tests.mjs (1 file)
```

- 51 Vitest spec files, 523 individual tests, all passing (measured with Node
  v24.18.0 via `nvm`, after `npm --prefix webapp ci` — 430 packages, 0
  vulnerabilities).
- Split by naming convention: 20 legacy v1-era spec files (`app-*.test.tsx`,
  `api*.test.ts`, `guards.test.ts`, `management-*`, `package-management`,
  `routing-confirmation`, `execution-*`, `spec-screens`, `error-banner`) vs.
  31 `v2-*.test.ts(x)` files covering the current `v2/`-contract-wired
  implementation — consistent with `CLAUDE.md`'s description of `features/`
  having both legacy and `v2/` subtrees.
- `webapp/tests/browser/` has exactly one file, the Playwright/real-Chrome
  runner harness (`run-browser-tests.mjs`); the browser test *cases* it
  drives are not separate committed spec files in this directory (confirmed
  by direct listing, not inferred).

### On-device Unity (`firmware/test_app/`)

```bash
find firmware/test_app -name 'test_*.c'   # 7 files
```

- `test_auth.c`, `test_limits.c`, `test_macro_executor.c`,
  `test_macro_parser.c`, `test_main.c` (harness entry/menu), `test_usb_state.c`,
  `test_uuid.c`.
- Unity `TEST_CASE` tags present (`grep -oE '"\[[^"]*\]"' firmware/test_app/main/*.c`):
  `[auth]`, `[benchmark]`, `[device]`, `[executor]`, `[limits]`,
  `[macro_parser]`, `[usb]`, `[uuid]`. This roster is unchanged from what
  `V2_143_DEV_STATUS_DOCS_SYNC_2026-08-09.md` already verified and fixed in
  `README.md`'s test-menu table.

### Hardware scripts (`tests/hardware/`)

```bash
find tests/hardware -maxdepth 1 -type f   # 8 files
```

- Harness/support modules (4): `device_client.py`, `hid_capture.py`,
  `hil_state.py`, `provision_device.py`.
- pytest-style test files (3): `test_acceptance_reset.py`,
  `test_httpd_concurrency.py`, `test_network_security.py`.
- `README.md` (1).

### Schemas, generated files, static assets, scripts, CI workflows, docs

- **JSON Schemas** (`docs/schemas/`, 3 files): `all-data-backup.schema.json`,
  `diagnostic-report.schema.json`, `macro-set-package.schema.json`. These are
  v1-era schema files (package/set/backup shapes); per `V2_MIGRATION_MAP.md`
  §5 they are **not** part of the v2 contract set (that lives in
  `contracts/v2/`) and are not v2 requirements sources.
- **v2 contract fixtures** (`contracts/v2/`, 8 files): `api/examples.json`,
  `api/routes.json`, `api/setup-route-policy.json`, `device-settings.json`,
  `limits.json`, `macro-conformance.json`, `repository/canonical.json`,
  `repository/fixtures.json` — matches the count the 2026-08-09 audit
  verified via `git ls-files contracts/v2/`.
- **Generated/do-not-hand-edit files**: `docs/SPEC_V2_TEST_TRACEABILITY.md`
  (regenerated by `scripts/generate-spec-traceability.py`); the webfs image
  and flash manifest (`scripts/build-webfs-image.sh`,
  `scripts/generate-flash-manifest.sh`) are build-time artifacts, not
  committed to the repo.
- **Static assets**: none exist. `webapp/` has no `public/` directory and no
  image/icon/font files anywhere under `webapp/src/` (`find ... -iname
  '*.png' -o -iname '*.svg' -o -iname '*.ico' -o -iname '*.woff*'` → zero
  hits) — `webapp/index.html` is a bare shell with no external asset
  references, consistent with the "fully local, no remote assets" policy.
- **Scripts** (`scripts/`, top level, 49 files): build/test/lint/coverage
  entry points plus the `check-v2-*.py`/`.sh` family (10 files:
  `check-v2-034-capacity.py`, `check-v2-api-routes.py`,
  `check-v2-auth-policy.py`, `check-v2-contracts.sh`,
  `check-v2-device-settings-policy.py`, `check-v2-limits.py`,
  `check-v2-phase2-architecture.py`, `check-v2-settings-schema.py`,
  `check-v2-setup-contract.py`, `check-v2-setup-route-policy.py`) enforcing
  v1→v2 migration policy, per `CLAUDE.md`.
- **CI workflows** (`.github/workflows/`, 6 files): `browser-tests.yml`,
  `device-tests-build.yml`, `host-tests.yml`, `publish-ci-status.yml`,
  `quality.yml`, `README.md`.
- **Documentation** (`docs/`, top level, 52 `.md` files) +
  `docs/implementation-v2/` (51 `.md` files, this report becomes the 52nd) +
  `docs/schemas/` (3) + `docs/mockups/README.md` (1) + `docs/review-source/`
  (4 npm-audit-history docs). See V2-002 below for which of the top-level 52
  still reference v1 authority and how.

**All V2-001 sub-items closed** — see `docs/TODO_V2.md`.

---

## V2-002 — Create the explicit migration map

### Documentation and scripts that still point to v1 authority — swept

Full-repository sweep (`grep -rln "docs/SPEC\.md\|docs/TODO\.md"` across
`.md`/`.sh`/`.py`/`.ts`/`.tsx`/`.json`/`.yml`, then individually read every
hit not already accounted for in
[`V2_143_DOC_AUTHORITY_SYNC_2026-08-09.md`](V2_143_DOC_AUTHORITY_SYNC_2026-08-09.md)
and
[`V2_143_DEV_STATUS_DOCS_SYNC_2026-08-09.md`](V2_143_DEV_STATUS_DOCS_SYNC_2026-08-09.md)):

**Already clean** (verified, not just cited from the prior reports):
`CLAUDE.md`, `README.md`, `docs/README.md`, `docs/TODO.md`, `docs/API.md`,
`docs/RECOVERY.md`, `docs/PROVISIONING_SECURITY.md`,
`docs/HARDWARE_TEST_PLAN.md`, `docs/IMPLEMENTATION_STATUS.md`,
`docs/UNIT_TESTS1_PROGRESS.md`, `docs/RELEASE_NOTES.md`,
`docs/TODO_EVIDENCE.md`, `docs/SPEC_TEST_TRACEABILITY.md`,
`docs/FRONTEND_TESTS_PROGRESS.md`,
`docs/CLAUDE_CODE_PHYSICAL_ESP32S3_V2_HANDOFF_2026-08-08.md`,
`scripts/generate-spec-traceability.py` (already targets `docs/SPEC_V2.md`,
confirmed by `grep`), `webapp/README.md` (zero `SPEC.md`/`TODO.md`
references), all of `.github/workflows/*.yml`, all of `scripts/*.sh` and
`scripts/*.py` (zero stale-authority hits anywhere in `scripts/`).

**Correctly historical, left as-is** (dated narrative describing what was
authoritative *when written*, before the rebuild; rewriting them would
misrepresent history, matching the standard the prior V2-143 tracks already
applied): the `FIX1_*`/`ESP32_MACRO_KEYBOARD_*` family,
`docs/CLAUDE_CODE_HANDOFF_2026-07-31.md`,
`docs/HANDOFF_2026-08-02_SIMPLIFICATION.md`,
`docs/TODO_SPEC_ALIGNMENT_2026-08-02.md`,
`docs/SPEC_CHANGE_AUDIT_2026-08-03.md`,
`docs/UNIT_TESTS1_HANDOFF_STATUS_2026-07-24.md`, `docs/UNIT_TESTS1_TODO.md`.

**Correctly non-authoritative self-references**: `docs/SPEC_V2.md` and
`docs/TODO_V2.md` each mention `docs/SPEC.md`/`docs/TODO.md` only to state
they are retired — this is the required disclosure, not a violation.
`docs/PHASE_1_COMPLETION_REPAIR_SPEC_2026-08-04.md` and
`..._TODO_2026-08-04.md` (dated *after* rebuild start) both cite
`docs/SPEC.md` only in the context of identifying and repairing a stale
reference elsewhere (e.g. "Remove references that imply retired `docs/SPEC.md`
is authoritative") — correct usage, not a violation, and the specific
traceability-generator problem they describe has since been fixed (confirmed:
`scripts/generate-spec-traceability.py` targets `docs/SPEC_V2.md` today).

**Still genuinely open — 3 findings, outside this track's file-edit surface**
(firmware/webapp code, explicitly excluded from this track's scope; recorded
here rather than fixed):

1. `webapp/src/features/settings/README.md:11` — present-tense: "Destructive
   operations require the confirmations defined in `docs/SPEC.md`." This
   asserts a currently-binding requirement from a frozen, retired v1 document
   — the same pattern `CLAUDE.md`'s spec-freeze rule calls out as a
   violation regardless of which file the words land in. Should read
   `docs/SPEC_V2.md` (its §12.4/§13.9/§15 already cover destructive-action
   confirmation in the v2 model). Already flagged, and deliberately left
   untouched as out-of-surface, by
   `V2_143_DOC_AUTHORITY_SYNC_2026-08-09.md` ("outside this track's scope").
   Still open.
2. `firmware/components/serial_console/include/serial_console.h:9` —
   "deliberately outside SPEC.md's reviewed production security model."
   Narrative/historical (explains a past scoping decision), not a live
   requirement citation; lower severity than (1) but still a bare `SPEC.md`
   reference that should eventually retarget to `SPEC_V2.md` for consistency.
3. `firmware/components/wifi_ap/include/wifi_ap.h:64` — "SPEC.md's previously
   deferred 'station mode' feature." Same narrative/historical character as
   (2).

None of these three were touched — they sit in `firmware/` and `webapp/src/`,
explicitly excluded from this track's file surface.

**Checkbox closed** (identification complete; the sweep is exhaustive across
tracked doc/script/config file types, and every hit was individually
adjudicated) — see `docs/TODO_V2.md`. Fixing the three residual findings
above is left for a track with firmware/webapp write scope.

### For every retained subsystem, name the v2 requirement and tests proving it remains valid

The 2026-08-09 audit found this literally absent from `V2_MIGRATION_MAP.md`
(`grep -n "§\|SPEC_V2#\|test_[a-z_]*\.c\|\.test\.ts"` → zero matches) and
noted the existing generated traceability doc
(`docs/SPEC_V2_TEST_TRACEABILITY.md`) is source-level only (does it cite
`SPEC_V2 §N` *anywhere* in the test suite) rather than subsystem-level (does
*this specific retained subsystem* have a named requirement and test). That
remains true — `docs/SPEC_V2_TEST_TRACEABILITY.md` shows only 9 section
citations across the whole native suite and 4 across the web suite, and a
direct check this session
(`grep -rl "SPEC_V2 §" tests/host/*.c tests/host/*.inc`) found only 4 files
(`executor_validation_tests.inc`, `executor_execution_tests.inc`,
`executor_confirmation_tests.inc`, `executor_cancellation_race_tests.inc`)
carry an explicit in-file section citation. Most retained subsystems are
tested for correct behavior without a comment naming the spec section.

The table below closes that gap at the granularity `V2_MIGRATION_MAP.md`
itself uses (its own table rows), for every row marked **Retain** or
**Retain/Adapt** in map §2 (firmware) and §4 (tests) — verified against
`docs/SPEC_V2.md`'s actual section content and the actual test files, not
inferred from file names alone.

| Retained subsystem (map §) | v2 requirement | Proving test(s) | Note |
| --- | --- | --- | --- |
| `support`: operation results, lifecycle/subsystem health (§2.2) | §16.1 No silent failures | `tests/host/test_app_operation_result.c`, `test_app_lifecycle_health.c`, `test_subsystem_health.c` | |
| `support`: UUID utilities (§2.2) | §8.3/§8.4 canonical lowercase UUID v4; §11.1 opaque `lastSelectedPackageId` | `tests/host/test_web_settings.c`, `test_settings_contract_v2.c`, `test_macro_parser.c` (UUID validation exercised inline, no dedicated `test_uuid.c` on the host side) | No standalone host UUID test file; coverage is indirect. |
| `support`: CRC helpers (§2.2, "retain only for unrelated firmware uses") | — | `tests/host/test_app_crc32.c` | **Gap found**: `grep -rln "app_crc32" firmware/components/*/` shows zero production callers outside `support` itself. The "unrelated firmware use" the map's retain decision anticipates does not currently exist — this helper is retained but unused in production. Not a v1-authority violation, but not proof of a v2 requirement either; recorded honestly rather than inventing a citation. |
| `support`: clocks/random/memory/bounded helpers (§2.2) | §12.2 32-byte random session tokens; §12.2 PBKDF2 derivation timing | `tests/host/test_support.c`; exercised transitively via `fake_clock`/`fake_random` in `test_auth.c`, executor tests | |
| `macro_model`: `app_error.c`/`app_uuid.c` (§2.3) | §8.3/§8.4 UUID shape | `tests/host/test_macro_model.c` | |
| `macro_parser` (§2.4) | §7.5–§7.13 (character support, escaping, directives, chords, delay, grammar, execution limits) | `tests/host/test_macro_parser.c`; `tests/v2_contracts/test_macro_conformance.c`, `test_macro_canonical_tokens.c` (shared corpus) | |
| `macro_executor` (§2.5) | §7.4 concurrency; §7.11 execution limits; §7.12 physical confirmation | `tests/host/test_macro_executor.c` + `executor_*_tests.inc` (4 of these carry explicit `SPEC_V2 §` citations, confirmed above) | Strongest-cited subsystem in the tree. |
| `usb_keyboard` (§2.6) | §7.1 USB identity; §7.2 USB state; §7.3 report safety invariant | `tests/host/test_usb_keyboard.c`, `test_usb_health.c` | |
| `auth`: health and constant-time comparison (§2.7) | §12.2 authentication and sessions | `tests/host/test_auth.c`, `test_auth_health.c`, `auth_existing_tests.inc` | **Nuance**: `auth_core_constant_time_equal` (`firmware/components/auth/auth_core_common.c:20`) is called from password/session verification, but no test calls it directly or asserts a timing property — timing-safety is inherently hard to assert as a functional test. Coverage is indirect (via correct match/non-match behavior), not a dedicated constant-time proof. |
| `wifi_ap`: AP + optional station (§2.9) | §12.1 Wi-Fi | `tests/host/test_wifi_ap.c`, `test_wifi_ap_station.c` | |
| `storage`: mount/no-format policy (§2.13) | §10.9 mount and recovery policy | `tests/host/test_storage_mount.c` | |
| `storage`: `storage_fs_ops.c` bounded wrappers (§2.13) | §10 (blob storage generally — bounded, error-propagating I/O underlies §10.2–§10.6) | `tests/host/test_storage_blob.c`, `test_storage_blob_upload.c`, `test_storage_blob_access.c` | |
| `storage`: atomic-commit primitives (§2.13) | §10.3 adding a blob (atomic `.gz.tmp`→`.gz`) | `tests/host/test_storage_atomic.c` | |
| `storage`: health/capacity reporting (§2.13) | §10.7 capacity and limits | `tests/host/test_storage_health.c` | |
| `web_server`: static file serving/path validation/content typing (§2.14) | §14.8 static assets | `tests/host/test_web_server_adapter_json_static.inc`, `test_web_server_adapter_main.inc` | |
| `web_server`: HTTP adapter/body/auth/cookie primitives (§2.14) | §13.1 general rules; §12.2 sessions/cookies | `tests/host/test_web_server_adapter_auth.inc`, `test_web_server_adapter_body.inc`, `test_web_request_policy.c` | |
| Test family — generic web security/static/adapter (§4.1) | §13.1/§13.2 error envelope; §14.8 static assets | `tests/host/test_web_security.c` + `web_security_*.inc` (4 fragments) | |
| Test family — storage mount/fs/atomic primitives (§4.1) | §10.1 layout; §10.9 mount/recovery | `test_storage_mount.c`, `test_storage_atomic.c`, `test_storage_parent_sync.c` | |

**Checkbox closed** for the granularity `V2_MIGRATION_MAP.md` itself defines
(its own table rows) — see `docs/TODO_V2.md`. This table lives in this
report rather than being merged into `V2_MIGRATION_MAP.md`'s body, to avoid
rewriting a file another track may also be actively editing; a future pass
could fold it in directly if wanted.

---

## Phase 0 exit gate

- **"Baseline evidence is committed."** Stays **unchecked**. V2-000's core
  execution items (`check-all.sh` run, full failure list, complete
  build/image/webfs sizing) remain genuinely open for the reasons detailed
  above — this report adds substantial *new*, real evidence but does not
  complete V2-000 as literally written. Marking this gate item complete while
  V2-000 itself has four of five sub-items still open would be exactly the
  kind of premature checkbox this task was told to avoid.
- The other three Phase 0 exit-gate lines were already `[x]` before this
  session and are unaffected by this work (no code changed).

---

## Commands run this session (representative)

```bash
# V2-000
git cat-file -t ad75859f56986a81a2faf01832008b69b26a94e1
git diff ad75859f HEAD -- scripts/verify-toolchain.sh
git show ad75859f:.nvmrc
git -C "$HOME/esp/esp-idf-v5.5.5" describe --tags
"$HOME/.nvm/versions/node/v24.18.0/bin/node" --version
gh api repos/ekkus93/esp32-macro-keyboard/commits/ad75859f.../check-runs
gh api repos/ekkus93/esp32-macro-keyboard/actions/jobs/<id>/logs   # x6
git show ad75859f:firmware/partitions.csv

# V2-001
find tests/host -maxdepth 1 -name 'test_*.c' | wc -l
find tests/host -maxdepth 1 -name '*.inc' | wc -l
./scripts/run-tests.sh
npm --prefix webapp ci
npm --prefix webapp run test -- --run
find firmware/test_app -name 'test_*.c'
find tests/hardware -maxdepth 1 -type f
find docs/schemas contracts/v2 scripts .github/workflows -type f

# V2-002
grep -rln "docs/SPEC\.md\|docs/TODO\.md" .
grep -rln "SPEC\.md" firmware/ webapp/src/ | grep -v SPEC_V2
grep -rl "SPEC_V2 §" tests/host/*.c tests/host/*.inc
grep -rln "app_crc32" firmware/components/*/
grep -rn "auth_core_constant_time_equal" tests/host/*.c tests/host/*.inc
```

## Statement

No unchecked task in Phases 0–2 is being claimed complete beyond what is
explicitly marked `[x]` above and in `docs/TODO_V2.md`. V2-000's execution
gate, full failure list, and image/webfs sizing remain honestly open — the
historical record does not exist for them and cannot be fabricated now. The
Phase 0 exit gate's "baseline evidence is committed" line remains open for
the same reason. V2-001 and the identification/traceability portions of
V2-002 are complete, backed by commands executed and re-verified in this
session. No production firmware, webapp, or script behavior was changed.
