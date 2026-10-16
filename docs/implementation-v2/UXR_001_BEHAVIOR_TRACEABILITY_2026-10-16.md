# UXR-001 — UI behavior traceability

**Date:** 2026-10-16
**Scope:** presentation changes in the Material 3 redesign. Normative behavior
remains in `SPEC_V2.md` and `UI_UX_SPEC_V2.md`.

| UI flow | Normative behavior | Regression coverage |
| --- | --- | --- |
| Startup, setup, sign-in, reconnect and recovery | `SPEC_V2.md` §§12.2–12.3, 13.4–13.6; `UI_UX_SPEC_V2.md` §§3, 8, 15 | `v2-startup-decision-table`, `v2-first-run-setup-page`, `v2-sign-in-page`, `v2-repository-startup-screen`, `v2-device-reconnect*`; browser `startup.mjs` |
| Shell, routing, phone orientation and active-send access | `SPEC_V2.md` §§14.1, 14.7; `UI_UX_SPEC_V2.md` §§2, 12–14 | `v2-routing`, `v2-app-shell`, `v2-app-v2-orientation`; browser `visibleControls.mjs`, `browser.mjs` |
| Macro list, source visibility, execution and cancellation | `SPEC_V2.md` §§7, 14.5–14.6; `UI_UX_SPEC_V2.md` §§5, 12.3 | `v2-macros-page-*`, `v2-send-client`; browser `browser.mjs`, `visibleControls.mjs` |
| Macro editor and key insertion | `SPEC_V2.md` §§7.5–7.11; `UI_UX_SPEC_V2.md` §§7.1–7.3 | `v2-macro-editor-page`, `v2-macro-insertions`; browser `macroEditing.mjs` |
| Package choice and management | `SPEC_V2.md` §§8.3–8.6, 11.2; `UI_UX_SPEC_V2.md` §§3.6, 6–7 | `v2-package-management-page`, `v2-routing`; browser `browser.mjs`, `visibleControls.mjs` |
| Working copy, dirty state and unsaved-work guard | `SPEC_V2.md` §§8.5–8.7; `UI_UX_SPEC_V2.md` §§7.2–7.3 | `v2-unsaved-changes-prompt`, `v2-snapshots-page-protection`; browser `browser.mjs`, `snapshots.mjs` |
| Snapshots, storage capacity, retention and restore | `SPEC_V2.md` §§9–10; `UI_UX_SPEC_V2.md` §§9.1–9.7 | `v2-snapshot-client`, `v2-snapshots-page-*`; browser `snapshots.mjs`, H4/H5 storage recovery |
| Repository import/export | `SPEC_V2.md` §§8.7–8.8, 9; `UI_UX_SPEC_V2.md` §10 | `v2-snapshots-page-transfer`, `v2-snapshot-client`; browser `snapshots.mjs` |
| Device settings, credentials, network and reset | `SPEC_V2.md` §§11–12, 13.9, 13.12; `UI_UX_SPEC_V2.md` §11 | `v2-settings-page`, `v2-settings-*`, `v2-device-reconnect*`; browser `settings.mjs`, `visibleControls.mjs` |
| Diagnostics and privacy | `SPEC_V2.md` §§13.13, 16.3; `UI_UX_SPEC_V2.md` §15 | `v2-diagnostics-*`; browser `settings.mjs` (diagnostics export) |
| HTTP/API errors and offline recovery | `SPEC_V2.md` §§13.2, 16.1; `UI_UX_SPEC_V2.md` §15 | `v2-error-text`, client and startup decision tests; browser startup and recovery workflows |
| Visual, accessibility and quality constraints | `SPEC_V2.md` §§17–18; `UI_UX_SPEC_V2.md` §§12–16 | browser `visibleControls.mjs` (viewport and axe), `test:browser`, `test:visual`, contrast tests |

The screen map was checked against these sections. Subpages preserve existing
route meanings and device calls. Settings route query parameters only select a
presentation subsection; they do not add API or persistence behavior. Existing
snapshot replacement, typed deletion, unsaved-work protection, package
selection preference, source privacy, send cancellation, and phone landscape
blocking remain covered by the listed tests. The detailed visual and
accessibility qualification is tracked separately in UXR-071/072.
