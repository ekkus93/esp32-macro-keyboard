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
- Chrome or Chromium from the pinned `ubuntu-24.04` runner image
- npm graph: exact committed `webapp/package-lock.json`, installed with `npm ci`
- Python tools:
  - `cmakelang==0.6.13`
  - `yamllint==1.38.0`
- Go tool: `mvdan.cc/sh/v3/cmd/shfmt@v3.11.0`
- Ubuntu packages supplied by the pinned runner archive:
  - `clang-format`
  - `clang-tidy`
  - `jq`
  - `libcjson-dev`
  - `shellcheck`

The runner and reusable actions are immutable in the workflow. Ubuntu package
names are tied to the `ubuntu-24.04` runner archive rather than mutable
`ubuntu-latest`. A runner-image, action, or package-set migration requires a
reviewed workflow change and a clean full-gate result.

## Cache policy

The npm download cache key includes the Node environment and
`webapp/package-lock.json`. The cache contains downloaded dependency material,
not `node_modules`, firmware outputs, generated web assets, test results, or any
other mutable build output. `npm ci` and every validation step must succeed from
a cold cache.

## Security and mutation policy

- Workflow permission is `contents: read`.
- `actions/checkout` uses `persist-credentials: false`.
- CI never commits, pushes, rewrites branches, or deletes its own workflow.
- No encoded or generated source transformation is executed.
- Dependency auditing may use the network, but the normal embedded development
  gate remains usable offline after dependencies and toolchains are installed.
- A dependency-audit exception must be narrow, dev-only, documented, tested,
  and time-limited. Any changed finding fails closed.

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
