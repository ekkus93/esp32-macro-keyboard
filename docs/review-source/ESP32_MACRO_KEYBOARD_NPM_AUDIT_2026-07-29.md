# Frontend npm audit review

**Generated:** 2026-07-29

## Result

- Critical: 0
- High: 5
- Runtime/shipped dependency findings: 0
- Accepted advisory source: `1124334`
- Acceptance expires: `2026-09-30`

The five reported package names are one transitive ESLint development-tool
advisory graph. Every installed node is marked `dev: true` in the committed
lockfile and is absent from the static assets served by the ESP32.

CI must fail if the finding names, severity, advisory source, dev-only
classification, or expiration date changes.

## Reviewed direct upgrades

- `@eslint/js`: `9.31.0` to `9.39.4`
- `@vitest/coverage-v8`: `3.2.4` to `4.1.10`
- `eslint`: `9.31.0` to `9.39.4`
- `markdownlint-cli2`: `0.23.1` to `0.23.2`
- `typescript-eslint`: `8.38.0` to `8.65.0`
- `vite`: `7.0.6` to `7.3.6`
- `vitest`: `3.2.4` to `4.1.10`
