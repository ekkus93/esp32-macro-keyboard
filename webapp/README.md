# Web Application

This is the mobile-first local interface built with TypeScript, React, Tailwind
CSS, and Vite.

## Current implementation

The application has hash-based routing for all planned screens. Login, CSRF token
handling, execution polling, cancellation, and visible request errors use the real
same-origin firmware API.

Most set, macro, procedure, import/export, settings, and diagnostic screens are
currently presentation scaffolds with representative data. Their buttons must not
be interpreted as completed persistence or API workflows.

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

A committed `package-lock.json` does not exist yet. Do not claim reproducible
`npm ci` validation or enable the full automatic quality workflow until the lockfile
is generated from a successful dependency resolution and committed.
