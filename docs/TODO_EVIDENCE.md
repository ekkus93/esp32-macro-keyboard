# TODO evidence ledger

This ledger supplements `docs/TODO.md`; it does not redefine requirements or
mark a completion gate passed without its required evidence.

| Area | Evidence | Status |
|---|---|---|
| 1.1 repository structure | Tracked paths and `scripts/bootstrap-repo.sh` | Implemented; ShellCheck/shfmt gate pending installed tools |
| 1.2 exact ESP-IDF pin | Installer, verifier, `.env.example`, development guide | Source implemented; clean recursive install not run here |
| 1.3–1.4 skeleton/partitions | Firmware CMake, exact manifest constraints, defaults, CSV/checker | Source implemented; IDF resolution/build and lock pending |
| 1.5 frontend bootstrap | Exact dependency declarations and Vite/React/TS/Tailwind source | Source implemented; lockfile and `npm ci` pending |
| 2 quality gate | Strict configs/check scripts and manual workflow entry point | Partial; unavailable tools and lockfiles remain blocking |
| 3 shared model | Limits, UUIDs, stable errors, bounded C/TS models | Host model/parser build passes |
| 4 parser/compiler | Parser, US keymap, location errors, limits, fuzz corpus | Strict host tests pass |
| 5 storage | Separate mounts, paths, verified atomic writes, manifest foundation | Partial; repositories, quarantine, recovery phases, fault injection open |
| 6 USB/executor | Descriptors, state, reports, owned executor, cancellation, watchdog | Partial; IDF build and hardware tests open |
| 7 controls/startup | Kconfig, debounce, indicator task, staged startup/rollback | Partial; reset gestures and hardware evidence open |
| 8 auth/SoftAP | PBKDF2, RAM sessions, CSRF, throttle, protected AP | Partial; persistent secure provisioning open |
| 9 API | Static server, auth, status, limits, current execution, cancel | Partial; submission and resource APIs open |
| 10–11 web app | Typed client, shell, navigation, all required screen states | Foundation only; persistence flows, installed-tool checks, a11y/E2E open |
| 12 package schemas | Three committed versioned schemas | Schema source implemented; firmware import/export open |
| 13 security review | Committed review and direct fixes from each pass | Active; blocking findings remain |
| 14 hardware validation | Test plan contains no fabricated passes | Not run |
| 15 release | Blocker list and unreleased notes | Not ready |
