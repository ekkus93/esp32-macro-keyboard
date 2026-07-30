# CI reproducibility

The authoritative release-oriented workflow is `.github/workflows/quality.yml`.
It invokes the same `./scripts/check-all.sh` entry point used by developers and
has read-only repository permission.

## Pinned environment

- GitHub-hosted runner image: `ubuntu-24.04`
- Checkout action:
  `actions/checkout@11d5960a326750d5838078e36cf38b85af677262` (`v4`)
- Node setup action:
  `actions/setup-node@49933ea5288caeca8642d1e84afbd3f7d6820020` (`v4`)
- ESP-IDF: exact recursive tag `v5.5.5`, installed by
  `scripts/install-esp-idf.sh`
- Node.js: exact version from `.nvmrc` and the matching `engines.node` field in
  `webapp/package.json`
- Go toolchain: exact `go1.25.12` selected through `GOTOOLCHAIN`
- Chrome or Chromium from the pinned `ubuntu-24.04` runner image
- npm graph: exact committed `webapp/package-lock.json`, installed with `npm ci`
- Python tools:
  - `cmakelang==0.6.13`
  - `yamllint==1.38.0`
- Go tools:
  - `github.com/rhysd/actionlint/cmd/actionlint@v1.7.12`
  - `mvdan.cc/sh/v3/cmd/shfmt@v3.11.0`
- Ubuntu packages supplied by the pinned runner archive:
  - `clang-format`
  - `clang-tidy`
  - `jq`
  - `libcjson-dev`
  - `shellcheck`

The runner and reusable actions are immutable in the workflow. Ubuntu package
names are tied to the `ubuntu-24.04` runner archive rather than mutable
`ubuntu-latest`. A runner-image, action, toolchain, or package-set migration
requires a reviewed workflow change and a clean full-gate result.

## Cache policy

The npm download cache key includes the Node environment and
`webapp/package-lock.json`. The cache contains downloaded dependency material,
not `node_modules`, firmware outputs, generated web assets, test results, or any
other mutable build output. `npm ci` and every validation step must succeed from
a cold cache.

## Security and mutation policy

- Workflow permission is `contents: read` for normal CI gates.
- `actions/checkout` uses `persist-credentials: false`.
- CI never commits, pushes, rewrites branches, or deletes its own workflow.
- No encoded or generated source transformation is executed.
- Dependency auditing may use the network, but the normal embedded development
  gate remains usable offline after dependencies and toolchains are installed.
- A dependency-audit exception must be narrow, dev-only, documented, tested,
  and time-limited. Any changed finding fails closed.
- `actionlint` validates every GitHub Actions workflow as part of
  `scripts/check-scripts.sh`; workflow syntax and expression errors fail the
  authoritative gate.

## Connector-readable CI status

The full project adoption specification is
`docs/CHATGPT_READABLE_GITHUB_ACTIONS_CI_STATUS_BRIDGE_SPEC.md`.

`.github/workflows/publish-ci-status.yml` publishes the latest applicable
`master` state of the four permanent validation workflows to stable issues:

- Quality: issue #19;
- Host Tests: issue #20;
- Browser Tests: issue #21;
- Device Test Build: issue #22.

The publisher listens for requested, in-progress, and completed run states. It
records the exact run and attempt IDs, commit, branch, event, runner information,
job IDs and step states, abnormal steps, timing, and artifact metadata in both a
readable summary and a versioned JSON payload. It explicitly distinguishes
metadata that is not yet available from a valid empty result.

The publisher has only `actions: read`, `contents: read`, and `issues: write`.
It sparse-checks out only `tools/ci_status` from trusted `master`, with persisted
credentials disabled, and never checks out or executes the triggering run's
branch or commit. The checked-in generator is exercised by permanent unit tests
from `scripts/check-scripts.sh`.

Fork-originated runs, non-`master` branches, pull-request events, and unrelated
event categories are ignored. The latest-run query is branch-scoped, and the
generator filters by trusted repository, branch, and event before an issue may
be updated. A stale event sets an explicit step output that prevents every later
publication step from running.

Jobs and artifacts are paginated. Before writing, the publisher validates the
configured issue number, title, open state, and automation marker; after writing,
it validates ownership again. The machine-readable JSON is bounded to 60,000
UTF-8 bytes and uses explicit safe compaction that preserves all job IDs, final
job conclusions, artifacts, and abnormal steps. It fails rather than silently
publishing truncated or invalid JSON.

Each issue is an overwritten latest-state snapshot, not a historical log and
not a replacement for complete job logs. During a Ralph loop, the issue's
machine-readable `workflow.head_sha` must match the exact candidate SHA before
the status is used as evidence.

GitHub activates a `workflow_run` publisher only after its workflow file exists
on the default branch. The publisher must therefore remain on `master`.

## Release limitations

This document does not claim that firmware-slot, webfs, RAM, heap, or task-stack
budgets are complete. Those measurements remain part of release preparation and
must be recorded from actual builds and hardware before version 0.1 release.

## Real Chrome browser validation

Phase 17.10 has a dedicated read-only workflow at
`.github/workflows/browser-tests.yml`. It:

- uses pinned checkout and Node setup action commits;
- builds the production Vite bundle before testing;
- launches Chrome or Chromium from the pinned Ubuntu runner image;
- fails when no supported browser is present;
- serves a deterministic same-origin API fixture without external assets;
- drives the page through the Chrome DevTools Protocol using Node 24 built-ins;
- checks keyboard activation, modal focus, visible status text, 44 by 44
  CSS-pixel targets, reorder alternatives, reconnect refresh, and the complete
  execution workflow.

`scripts/check-webapp.sh` runs the same browser harness after the unit,
coverage, static, and production-build gates, so the authoritative Quality
workflow cannot pass by skipping real-browser behavior.
