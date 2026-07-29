# CI reproducibility

The authoritative release-oriented workflow is `.github/workflows/quality.yml`.
It invokes the same `./scripts/check-all.sh` entry point used by developers and
has read-only repository permission.

## Pinned environment

- GitHub-hosted runner image: `ubuntu-24.04`
- ESP-IDF: exact recursive tag `v5.5.5`, installed by
  `scripts/install-esp-idf.sh`
- Node.js: exact version from `.nvmrc` and the matching `engines.node` field in
  `webapp/package.json`
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

The Ubuntu package names are pinned to the `ubuntu-24.04` image rather than to
mutable `ubuntu-latest`. A runner-image migration requires a reviewed workflow
change and a clean full-gate result.

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
