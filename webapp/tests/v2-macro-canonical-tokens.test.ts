import { describe, expect, test } from "vitest";
import { compileMacro } from "../src/v2/macroCompiler";

const timing = { keyPressMs: 8, interKeyMs: 15 } as const;

describe("v2 canonical chord tokens", () => {
  test("accepts uppercase letters, digits, and named keys", () => {
    expect(compileMacro("{CTRL+A}", timing).ok).toBe(true);
    expect(compileMacro("{ALT+1}", timing).ok).toBe(true);
    expect(compileMacro("{SHIFT+F12}", timing).ok).toBe(true);
  });

  test("rejects lowercase and punctuation ordinary-key tokens", () => {
    const lowercase = compileMacro("{CTRL+a}", timing);
    expect(lowercase.ok).toBe(false);
    if (!lowercase.ok) {
      expect(lowercase.error.message).toBe("invalid chord");
    }

    const punctuation = compileMacro("{CTRL+!}", timing);
    expect(punctuation.ok).toBe(false);
    if (!punctuation.ok) {
      expect(punctuation.error.message).toBe("invalid chord");
    }
  });

  test("rejects standalone letter and digit directives", () => {
    for (const source of ["{A}", "{1}"]) {
      const result = compileMacro(source, timing);
      expect(result.ok).toBe(false);
      if (!result.ok) {
        expect(result.error.message).toBe("unknown key directive");
      }
    }
  });
});
