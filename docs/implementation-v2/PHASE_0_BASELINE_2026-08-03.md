# ESP32 Macro Keyboard v2 — Phase 0 Baseline

**Phase:** 0 — Baseline, inventory, and migration map  
**Baseline branch:** `master`  
**Starting commit:** `ad75859f56986a81a2faf01832008b69b26a94e1`  
**Inventory commit:** `16977317c17a4e2079cdf1a37e8fdb001600a4dc`  
**Date:** 2026-08-03

## 1. Scope

This report records the remote repository state before v2 production behavior is
modified. The detailed subsystem disposition is in
[`V2_MIGRATION_MAP.md`](V2_MIGRATION_MAP.md).

No production firmware or React behavior was changed by the migration-map commit.

## 2. Starting state

The latest `master` commit when Phase 0 began was:

```text
ad75859f56986a81a2faf01832008b69b26a94e1
Add authoritative v2 implementation TODO
```

The preceding commits synchronized the two authoritative v2 specifications.

The repository is currently a v1 implementation beneath v2 specifications. The
main structural mismatch is not a small defect:

- firmware owns package and macro repositories, repository JSON, indexes,
  revisions, backup/restore/import/replace and package/macro HTTP CRUD;
- React loads packages and settings from firmware and uses route-level
  confirmation, progress and result pages;
- the v2 target instead gives React the only package/macro repository, stores
  complete gzip snapshots as opaque blobs, uses singular send APIs, and keeps the
  normal send workflow inline on the Macros page.

## 3. Declared toolchain baseline

Repository enforcement declares:

| Tool/property | Required value | Repository source |
| --- | --- | --- |
| ESP-IDF | exact tag `v5.5.5` | `scripts/verify-toolchain.sh` |
| ESP target | `esp32s3` | `scripts/verify-toolchain.sh` |
| Node.js | exactly `v24.18.0` | `.nvmrc`, `webapp/package.json`, `scripts/verify-toolchain.sh` |
| React | `19.1.1` | `webapp/package.json` |
| TypeScript | `5.8.3` | `webapp/package.json` |
| Vite | `7.3.6` | `webapp/package.json` |
| Vitest | `4.1.10` | `webapp/package.json` |

These are declared repository requirements. They have not yet been re-executed
in a clean local checkout for this phase.

## 4. Partition baseline

The committed `firmware/partitions.csv` currently defines:

| Partition | Size |
| --- | ---: |
| `nvs` | `0x6000` = 24 KiB |
| `otadata` | `0x2000` = 8 KiB |
| `phy_init` | `0x1000` = 4 KiB |
| `nvs_keys` | `0x1000` = 4 KiB |
| `ota_0` | `0x280000` = 2.5 MiB |
| `ota_1` | `0x280000` = 2.5 MiB |
| `webfs` | `0x100000` = 1 MiB |
| `userdata` | `0x80000` = 512 KiB |
| `coredump` | `0x10000` = 64 KiB |

The exact current application image size and generated webfs size still require a
clean build and are intentionally not inferred from documentation.

## 5. Authoritative local gate

The committed `scripts/check-all.sh` is fail-fast and currently invokes, in
order:

1. toolchain verification;
2. formatting;
3. static-analysis policy;
4. partition checks;
5. production configuration checks;
6. credential logging checks;
7. mount policy;
8. layer boundaries;
9. removed-feature checks;
10. USB identity;
11. frontend persisted-state checks;
12. setup route isolation;
13. firmware build and clang-tidy;
14. stack usage;
15. webfs image build;
16. flash manifest generation;
17. release budgets;
18. frontend checks;
19. script checks;
20. documentation checks;
21. native host tests.

Required command:

```bash
./scripts/check-all.sh
```

### Execution status

**Not executed in this connector-only environment.** The available GitHub
connector can read and update repository content but the execution container
cannot resolve `github.com`, so it cannot clone the repository. No local ESP-IDF
checkout, Node dependency tree, build directory, USB device, or ESP32 board is
available through the connector.

GitHub's combined commit-status query for the starting commit returned no status
contexts. That is not evidence that the gate passed or failed.

The following Phase 0 items therefore remain open:

- actual ESP-IDF and Node versions in the implementation environment;
- `./scripts/check-all.sh` result;
- exact current failure list;
- exact host and frontend test counts from execution;
- firmware build result;
- app image size;
- generated webfs size;
- confirmation of a clean local user worktree.

No value from an old README, historical report, or earlier CI run is promoted to
new baseline evidence.

## 6. Static inventory findings

### Firmware

The component inventory includes:

- `support`;
- `macro_model`;
- `macro_parser`;
- `macro_executor`;
- `usb_keyboard`;
- `auth`;
- `provisioning`;
- `wifi_ap`;
- `serial_console`;
- `device_controls`;
- `app_core`;
- `storage`;
- `web_server`;
- `firmware/main`;
- `firmware/test_app`.

The current `storage` build registers low-level mount/atomic helpers together
with repository JSON, object JSON, indexes, package and macro repositories,
locks, document state, package readers/writers, backup, restore, replace, import
and export. The current `web_server` build registers package, macro and execution
API modules. These findings establish the main rewrite/delete scope.

### React

The current `App.tsx`:

- fetches firmware settings and package lists;
- resolves the active package from `settings.activePackageId`;
- routes Send through `confirm` → `execution` → `result`;
- treats package management, package import/export and firmware package CRUD as
  application services.

The current API layer exposes:

- setup-state and split setup credential/restart routes;
- settings revisions;
- package CRUD, duplication, ordering and selection;
- macro CRUD and server validation;
- plural `/api/v1/executions` routes;
- package-oriented import/export and diagnostics.

`PackageSelectionPage.tsx` uses `localStorage` for recent-package state. That use
is explicitly deleted in v2.

### Tests and gates

The host CMake suite has reusable support, parser, executor, auth, USB, Wi-Fi,
provisioning, device-control, startup, web-security and low-level storage tests,
but also extensive repository/package/index/restore/import/replace tests that
must be removed or replaced by opaque-blob coverage.

The webapp scripts provide typecheck, lint, stylelint, formatting, Vitest,
coverage, browser tests, build and audit entry points. Their harnesses are
candidates for retention while fixtures and workflows are rewritten.

## 7. Remote-worktree statement

The remote `master` branch has a definite commit state and cannot itself contain
uncommitted files. This report cannot inspect a user's separate local clone, so
it does not claim that all external worktrees are clean. Before production code
changes are applied from a local environment, the implementer must run:

```bash
git status --short
git rev-parse HEAD
```

and record the results.

## 8. Phase 0 completion status

### Completed

- Starting remote `master` SHA recorded.
- Declared toolchain and partition configuration recorded.
- Production firmware, React, tests, scripts and documentation families
  inventoried.
- Explicit retain/adapt/rewrite/delete map committed.
- Known v1 architecture and forbidden v2 carryovers identified.
- No production behavior changed.

### Still required before Phase 0 exit

- Execute the complete baseline gate from a clean checkout.
- Record exact failures, test counts, build state and image sizes.
- Record actual tool versions.
- Confirm the executing local worktree contains no unrelated changes.
- Update this report with the execution evidence and exact commit SHA.

No unchecked runtime or hardware item is claimed complete.
