import { describe, expect, test } from "vitest";
import canonicalRepository from "../../contracts/v2/repository/canonical.json";
import {
  serializeRepositoryV1,
  validateRepositoryV1,
} from "../src/v2/repository";

const acceptSource = (): null => null;

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
  activePackageId?: string;
}

class PrototypeBearingRepository {
  readonly format = "esp32-macro-keyboard-repository";
  readonly schemaVersion = 1;
  readonly packages: MutablePackage[] = [];
}

function mutableCanonical(): MutableRepository {
  return structuredClone(canonicalRepository) as MutableRepository;
}

describe("v2 repository contract", () => {
  test("accepts and canonically serializes the checked-in repository", () => {
    const result = validateRepositoryV1(canonicalRepository, acceptSource);
    expect(result.ok).toBe(true);
    if (!result.ok) {
      throw new Error("canonical repository was rejected");
    }
    expect(serializeRepositoryV1(result.value)).toBe(
      JSON.stringify(canonicalRepository),
    );
  });

  test("accepts an exact empty repository", () => {
    const result = validateRepositoryV1(
      {
        format: "esp32-macro-keyboard-repository",
        schemaVersion: 1,
        packages: [],
      },
      acceptSource,
    );
    expect(result).toEqual({
      ok: true,
      value: {
        format: "esp32-macro-keyboard-repository",
        schemaVersion: 1,
        packages: [],
      },
    });
  });

  test("rejects activePackageId and every unknown root field", () => {
    const invalid = mutableCanonical();
    invalid.activePackageId = "550e8400-e29b-41d4-a716-446655440000";
    const result = validateRepositoryV1(invalid, acceptSource);
    expect(result.ok).toBe(false);
    if (result.ok) {
      throw new Error("repository with activePackageId was accepted");
    }
    expect(result.issues).toContainEqual({
      path: "$",
      code: "invalid_fields",
      message: "Repository contains missing or unknown fields.",
    });
  });

  test("rejects noncanonical and duplicate package IDs", () => {
    const uppercase = mutableCanonical();
    uppercase.packages[0]!.id = uppercase.packages[0]!.id.toUpperCase();
    expect(validateRepositoryV1(uppercase, acceptSource).ok).toBe(false);

    const duplicate = mutableCanonical();
    duplicate.packages.push(structuredClone(duplicate.packages[0]!));
    const result = validateRepositoryV1(duplicate, acceptSource);
    expect(result.ok).toBe(false);
    if (result.ok) {
      throw new Error("duplicate package ID was accepted");
    }
    expect(result.issues.some((issue) => issue.code === "duplicate_id")).toBe(
      true,
    );
  });

  test("rejects duplicate macro IDs across different packages", () => {
    const invalid = mutableCanonical();
    invalid.packages.push({
      id: "123e4567-e89b-42d3-a456-426614174000",
      name: "Second package",
      macros: [structuredClone(invalid.packages[0]!.macros[0]!)],
    });
    const result = validateRepositoryV1(invalid, acceptSource);
    expect(result.ok).toBe(false);
    if (result.ok) {
      throw new Error("globally duplicate macro ID was accepted");
    }
    expect(result.issues).toContainEqual({
      path: "$.packages[1].macros[0].id",
      code: "duplicate_id",
      message: "Macro ID must be unique across the repository.",
    });
  });

  test("enforces UTF-8 byte and timing boundaries", () => {
    const valid = mutableCanonical();
    valid.packages[0]!.name = "n".repeat(64);
    valid.packages[0]!.macros[0]!.name = "m".repeat(64);
    valid.packages[0]!.macros[0]!.source = "s".repeat(4096);
    valid.packages[0]!.macros[0]!.keyPressMs = 0;
    valid.packages[0]!.macros[0]!.interKeyMs = 10_000;
    expect(validateRepositoryV1(valid, acceptSource).ok).toBe(true);

    const invalid = mutableCanonical();
    invalid.packages[0]!.name = "é".repeat(33);
    invalid.packages[0]!.macros[0]!.source = "é".repeat(2049);
    invalid.packages[0]!.macros[0]!.keyPressMs = -1;
    invalid.packages[0]!.macros[0]!.interKeyMs = 10_001;
    expect(validateRepositoryV1(invalid, acceptSource).ok).toBe(false);
  });

  test("rejects sparse arrays, custom prototypes, and non-finite numbers", () => {
    const sparse = mutableCanonical();
    const packages: MutablePackage[] = [];
    packages.length = 1;
    sparse.packages = packages;
    expect(validateRepositoryV1(sparse, acceptSource).ok).toBe(false);

    expect(
      validateRepositoryV1(new PrototypeBearingRepository(), acceptSource).ok,
    ).toBe(false);

    const nonFinite = mutableCanonical();
    nonFinite.packages[0]!.macros[0]!.keyPressMs = Number.POSITIVE_INFINITY;
    expect(validateRepositoryV1(nonFinite, acceptSource).ok).toBe(false);
  });

  test("requires macro-language validation from the caller", () => {
    const result = validateRepositoryV1(
      canonicalRepository,
      () => "Unknown directive at byte 0.",
    );
    expect(result.ok).toBe(false);
    if (result.ok) {
      throw new Error("repository with invalid macro source was accepted");
    }
    expect(result.issues).toContainEqual({
      path: "$.packages[0].macros[0].source",
      code: "invalid_macro_source",
      message: "Unknown directive at byte 0.",
    });
  });
});
