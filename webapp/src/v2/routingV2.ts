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
 * required screen #14 but is not one of the four fixed bottom-navigation
 * destinations, so it is not its own nav entry. A route may encode a macro
 * selection (the editor, TODO_V2 V2-100) but the in-memory repository
 * working copy remains the source of truth; a URL never causes a firmware
 * package or macro lookup (UI_UX_SPEC_V2 §2.3).
 */

export const screensV2 = [
  "macros",
  "packages",
  "snapshots",
  "settings",
  "diagnostics",
  "macro-editor",
] as const;

export type ScreenV2 = (typeof screensV2)[number];

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

export type SnapshotsView = "list" | "files";

/**
 * Snapshots has two screens under one route: the snapshot list, and "Import
 * and export" (`#/snapshots?view=files`). Any other query is the list, so a
 * malformed hash can only ever land on the safe, ordinary screen.
 */
export function snapshotsViewFromHash(): SnapshotsView {
  const [route, query] = hashRouteAndQuery();
  if (route !== "snapshots") {
    return "list";
  }
  const parameters = new URLSearchParams(query);
  return parameters.get("view") === "files" ? "files" : "list";
}

export const settingsGroups = [
  "device",
  "network",
  "security",
  "usb",
  "reset",
] as const;
export type SettingsGroup = (typeof settingsGroups)[number];

export const settingsSections = [
  "access-point",
  "station",
  "password",
  "session",
  "restart",
  "reset-settings",
  "factory-reset",
] as const;
export type SettingsSection = (typeof settingsSections)[number];

/**
 * Settings is a list of groups; each group is its own screen
 * (`#/settings?group=network`). No group, or one that is not recognised, is
 * the list, so a malformed hash can only land on the ordinary screen.
 */
export function settingsGroupFromHash(): SettingsGroup | null {
  const [route, query] = hashRouteAndQuery();
  if (route !== "settings") {
    return null;
  }
  const group = new URLSearchParams(query).get("group");
  return settingsGroups.find((candidate) => candidate === group) ?? null;
}

export function navigateToSettingsGroup(group: SettingsGroup): void {
  window.location.hash = `/settings?group=${group}`;
}

export function settingsSectionFromHash(): SettingsSection | null {
  const [route, query] = hashRouteAndQuery();
  if (route !== "settings") {
    return null;
  }
  const group = settingsGroupFromHash();
  const section = new URLSearchParams(query).get("section");
  if (group === "network") {
    return section === "access-point" || section === "station" ? section : null;
  }
  if (group === "security") {
    return section === "password" || section === "session" ? section : null;
  }
  if (group === "reset") {
    return section === "restart" ||
      section === "reset-settings" ||
      section === "factory-reset"
      ? section
      : null;
  }
  return null;
}

export function navigateToSettingsSection(section: SettingsSection): void {
  const group =
    section === "access-point" || section === "station"
      ? "network"
      : section === "password" || section === "session"
        ? "security"
        : "reset";
  window.location.hash = `/settings?group=${group}&section=${section}`;
}

export function navigateToSnapshotsFiles(): void {
  window.location.hash = "/snapshots?view=files";
}

export function navigateV2(target: ScreenV2): void {
  window.location.hash = `/${target}`;
}

export function navigateToAddMacro(): void {
  window.location.hash = "/macro-editor";
}

export function navigateToEditMacro(macroId: string): void {
  window.location.hash = `/macro-editor?macroId=${encodeURIComponent(macroId)}`;
}

/** UI_UX_SPEC_V2 §5.3/§5.4 — Quick Send never leaves the Macros page. */
export function navigateToMacros(): void {
  navigateV2("macros");
}

export function navigateToSettings(): void {
  navigateV2("settings");
}

/** Diagnostics (UI_UX_SPEC_V2 §4 screen 14) is reachable only from Settings. */
export function navigateToDiagnostics(): void {
  navigateV2("diagnostics");
}
