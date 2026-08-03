import { describe, expect, test } from "vitest";
import canonicalRepository from "../../contracts/v2/repository/canonical.json";
import {
  createEmptyRepository,
  validateRepositoryForUse,
} from "../src/v2/repositoryValidation";

interface MutableMacro {
  id: string;
  name: string;
  source: string;
  keyPressMs: number;
  interKeyMs: number;
}

interface MutablePackage {
  id: string;
  name: string;
  macros: MutableMacro[];
}

interface MutableRepository {
  format: string;
  schemaVersion: number;
  packages: MutablePackage[];
}

function mutableCanonical(): MutableRepository {
  return structuredClone(canonicalRepository);
}

function firstMacro(repository: MutableRepository): MutableMacro {
  const pkg = repository.packages[0];
  if (pkg === undefined) {
    throw new Error("canonical fixture has no package");
  }
  const macro = pkg.macros[0];
  if (macro === undefined) {
    throw new Error("canonical fixture has no macro");
  }
  return macro;
}

describe("v2 repository compiled validation", () => {
  test("accepts the canonical repository", () => {
    expect(validateRepositoryForUse(canonicalRepository).ok).toBe(true);
  });

  test("creates the exact empty repository", () => {
    expect(createEmptyRepository()).toEqual({
      format: "esp32-macro-keyboard-repository",
      schemaVersion: 1,
      packages: [],
    });
  });

  test("rejects invalid macro grammar at the exact macro path", () => {
    const invalid = mutableCanonical();
    firstMacro(invalid).source = "{BAD}";
    const result = validateRepositoryForUse(invalid);
    expect(result).toEqual({
      ok: false,
      issues: [
        {
          path: "$.packages[0].macros[0].source",
          code: "invalid_macro_source",
          message: "unknown key directive",
        },
      ],
    });
  });

  test("rejects a valid source whose configured timing exceeds 300 seconds", () => {
    const invalid = mutableCanonical();
    const macro = firstMacro(invalid);
    macro.source = "abcdefghijklmnop";
    macro.keyPressMs = 10_000;
    macro.interKeyMs = 10_000;
    const result = validateRepositoryForUse(invalid);
    expect(result).toEqual({
      ok: false,
      issues: [
        {
          path: "$.packages[0].macros[0].source",
          code: "macro_limit",
          message: "estimated duration limit exceeded",
        },
      ],
    });
  });
});
