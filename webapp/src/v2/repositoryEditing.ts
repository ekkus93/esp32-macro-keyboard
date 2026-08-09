import { utf8ByteLength, type Repository, type RepositoryMacro, type RepositoryPackage } from "./repository";
import { v2Limits } from "./limits";

/**
 * Pure, network-free repository-editing helpers, per SPEC_V2 §8.5/§8.6 and
 * TODO_V2 V2-101 (macro CRUD/ordering) and V2-102 (package management).
 *
 * Every function here takes a {@link Repository} and returns a brand-new one
 * (or a `{ repository, ... }` tuple for operations that mint a new ID the
 * caller needs) — none of them touch a
 * {@link import("./repositoryWorkingCopy").RepositoryWorkingCopyStore}
 * directly. That keeps "does this change actually dirty the repository"
 * entirely the caller's decision (UI_UX_SPEC_V2 §7.2): a caller compares the
 * result against the current repository (see {@link macrosEqual} /
 * {@link packagesShallowEqual}) before deciding whether to call
 * `store.applyContentChange`, so a no-op edit — Save with nothing actually
 * changed, Rename to the same name — never produces a spurious dirty
 * transition.
 *
 * IDs are minted with `crypto.randomUUID()` (SPEC_V2 §8.5 "IDs are generated
 * in the browser with crypto.randomUUID()"), which is itself a canonical
 * lowercase UUID v4, so every ID this module mints is valid by construction
 * and — because `crypto.randomUUID()` never repeats in practice — preserves
 * the global macro-ID/package-ID uniqueness SPEC_V2 §8.5 requires without
 * this module needing its own collision-detection loop.
 */

export function createMacroId(): string {
  return crypto.randomUUID();
}

export function createPackageId(): string {
  return crypto.randomUUID();
}

/** True when two macros have identical field values (id included). */
export function macrosEqual(a: RepositoryMacro, b: RepositoryMacro): boolean {
  return (
    a.id === b.id &&
    a.name === b.name &&
    a.source === b.source &&
    a.keyPressMs === b.keyPressMs &&
    a.interKeyMs === b.interKeyMs
  );
}

/** True when two packages have the same id, name, and macro list (by id and content, in order). */
export function packagesEqual(
  a: RepositoryPackage,
  b: RepositoryPackage,
): boolean {
  return (
    a.id === b.id &&
    a.name === b.name &&
    a.macros.length === b.macros.length &&
    a.macros.every((macro, index) => {
      const other = b.macros[index];
      return other !== undefined && macrosEqual(macro, other);
    })
  );
}

export function findPackage(
  repository: Repository,
  packageId: string,
): RepositoryPackage | undefined {
  return repository.packages.find((pkg) => pkg.id === packageId);
}

export interface MacroLocation {
  pkg: RepositoryPackage;
  packageIndex: number;
  macro: RepositoryMacro;
  macroIndex: number;
}

export function findMacro(
  repository: Repository,
  macroId: string,
): MacroLocation | undefined {
  for (const [packageIndex, pkg] of repository.packages.entries()) {
    const macroIndex = pkg.macros.findIndex((macro) => macro.id === macroId);
    if (macroIndex >= 0) {
      const macro = pkg.macros[macroIndex];
      if (macro !== undefined) {
        return { pkg, packageIndex, macro, macroIndex };
      }
    }
  }
  return undefined;
}

function replacePackage(
  repository: Repository,
  packageId: string,
  next: RepositoryPackage,
): Repository {
  return {
    ...repository,
    packages: repository.packages.map((pkg) =>
      pkg.id === packageId ? next : pkg,
    ),
  };
}

/** Truncates to at most `maxBytes` UTF-8 bytes without splitting a multi-byte code point. */
function truncateToUtf8Bytes(value: string, maxBytes: number): string {
  if (utf8ByteLength(value) <= maxBytes) {
    return value;
  }
  let result = "";
  for (const character of value) {
    const candidate = result + character;
    if (utf8ByteLength(candidate) > maxBytes) {
      break;
    }
    result = candidate;
  }
  return result;
}

function copyName(originalName: string, maxBytes: number): string {
  return truncateToUtf8Bytes(`${originalName} copy`, maxBytes);
}

// --- Macro operations (TODO_V2 V2-101) ---------------------------------

/** Appends a new macro (freshly minted `id`) to the end of the named package's macro list. */
export function addMacro(
  repository: Repository,
  packageId: string,
  macro: RepositoryMacro,
): Repository {
  const pkg = findPackage(repository, packageId);
  if (pkg === undefined) {
    return repository;
  }
  return replacePackage(repository, packageId, {
    ...pkg,
    macros: [...pkg.macros, macro],
  });
}

