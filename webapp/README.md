# Web Application

This is the mobile-first local interface built with TypeScript, React, Tailwind
CSS, and Vite.

## Current implementation

`main.tsx` mounts `AppV2` (`src/AppV2.tsx`), the only application tree in the
codebase: hash-based routing over Setup/Sign In, the authenticated startup state
machine, Macros (list, editor, Quick Send, optional preview), Packages, Snapshots
(save, load, delete, import/export), Settings, and Diagnostics. These are wired to
the real same-origin `/api/v1/*` firmware API through the `v2/` contract layer
(`apiClient.ts`, `repository.ts`, `snapshotClient.ts`, `sendClient.ts`,
`settingsClient.ts`, `diagnosticsClient.ts`, and friends) — not representative
data. Settings and Diagnostics shipped in Phase 12 (V2-120–V2-122 in
`docs/TODO_V2.md`), including the destructive device actions (restart,
reset-settings, factory-reset) and their reconnect handling.

The retired v1 application (`src/App.tsx` and the route tree it rendered — v1
`features/execution/`, `features/package/`, `features/macros/MacroEditorPage.tsx`
and `MacroLibraryPage.tsx`, the non-`v2/` parts of `features/auth/` and
`features/settings/`, `src/routing.ts`, `src/api/`, `src/types/models.ts`, and the
v1-only shared components `AppShell`, `ConnectivityBanner`, `AccessibleDialog`) was
deleted in V2-140 — it was never the mounted entry point, was not served to users,
and its package/macro/settings/diagnostic screens were presentation scaffolds over
representative data, not real persistence. Nothing in the codebase references it
anymore.

## Commands

The exact Node.js version is pinned in `.nvmrc`.

```bash
nvm use
cd webapp
npm run typecheck
npm run lint
npm run stylelint
npm run format:check
npm test
npm run build
```

A committed `package-lock.json` exists (`webapp/package-lock.json`) — `npm ci` is
safe to use for reproducible installs.
