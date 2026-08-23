import { describe, expect, test } from "vitest";
import { compileMacro } from "../src/v2/macroCompiler";

const timing = { keyPressMs: 8, interKeyMs: 15 } as const;

/* Directive spelling is uppercase and canonical, for both a standalone
 * modifier tap and a modifier/key inside a [...] simultaneous-key group --
 * there is no case-insensitive fallback anywhere in the grammar. The retired
 * {MOD+KEY} chord syntax this file used to cover no longer parses at all
 * (checked here too): '+' has no meaning inside a directive body since the
 * [...] group replaced it. */
describe("v2 canonical directive tokens", () => {
  test("accepts canonical standalone modifiers and group members", () => {
    expect(compileMacro("{CTRL}", timing).ok).toBe(true);
    expect(compileMacro("{ALT}", timing).ok).toBe(true);
    expect(compileMacro("[{SHIFT}{F12}]", timing).ok).toBe(true);
    expect(compileMacro("[{CTRL}a]", timing).ok).toBe(true);
  });

  test("rejects lowercase modifier and named-key directives", () => {
    for (const source of ["{ctrl}", "[{ctrl}a]", "[{CTRL}{f2}]"]) {
      const result = compileMacro(source, timing);
      expect(result.ok).toBe(false);
      if (!result.ok) {
        expect(result.error.message).toBe("unknown key directive");
      }
    }
  });

  test("rejects standalone letter and digit directives, and the retired chord syntax", () => {
    for (const source of ["{A}", "{1}", "{CTRL+A}"]) {
      const result = compileMacro(source, timing);
      expect(result.ok).toBe(false);
      if (!result.ok) {
        expect(result.error.message).toBe("unknown key directive");
      }
    }
  });
});
