export const screens = [
  "sets",
  "procedures",
  "procedure",
  "instruction",
  "procedure-editor",
  "macros",
  "macro-editor",
  "confirm",
  "execution",
  "result",
  "manage-sets",
  "set-editor",
  "import",
  "export",
  "delete-set",
  "settings",
  "diagnostics",
] as const;

export type Screen = (typeof screens)[number];

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

export function routeFromHash(fallback: Screen = "sets"): Screen {
  const [route] = hashRouteAndQuery();
  return screens.includes(route as Screen) ? (route as Screen) : fallback;
}

export function macroEditorTargetFromHash(): MacroEditorTarget {
  const [route, query] = hashRouteAndQuery();
  if (route !== "macro-editor") {
    return { kind: "invalid" };
  }
  const parameters = new URLSearchParams(query);
  const keys = Array.from(parameters.keys());
  if (keys.length === 0) {
    return { kind: "create" };
  }
  if (keys.length !== 1 || keys[0] !== "macroId") {
    return { kind: "invalid" };
  }
  const macroId = parameters.get("macroId");
  return macroId !== null && uuidPattern.test(macroId)
    ? { kind: "edit", macroId }
    : { kind: "invalid" };
}

export function navigate(target: Screen): void {
  window.location.hash = `/${target}`;
}

export function navigateToMacroEditor(macroId: string | null): void {
  window.location.hash =
    macroId === null
      ? "/macro-editor"
      : `/macro-editor?macroId=${encodeURIComponent(macroId)}`;
}

export function replaceMacroEditorTarget(macroId: string): void {
  const hash = `#/macro-editor?macroId=${encodeURIComponent(macroId)}`;
  const next = `${window.location.pathname}${window.location.search}${hash}`;
  window.history.replaceState(null, "", next);
}
