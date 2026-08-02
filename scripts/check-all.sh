#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root
cd "${repo_root}"

# SPEC 21.5: this is the authoritative local quality gate; "CI MUST call the same
# command", and it "MUST fail on the first failed phase ... it MUST never mask
# failures." That is what `set -euo pipefail` above buys, and why no phase here
# may be wrapped in `|| true`.
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
./scripts/check-firmware.sh
bash ./scripts/check-stack-usage.sh
bash ./scripts/build-webfs-image.sh
bash ./scripts/generate-flash-manifest.sh
bash ./scripts/check-release-budgets.sh
./scripts/check-webapp.sh
./scripts/check-scripts.sh
./scripts/check-docs.sh
./scripts/run-tests.sh
