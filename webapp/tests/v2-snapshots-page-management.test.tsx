import { describe, expect, test, vi } from "vitest";
import type { Repository } from "../src/v2/repository";
import type { RepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import type { LoadSnapshotResult } from "../src/v2/snapshotClient";
import { buttonWithText, click, requiredElement } from "./render";
import {
  otherPackageId,
  packageId,
  renderPage,
  repository,
  twoPackageRepository,
  waitUntil,
} from "./snapshotsPageHarness";

describe("SnapshotsPage — V2-111 Snapshot management", () => {
  test("shows blob ID, stored size, loaded indicator, storage usage, and retention target", async () => {
    const { container, unmount } = await renderPage({
      deps: {
        listSnapshots: vi.fn().mockResolvedValue({
          blobs: [
            { id: "3", sizeBytes: 200 },
            { id: "1", sizeBytes: 500 },
          ],
          usedBytes: 700,
          remainingBytes: 130_372,
        }),
      },
      loadedBlobId: "3",
    });
    await waitUntil(
      () => container.textContent?.includes("200 bytes") === true,
      "blob list to render",
    );
    expect(container.textContent).toContain("Snapshot 3 (loaded)");
    expect(container.textContent).toContain("Snapshot 1");
    expect(container.textContent).not.toContain("Snapshot 1 (loaded)");
    expect(container.textContent).toContain("500 bytes");
    expect(container.textContent).toContain("used 700 bytes");
    expect(container.textContent).toContain("remaining 130372 bytes");
    expect(container.textContent).toContain("retention target 5");
    await unmount();
  });

  test("shows no device-generated dates anywhere", async () => {
    const { container, unmount } = await renderPage();
    await waitUntil(
      () => container.textContent?.includes("500 bytes") === true,
      "blob list to render",
    );
    // No blob date field exists in the wire contract (SPEC_V2 §10.2); this
    // guards against ever inventing one client-side.
    expect(container.querySelector("time")).toBeNull();
    await unmount();
  });

  test("provides Load, Download, Delete, and Save current snapshot", async () => {
    const { unmount } = await renderPage();
    await waitUntil(
      () => document.querySelector('[aria-label="Load snapshot 1"]') !== null,
      "row actions to render",
    );
    expect(
      document.querySelector('[aria-label="Load snapshot 1"]'),
    ).not.toBeNull();
    expect(
      document.querySelector('[aria-label="Download snapshot 1"]'),
    ).not.toBeNull();
    expect(
      document.querySelector('[aria-label="Delete snapshot 1"]'),
    ).not.toBeNull();
    buttonWithText("Save current snapshot");
    await unmount();
  });

  test("Save current snapshot calls the single shared save handler", async () => {
    const { onSaveSnapshot, unmount } = await renderPage();
    await click(buttonWithText("Save current snapshot"));
    expect(onSaveSnapshot).toHaveBeenCalledTimes(1);
    await unmount();
  });
});

describe("SnapshotsPage — V2-112 advisory retention target", () => {
  test("shows a non-blocking cleanup indicator once count exceeds target", async () => {
    const { container, unmount } = await renderPage({
      deps: {
        listSnapshots: vi.fn().mockResolvedValue({
          blobs: Array.from({ length: 6 }, (_, index) => ({
            id: String(index + 1),
            sizeBytes: 10,
          })),
          usedBytes: 60,
          remainingBytes: 100,
        }),
      },
      retentionTarget: 5,
    });
    await waitUntil(
      () =>
        container.textContent?.includes("above the retention target") === true,
      "cleanup indicator to render",
    );
    await unmount();
  });

  test("shows no cleanup indicator when count is at or below target", async () => {
    const { container, unmount } = await renderPage({ retentionTarget: 5 });
    await waitUntil(
      () => container.textContent?.includes("500 bytes") === true,
      "blob list to render",
    );
    expect(container.textContent).not.toContain("above the retention target");
    await unmount();
  });

  test("a target of 0 disables the cleanup indicator even with many snapshots", async () => {
    const { container, unmount } = await renderPage({
      deps: {
        listSnapshots: vi.fn().mockResolvedValue({
          blobs: Array.from({ length: 20 }, (_, index) => ({
            id: String(index + 1),
            sizeBytes: 10,
          })),
          usedBytes: 200,
          remainingBytes: 100,
        }),
      },
      retentionTarget: 0,
    });
    await waitUntil(
      () => container.textContent?.includes("retention target 0") === true,
      "list to render",
    );
    expect(container.textContent).not.toContain("above the retention target");
    await unmount();
  });

  test("saving a sixth snapshot never triggers a delete call, at any count above target", async () => {
    const deleteSnapshot = vi.fn().mockResolvedValue(undefined);
    const { onSaveSnapshot, unmount } = await renderPage({
      deps: {
        listSnapshots: vi.fn().mockResolvedValue({
          blobs: Array.from({ length: 6 }, (_, index) => ({
            id: String(index + 1),
            sizeBytes: 10,
          })),
          usedBytes: 60,
          remainingBytes: 100,
        }),
        deleteSnapshot,
      },
      retentionTarget: 5,
    });
    await click(buttonWithText("Save current snapshot"));
    expect(onSaveSnapshot).toHaveBeenCalledTimes(1);
    expect(deleteSnapshot).not.toHaveBeenCalled();
    await unmount();
  });
});

describe("SnapshotsPage — V2-110/V2-111 manual load", () => {
  test("loading with a clean working copy loads immediately, validates first, and resolves the package", async () => {
    const loaded = repository();
    const loadSnapshotIntoWorkingCopy = vi
      .fn<
        (
          id: string,
          store: RepositoryWorkingCopyStore,
        ) => Promise<LoadSnapshotResult>
      >()
      .mockImplementation((_id, store) => {
        store.replaceWorkingCopy(loaded);
        return Promise.resolve({
          ok: true,
          repository: loaded,
          created: false,
        });
      });
    const {
      onSelectionChange,
      onOpenMacros,
      onWorkingCopyOriginChanged,
      store,
      unmount,
    } = await renderPage({ deps: { loadSnapshotIntoWorkingCopy } });
    await waitUntil(
      () => document.querySelector('[aria-label="Load snapshot 1"]') !== null,
      "row to render",
    );
    await click(
      requiredElement('[aria-label="Load snapshot 1"]', HTMLButtonElement),
    );
    await waitUntil(
      () => onOpenMacros.mock.calls.length > 0,
      "onOpenMacros after load",
    );
    expect(loadSnapshotIntoWorkingCopy).toHaveBeenCalledWith("1", store);
    expect(onWorkingCopyOriginChanged).toHaveBeenCalledWith("1");
    expect(onSelectionChange).toHaveBeenCalledWith(packageId);
    await unmount();
  });

  test("selection persistence failure after load still opens the resolved package and reports the failure", async () => {
    const loaded: Repository = {
      format: "esp32-macro-keyboard-repository",
      schemaVersion: 1,
      packages: [{ id: otherPackageId, name: "Loaded package", macros: [] }],
    };
    const loadSnapshotIntoWorkingCopy = vi
      .fn<
        (
          id: string,
          store: RepositoryWorkingCopyStore,
        ) => Promise<LoadSnapshotResult>
      >()
      .mockImplementation((_id, store) => {
        store.replaceWorkingCopy(loaded);
        return Promise.resolve({
          ok: true,
          repository: loaded,
          created: false,
        });
      });
    const failure = new TypeError("settings write failed");
    const persistSelectedPackageId = vi.fn().mockRejectedValue(failure);
    const {
      onOpenMacros,
      onSelectionChange,
      onSelectionPersistenceFailure,
      onSelectionPersistenceSuccess,
      store,
      unmount,
    } = await renderPage({
      deps: { loadSnapshotIntoWorkingCopy, persistSelectedPackageId },
      persistedPackageId: packageId,
    });
    await waitUntil(
      () => document.querySelector('[aria-label="Load snapshot 1"]') !== null,
      "row to render",
    );
    await click(
      requiredElement('[aria-label="Load snapshot 1"]', HTMLButtonElement),
    );
    await waitUntil(
      () => onOpenMacros.mock.calls.length > 0,
      "onOpenMacros after failed preference write",
    );

    expect(onSelectionChange).toHaveBeenCalledWith(otherPackageId);
    expect(onSelectionPersistenceSuccess).not.toHaveBeenCalled();
    expect(onSelectionPersistenceFailure).toHaveBeenCalledWith({
      packageId: otherPackageId,
      previousPackageId: packageId,
      error: failure,
    });
    expect(store.getIsDirty()).toBe(false);
    await unmount();
  });

  test("loading a repository with several packages and no resolvable selection opens the package chooser", async () => {
    const loaded = twoPackageRepository();
    const loadSnapshotIntoWorkingCopy = vi
      .fn<
        (
          id: string,
          store: RepositoryWorkingCopyStore,
        ) => Promise<LoadSnapshotResult>
      >()
      .mockResolvedValue({ ok: true, repository: loaded, created: false });
    const { onOpenPackages, onOpenMacros, unmount } = await renderPage({
      deps: { loadSnapshotIntoWorkingCopy },
      persistedPackageId: "does-not-exist",
    });
    await waitUntil(
      () => document.querySelector('[aria-label="Load snapshot 1"]') !== null,
      "row to render",
    );
    await click(
      requiredElement('[aria-label="Load snapshot 1"]', HTMLButtonElement),
    );
    await waitUntil(
      () => onOpenPackages.mock.calls.length > 0,
      "onOpenPackages after load",
    );
    expect(onOpenMacros).not.toHaveBeenCalled();
    await unmount();
  });
});
