# Web Application

This is the mobile-first local interface built with TypeScript, React, Tailwind
CSS, and Vite.

## Current implementation

`main.tsx` mounts `AppV2` (`src/AppV2.tsx`), the active v2 application: hash-based
routing over Setup/Sign In, the authenticated startup state machine, Macros (list,
editor, Quick Send, optional preview), Packages, and Snapshots (save, load, delete,
import/export). These are wired to the real same-origin `/api/v1/*` firmware API
through the `v2/` contract layer (`apiClient.ts`, `repository.ts`,
`snapshotClient.ts`, `sendClient.ts`, and friends) — not representative data.
Settings and Diagnostics are not implemented in v2 yet; their routes render a
placeholder screen (Phase 12 / V2-120–V2-122 in `docs/TODO_V2.md`, not started).

The retired v1 application (`src/App.tsx`) still exists in the tree and still has
its own test suite, but it is not the mounted entry point and is not served to
users. Its package/macro/settings/diagnostic screens (imported from the legacy
`api/routes.ts`/`types/models.ts`) genuinely are presentation scaffolds with
representative data — their buttons must not be interpreted as completed
persistence or API workflows — but that applies to the retired v1 app, not to the
v2 screens described above.

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
