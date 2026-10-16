import { compileMacro } from "../../../v2/macroCompiler";

/**
 * What the editor's key picker can insert, and how it spells it. Everything
 * here produces macro-language source (SPEC_V2 §7): the picker never invents
 * syntax of its own, and `validateCombo` checks every combo it builds with
 * the same `compileMacro` the editor validates with, so the picker cannot
 * hand the editor a group the compiler would reject.
 *
 * The named-key list is the full set of standalone directives the language
 * defines for keys (Tab, the function row, navigation keys, ...). CTRL, ALT,
 * SHIFT and GUI are standalone directives too (SPEC_V2 §7.7): outside a
 * `[...]` group each taps that modifier alone; inside one they are held with
 * the other keys.
 */
export interface KeyChoice {
  /** The directive name without braces, e.g. `TAB`. */
  token: string;
  /** What a person sees, in sentence case. */
  label: string;
}

export interface KeyGroup {
  label: string;
  keys: readonly KeyChoice[];
}

const functionKeys: readonly KeyChoice[] = Array.from(
  { length: 12 },
  (_, index) => ({
    token: `F${String(index + 1)}`,
    label: `F${String(index + 1)}`,
  }),
);

export const specialKeyGroups: readonly KeyGroup[] = [
  {
    label: "Editing keys",
    keys: [
      { token: "ENTER", label: "Enter" },
      { token: "TAB", label: "Tab" },
      { token: "ESC", label: "Esc" },
      { token: "BACKSPACE", label: "Backspace" },
      { token: "DELETE", label: "Delete" },
      { token: "INSERT", label: "Insert" },
      { token: "SPACE", label: "Space" },
    ],
  },
  {
    label: "Navigation keys",
    keys: [
      { token: "UP", label: "Up" },
      { token: "DOWN", label: "Down" },
      { token: "LEFT", label: "Left" },
      { token: "RIGHT", label: "Right" },
      { token: "HOME", label: "Home" },
      { token: "END", label: "End" },
      { token: "PAGEUP", label: "Page up" },
      { token: "PAGEDOWN", label: "Page down" },
    ],
  },
  { label: "Function keys", keys: functionKeys },
];

export const modifierKeys = [
  { token: "CTRL", label: "Ctrl" },
  { token: "ALT", label: "Alt" },
  { token: "SHIFT", label: "Shift" },
  { token: "GUI", label: "GUI" },
] as const satisfies readonly KeyChoice[];

export type ModifierToken = (typeof modifierKeys)[number]["token"];

/** Every named key, including modifiers, for the combo builder's key menu. */
export const allNamedKeys: readonly KeyChoice[] = [
  ...specialKeyGroups.flatMap((group) => group.keys),
  ...modifierKeys,
];

/** The escapes for characters the language gives a meaning (SPEC_V2 §7). */
export const literalEscapes = [
  { label: "Literal {", source: "{{" },
  { label: "Literal }", source: "}}" },
  { label: "Literal [", source: "[[" },
  { label: "Literal ]", source: "]]" },
] as const;

export type ComboKey =
  | { kind: "named"; token: string }
  | { kind: "character"; character: string };

/**
 * One typed character as macro source. The four characters the language
 * reserves are doubled; everything else is itself.
 */
export function characterSource(character: string): string {
  switch (character) {
    case "{":
      return "{{";
    case "}":
      return "}}";
    case "[":
      return "[[";
    case "]":
      return "]]";
    default:
      return character;
  }
}

function keySource(key: ComboKey): string {
  return key.kind === "named"
    ? `{${key.token}}`
    : characterSource(key.character);
}

/**
 * The simultaneous-key group for the chosen modifiers and keys, e.g.
 * `[{CTRL}{SHIFT}t]`. Modifiers come first in a fixed order (Ctrl, Alt, Shift,
 * GUI) regardless of the order they were chosen in, so the same combo always
 * reads the same.
 */
export function buildComboSource(
  modifiers: readonly ModifierToken[],
  keys: readonly ComboKey[],
): string {
  const held = modifierKeys
    .filter((modifier) => modifiers.includes(modifier.token))
    .map((modifier) => `{${modifier.token}}`);
  return `[${held.join("")}${keys.map(keySource).join("")}]`;
}

export type ComboValidation =
  | { ok: true; source: string }
  | { ok: false; source: string; message: string };

/**
 * Validates a combo exactly as the editor will: by compiling it. An empty
 * combo is reported before compiling, because `[]` would otherwise surface as
 * the compiler's "empty simultaneous-key group" for a person who has simply
 * not chosen anything yet.
 */
export function validateCombo(
  modifiers: readonly ModifierToken[],
  keys: readonly ComboKey[],
): ComboValidation {
  const source = buildComboSource(modifiers, keys);
  if (modifiers.length === 0 && keys.length === 0) {
    return { ok: false, source, message: "Choose at least one key." };
  }
  const compiled = compileMacro(source, { keyPressMs: 8, interKeyMs: 15 });
  return compiled.ok
    ? { ok: true, source }
    : { ok: false, source, message: compiled.error.message };
}
