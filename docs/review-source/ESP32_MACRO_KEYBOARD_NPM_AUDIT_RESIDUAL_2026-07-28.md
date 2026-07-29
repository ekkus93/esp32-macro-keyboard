# Residual frontend npm audit after intended upgrades

## Summary

- High: 5
- Critical: 0
- Total: 5

## Findings

### `@vitest/coverage-v8`

- Severity: `high`
- Direct dependency: `true`
- Affected range: `<=3.2.7`
- Nodes: `node_modules/@vitest/coverage-v8`
- Fix available: `{"isSemVerMajor": true, "name": "@vitest/coverage-v8", "version": "4.1.10"}`
- Via:
  - transitive through `test-exclude`

### `brace-expansion`

- Severity: `high`
- Direct dependency: `false`
- Affected range: `<=5.0.7`
- Nodes: `node_modules/glob/node_modules/brace-expansion`
- Fix available: `{"isSemVerMajor": true, "name": "@vitest/coverage-v8", "version": "4.1.10"}`
- Via:
  - `high` — brace-expansion: DoS via unbounded expansion length causing an out-of-memory process crash; range `<=5.0.7`; source `1124334`

### `glob`

- Severity: `high`
- Direct dependency: `false`
- Affected range: `4.3.0 - 10.5.0`
- Nodes: `node_modules/glob`
- Fix available: `{"isSemVerMajor": true, "name": "@vitest/coverage-v8", "version": "4.1.10"}`
- Via:
  - transitive through `minimatch`

### `minimatch`

- Severity: `high`
- Direct dependency: `false`
- Affected range: `2.0.0 - 10.0.2`
- Nodes: `node_modules/glob/node_modules/minimatch`
- Fix available: `{"isSemVerMajor": true, "name": "@vitest/coverage-v8", "version": "4.1.10"}`
- Via:
  - transitive through `brace-expansion`

### `test-exclude`

- Severity: `high`
- Direct dependency: `false`
- Affected range: `5.2.0 - 7.0.2`
- Nodes: `node_modules/test-exclude`
- Fix available: `{"isSemVerMajor": true, "name": "@vitest/coverage-v8", "version": "4.1.10"}`
- Via:
  - transitive through `glob`
