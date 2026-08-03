export const screens = [
  "packages",
  "macros",
  "macro-editor",
  "confirm",
  "execution",
  "result",
  "manage-packages",
  "package-editor",
  "import",
  "export",
  "delete-package",
  "settings",
  "diagnostics",
] as const;

export type Screen = (typeof screens)[number];

export type MacroEditorTarget =
  | { kind: "create" }
  | { kind: "edit"; macroId: string }
  | { kind: "invalid" };

export type ExecutionConfirmationTarget =
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

function hasExactKeys(
  parameters: URLSearchParams,
  expectedKeys: readonly string[],
): boolean {
  const keys = Array.from(parameters.keys()).sort();
  return (
    keys.length === expectedKeys.length &&
    keys.every((key, index) => key === expectedKeys[index])
  );
}

function validUuidParameter(
  parameters: URLSearchParams,
  name: string,
): string | null {
  const value = parameters.get(name);
  return value !== null && uuidPattern.test(value) ? value : null;
}

export function routeFromHash(fallback: Screen = "packages"): Screen {
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
  const macroId = validUuidParameter(parameters, "macroId");
  return macroId === null ? { kind: "invalid" } : { kind: "edit", macroId };
}

export function executionConfirmationTargetFromHash(): ExecutionConfirmationTarget {
  const [route, query] = hashRouteAndQuery();
  if (route !== "confirm") {
    return { kind: "invalid" };
  }
  const parameters = new URLSearchParams(query);
  const macroId = validUuidParameter(parameters, "macroId");
  if (macroId === null || !hasExactKeys(parameters, ["macroId"])) {
    return { kind: "invalid" };
  }
  return { kind: "valid", macroId };
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

export function navigateToMacroConfirmation(macroId: string): void {
  window.location.hash = `/confirm?macroId=${encodeURIComponent(macroId)}`;
}
