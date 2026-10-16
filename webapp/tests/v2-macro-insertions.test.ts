import { describe, expect, test } from "vitest";
import {
  allNamedKeys,
  buildComboSource,
  characterSource,
  literalEscapes,
  modifierKeys,
  specialKeyGroups,
  validateCombo,
} from "../src/features/macros/v2/macroInsertions";
import { compileMacro } from "../src/v2/macroCompiler";

const timing = { keyPressMs: 8, interKeyMs: 15 };

describe("macro key picker vocabulary", () => {
  test("every named key the picker offers compiles as a standalone directive", () => {
    for (const key of allNamedKeys) {
      expect(
        compileMacro(`{${key.token}}`, timing).ok,
        `{${key.token}} should compile`,
      ).toBe(true);
    }
  });

  test("offers Tab, the function row, navigation keys and all four modifiers", () => {
    const tokens = new Set(allNamedKeys.map((key) => key.token));
    for (const token of ["TAB", "F1", "F12", "PAGEUP", "HOME", "UP"]) {
      expect(tokens.has(token)).toBe(true);
    }
    expect(modifierKeys.map((modifier) => modifier.token)).toEqual([
      "CTRL",
      "ALT",
      "SHIFT",
      "GUI",
    ]);
    expect(specialKeyGroups.map((group) => group.label)).toEqual([
      "Editing keys",
      "Navigation keys",
      "Function keys",
    ]);
  });

  test("every literal escape compiles to exactly one key", () => {
    for (const escape of literalEscapes) {
      const compiled = compileMacro(escape.source, timing);
      expect(compiled.ok, escape.source).toBe(true);
      if (compiled.ok) {
        expect(compiled.actions).toHaveLength(1);
      }
    }
  });
});

describe("macro combo builder", () => {
  test("builds the documented example, modifiers first in a fixed order", () => {
    expect(
      buildComboSource(
        ["SHIFT", "CTRL"],
        [{ kind: "character", character: "t" }],
      ),
    ).toBe("[{CTRL}{SHIFT}t]");
  });

  test("builds named keys and several keys in one group", () => {
    expect(
      buildComboSource(["CTRL", "ALT"], [{ kind: "named", token: "DELETE" }]),
    ).toBe("[{CTRL}{ALT}{DELETE}]");
    expect(
      buildComboSource(
        ["GUI"],
        [
          { kind: "character", character: "a" },
          { kind: "character", character: "b" },
        ],
      ),
    ).toBe("[{GUI}ab]");
  });

  test("doubles the four reserved characters", () => {
    expect(characterSource("{")).toBe("{{");
    expect(characterSource("}")).toBe("}}");
    expect(characterSource("[")).toBe("[[");
    expect(characterSource("]")).toBe("]]");
    expect(characterSource("t")).toBe("t");
  });

  test("accepts a valid combo and returns the source to insert", () => {
    const result = validateCombo(
      ["CTRL", "SHIFT"],
      [{ kind: "character", character: "t" }],
    );
    expect(result).toEqual({ ok: true, source: "[{CTRL}{SHIFT}t]" });
  });

  test("a reserved character inside a group still compiles", () => {
    for (const character of ["{", "}", "[", "]"]) {
      const result = validateCombo(
        ["CTRL"],
        [{ kind: "character", character }],
      );
      expect(result.ok, character).toBe(true);
    }
  });

  test("rejects an empty combo with a plain message before compiling", () => {
    expect(validateCombo([], [])).toEqual({
      ok: false,
      source: "[]",
      message: "Choose at least one key.",
    });
  });

  test("rejects a duplicate key and more than six keys, with the compiler's reason", () => {
    const duplicate = validateCombo(
      [],
      [
        { kind: "character", character: "a" },
        { kind: "character", character: "a" },
      ],
    );
    expect(duplicate.ok).toBe(false);
    const seven = validateCombo(
      [],
      "abcdefg"
        .split("")
        .map((character) => ({ kind: "character", character })),
    );
    expect(seven.ok).toBe(false);
    if (!seven.ok) {
      expect(seven.message).toContain("6-key limit");
    }
  });

  test("a combo made only of modifiers is valid", () => {
    expect(validateCombo(["CTRL", "ALT"], []).ok).toBe(true);
  });
});
