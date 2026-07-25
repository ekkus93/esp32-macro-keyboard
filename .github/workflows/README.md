# GitHub Actions Workflows

The repository currently defines three workflows:

- `host-tests.yml` runs, for pushes to `master`, pull requests, tags, and manual
  dispatch: the normal native host suite, an AddressSanitizer/UndefinedBehaviorSanitizer
  run, native coverage with the enforced pure-policy gate, the frontend
  typecheck/lint/stylelint/format/Vitest stack, and frontend coverage.
- `device-tests-build.yml` formats and builds the ESP32-S3 Unity test firmware with
  ESP-IDF v5.5.5 for the same events.
- `quality.yml` runs the complete first-party quality gate (`check-all.sh`:
  toolchain, format, firmware build + clang-tidy, webapp, scripts, docs, host
  tests) for pushes to `master`, pull requests, tags, and manual dispatch.

Host and device-test artifacts are uploaded only for tag pushes. Normal branch and
pull-request runs do not retain build artifacts.

Workflows must call the same first-party scripts used locally. They must not
suppress warnings, ignore failed commands, lint ESP-IDF or third-party dependencies,
or upload ordinary check-in artifacts.
