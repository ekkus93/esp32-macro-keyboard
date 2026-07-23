# GitHub Actions Workflows

The repository currently defines three workflows:

- `host-tests.yml` runs native host tests for pushes to `master`, pull requests,
  tags, and manual dispatch.
- `device-tests-build.yml` formats and builds the ESP32-S3 Unity test firmware with
  ESP-IDF v5.5.5 for the same events.
- `quality.yml` runs the complete first-party quality gate manually. It remains
  `workflow_dispatch`-only until reproducible frontend and ESP-IDF dependency
  lockfiles are committed and the full workflow is green.

Host and device-test artifacts are uploaded only for tag pushes. Normal branch and
pull-request runs do not retain build artifacts.

Workflows must call the same first-party scripts used locally. They must not
suppress warnings, ignore failed commands, lint ESP-IDF or third-party dependencies,
or upload ordinary check-in artifacts.
