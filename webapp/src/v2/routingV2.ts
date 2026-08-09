/**
 * Hash routing for the v2 authenticated application shell, per
 * UI_UX_SPEC_V2 §2.2/§2.3 (TODO_V2 V2-090):
 *
 * ```text
 * Macros | Packages | Snapshots | Settings
 * ```
 *
 * Only Macros is a real destination as of Phase 9 — Packages, Snapshots, and
 * Settings render a placeholder until their own phases land (Phase 10-12).
 * A route may encode a macro selection (the optional preview screen,
 * TODO_V2 V2-094) but the in-memory repository working copy remains the
 * source of truth; a URL never causes a firmware package or macro lookup
 * (UI_UX_SPEC_V2 §2.3).
 */

export const screensV2 = [
  "macros",
  "packages",
  "snapshots",
  "settings",
  "macro-preview",
  "macro-editor",
] as const;

export type ScreenV2 = (typeof screensV2)[number];

export type MacroPreviewTarget =
  | { kind: "valid"; macroId: string }
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

export function navigateV2(target: ScreenV2): void {
  window.location.hash = `/${target}`;
}

export function navigateToMacroPreview(macroId: string): void {
  window.location.hash = `/macro-preview?macroId=${encodeURIComponent(macroId)}`;
}

/** UI_UX_SPEC_V2 §5.3/§5.5 — Quick Send never leaves the Macros page. */
export function navigateToMacros(): void {
  navigateV2("macros");
}
