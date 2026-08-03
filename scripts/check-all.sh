#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root
cd "${repo_root}"

# This is the authoritative local quality gate. CI must call the same command,
# every phase must fail closed, and no phase may mask a nonzero exit status.
./scripts/verify-toolchain.sh
./scripts/check-format.sh
./scripts/check-static-analysis-policy.sh
./scripts/check-partitions.sh
bash ./scripts/check-production-config.sh
bash ./scripts/check-credential-logging.sh
bash ./scripts/check-mount-policy.sh
bash ./scripts/check-layer-boundaries.sh
bash ./scripts/check-removed-features.sh
bash ./scripts/check-usb-identity.sh
bash ./scripts/check-frontend-persisted-state.sh
bash ./scripts/check-setup-route-isolation.sh
bash ./scripts/check-v2-contracts.sh --native-only
./scripts/check-firmware.sh
bash ./scripts/check-stack-usage.sh
bash ./scripts/build-webfs-image.sh
bash ./scripts/generate-flash-manifest.sh
bash ./scripts/check-release-budgets.sh
./scripts/check-webapp.sh
./scripts/check-scripts.sh
./scripts/check-docs.sh
./scripts/run-tests.sh
