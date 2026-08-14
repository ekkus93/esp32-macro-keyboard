#!/usr/bin/env bash
set -euo pipefail

actionlint

# First-party shell sources: the authoritative scripts, the script regression
# tests, and their fakes (these have a bash shebang but no extension, so the
# *.sh globs above do not already match them).
shell_files=(
	scripts/*.sh tests/scripts/*.sh tests/scripts/fakes/run-clang-tidy
	tests/scripts/fakes/npm tests/scripts/fakes/littlefs-python
	tests/scripts/fakes/idf.py tests/scripts/fakes/esptool.py
)

shellcheck "${shell_files[@]}"
shfmt -d "${shell_files[@]}"
bash -n "${shell_files[@]}"

# Shared v2 contracts must not drift between their reviewed JSON sources and
# firmware/web mirrors.
python3 scripts/check-v2-limits.py
python3 scripts/check-v2-settings-schema.py
python3 scripts/check-v2-device-settings-policy.py
python3 scripts/check-v2-setup-route-policy.py
python3 scripts/check-v2-api-routes.py
python3 scripts/check-web-route-dispatch-sync.py
python3 scripts/check-h9-architecture.py
python3 scripts/check-h9-production-audit.py
python3 scripts/check-h2-architecture.py
python3 scripts/check-h3-architecture.py
python3 scripts/check-v2-auth-policy.py
python3 tests/scripts/test-generate-v2-macro-corpus.py
python3 tests/scripts/test-v2-035-hardware.py
python3 tests/scripts/test-h5-055-storage-evidence.py

# Regression tests for the fail-closed clang-tidy gate (FIX1 Phase 2.2), the
# first-party include-cycle detection (FIX1 RESPONSES Q1), the static-analysis
# exception policy (FIX1 RESPONSES Q2), partition integrity (FIX1 Phase 14.1),
# production NVS encryption policy (FIX1 Phase 14.2), credential output
# isolation (FIX1 Phase 14.4), HMAC bootstrap-label and setup-route isolation
# (FIX1 Phase 14.5), the reviewed npm-audit exception policy, the Phase 18.5
# secret-sentinel scanner, the Phase 18.5 frontend persisted-state policy, the
# Phase 21.1 release-budget gate, the SPEC §23 webfs packaging pipeline and
# flash manifest, the first-party stack-usage ratchet, and the bounded CI status
# issue generator.
bash tests/scripts/test-check-firmware.sh
bash tests/scripts/test-clang-tidy-include-cycle.sh
bash tests/scripts/test-static-analysis-policy.sh
bash tests/scripts/test-check-partitions.sh
bash tests/scripts/test-check-production-config.sh
bash tests/scripts/test-check-credential-logging.sh
bash scripts/check-credential-logging.sh tests/host
bash scripts/check-credential-logging.sh firmware/test_app
bash tests/scripts/test-test-assert-redaction.sh
bash tests/scripts/test-check-h9-production-audit.sh
bash tests/scripts/test-generate-setup-label.sh
bash tests/scripts/test-check-setup-route-isolation.sh
bash tests/scripts/test-check-web-route-dispatch-sync.sh
bash tests/scripts/test-check-frontend-persisted-state.sh
bash tests/scripts/test-check-release-budgets.sh
bash tests/scripts/test-build-webfs-image.sh
bash tests/scripts/test-generate-flash-manifest.sh
bash tests/scripts/test-check-stack-usage.sh
python3 tests/scripts/test-check-npm-audit.py
python3 tests/scripts/test-secret-sentinel.py
python3 tools/ci_status/test_publish_status.py
