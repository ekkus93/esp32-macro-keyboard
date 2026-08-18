# Development setup

## ESP-IDF

The firmware requires the exact ESP-IDF tag `v5.5.5`.

```bash
./scripts/install-esp-idf.sh
. "$HOME/esp/esp-idf-v5.5.5/export.sh"
./scripts/verify-toolchain.sh
```

The installer clones recursively and refuses to reuse an unrelated checkout.
The verifier rejects moving branches and all other tags.

## Node.js

The frontend is pinned by `.nvmrc`.

```bash
nvm install
nvm use
cd webapp
npm install --package-lock-only
npm ci
```

The first successful dependency resolution must commit `package-lock.json`.
Do not use `npm ci` without the committed lockfile and do not fabricate one.

## Quality gate

```bash
./scripts/check-all.sh
```

All first-party warnings and lint findings are defects. Do not suppress them.
The checks intentionally exclude ESP-IDF, managed components, `node_modules`,
and generated build output.

The `Quality` workflow runs the full gate (`check-all.sh`) on pushes to `master`,
pull requests, tags, and manual dispatch.

## Frontend visual-regression baselines

`npm --prefix webapp run test:visual` (part of `check-webapp.sh`) compares the
built app's computed style and geometry, in ~35 scenarios at every viewport
`webapp/tests/browser/visual/scenarios.mjs` covers, against the checked-in
baselines in `webapp/tests/browser/visual/baselines/`. It is what catches a
spacing, colour, or layout regression that the rest of the gate — typecheck,
lint, unit tests, touch-target/responsive assertions, axe-core — cannot see
(`docs/WEBAPP_TAILWIND_SPEC_2026-08-18.md` §10.1).

If it fails, **read the reported diff before doing anything else**: it names
the exact element, class, and property that changed. Only once you understand
*why* it changed and agree the new rendering is correct:

```bash
node webapp/tests/browser/visual/run-visual-tests.mjs --update-baselines
```

Review the resulting `git diff` on the baseline JSON like any other change,
and commit it as its own commit, separate from whatever caused it. See
`webapp/tests/browser/visual/baselines/README.md` for the full workflow, why
baselines are JSON rather than screenshots, and the `--grep`/`--baseline-dir`
flags for iterating on one scenario or diffing two builds directly.

## Build

```bash
cd firmware
idf.py set-target esp32s3
idf.py build
```

Build and hardware validation require the selected board's native USB D+ and D-
connection, not only its USB-to-UART connector.
