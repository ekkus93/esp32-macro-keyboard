import { describe, expect, test } from "vitest";
import rawCorpus from "../../contracts/v2/macro-conformance.json";
import {
  compileMacro,
  type MacroAction,
  type MacroCompileError,
} from "../src/v2/macroCompiler";
import {
  macroErrorMessageClass,
  type MacroErrorMessageClass,
} from "../src/v2/macroErrorClass";

interface CorpusValid {
  estimatedDurationMs: number;
  actions: MacroAction[];
}

interface CorpusInvalid {
  code: MacroCompileError["code"];
  byteOffset: number;
  line: number;
  column: number;
  messageClass: MacroErrorMessageClass;
}

interface CorpusCase {
  name: string;
  source: string;
  keyPressMs: number;
  interKeyMs: number;
  valid?: CorpusValid;
  invalid?: CorpusInvalid;
}

interface Corpus {
  format: string;
  version: number;
  cases: CorpusCase[];
}

const corpus = rawCorpus as Corpus;

describe("v2 shared macro conformance corpus", () => {
  test("has the reviewed identity and nonempty case list", () => {
    expect(corpus.format).toBe("esp32-macro-keyboard-macro-conformance");
    expect(corpus.version).toBe(1);
    expect(corpus.cases.length).toBeGreaterThan(0);
  });

  for (const corpusCase of corpus.cases) {
    test(corpusCase.name, () => {
      const result = compileMacro(corpusCase.source, {
        keyPressMs: corpusCase.keyPressMs,
        interKeyMs: corpusCase.interKeyMs,
      });
      if (corpusCase.valid !== undefined) {
        expect(corpusCase.invalid).toBeUndefined();
        expect(result).toEqual({ ok: true, ...corpusCase.valid });
        return;
      }

      expect(corpusCase.invalid).toBeDefined();
      if (result.ok || corpusCase.invalid === undefined) {
        throw new Error(`Expected ${corpusCase.name} to fail`);
      }
      expect({
        code: result.error.code,
        byteOffset: result.error.byteOffset,
        line: result.error.line,
        column: result.error.column,
        messageClass: macroErrorMessageClass(result.error),
      }).toEqual(corpusCase.invalid);
    });
  }
});

describe("v2 macro compiler boundaries", () => {
  test("accepts all printable ASCII when braces are escaped", () => {
    let source = "";
    for (let code = 0x20; code <= 0x7e; code += 1) {
      const character = String.fromCharCode(code);
      source += character === "{" ? "{{" : character === "}" ? "}}" : character;
    }
    const result = compileMacro(source, { keyPressMs: 0, interKeyMs: 0 });
    expect(result.ok).toBe(true);
    if (!result.ok) {
      throw new Error(result.error.message);
    }
    expect(result.actions).toHaveLength(95);
    expect(result.estimatedDurationMs).toBe(0);
  });

  test("accepts exactly 4096 compiled actions and rejects the next", () => {
    const maximum = compileMacro("a".repeat(4096), {
      keyPressMs: 0,
      interKeyMs: 0,
    });
    expect(maximum.ok).toBe(true);
    if (!maximum.ok) {
      throw new Error(maximum.error.message);
    }
    expect(maximum.actions).toHaveLength(4096);

    const excessive = compileMacro("a".repeat(4097), {
      keyPressMs: 0,
      interKeyMs: 0,
    });
    expect(excessive).toEqual({
      ok: false,
      error: {
        code: "macro_limit",
        byteOffset: 0,
        line: 1,
        column: 1,
        message: "macro source exceeds the byte limit",
      },
    });
  });

  test("accepts exactly 300 seconds and rejects a longer estimate", () => {
    const maximum = compileMacro("abcdefghijklmno", {
      keyPressMs: 10_000,
      interKeyMs: 10_000,
    });
    expect(maximum.ok).toBe(true);
    if (!maximum.ok) {
      throw new Error(maximum.error.message);
    }
    expect(maximum.estimatedDurationMs).toBe(300_000);

    const excessive = compileMacro("abcdefghijklmnop", {
      keyPressMs: 10_000,
      interKeyMs: 10_000,
    });
    expect(excessive).toEqual({
      ok: false,
      error: {
        code: "macro_limit",
        byteOffset: 15,
        line: 1,
        column: 16,
        message: "estimated duration limit exceeded",
      },
    });
  });

  test("rejects noninteger and out-of-range timing", () => {
    expect(compileMacro("a", { keyPressMs: 0.5, interKeyMs: 0 })).toEqual({
      ok: false,
      error: {
        code: "invalid_argument",
        byteOffset: 0,
        line: 1,
        column: 1,
        message: "invalid macro timing",
      },
    });
    expect(compileMacro("a", { keyPressMs: 0, interKeyMs: 10_001 }).ok).toBe(
      false,
    );
  });
});
