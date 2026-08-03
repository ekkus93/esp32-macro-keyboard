# Phase 1 CI Validation Request

**Purpose:** Trigger and preserve an inspectable pull-request execution of the
repository's authoritative Quality workflow for the Phase 1 v2 contract
foundation.

**Validation base:** `master` at
`ee139a90f8aaf56801e7bc8888128e47f5687209`

The pull request containing this file introduces no production behavior. Its
Quality run must execute the same authoritative command used for release
validation:

```bash
./scripts/check-all.sh
```

The run is intended to provide clean-checkout evidence for:

- formatting and static-analysis policy;
- shared v2 contract drift checks;
- native CMake/CTest contract suites;
- frontend type checking, linting, formatting, and Vitest;
- ESP-IDF v5.5.5 firmware compilation;
- documentation and script checks;
- the complete existing host-test gate.

A passing pull-request run may be cited in the Phase 1 implementation report.
A failing run must be diagnosed and repaired without weakening or bypassing any
gate. This file alone is not evidence that the checks passed.
