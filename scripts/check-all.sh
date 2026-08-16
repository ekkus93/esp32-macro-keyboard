#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
readonly repo_root
cd "${repo_root}"

./scripts/verify-toolchain.sh
./scripts/check-format.sh
./scripts/check-static-analysis-policy.sh
./scripts/check-partitions.sh
python3 ./scripts/check-v2-034-capacity.py
python3 ./scripts/check-v2-device-settings-policy.py
bash ./scripts/check-production-config.sh
python3 ./scripts/check-no-wall-clock.py
bash ./scripts/check-credential-logging.sh
bash ./scripts/check-mount-policy.sh
bash ./scripts/check-layer-boundaries.sh
bash ./scripts/check-removed-features.sh
python3 ./scripts/check-v2-phase2-architecture.py
bash ./scripts/check-usb-identity.sh
bash ./scripts/check-frontend-persisted-state.sh
bash ./scripts/check-setup-route-isolation.sh
python3 ./scripts/check-web-route-dispatch-sync.py
python3 ./scripts/check-v2-auth-policy.py
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
