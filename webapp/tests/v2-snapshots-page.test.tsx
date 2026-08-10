import { act } from "react";
import { describe, expect, test, vi } from "vitest";
import { SnapshotsPage } from "../src/features/snapshots/v2/SnapshotsPage";
import type { SnapshotsPageDependencies } from "../src/features/snapshots/v2/SnapshotsPage";
import type { Repository } from "../src/v2/repository";
import { createRepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import type { RepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import type {
  ImportResult,
  LoadSnapshotResult,
  ReplaceSnapshotResult,
  SnapshotListResult,
} from "../src/v2/snapshotClient";
import type { BlobCreatedResponse } from "../src/v2/apiTypes";
import {
  buttonWithText,
  click,
  flushReact,
  render,
  requiredElement,
  setInputValue,
} from "./render";

const packageId = "550e8400-e29b-41d4-a716-446655440000";
const otherPackageId = "550e8400-e29b-41d4-a716-446655440001";

function repository(): Repository {
  return {
    format: "esp32-macro-keyboard-repository",
    schemaVersion: 1,
    packages: [{ id: packageId, name: "Lab bench", macros: [] }],
  };
}

function renamedRepository(): Repository {
  return {
    format: "esp32-macro-keyboard-repository",
    schemaVersion: 1,
    packages: [{ id: packageId, name: "Renamed", macros: [] }],
  };
}

function twoPackageRepository(): Repository {
  return {
    format: "esp32-macro-keyboard-repository",
    schemaVersion: 1,
    packages: [
      { id: packageId, name: "Lab bench", macros: [] },
      { id: otherPackageId, name: "Second package", macros: [] },
    ],
  };
}

function defaultList(): SnapshotListResult {
  return {
    blobs: [{ id: "1", sizeBytes: 500 }],
    usedBytes: 500,
    remainingBytes: 130_572,
  };
}

interface Overrides {
  store?: RepositoryWorkingCopyStore;
  selectedPackageId?: string;
  retentionTarget?: number;
  loadedBlobId?: string | null;
  deps?: Partial<SnapshotsPageDependencies>;
  saving?: boolean;
  saveError?: string | null;
}

function makeDependencies(
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

async function renderPage(overrides: Overrides = {}) {
  const store =
    overrides.store ?? createRepositoryWorkingCopyStore(repository());
  const deps = makeDependencies(overrides.deps);
  const onSelectionChange = vi.fn();
  const onOpenMacros = vi.fn();
  const onOpenPackages = vi.fn();
  const onWorkingCopyOriginChanged = vi.fn();
  const onSaveSnapshot = vi.fn().mockResolvedValue(undefined);

  const view = await render(
    <SnapshotsPage
      dependencies={deps}
      loadedBlobId={overrides.loadedBlobId ?? "1"}
      onOpenMacros={onOpenMacros}
      onOpenPackages={onOpenPackages}
      onSaveSnapshot={onSaveSnapshot}
      onSelectionChange={onSelectionChange}
      onWorkingCopyOriginChanged={onWorkingCopyOriginChanged}
      retentionTarget={overrides.retentionTarget ?? 5}
      saveError={overrides.saveError ?? null}
      saving={overrides.saving ?? false}
      selectedPackageId={overrides.selectedPackageId ?? packageId}
      store={store}
    />,
  );
  await flushReact();
  return {
    ...view,
    store,
    deps,
    onSelectionChange,
    onOpenMacros,
    onOpenPackages,
    onWorkingCopyOriginChanged,
    onSaveSnapshot,
  };
}

async function waitUntil(
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
      selectedPackageId: "does-not-exist",
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

describe("SnapshotsPage — V2-113 dirty-work protection during load", () => {
  function dirtyStore(): RepositoryWorkingCopyStore {
    const store = createRepositoryWorkingCopyStore(repository());
    store.applyContentChange(twoPackageRepository());
    return store;
  }

  test("loading while dirty warns with Cancel, Export working copy, Save snapshot, and Discard changes and load", async () => {
    const { container, store, unmount } = await renderPage({
      store: dirtyStore(),
    });
    await waitUntil(
      () => document.querySelector('[aria-label="Load snapshot 1"]') !== null,
      "row to render",
    );
    await click(
      requiredElement('[aria-label="Load snapshot 1"]', HTMLButtonElement),
    );
    await flushReact();
    expect(container.textContent).toContain("Unsaved changes");
    buttonWithText("Cancel");
    buttonWithText("Export working copy");
    buttonWithText("Save snapshot");
    buttonWithText("Discard changes and load");
    expect(store.getIsDirty()).toBe(true);
    await unmount();
  });

  test("Cancel dismisses the warning without loading anything", async () => {
    const loadSnapshotIntoWorkingCopy = vi.fn();
    const { unmount } = await renderPage({
      store: dirtyStore(),
      deps: { loadSnapshotIntoWorkingCopy },
    });
    await waitUntil(
      () => document.querySelector('[aria-label="Load snapshot 1"]') !== null,
      "row to render",
    );
    await click(
      requiredElement('[aria-label="Load snapshot 1"]', HTMLButtonElement),
    );
    await flushReact();
    await click(buttonWithText("Cancel"));
    await flushReact();
    expect(document.body.textContent).not.toContain("Discard changes and load");
    expect(loadSnapshotIntoWorkingCopy).not.toHaveBeenCalled();
    await unmount();
  });

  test("Export working copy exports the CURRENT dirty working copy, not the target snapshot, and keeps the warning open", async () => {
    const exportRepository = vi.fn().mockResolvedValue({
      bytes: new Uint8Array([1, 2, 3]),
      filename: "repository.emk-repository.json.gz",
      mimeType: "application/gzip",
    });
    const saveAsFile = vi.fn();
    const store = dirtyStore();
    const { unmount } = await renderPage({
      store,
      deps: { exportRepository, saveAsFile },
    });
    await waitUntil(
      () => document.querySelector('[aria-label="Load snapshot 1"]') !== null,
      "row to render",
    );
    await click(
      requiredElement('[aria-label="Load snapshot 1"]', HTMLButtonElement),
    );
    await flushReact();
    await click(buttonWithText("Export working copy"));
    await waitUntil(() => saveAsFile.mock.calls.length > 0, "export to save");
    expect(exportRepository).toHaveBeenCalledWith(store.getRepository());
    expect(document.body.textContent).toContain("Discard changes and load");
    await unmount();
  });

  test("Save snapshot saves, then automatically continues the load once clean", async () => {
    const store = dirtyStore();
    const loaded = repository();
    const loadSnapshotIntoWorkingCopy = vi
      .fn<
        (
          id: string,
          storeArgument: RepositoryWorkingCopyStore,
        ) => Promise<LoadSnapshotResult>
      >()
      .mockImplementation((_id, storeArgument) => {
        storeArgument.replaceWorkingCopy(loaded);
        return Promise.resolve({
          ok: true,
          repository: loaded,
          created: false,
        });
      });
    const onSaveSnapshot = vi.fn().mockImplementation(() => {
      store.markSaved(store.getRepository());
      return Promise.resolve();
    });
    const view = await render(
      <SnapshotsPage
        dependencies={makeDependencies({ loadSnapshotIntoWorkingCopy })}
        loadedBlobId={null}
        onOpenMacros={vi.fn()}
        onOpenPackages={vi.fn()}
        onSaveSnapshot={onSaveSnapshot}
        onSelectionChange={vi.fn()}
        onWorkingCopyOriginChanged={vi.fn()}
        retentionTarget={5}
        saveError={null}
        saving={false}
        selectedPackageId={packageId}
        store={store}
      />,
    );
    await flushReact();
    await waitUntil(
      () => document.querySelector('[aria-label="Load snapshot 1"]') !== null,
      "row to render",
    );
    await click(
      requiredElement('[aria-label="Load snapshot 1"]', HTMLButtonElement),
    );
    await flushReact();
    await click(buttonWithText("Save snapshot"));
    await waitUntil(
      () => loadSnapshotIntoWorkingCopy.mock.calls.length > 0,
      "load to proceed after save",
    );
    expect(onSaveSnapshot).toHaveBeenCalledTimes(1);
    await view.unmount();
  });

  test("Discard changes and load discards edits, then loads immediately", async () => {
    const store = dirtyStore();
    const loaded = repository();
    const loadSnapshotIntoWorkingCopy = vi
      .fn<
        (
          id: string,
          storeArgument: RepositoryWorkingCopyStore,
        ) => Promise<LoadSnapshotResult>
      >()
      .mockImplementation((_id, storeArgument) => {
        storeArgument.replaceWorkingCopy(loaded);
        return Promise.resolve({
          ok: true,
          repository: loaded,
          created: false,
        });
      });
    const { unmount } = await renderPage({
      store,
      deps: { loadSnapshotIntoWorkingCopy },
    });
    await waitUntil(
      () => document.querySelector('[aria-label="Load snapshot 1"]') !== null,
      "row to render",
    );
    await click(
      requiredElement('[aria-label="Load snapshot 1"]', HTMLButtonElement),
    );
    await flushReact();
    await click(buttonWithText("Discard changes and load"));
    await waitUntil(
      () => loadSnapshotIntoWorkingCopy.mock.calls.length > 0,
      "load after discard",
    );
    await unmount();
  });

  test("never deletes or modifies any stored snapshot merely by loading", async () => {
    const deleteSnapshot = vi.fn();
    const loadSnapshotIntoWorkingCopy = vi
      .fn<
        (
          id: string,
          storeArgument: RepositoryWorkingCopyStore,
        ) => Promise<LoadSnapshotResult>
      >()
      .mockResolvedValue({
        ok: true,
        repository: repository(),
        created: false,
      });
    const { unmount } = await renderPage({
      deps: { deleteSnapshot, loadSnapshotIntoWorkingCopy },
    });
    await waitUntil(
      () => document.querySelector('[aria-label="Load snapshot 1"]') !== null,
      "row to render",
    );
    await click(
      requiredElement('[aria-label="Load snapshot 1"]', HTMLButtonElement),
    );
    await waitUntil(
      () => loadSnapshotIntoWorkingCopy.mock.calls.length > 0,
      "load to run",
    );
    expect(deleteSnapshot).not.toHaveBeenCalled();
    await unmount();
  });
});

describe("SnapshotsPage — V2-114 unreadable snapshot recovery", () => {
  test("shows the failing blob and exact error, keeps it stored, and still allows Download/Delete/another Load", async () => {
    const loadSnapshotIntoWorkingCopy = vi
      .fn<
        (
          id: string,
          store: RepositoryWorkingCopyStore,
        ) => Promise<LoadSnapshotResult>
      >()
      .mockResolvedValue({
        ok: false,
        reason: "unreadable",
        message: "The stored snapshot could not be decompressed.",
      });
    const { container, unmount } = await renderPage({
      deps: { loadSnapshotIntoWorkingCopy },
    });
    await waitUntil(
      () => document.querySelector('[aria-label="Load snapshot 1"]') !== null,
      "row to render",
    );
    await click(
      requiredElement('[aria-label="Load snapshot 1"]', HTMLButtonElement),
    );
    await waitUntil(
      () =>
        container.textContent?.includes("could not be decompressed") === true,
      "decode error to render",
    );
    expect(container.textContent).toContain("Snapshot 1 could not be opened");
    // The failed blob remains fully actionable — never silently dropped or
    // replaced with a fallback.
    expect(
      document.querySelector('[aria-label="Download snapshot 1"]'),
    ).not.toBeNull();
    expect(
      document.querySelector('[aria-label="Delete snapshot 1"]'),
    ).not.toBeNull();
    expect(
      document.querySelector('[aria-label="Load snapshot 1"]'),
    ).not.toBeNull();
    await unmount();
  });

  test("an invalid-schema snapshot is reported without silently loading anything else", async () => {
    const loadSnapshotIntoWorkingCopy = vi
      .fn<
        (
          id: string,
          store: RepositoryWorkingCopyStore,
        ) => Promise<LoadSnapshotResult>
      >()
      .mockResolvedValue({ ok: false, reason: "invalid", issues: [] });
    const { container, onOpenMacros, unmount } = await renderPage({
      deps: { loadSnapshotIntoWorkingCopy },
    });
    await waitUntil(
      () => document.querySelector('[aria-label="Load snapshot 1"]') !== null,
      "row to render",
    );
    await click(
      requiredElement('[aria-label="Load snapshot 1"]', HTMLButtonElement),
    );
    await waitUntil(
      () =>
        container.textContent?.includes(
          "does not contain a valid repository",
        ) === true,
      "invalid-schema error to render",
    );
    expect(onOpenMacros).not.toHaveBeenCalled();
    await unmount();
  });
});

describe("SnapshotsPage — V2-111 download and delete", () => {
  test("Download saves the raw stored bytes without any decompression or validation call", async () => {
    const bytes = new Uint8Array([9, 9, 9]);
    const downloadSnapshotBytes = vi.fn().mockResolvedValue(bytes);
    const saveAsFile = vi.fn();
    const importRepository = vi.fn();
    const { unmount } = await renderPage({
      deps: { downloadSnapshotBytes, saveAsFile, importRepository },
    });
    await waitUntil(
      () =>
        document.querySelector('[aria-label="Download snapshot 1"]') !== null,
      "row to render",
    );
    await click(
      requiredElement('[aria-label="Download snapshot 1"]', HTMLButtonElement),
    );
    await waitUntil(() => saveAsFile.mock.calls.length > 0, "download to save");
    expect(downloadSnapshotBytes).toHaveBeenCalledWith("1");
    expect(saveAsFile).toHaveBeenCalledWith(
      bytes,
      "snapshot-1.emk-repository.json.gz",
      "application/gzip",
    );
    expect(importRepository).not.toHaveBeenCalled();
    await unmount();
  });

  test("Delete requires typing the exact blob ID before Confirm delete is enabled", async () => {
    const deleteSnapshot = vi.fn().mockResolvedValue(undefined);
    const { container, unmount } = await renderPage({
      deps: { deleteSnapshot },
    });
    await waitUntil(
      () => document.querySelector('[aria-label="Delete snapshot 1"]') !== null,
      "row to render",
    );
    await click(
      requiredElement('[aria-label="Delete snapshot 1"]', HTMLButtonElement),
    );
    await flushReact();
    expect(container.textContent).toContain("permanently deletes snapshot");
    expect(container.textContent).toContain("cannot be undone");
    const confirmButton = buttonWithText("Confirm delete");
    expect(confirmButton.disabled).toBe(true);

    const idInput = requiredElement(
      "#snapshot-delete-confirm-1",
      HTMLInputElement,
    );
    await setInputValue(idInput, "wrong-id");
    expect(buttonWithText("Confirm delete").disabled).toBe(true);

    await setInputValue(idInput, "1");
    expect(buttonWithText("Confirm delete").disabled).toBe(false);
    await click(buttonWithText("Confirm delete"));
    await waitUntil(
      () => deleteSnapshot.mock.calls.length > 0,
      "delete to run",
    );
    expect(deleteSnapshot).toHaveBeenCalledWith("1");
    await unmount();
  });

  // TODO_V2 V2-133/UI_UX_SPEC_V2 §14 "Dialogs trap focus and restore it to
  // their invoking control". The "Delete snapshot 1" trigger is itself
  // unmounted (replaced by the confirmation panel) the same render that
  // opens it, so this asserts focus restoration on the *reappeared*
  // trigger, not object identity with the original (now-gone) node.
  test("the delete confirmation traps focus and Escape cancels without deleting", async () => {
    const deleteSnapshot = vi.fn();
    const { unmount } = await renderPage({ deps: { deleteSnapshot } });
    await waitUntil(
      () => document.querySelector('[aria-label="Delete snapshot 1"]') !== null,
      "row to render",
    );
    await click(
      requiredElement('[aria-label="Delete snapshot 1"]', HTMLButtonElement),
    );
    await flushReact();
    const dialog = requiredElement('[role="alertdialog"]', HTMLDivElement);
    expect(dialog.contains(document.activeElement)).toBe(true);

    await act(async () => {
      document.dispatchEvent(
        new KeyboardEvent("keydown", {
          key: "Escape",
          bubbles: true,
          cancelable: true,
        }),
      );
      await Promise.resolve();
    });
    expect(document.querySelector('[role="alertdialog"]')).toBeNull();
    expect(deleteSnapshot).not.toHaveBeenCalled();
    expect(document.activeElement).toBe(
      requiredElement('[aria-label="Delete snapshot 1"]', HTMLButtonElement),
    );
    await unmount();
  });

  test("deleting the currently-loaded snapshot clears the loaded-snapshot association", async () => {
    const deleteSnapshot = vi.fn().mockResolvedValue(undefined);
    const { onWorkingCopyOriginChanged, unmount } = await renderPage({
      deps: { deleteSnapshot },
      loadedBlobId: "1",
    });
    await waitUntil(
      () => document.querySelector('[aria-label="Delete snapshot 1"]') !== null,
      "row to render",
    );
    await click(
      requiredElement('[aria-label="Delete snapshot 1"]', HTMLButtonElement),
    );
    await flushReact();
    await setInputValue(
      requiredElement("#snapshot-delete-confirm-1", HTMLInputElement),
      "1",
    );
    await click(buttonWithText("Confirm delete"));
    await waitUntil(
      () => onWorkingCopyOriginChanged.mock.calls.length > 0,
      "origin change after delete",
    );
    expect(onWorkingCopyOriginChanged).toHaveBeenCalledWith(null);
    await unmount();
  });

  test("mounting and viewing the page never issues a delete call on its own", async () => {
    const deleteSnapshot = vi.fn();
    const { unmount } = await renderPage({ deps: { deleteSnapshot } });
    await flushReact();
    await flushReact();
    expect(deleteSnapshot).not.toHaveBeenCalled();
    await unmount();
  });
});

describe("SnapshotsPage — V2-116 advanced non-atomic replace", () => {
  test("delete succeeds, add succeeds: updates the loaded-snapshot association", async () => {
    const created: BlobCreatedResponse = { id: "9", sizeBytes: 42 };
    const replaceSnapshotWithWorkingCopy = vi
      .fn<() => Promise<ReplaceSnapshotResult>>()
      .mockResolvedValue({ ok: true, deletedId: "1", created });
    const { container, onWorkingCopyOriginChanged, unmount } = await renderPage(
      {
        deps: { replaceSnapshotWithWorkingCopy },
      },
    );
    await waitUntil(
      () =>
        document.querySelector(
          '[aria-label="Show advanced options for snapshot 1"]',
        ) !== null,
      "row to render",
    );
    await click(
      requiredElement(
        '[aria-label="Show advanced options for snapshot 1"]',
        HTMLButtonElement,
      ),
    );
    await flushReact();
    await click(
      requiredElement(
        '[aria-label="Replace snapshot 1 with current working copy"]',
        HTMLButtonElement,
      ),
    );
    await flushReact();
    expect(container.textContent).toContain("no atomic replace");
    await click(
      requiredElement(
        '[aria-label="Confirm replace snapshot 1"]',
        HTMLButtonElement,
      ),
    );
    await waitUntil(
      () => onWorkingCopyOriginChanged.mock.calls.length > 0,
      "origin change after replace",
    );
    expect(replaceSnapshotWithWorkingCopy).toHaveBeenCalledTimes(1);
    expect(onWorkingCopyOriginChanged).toHaveBeenCalledWith("9");
    await unmount();
  });

  test("delete succeeds, add fails: warns the deleted blob was not restored", async () => {
    const replaceSnapshotWithWorkingCopy = vi
      .fn<() => Promise<ReplaceSnapshotResult>>()
      .mockResolvedValue({
        ok: false,
        stage: "add",
        deletedId: "1",
        error: new Error("storage_full: No space remains."),
      });
    const { container, unmount } = await renderPage({
      deps: { replaceSnapshotWithWorkingCopy },
    });
    await waitUntil(
      () =>
        document.querySelector(
          '[aria-label="Show advanced options for snapshot 1"]',
        ) !== null,
      "row to render",
    );
    await click(
      requiredElement(
        '[aria-label="Show advanced options for snapshot 1"]',
        HTMLButtonElement,
      ),
    );
    await flushReact();
    await click(
      requiredElement(
        '[aria-label="Replace snapshot 1 with current working copy"]',
        HTMLButtonElement,
      ),
    );
    await flushReact();
    await click(
      requiredElement(
        '[aria-label="Confirm replace snapshot 1"]',
        HTMLButtonElement,
      ),
    );
    await waitUntil(
      () =>
        container.textContent?.includes("was deleted, but uploading") === true,
      "replace-add-failure warning",
    );
    expect(container.textContent).toContain("not");
    expect(container.textContent).toContain("restored");
    await unmount();
  });

  test("delete fails: reports the failure and nothing was changed", async () => {
    const replaceSnapshotWithWorkingCopy = vi
      .fn<() => Promise<ReplaceSnapshotResult>>()
      .mockResolvedValue({
        ok: false,
        stage: "delete",
        deletedId: null,
        error: new Error("not_found: No such blob."),
      });
    const { container, unmount } = await renderPage({
      deps: { replaceSnapshotWithWorkingCopy },
    });
    await waitUntil(
      () =>
        document.querySelector(
          '[aria-label="Show advanced options for snapshot 1"]',
        ) !== null,
      "row to render",
    );
    await click(
      requiredElement(
        '[aria-label="Show advanced options for snapshot 1"]',
        HTMLButtonElement,
      ),
    );
    await flushReact();
    await click(
      requiredElement(
        '[aria-label="Replace snapshot 1 with current working copy"]',
        HTMLButtonElement,
      ),
    );
    await flushReact();
    await click(
      requiredElement(
        '[aria-label="Confirm replace snapshot 1"]',
        HTMLButtonElement,
      ),
    );
    await waitUntil(
      () =>
        container.textContent?.includes("Could not delete snapshot") === true,
      "replace-delete-failure warning",
    );
    expect(container.textContent).toContain("Nothing was changed");
    await unmount();
  });

  test("normal Save current snapshot never calls the replace path", async () => {
    const replaceSnapshotWithWorkingCopy = vi.fn();
    const { onSaveSnapshot, unmount } = await renderPage({
      deps: { replaceSnapshotWithWorkingCopy },
    });
    await click(buttonWithText("Save current snapshot"));
    expect(onSaveSnapshot).toHaveBeenCalledTimes(1);
    expect(replaceSnapshotWithWorkingCopy).not.toHaveBeenCalled();
    await unmount();
  });
});

describe("SnapshotsPage — V2-115 export", () => {
  test("Export working copy downloads the .emk-repository.json.gz gzip bytes without any network/upload call", async () => {
    const bytes = new Uint8Array([1, 2, 3, 4]);
    const exportRepository = vi.fn().mockResolvedValue({
      bytes,
      filename: "repository.emk-repository.json.gz",
      mimeType: "application/gzip",
    });
    const saveAsFile = vi.fn();
    const { store, onSaveSnapshot, unmount } = await renderPage({
      deps: { exportRepository, saveAsFile },
    });
    await click(buttonWithText("Export working copy"));
    await waitUntil(() => saveAsFile.mock.calls.length > 0, "export to save");
    expect(exportRepository).toHaveBeenCalledWith(store.getRepository());
    expect(saveAsFile).toHaveBeenCalledWith(
      bytes,
      "repository.emk-repository.json.gz",
      "application/gzip",
    );
    expect(onSaveSnapshot).not.toHaveBeenCalled();
    await unmount();
  });
});

describe("SnapshotsPage — V2-115 import", () => {
  function fakeFile(): File {
    return new File([new Uint8Array([1, 2, 3])], "repository.json.gz", {
      type: "application/gzip",
    });
  }

  async function chooseFile(file: File): Promise<void> {
    const input = requiredElement('input[type="file"]', HTMLInputElement);
    Object.defineProperty(input, "files", {
      configurable: true,
      value: [file],
    });
    await act(async () => {
      input.dispatchEvent(new Event("change", { bubbles: true }));
      await Promise.resolve();
    });
    await flushReact();
  }

  test("shows package and macro counts before confirmation, without touching the store", async () => {
    const imported = twoPackageRepository();
    const importResult: ImportResult = {
      ok: true,
      repository: imported,
      packageCount: 2,
      macroCount: 0,
    };
    const readFileBytes = vi.fn().mockResolvedValue(new Uint8Array([1]));
    const importRepository = vi.fn().mockResolvedValue(importResult);
    const { container, store, unmount } = await renderPage({
      deps: { readFileBytes, importRepository },
    });
    await chooseFile(fakeFile());
    await waitUntil(
      () => container.textContent?.includes("2 packages") === true,
      "import counts to render",
    );
    expect(container.textContent).toContain("0 macros");
    expect(store.getIsDirty()).toBe(false);
    expect(store.getRepository()).not.toEqual(imported);
    await unmount();
  });

  // TODO_V2 V2-133/UI_UX_SPEC_V2 §14 "Dialogs trap focus and restore it to
  // their invoking control". This confirmation is a sibling of its trigger
  // ("Import repository…" stays mounted; the "ready" panel is added, not
  // swapped in), so the trap's automatic capture is exactly right — but the
  // trigger is a native file input's proxy button, which real focus after a
  // file selection may or may not land on depending on the browser, so this
  // only asserts the trap itself (initial focus, Escape) rather than a
  // specific restore target.
  test("the import confirmation traps focus and Escape cancels it without touching the store", async () => {
    const imported = twoPackageRepository();
    const importResult: ImportResult = {
      ok: true,
      repository: imported,
      packageCount: 2,
      macroCount: 0,
    };
    const readFileBytes = vi.fn().mockResolvedValue(new Uint8Array([1]));
    const importRepository = vi.fn().mockResolvedValue(importResult);
    const { container, store, unmount } = await renderPage({
      deps: { readFileBytes, importRepository },
    });
    await chooseFile(fakeFile());
    await waitUntil(
      () => container.textContent?.includes("2 packages") === true,
      "import counts to render",
    );
    const dialog = requiredElement('[role="alertdialog"]', HTMLDivElement);
    expect(dialog.contains(document.activeElement)).toBe(true);

    await act(async () => {
      document.dispatchEvent(
        new KeyboardEvent("keydown", {
          key: "Escape",
          bubbles: true,
          cancelable: true,
        }),
      );
      await Promise.resolve();
    });
    expect(document.querySelector('[role="alertdialog"]')).toBeNull();
    expect(store.getIsDirty()).toBe(false);
    expect(store.getRepository()).not.toEqual(imported);
    await unmount();
  });

  test("confirming a clean import replaces the working copy, marks it dirty, and never uploads automatically", async () => {
    const imported = twoPackageRepository();
    const importResult: ImportResult = {
      ok: true,
      repository: imported,
      packageCount: 2,
      macroCount: 0,
    };
    const readFileBytes = vi.fn().mockResolvedValue(new Uint8Array([1]));
    const importRepository = vi.fn().mockResolvedValue(importResult);
    const {
      container,
      store,
      onSaveSnapshot,
      onWorkingCopyOriginChanged,
      unmount,
    } = await renderPage({
      deps: { readFileBytes, importRepository },
      selectedPackageId: "does-not-exist",
    });
    await chooseFile(fakeFile());
    await waitUntil(
      () => container.textContent?.includes("2 packages") === true,
      "import counts to render",
    );
    await click(buttonWithText("Replace working copy with this import"));
    await waitUntil(
      () => store.getRepository() === imported,
      "store to be replaced by import",
    );
    expect(store.getIsDirty()).toBe(true);
    expect(onSaveSnapshot).not.toHaveBeenCalled();
    expect(onWorkingCopyOriginChanged).toHaveBeenCalledWith(null);
    await unmount();
  });

  test("importing while dirty warns first, offering Cancel/Export/Save/Discard changes and import", async () => {
    const imported = twoPackageRepository();
    const importResult: ImportResult = {
      ok: true,
      repository: imported,
      packageCount: 2,
      macroCount: 0,
    };
    const store = createRepositoryWorkingCopyStore(repository());
    store.applyContentChange(renamedRepository());
    const readFileBytes = vi.fn().mockResolvedValue(new Uint8Array([1]));
    const importRepository = vi.fn().mockResolvedValue(importResult);
    const { container, unmount } = await renderPage({
      store,
      deps: { readFileBytes, importRepository },
    });
    await chooseFile(fakeFile());
    await waitUntil(
      () => container.textContent?.includes("2 packages") === true,
      "import counts to render",
    );
    await click(buttonWithText("Replace working copy with this import"));
    await flushReact();
    expect(container.textContent).toContain("Unsaved changes");
    buttonWithText("Cancel");
    buttonWithText("Export working copy");
    buttonWithText("Save snapshot");
    buttonWithText("Discard changes and import");
    // The warning gates replacement — the working copy is not the import yet.
    expect(store.getRepository()).not.toEqual(imported);
    await unmount();
  });

  test("Discard changes and import replaces the working copy and dirties it", async () => {
    const imported = twoPackageRepository();
    const importResult: ImportResult = {
      ok: true,
      repository: imported,
      packageCount: 2,
      macroCount: 0,
    };
    const store = createRepositoryWorkingCopyStore(repository());
    store.applyContentChange(renamedRepository());
    const readFileBytes = vi.fn().mockResolvedValue(new Uint8Array([1]));
    const importRepository = vi.fn().mockResolvedValue(importResult);
    const { container, unmount } = await renderPage({
      store,
      deps: { readFileBytes, importRepository },
    });
    await chooseFile(fakeFile());
    await waitUntil(
      () => container.textContent?.includes("2 packages") === true,
      "import counts to render",
    );
    await click(buttonWithText("Replace working copy with this import"));
    await flushReact();
    await click(buttonWithText("Discard changes and import"));
    await waitUntil(
      () => store.getRepository() === imported,
      "store to be replaced after discard-and-import",
    );
    expect(store.getIsDirty()).toBe(true);
    await unmount();
  });

  test("an unreadable or invalid import file shows the exact error and never replaces the working copy", async () => {
    const readFileBytes = vi.fn().mockResolvedValue(new Uint8Array([1]));
    const importRepository = vi.fn().mockResolvedValue({
      ok: false,
      reason: "unreadable",
      message: "The stored snapshot could not be decompressed.",
    });
    const { container, store, unmount } = await renderPage({
      deps: { readFileBytes, importRepository },
    });
    const before = store.getRepository();
    await chooseFile(fakeFile());
    await waitUntil(
      () =>
        container.textContent?.includes("could not be decompressed") === true,
      "import decode error to render",
    );
    expect(store.getRepository()).toBe(before);
    expect(store.getIsDirty()).toBe(false);
    await unmount();
  });

  test("cancelling the import confirmation leaves the working copy untouched", async () => {
    const imported = twoPackageRepository();
    const importResult: ImportResult = {
      ok: true,
      repository: imported,
      packageCount: 2,
      macroCount: 0,
    };
    const readFileBytes = vi.fn().mockResolvedValue(new Uint8Array([1]));
    const importRepository = vi.fn().mockResolvedValue(importResult);
    const { container, store, unmount } = await renderPage({
      deps: { readFileBytes, importRepository },
    });
    const before = store.getRepository();
    await chooseFile(fakeFile());
    await waitUntil(
      () => container.textContent?.includes("2 packages") === true,
      "import counts to render",
    );
    await click(buttonWithText("Cancel"));
    await flushReact();
    expect(store.getRepository()).toBe(before);
    expect(container.textContent).not.toContain("2 packages");
    await unmount();
  });
});
