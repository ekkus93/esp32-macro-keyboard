#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root
cd "${repo_root}"

./scripts/verify-toolchain.sh
./scripts/check-format.sh
./scripts/check-static-analysis-policy.sh
./scripts/check-partitions.sh
./scripts/check-production-config.sh
./scripts/check-firmware.sh
./scripts/check-webapp.sh
./scripts/check-scripts.sh
./scripts/check-docs.sh
./scripts/run-tests.sh
