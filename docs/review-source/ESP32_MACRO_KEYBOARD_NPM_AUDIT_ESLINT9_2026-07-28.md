# ESLint 9.39.4 frontend audit diagnostic

## Summary

- High: 5
- Critical: 0
- Total: 5

## Findings

### `@eslint/config-array`

- Severity: `high`
- Direct dependency: `false`
- Affected range: `<=0.22.0`
- Nodes: `node_modules/@eslint/config-array`
- Fix available: `{"isSemVerMajor": true, "name": "eslint", "version": "10.8.0"}`
- Via:
  - transitive through `minimatch`

### `@eslint/eslintrc`

- Severity: `high`
- Direct dependency: `false`
- Affected range: `0.0.1 || >=0.1.1`
- Nodes: `node_modules/@eslint/eslintrc`
- Fix available: `{"isSemVerMajor": true, "name": "eslint", "version": "10.8.0"}`
- Via:
  - transitive through `minimatch`

### `brace-expansion`

- Severity: `high`
- Direct dependency: `false`
- Affected range: `<=5.0.7`
- Nodes: `node_modules/brace-expansion`
- Fix available: `{"isSemVerMajor": true, "name": "eslint", "version": "10.8.0"}`
- Via:
  - `high` — brace-expansion: DoS via unbounded expansion length causing an out-of-memory process crash; range `<=5.0.7`; source `1124334`

### `eslint`

- Severity: `high`
- Direct dependency: `true`
- Affected range: `0.12.0 - 2.0.0-rc.1 || 4.1.0 - 10.0.0-rc.2`
- Nodes: `node_modules/eslint`
- Fix available: `{"isSemVerMajor": true, "name": "eslint", "version": "10.8.0"}`
- Via:
  - transitive through `@eslint/config-array`
  - transitive through `@eslint/eslintrc`
  - transitive through `minimatch`

### `minimatch`

- Severity: `high`
- Direct dependency: `false`
- Affected range: `2.0.0 - 10.0.2`
- Nodes: `node_modules/minimatch`
- Fix available: `{"isSemVerMajor": true, "name": "eslint", "version": "10.8.0"}`
- Via:
  - transitive through `brace-expansion`
