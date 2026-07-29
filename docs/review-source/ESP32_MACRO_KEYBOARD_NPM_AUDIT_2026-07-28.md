# Frontend npm audit diagnostic

**Generated:** 2026-07-29

**Scope:** the exact committed `webapp/package-lock.json`; this is a diagnostic
input for the code-review fixes Ralph loop, not an accepted-risk waiver.

## Summary

- Info: 0
- Low: 0
- Moderate: 0
- High: 16
- Critical: 2
- Total: 18

## Findings

### `@eslint/config-array`

- Severity: `high`
- Direct dependency: `false`
- Affected range: `<=0.22.0`
- Installed nodes: `node_modules/@eslint/config-array`
- Fix available: `{"isSemVerMajor": true, "name": "eslint", "version": "10.8.0"}`
- Via:
  - transitive through `minimatch`

### `@eslint/eslintrc`

- Severity: `high`
- Direct dependency: `false`
- Affected range: `0.0.1 || >=0.1.1`
- Installed nodes: `node_modules/@eslint/eslintrc`
- Fix available: `{"isSemVerMajor": true, "name": "eslint", "version": "10.8.0"}`
- Via:
  - transitive through `minimatch`

### `@typescript-eslint/eslint-plugin`

- Severity: `high`
- Direct dependency: `false`
- Affected range: `<=8.55.1-alpha.3`
- Installed nodes: `node_modules/@typescript-eslint/eslint-plugin`
- Fix available: `true`
- Via:
  - transitive through `@typescript-eslint/type-utils`
  - transitive through `@typescript-eslint/utils`
  - transitive through `eslint`

### `@typescript-eslint/parser`

- Severity: `high`
- Direct dependency: `false`
- Affected range: `1.1.1-alpha.0 - 8.56.1-alpha.2`
- Installed nodes: `node_modules/@typescript-eslint/parser`
- Fix available: `{"isSemVerMajor": false, "name": "typescript-eslint", "version": "8.65.0"}`
- Via:
  - transitive through `@typescript-eslint/typescript-estree`
  - transitive through `eslint`

### `@typescript-eslint/type-utils`

- Severity: `high`
- Direct dependency: `false`
- Affected range: `5.9.2-alpha.0 - 8.56.1-alpha.2`
- Installed nodes: `node_modules/@typescript-eslint/type-utils`
- Fix available: `true`
- Via:
  - transitive through `@typescript-eslint/typescript-estree`
  - transitive through `@typescript-eslint/utils`
  - transitive through `eslint`

### `@typescript-eslint/typescript-estree`

- Severity: `high`
- Direct dependency: `false`
- Affected range: `6.16.0 - 8.56.1-alpha.2`
- Installed nodes: `node_modules/@typescript-eslint/typescript-estree`
- Fix available: `{"isSemVerMajor": false, "name": "typescript-eslint", "version": "8.65.0"}`
- Via:
  - transitive through `minimatch`

### `@typescript-eslint/utils`

- Severity: `high`
- Direct dependency: `false`
- Affected range: `<=8.56.1-alpha.2`
- Installed nodes: `node_modules/@typescript-eslint/utils`
- Fix available: `{"isSemVerMajor": false, "name": "typescript-eslint", "version": "8.65.0"}`
- Via:
  - transitive through `@typescript-eslint/typescript-estree`
  - transitive through `eslint`

### `@vitest/coverage-v8`

- Severity: `critical`
- Direct dependency: `true`
- Affected range: `<=3.2.7`
- Installed nodes: `node_modules/@vitest/coverage-v8`
- Fix available: `{"isSemVerMajor": false, "name": "@vitest/coverage-v8", "version": "3.2.7"}`
- Via:
  - transitive through `test-exclude`
  - transitive through `vitest`

### `brace-expansion`

- Severity: `high`
- Direct dependency: `false`
- Affected range: `<=5.0.7`
- Installed nodes: `node_modules/@typescript-eslint/typescript-estree/node_modules/brace-expansion, node_modules/brace-expansion, node_modules/glob/node_modules/brace-expansion`
- Fix available: `{"isSemVerMajor": true, "name": "eslint", "version": "10.8.0"}`
- Via:
  - `high` — brace-expansion: DoS via unbounded expansion length causing an out-of-memory process crash; range `<=5.0.7`; source `1124334`

### `eslint`

- Severity: `high`
- Direct dependency: `true`
- Affected range: `0.12.0 - 2.0.0-rc.1 || 4.1.0 - 10.0.0-rc.2`
- Installed nodes: `node_modules/eslint`
- Fix available: `{"isSemVerMajor": true, "name": "eslint", "version": "10.8.0"}`
- Via:
  - transitive through `@eslint/config-array`
  - transitive through `@eslint/eslintrc`
  - transitive through `minimatch`

