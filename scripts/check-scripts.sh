#!/usr/bin/env bash
set -euo pipefail

actionlint

# First-party shell sources: the authoritative scripts, the script regression
# tests, and their fakes (the fake analyzer has a bash shebang but no extension).
shell_files=(scripts/*.sh tests/scripts/*.sh tests/scripts/fakes/run-clang-tidy)

shellcheck "${shell_files[@]}"
shfmt -d "${shell_files[@]}"
bash -n "${shell_files[@]}"

# Regression tests for the fail-closed clang-tidy gate (FIX1 Phase 2.2), the
# first-party include-cycle detection (FIX1 RESPONSES Q1), the static-analysis
# exception policy (FIX1 RESPONSES Q2), partition integrity (FIX1 Phase 14.1),
# production NVS encryption policy (FIX1 Phase 14.2), credential output
# isolation (FIX1 Phase 14.4), HMAC bootstrap-label and setup-route isolation
# (FIX1 Phase 14.5), the bounded Phase 18.1 package reader, the reviewed
# npm-audit exception policy, the Phase 18.5 secret-sentinel scanner, the
# Phase 18.5 frontend persisted-state policy, and the bounded CI status issue
# generator.
bash tests/scripts/test-check-firmware.sh
bash tests/scripts/test-clang-tidy-include-cycle.sh
bash tests/scripts/test-static-analysis-policy.sh
bash tests/scripts/test-check-partitions.sh
bash tests/scripts/test-check-production-config.sh
bash tests/scripts/test-check-credential-logging.sh
bash tests/scripts/test-generate-setup-label.sh
bash tests/scripts/test-check-setup-route-isolation.sh
bash tests/scripts/test-storage-package.sh
bash tests/scripts/test-check-frontend-persisted-state.sh
python3 tests/scripts/test-check-npm-audit.py
python3 tests/scripts/test-secret-sentinel.py
python3 tools/ci_status/test_publish_status.py
