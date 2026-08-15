import { vi } from "vitest";
import { SnapshotsPage } from "../src/features/snapshots/v2/SnapshotsPage";
import type { SnapshotsPageDependencies } from "../src/features/snapshots/v2/SnapshotsPage";
import type { Repository } from "../src/v2/repository";
import { createRepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import type { RepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import type { SnapshotListResult } from "../src/v2/snapshotClient";
import { flushReact, render } from "./render";

export const packageId = "550e8400-e29b-41d4-a716-446655440000";
export const otherPackageId = "550e8400-e29b-41d4-a716-446655440001";

export function repository(): Repository {
  return {
    format: "esp32-macro-keyboard-repository",
    schemaVersion: 1,
    packages: [{ id: packageId, name: "Lab bench", macros: [] }],
  };
}

export function renamedRepository(): Repository {
  return {
    format: "esp32-macro-keyboard-repository",
    schemaVersion: 1,
    packages: [{ id: packageId, name: "Renamed", macros: [] }],
  };
}

export function twoPackageRepository(): Repository {
  return {
    format: "esp32-macro-keyboard-repository",
    schemaVersion: 1,
    packages: [
      { id: packageId, name: "Lab bench", macros: [] },
      { id: otherPackageId, name: "Second package", macros: [] },
    ],
  };
}

export function defaultList(): SnapshotListResult {
  return {
    blobs: [{ id: "1", sizeBytes: 500 }],
    usedBytes: 500,
    remainingBytes: 130_572,
  };
}

export interface Overrides {
  store?: RepositoryWorkingCopyStore;
  persistedPackageId?: string | null;
  retentionTarget?: number;
  loadedBlobId?: string | null;
  deps?: Partial<SnapshotsPageDependencies>;
  saving?: boolean;
  saveError?: string | null;
}

export function makeDependencies(
  overrides: Partial<SnapshotsPageDependencies> = {},
): SnapshotsPageDependencies {
  return {
    listSnapshots: vi.fn().mockResolvedValue(defaultList()),
    loadSnapshotIntoWorkingCopy: vi.fn(),
    downloadSnapshotBytes: vi.fn(),
    deleteSnapshot: vi.fn().mockResolvedValue(undefined),
    exportRepository: vi.fn(),
    importRepository: vi.fn(),
    replaceSnapshotWithWorkingCopy: vi.fn(),
    persistSelectedPackageId: vi.fn().mockResolvedValue(null),
    saveAsFile: vi.fn(),
    readFileBytes: vi.fn(),
    ...overrides,
  };
}

export async function renderPage(overrides: Overrides = {}) {
  const store =
    overrides.store ?? createRepositoryWorkingCopyStore(repository());
  const deps = makeDependencies(overrides.deps);
  const onSelectionChange = vi.fn();
  const onSelectionPersistenceFailure = vi.fn();
  const onSelectionPersistenceSuccess = vi.fn();
  const onOpenMacros = vi.fn();
  const onOpenPackages = vi.fn();
  const onWorkingCopyOriginChanged = vi.fn();
  const onSaveSnapshot = vi.fn().mockResolvedValue(undefined);
  const persistedPackageId =
    overrides.persistedPackageId === undefined
      ? packageId
      : overrides.persistedPackageId;

  const view = await render(
    <SnapshotsPage
      dependencies={deps}
      loadedBlobId={overrides.loadedBlobId ?? "1"}
      onOpenMacros={onOpenMacros}
      onOpenPackages={onOpenPackages}
      onSaveSnapshot={onSaveSnapshot}
      onSelectionChange={onSelectionChange}
      onSelectionPersistenceFailure={onSelectionPersistenceFailure}
      onSelectionPersistenceSuccess={onSelectionPersistenceSuccess}
      onWorkingCopyOriginChanged={onWorkingCopyOriginChanged}
      persistedPackageId={persistedPackageId}
      retentionTarget={overrides.retentionTarget ?? 5}
      saveError={overrides.saveError ?? null}
      saving={overrides.saving ?? false}
      store={store}
    />,
  );
  await flushReact();
  return {
    ...view,
    store,
    deps,
    onSelectionChange,
    onSelectionPersistenceFailure,
    onSelectionPersistenceSuccess,
    onOpenMacros,
    onOpenPackages,
    onWorkingCopyOriginChanged,
    onSaveSnapshot,
  };
}

export async function waitUntil(
  predicate: () => boolean,
  description: string,
): Promise<void> {
  for (let attempt = 0; attempt < 50; attempt += 1) {
    if (predicate()) {
      return;
    }
    await flushReact();
  }
  throw new Error(`Timed out waiting for: ${description}`);
}
