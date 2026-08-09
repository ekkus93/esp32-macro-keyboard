/**
 * Hash routing for the v2 authenticated application shell, per
 * UI_UX_SPEC_V2 §2.2/§2.3 (TODO_V2 V2-090):
 *
 * ```text
 * Macros | Packages | Snapshots | Settings
 * ```
 *
 * Macros, macro editing, Packages, Snapshots, and Settings are all real
 * destinations as of Phase 12. `diagnostics` (TODO_V2 V2-122) is reachable
 * only from within Settings, per UI_UX_SPEC_V2 §4/§11 — Diagnostics is
 * required screen #15 but is not one of the four fixed bottom-navigation
 * destinations, so it is not its own nav entry. A route may encode a macro
 * selection (the optional preview screen, TODO_V2 V2-094, or the editor,
 * TODO_V2 V2-100) but the in-memory repository working copy remains the
 * source of truth; a URL never causes a firmware package or macro lookup
 * (UI_UX_SPEC_V2 §2.3).
 */

export const screensV2 = [
  "macros",
  "packages",
  "snapshots",
  "settings",
  "diagnostics",
  "macro-preview",
  "macro-editor",
] as const;

export type ScreenV2 = (typeof screensV2)[number];

export type MacroPreviewTarget =
  | { kind: "valid"; macroId: string }
  | { kind: "invalid" };

export type MacroEditorTarget =
  | { kind: "create" }
  | { kind: "edit"; macroId: string }
  | { kind: "invalid" };

const uuidPattern =
  /^[0-9a-f]{8}-[0-9a-f]{4}-[1-5][0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/i;

function hashRouteAndQuery(): [string, string] {
  const value = window.location.hash.replace(/^#\/?/, "");
  const separator = value.indexOf("?");
  return separator < 0
    ? [value, ""]
    : [value.slice(0, separator), value.slice(separator + 1)];
}

function isScreenV2(value: string): value is ScreenV2 {
  return (screensV2 as readonly string[]).includes(value);
}

export function routeFromHashV2(fallback: ScreenV2 = "macros"): ScreenV2 {
  const [route] = hashRouteAndQuery();
  return isScreenV2(route) ? route : fallback;
}

/**
 * Reads the optional-preview target (TODO_V2 V2-094) from the current hash.
 * `{ kind: "invalid" }` covers both "not on the preview route" and "on the
 * preview route with a missing/malformed macro ID" so a caller can fall back
 * to the Macros page in either case without distinguishing them.
 */
export function macroPreviewTargetFromHash(): MacroPreviewTarget {
  const [route, query] = hashRouteAndQuery();
  if (route !== "macro-preview") {
    return { kind: "invalid" };
  }
  const parameters = new URLSearchParams(query);
  const keys = Array.from(parameters.keys());
  const macroId = parameters.get("macroId");
  if (
    keys.length !== 1 ||
    keys[0] !== "macroId" ||
    macroId === null ||
    !uuidPattern.test(macroId)
  ) {
    return { kind: "invalid" };
  }
  return { kind: "valid", macroId };
}

/**
 * Reads the macro-editor target (TODO_V2 V2-100) from the current hash. No
 * `macroId` query parameter means "create a new macro" — any other malformed
 * combination (wrong key, extra keys, a non-UUID value) is `invalid` so the
 * caller can fall back rather than editing the wrong macro.
 */
export function macroEditorTargetFromHash(): MacroEditorTarget {
  const [route, query] = hashRouteAndQuery();
  if (route !== "macro-editor") {
    return { kind: "invalid" };
  }
  if (query.length === 0) {
    return { kind: "create" };
  }
  const parameters = new URLSearchParams(query);
  const keys = Array.from(parameters.keys());
  const macroId = parameters.get("macroId");
  if (
    keys.length !== 1 ||
    keys[0] !== "macroId" ||
    macroId === null ||
    !uuidPattern.test(macroId)
  ) {
    return { kind: "invalid" };
  }
  return { kind: "edit", macroId };
}

export function navigateV2(target: ScreenV2): void {
  window.location.hash = `/${target}`;
}

export function navigateToMacroPreview(macroId: string): void {
  window.location.hash = `/macro-preview?macroId=${encodeURIComponent(macroId)}`;
}

export function navigateToAddMacro(): void {
  window.location.hash = "/macro-editor";
}

export function navigateToEditMacro(macroId: string): void {
  window.location.hash = `/macro-editor?macroId=${encodeURIComponent(macroId)}`;
}

/** UI_UX_SPEC_V2 §5.3/§5.5 — Quick Send never leaves the Macros page. */
export function navigateToMacros(): void {
  navigateV2("macros");
}

export function navigateToSettings(): void {
  navigateV2("settings");
}

/** Diagnostics (UI_UX_SPEC_V2 §4 screen 15) is reachable only from Settings. */
export function navigateToDiagnostics(): void {
  navigateV2("diagnostics");
}
