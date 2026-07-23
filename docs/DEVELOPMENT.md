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

The manual `Quality` workflow remains `workflow_dispatch`-only until both npm and
ESP-IDF lockfiles exist and the entire workflow is green.

## Build

```bash
cd firmware
idf.py set-target esp32s3
idf.py build
```

Build and hardware validation require the selected board's native USB D+ and D-
connection, not only its USB-to-UART connector.
