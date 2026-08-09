import { describe, expect, test } from "vitest";
import type { Repository, RepositoryMacro } from "../src/v2/repository";
import {
  addMacro,
  addPackage,
  createMacroId,
  createPackageId,
  deleteMacro,
  deletePackage,
  duplicateMacro,
  duplicatePackage,
  findMacro,
  findPackage,
  macrosEqual,
  moveMacro,
  movePackage,
  packagesEqual,
  renamePackage,
  updateMacro,
} from "../src/v2/repositoryEditing";

const uuidPattern =
  /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/;

const packageAId = "550e8400-e29b-41d4-a716-446655440000";
const packageBId = "550e8400-e29b-41d4-a716-446655440001";
const macroAId = "6ba7b810-9dad-41d1-80b4-00c04fd430c8";
const macroBId = "6ba7b810-9dad-41d1-80b4-00c04fd430c9";

function macro(id: string, name: string): RepositoryMacro {
  return { id, name, source: "a", keyPressMs: 8, interKeyMs: 15 };
}

function repository(): Repository {
  return {
    format: "esp32-macro-keyboard-repository",
    schemaVersion: 1,
    packages: [
      {
        id: packageAId,
        name: "Package A",
        macros: [macro(macroAId, "Macro A"), macro(macroBId, "Macro B")],
      },
      { id: packageBId, name: "Package B", macros: [] },
    ],
  };
}

describe("repositoryEditing — ID generation", () => {
  test("mints canonical UUID v4 macro and package IDs", () => {
    expect(createMacroId()).toMatch(uuidPattern);
    expect(createPackageId()).toMatch(uuidPattern);
    expect(createMacroId()).not.toBe(createMacroId());
  });
});

describe("repositoryEditing — macro CRUD and ordering (V2-101)", () => {
  test("addMacro appends to the end of the named package only", () => {
    const next = addMacro(repository(), packageAId, macro("new-id", "New"));
    const pkg = findPackage(next, packageAId);
    expect(pkg?.macros.map((m) => m.id)).toEqual([
      macroAId,
      macroBId,
      "new-id",
    ]);
    expect(findPackage(next, packageBId)?.macros).toEqual([]);
  });

  test("addMacro against an unknown package is a no-op", () => {
    const original = repository();
    expect(addMacro(original, "missing", macro("x", "X"))).toEqual(original);
  });

  test("updateMacro replaces fields in place, preserving position and id", () => {
    const next = updateMacro(repository(), macroAId, {
      name: "Renamed",
      source: "b",
      keyPressMs: 1,
      interKeyMs: 2,
    });
    const located = findMacro(next, macroAId);
    expect(located?.macro).toEqual({
      id: macroAId,
      name: "Renamed",
      source: "b",
      keyPressMs: 1,
      interKeyMs: 2,
    });
    expect(located?.macroIndex).toBe(0);
  });

  test("duplicateMacro inserts a copy immediately after the original with a fresh ID", () => {
    const result = duplicateMacro(repository(), macroAId);
    expect(result).not.toBeNull();
    if (result === null) {
      return;
    }
    expect(result.newMacroId).toMatch(uuidPattern);
    expect(result.newMacroId).not.toBe(macroAId);
    const pkg = findPackage(result.repository, packageAId);
    expect(pkg?.macros.map((m) => m.name)).toEqual([
      "Macro A",
      "Macro A copy",
      "Macro B",
    ]);
    expect(pkg?.macros[1]?.id).toBe(result.newMacroId);
  });

  test("duplicateMacro against a missing macro returns null", () => {
    expect(duplicateMacro(repository(), "missing")).toBeNull();
  });

  test("deleteMacro removes exactly the targeted macro", () => {
    const next = deleteMacro(repository(), macroAId);
    const pkg = findPackage(next, packageAId);
    expect(pkg?.macros.map((m) => m.id)).toEqual([macroBId]);
  });

  test("moveMacro swaps adjacent macros and is a no-op past either boundary", () => {
    const moved = moveMacro(repository(), packageAId, 0, 1);
    expect(findPackage(moved, packageAId)?.macros.map((m) => m.id)).toEqual([
      macroBId,
      macroAId,
    ]);

    const original = repository();
    expect(moveMacro(original, packageAId, 0, -1)).toEqual(original);
    expect(moveMacro(original, packageAId, 1, 1)).toEqual(original);
  });

  test("macrosEqual distinguishes any differing field", () => {
    const a = macro(macroAId, "Macro A");
    expect(macrosEqual(a, macro(macroAId, "Macro A"))).toBe(true);
    expect(macrosEqual(a, { ...a, name: "Different" })).toBe(false);
    expect(macrosEqual(a, { ...a, source: "b" })).toBe(false);
    expect(macrosEqual(a, { ...a, keyPressMs: 1 })).toBe(false);
    expect(macrosEqual(a, { ...a, interKeyMs: 1 })).toBe(false);
  });
});