### `glob`

- Severity: `high`
- Direct dependency: `false`
- Affected range: `4.3.0 - 10.5.0`
- Installed nodes: `node_modules/glob`
- Fix available: `{"isSemVerMajor": false, "name": "@vitest/coverage-v8", "version": "3.2.7"}`
- Via:
  - transitive through `minimatch`

### `js-yaml`

- Severity: `high`
- Direct dependency: `false`
- Affected range: `5.0.0 - 5.2.1`
- Installed nodes: `node_modules/markdownlint-cli2/node_modules/js-yaml`
- Fix available: `{"isSemVerMajor": false, "name": "markdownlint-cli2", "version": "0.23.2"}`
- Via:
  - `high` — js-yaml: Exponential parsing time in flow collections leads to denial of service; range `>=5.0.0 <=5.2.1`; source `1124281`

### `markdownlint-cli2`

- Severity: `high`
- Direct dependency: `true`
- Affected range: `0.23.0 - 0.23.1`
- Installed nodes: `node_modules/markdownlint-cli2`
- Fix available: `{"isSemVerMajor": false, "name": "markdownlint-cli2", "version": "0.23.2"}`
- Via:
  - transitive through `js-yaml`

### `minimatch`

- Severity: `high`
- Direct dependency: `false`
- Affected range: `2.0.0 - 10.0.2`
- Installed nodes: `node_modules/@typescript-eslint/typescript-estree/node_modules/minimatch, node_modules/glob/node_modules/minimatch, node_modules/minimatch`
- Fix available: `{"isSemVerMajor": true, "name": "eslint", "version": "10.8.0"}`
- Via:
  - transitive through `brace-expansion`

### `test-exclude`

- Severity: `high`
- Direct dependency: `false`
- Affected range: `5.2.0 - 7.0.2`
- Installed nodes: `node_modules/test-exclude`
- Fix available: `{"isSemVerMajor": false, "name": "@vitest/coverage-v8", "version": "3.2.7"}`
- Via:
  - transitive through `glob`

### `typescript-eslint`

- Severity: `high`
- Direct dependency: `true`
- Affected range: `<=8.56.1-alpha.2`
- Installed nodes: `node_modules/typescript-eslint`
- Fix available: `{"isSemVerMajor": false, "name": "typescript-eslint", "version": "8.65.0"}`
- Via:
  - transitive through `@typescript-eslint/eslint-plugin`
  - transitive through `@typescript-eslint/parser`
  - transitive through `@typescript-eslint/typescript-estree`
  - transitive through `@typescript-eslint/utils`
  - transitive through `eslint`

### `vite`

- Severity: `high`
- Direct dependency: `true`
- Affected range: `7.0.0 - 7.3.3`
- Installed nodes: `node_modules/vite`
- Fix available: `{"isSemVerMajor": false, "name": "vite", "version": "7.3.6"}`
- Via:
  - `low` — Vite middleware may serve files starting with the same name with the public directory; range `>=7.0.0 <=7.0.6`; source `1107325`
  - `low` — Vite's `server.fs` settings were not applied to HTML files; range `>=7.0.0 <=7.0.6`; source `1107329`
  - `moderate` — vite allows server.fs.deny bypass via backslash on Windows; range `>=7.0.0 <=7.0.7`; source `1109136`
  - `moderate` — Vite Vulnerable to Path Traversal in Optimized Deps `.map` Handling; range `>=7.0.0 <=7.3.1`; source `1116230`
  - `high` — Vite Vulnerable to Arbitrary File Read via Vite Dev Server WebSocket; range `>=7.0.0 <=7.3.1`; source `1116235`
  - `moderate` — launch-editor: NTLMv2 hash disclosure via UNC path handling on Windows; range `>=7.0.0 <=7.3.4`; source `1120785`
  - `high` — vite: `server.fs.deny` bypass on Windows alternate paths; range `>=7.0.0 <=7.3.4`; source `1123526`

### `vitest`

- Severity: `critical`
- Direct dependency: `true`
- Affected range: `<3.2.6`
- Installed nodes: `node_modules/vitest`
- Fix available: `{"isSemVerMajor": false, "name": "@vitest/coverage-v8", "version": "3.2.7"}`
- Via:
  - `critical` — When Vitest UI server is listening, arbitrary file can be read and executed; range `<3.2.6`; source `1120126`

## Policy

This report must be regenerated after intentional dependency updates. Do not use
`npm audit fix --force` as a substitute for reviewing compatibility and the
resulting lockfile diff.
