import { describe, expect, test } from "vitest";
import canonicalRepository from "../../contracts/v2/repository/canonical.json";
import rawFixtures from "../../contracts/v2/repository/fixtures.json";
import { serializeRepository, validateRepository } from "../src/v2/repository";

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
  unexpected?: boolean;
}

interface FixtureExpectation {
  ok: boolean;
  code?: string;
  path?: string;
}

interface FixtureCase {
  name: string;
  kind: string;
  count?: number;
  value?: number;
  expected: FixtureExpectation;
}

interface FixtureCorpus {
  format: string;
  version: number;
  cases: FixtureCase[];
}

class PrototypeBearingRepository {
  readonly format = "esp32-macro-keyboard-repository";
  readonly schemaVersion = 1;
  readonly packages: MutablePackage[] = [];
}

const fixtures = rawFixtures as FixtureCorpus;

function mutableCanonical(): MutableRepository {
  return structuredClone(canonicalRepository);
}

function packageAt(repository: MutableRepository, index = 0): MutablePackage {
  const pkg = repository.packages[index];
  if (pkg === undefined) {
    throw new Error(`missing package ${String(index)}`);
  }
  return pkg;
}

function macroAt(pkg: MutablePackage, index = 0): MutableMacro {
  const macro = pkg.macros[index];
  if (macro === undefined) {
    throw new Error(`missing macro ${String(index)}`);
  }
  return macro;
}

function requiredCount(fixture: FixtureCase): number {
  if (fixture.count === undefined) {
    throw new Error(`${fixture.name} is missing count`);
  }
  return fixture.count;
}

function requiredValue(fixture: FixtureCase): number {
  if (fixture.value === undefined) {
    throw new Error(`${fixture.name} is missing value`);
  }
  return fixture.value;
}

function buildFixture(fixture: FixtureCase): unknown {
  if (fixture.kind === "canonical") {
    return mutableCanonical();
  }
  if (fixture.kind === "empty") {
    return {
      format: "esp32-macro-keyboard-repository",
      schemaVersion: 1,
      packages: [],
    };
  }
  if (fixture.kind === "malformedRoot") {
    return "not-an-object";
  }
  if (fixture.kind === "prototypeRoot") {
    return new PrototypeBearingRepository();
  }

  const repository = mutableCanonical();
  const pkg = packageAt(repository);
  const macro = macroAt(pkg);

  switch (fixture.kind) {
    case "packageName":
      pkg.name = "n".repeat(requiredCount(fixture));
      break;
    case "macroName":
      macro.name = "m".repeat(requiredCount(fixture));
      break;
    case "macroSource":
      macro.source = "s".repeat(requiredCount(fixture));
      break;
    case "keyPressMs":
      macro.keyPressMs = requiredValue(fixture);
      break;
    case "interKeyMs":
      macro.interKeyMs = requiredValue(fixture);
      break;
    case "malformedPackage":
      repository.packages = [null as unknown as MutablePackage];
      break;
    case "malformedMacro":
      pkg.macros = [null as unknown as MutableMacro];
      break;
    case "duplicatePackage":
      repository.packages.push(structuredClone(pkg));
      break;
    case "duplicateMacro":
      repository.packages.push({
        id: "123e4567-e89b-42d3-a456-426614174000",
        name: "Second package",
        macros: [structuredClone(macro)],
      });
      break;
    case "activePackageId":
      repository.activePackageId = pkg.id;
      break;
    case "unknownRoot":
      repository.unexpected = true;
      break;
    case "unknownPackage":
      (pkg as unknown as Record<string, unknown>).unexpected = true;
      break;
    case "unknownMacro":
      (macro as unknown as Record<string, unknown>).unexpected = true;
      break;
    case "nonFinite":
      macro.keyPressMs = Number.POSITIVE_INFINITY;
      break;
    case "sparsePackages": {
      const sparse: MutablePackage[] = [];
      sparse.length = 1;
      repository.packages = sparse;
      break;
    }
    case "sparseMacros": {
      const sparse: MutableMacro[] = [];
      sparse.length = 1;
      pkg.macros = sparse;
      break;
    }
    default:
      throw new Error(`unknown repository fixture kind: ${fixture.kind}`);
  }
  return repository;
}

describe("v2 repository fixture corpus", () => {
  test("has the reviewed identity and nonempty case list", () => {
    expect(fixtures.format).toBe("esp32-macro-keyboard-repository-fixtures");
    expect(fixtures.version).toBe(1);
    expect(fixtures.cases.length).toBeGreaterThan(0);
  });

  for (const fixture of fixtures.cases) {
    test(fixture.name, () => {
      const result = validateRepository(buildFixture(fixture), acceptSource);
      expect(result.ok).toBe(fixture.expected.ok);
      if (fixture.expected.ok) {
        return;
      }
      if (result.ok) {
        throw new Error(`${fixture.name} unexpectedly passed`);
      }
      expect(result.issues).toContainEqual(
        expect.objectContaining({
          code: fixture.expected.code,
          path: fixture.expected.path,
        }),
      );
    });
  }
});

describe("v2 repository canonical serialization", () => {
  test("accepts and canonically serializes the checked-in repository", () => {
    const result = validateRepository(canonicalRepository, acceptSource);
    expect(result.ok).toBe(true);
    if (!result.ok) {
      throw new Error("canonical repository was rejected");
    }
    expect(serializeRepository(result.value)).toBe(
      JSON.stringify(canonicalRepository),
    );
  });

  test("requires macro-language validation from the caller", () => {
    const result = validateRepository(
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
