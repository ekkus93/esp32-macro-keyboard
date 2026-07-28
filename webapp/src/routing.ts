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

export function routeFromHash(fallback: Screen = "sets"): Screen {
  const route = window.location.hash.replace(/^#\/?/, "");
  return screens.includes(route as Screen) ? (route as Screen) : fallback;
}

export function navigate(target: Screen): void {
  window.location.hash = `/${target}`;
}
