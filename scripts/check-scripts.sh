#!/usr/bin/env bash
set -euo pipefail

# First-party shell sources: the authoritative scripts, the script regression
# tests, and their fakes (the fake analyzer has a bash shebang but no extension).
shell_files=(scripts/*.sh tests/scripts/*.sh tests/scripts/fakes/run-clang-tidy)

shellcheck "${shell_files[@]}"
shfmt -d "${shell_files[@]}"
bash -n "${shell_files[@]}"

# Regression tests for the fail-closed clang-tidy gate (FIX1 Phase 2.2), the
# first-party include-cycle detection (FIX1 RESPONSES Q1), the static-analysis
# exception policy (FIX1 RESPONSES Q2), partition integrity (FIX1 Phase 14.1),
# and production NVS encryption policy (FIX1 Phase 14.2).
bash tests/scripts/test-check-firmware.sh
bash tests/scripts/test-clang-tidy-include-cycle.sh
bash tests/scripts/test-static-analysis-policy.sh
bash tests/scripts/test-check-partitions.sh
bash tests/scripts/test-check-production-config.sh