describe("repositoryEditing — package management (V2-102)", () => {
  test("addPackage appends to the end of the repository", () => {
    const next = addPackage(repository(), {
      id: "new-id",
      name: "New package",
      macros: [],
    });
    expect(next.packages.map((pkg) => pkg.id)).toEqual([
      packageAId,
      packageBId,
      "new-id",
    ]);
  });

  test("renamePackage is a no-op against an unknown package", () => {
    const original = repository();
    expect(renamePackage(original, "missing", "X")).toEqual(original);
  });

  test("duplicatePackage copies the package and reassigns every macro a fresh globally-unique ID", () => {
    const result = duplicatePackage(repository(), packageAId);
    expect(result).not.toBeNull();
    if (result === null) {
      return;
    }
    expect(result.newPackageId).toMatch(uuidPattern);
    const duplicated = findPackage(result.repository, result.newPackageId);
    expect(duplicated?.name).toBe("Package A copy");
    expect(duplicated?.macros).toHaveLength(2);
    const duplicatedIds = duplicated?.macros.map((m) => m.id) ?? [];
    expect(duplicatedIds).not.toContain(macroAId);
    expect(duplicatedIds).not.toContain(macroBId);
    expect(new Set(duplicatedIds).size).toBe(2);
    // Names carry over unchanged — only the package gets a "copy" suffix.
    expect(duplicated?.macros.map((m) => m.name)).toEqual([
      "Macro A",
      "Macro B",
    ]);
    // Inserted immediately after the original.
    expect(result.repository.packages.map((pkg) => pkg.id)).toEqual([
      packageAId,
      result.newPackageId,
      packageBId,
    ]);
  });

  test("deletePackage removes exactly the targeted package", () => {
    const next = deletePackage(repository(), packageAId);
    expect(next.packages.map((pkg) => pkg.id)).toEqual([packageBId]);
  });

  test("movePackage reorders and is a no-op past either boundary", () => {
    const moved = movePackage(repository(), 0, 1);
    expect(moved.packages.map((pkg) => pkg.id)).toEqual([
      packageBId,
      packageAId,
    ]);
    const original = repository();
    expect(movePackage(original, 0, -1)).toEqual(original);
    expect(movePackage(original, 1, 1)).toEqual(original);
  });

  test("packagesEqual distinguishes name, id, and macro content/order differences", () => {
    const pkg = repository().packages[0];
    if (pkg === undefined) {
      throw new Error("fixture missing package");
    }
    expect(packagesEqual(pkg, { ...pkg })).toBe(true);
    expect(packagesEqual(pkg, { ...pkg, name: "Different" })).toBe(false);
    expect(
      packagesEqual(pkg, { ...pkg, macros: [...pkg.macros].reverse() }),
    ).toBe(false);
    expect(packagesEqual(pkg, { ...pkg, macros: pkg.macros.slice(0, 1) })).toBe(
      false,
    );
  });
});

describe("repositoryEditing — name truncation on duplicate", () => {
  test("a 64-byte package name still fits after ' copy' by truncating, never splitting a UTF-8 code point", () => {
    const longName = "x".repeat(64);
    const source: Repository = {
      format: "esp32-macro-keyboard-repository",
      schemaVersion: 1,
      packages: [{ id: packageAId, name: longName, macros: [] }],
    };
    const result = duplicatePackage(source, packageAId);
    expect(result).not.toBeNull();
    if (result === null) {
      return;
    }
    const duplicated = findPackage(result.repository, result.newPackageId);
    expect(duplicated).toBeDefined();
    const encoder = new TextEncoder();
    expect(
      encoder.encode(duplicated?.name ?? "").byteLength,
    ).toBeLessThanOrEqual(64);
    expect(duplicated?.name.endsWith(" copy")).toBe(false);
  });
});
