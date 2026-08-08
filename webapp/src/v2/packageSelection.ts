import { v2PutJson } from "./apiClient";
import { isSettingsUpdatedResponse } from "./apiContracts";
import type { SettingsUpdatedResponse } from "./apiTypes";
import type { Repository } from "./repository";

/**
 * Package selection preference resolution and persistence, per SPEC_V2 §8.6,
 * UI_UX_SPEC_V2 §3.6, and TODO_V2 V2-074.
 *
 * `lastSelectedPackageId` is device-wide UI state stored by firmware as an
 * opaque UUID (or `null`) in settings — it is never part of repository JSON
 * (`Repository`/`RepositoryPackage` have no such field, so this cannot
 * happen by construction) and resolving or changing it never dirties the
 * repository working copy.
 */
export type PackageSelectionResolution =
  | { kind: "resolved"; packageId: string; shouldPersist: boolean }
  | { kind: "chooser" };

/**
 * Implements the exact 3-step algorithm from UI_UX_SPEC_V2 §3.6:
 *  1. When `lastSelectedPackageId` identifies a package in the loaded
 *     repository, open that package.
 *  2. Otherwise, when the repository contains exactly one package, open it
 *     (and the caller must persist that selection).
 *  3. Otherwise, show the Package Chooser.
 */
export function resolveSelectedPackage(
  repository: Repository,
  lastSelectedPackageId: string | null,
): PackageSelectionResolution {
  if (lastSelectedPackageId !== null) {
    const matched = repository.packages.some(
      (pkg) => pkg.id === lastSelectedPackageId,
    );
    if (matched) {
      return {
        kind: "resolved",
        packageId: lastSelectedPackageId,
        shouldPersist: false,
      };
    }
  }

  if (repository.packages.length === 1) {
    const solePackage = repository.packages[0];
    if (solePackage !== undefined) {
      return {
        kind: "resolved",
        packageId: solePackage.id,
        shouldPersist: true,
      };
    }
  }

  return { kind: "chooser" };
}

/**
 * Persists a package selection change via `PUT /api/v1/settings`, updating
 * NVS only when the selected ID actually changes (TODO_V2 V2-074). Returns
 * `null` without making a request when `packageId` already equals
 * `currentLastSelectedPackageId`.
 */
export async function persistSelectedPackageId(
  packageId: string | null,
  currentLastSelectedPackageId: string | null,
): Promise<SettingsUpdatedResponse | null> {
  if (packageId === currentLastSelectedPackageId) {
    return null;
  }
  return v2PutJson(
    "/api/v1/settings",
    { lastSelectedPackageId: packageId },
    isSettingsUpdatedResponse,
  );
}
