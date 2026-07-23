#!/usr/bin/env bash
set -euo pipefail

readonly repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "${repo_root}"

./scripts/verify-toolchain.sh
./scripts/check-format.sh
./scripts/check-firmware.sh
./scripts/check-webapp.sh
./scripts/check-scripts.sh
./scripts/check-docs.sh
./scripts/run-tests.sh