/** Replaces an existing macro's editable fields (id and package membership are unchanged). */
export function updateMacro(
  repository: Repository,
  macroId: string,
  changes: Omit<RepositoryMacro, "id">,
): Repository {
  const located = findMacro(repository, macroId);
  if (located === undefined) {
    return repository;
  }
  const macros = [...located.pkg.macros];
  macros[located.macroIndex] = { id: macroId, ...changes };
  return replacePackage(repository, located.pkg.id, {
    ...located.pkg,
    macros,
  });
}

/**
 * Duplicates a macro within its own package, immediately after the
 * original, with a freshly minted globally-unique ID (SPEC_V2 §8.5) and a
 * "<name> copy" name bounded to the same 64-byte limit as any other name.
 */
export function duplicateMacro(
  repository: Repository,
  macroId: string,
): { repository: Repository; newMacroId: string } | null {
  const located = findMacro(repository, macroId);
  if (located === undefined) {
    return null;
  }
  const newMacroId = createMacroId();
  const duplicate: RepositoryMacro = {
    ...located.macro,
    id: newMacroId,
    name: copyName(located.macro.name, v2Limits.macroNameMaxBytes),
  };
  const macros = [...located.pkg.macros];
  macros.splice(located.macroIndex + 1, 0, duplicate);
  return {
    repository: replacePackage(repository, located.pkg.id, {
      ...located.pkg,
      macros,
    }),
    newMacroId,
  };
}

export function deleteMacro(repository: Repository, macroId: string): Repository {
  const located = findMacro(repository, macroId);
  if (located === undefined) {
    return repository;
  }
  return replacePackage(repository, located.pkg.id, {
    ...located.pkg,
    macros: located.pkg.macros.filter((macro) => macro.id !== macroId),
  });
}

/** Moves the macro at `index` within `packageId` by `direction` (-1 up, +1 down). A no-op past either end. */
export function moveMacro(
  repository: Repository,
  packageId: string,
  index: number,
  direction: -1 | 1,
): Repository {
  const pkg = findPackage(repository, packageId);
  if (pkg === undefined) {
    return repository;
  }
  const target = index + direction;
  if (index < 0 || index >= pkg.macros.length || target < 0 || target >= pkg.macros.length) {
    return repository;
  }
  const macros = [...pkg.macros];
  const moved = macros[index];
  if (moved === undefined) {
    return repository;
  }
  macros.splice(index, 1);
  macros.splice(target, 0, moved);
  return replacePackage(repository, packageId, { ...pkg, macros });
}

// --- Package operations (TODO_V2 V2-102) --------------------------------

export function addPackage(
  repository: Repository,
  pkg: RepositoryPackage,
): Repository {
  return { ...repository, packages: [...repository.packages, pkg] };
}

export function renamePackage(
  repository: Repository,
  packageId: string,
  name: string,
): Repository {
  const pkg = findPackage(repository, packageId);
  if (pkg === undefined) {
    return repository;
  }
  return replacePackage(repository, packageId, { ...pkg, name });
}

/**
 * Duplicates a package (and every one of its macros, each reassigned a
 * fresh globally-unique ID so package-scoped and global macro-ID uniqueness
 * both hold) immediately after the original, with a fresh package ID and a
 * "<name> copy" name.
 */
export function duplicatePackage(
  repository: Repository,
  packageId: string,
): { repository: Repository; newPackageId: string } | null {
  const index = repository.packages.findIndex((pkg) => pkg.id === packageId);
  const pkg = repository.packages[index];
  if (index < 0 || pkg === undefined) {
    return null;
  }
  const newPackageId = createPackageId();
  const duplicate: RepositoryPackage = {
    id: newPackageId,
    name: copyName(pkg.name, v2Limits.packageNameMaxBytes),
    macros: pkg.macros.map((macro) => ({ ...macro, id: createMacroId() })),
  };
  const packages = [...repository.packages];
  packages.splice(index + 1, 0, duplicate);
  return { repository: { ...repository, packages }, newPackageId };
}

export function deletePackage(
  repository: Repository,
  packageId: string,
): Repository {
  return {
    ...repository,
    packages: repository.packages.filter((pkg) => pkg.id !== packageId),
  };
}

/** Moves the package at `index` by `direction` (-1 up, +1 down). A no-op past either end. */
export function movePackage(
  repository: Repository,
  index: number,
  direction: -1 | 1,
): Repository {
  const target = index + direction;
  if (
    index < 0 ||
    index >= repository.packages.length ||
    target < 0 ||
    target >= repository.packages.length
  ) {
    return repository;
  }
  const packages = [...repository.packages];
  const moved = packages[index];
  if (moved === undefined) {
    return repository;
  }
  packages.splice(index, 1);
  packages.splice(target, 0, moved);
  return { ...repository, packages };
}
