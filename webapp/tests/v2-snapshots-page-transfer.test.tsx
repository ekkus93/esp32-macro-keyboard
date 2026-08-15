import { act } from "react";
import { describe, expect, test, vi } from "vitest";
import type { Repository } from "../src/v2/repository";
import { createRepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import type { RepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import type {
  ImportResult,
  LoadSnapshotResult,
} from "../src/v2/snapshotClient";
import { buttonWithText, click, flushReact, requiredElement } from "./render";
import {
  otherPackageId,
  packageId,
  renamedRepository,
  renderPage,
  repository,
  twoPackageRepository,
  waitUntil,
} from "./snapshotsPageHarness";

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

  test("export/compression failure is visible, clears busy state, and changes no repository state", async () => {
    const failure = new Error("compression failed");
    const exportRepository = vi.fn().mockRejectedValue(failure);
    const store = createRepositoryWorkingCopyStore(repository());
    store.applyContentChange(renamedRepository());
    const repositoryBefore = store.getRepository();
    const {
      container,
      onSelectionChange,
      onWorkingCopyOriginChanged,
      unmount,
    } = await renderPage({
      deps: { exportRepository },
      loadedBlobId: "1",
      store,
    });

    await click(buttonWithText("Export working copy"));
    await waitUntil(
      () =>
        container.textContent?.includes("Could not export working copy") ===
        true,
      "visible export error",
    );

    expect(container.textContent).toContain("compression failed");
    expect(buttonWithText("Export working copy").disabled).toBe(false);
    expect(store.getRepository()).toBe(repositoryBefore);
    expect(store.getIsDirty()).toBe(true);
    expect(onSelectionChange).not.toHaveBeenCalled();
    expect(onWorkingCopyOriginChanged).not.toHaveBeenCalled();
    await unmount();
  });

  test("save-as-file failure is visible and leaves the working copy and association untouched", async () => {
    const bytes = new Uint8Array([4, 3, 2, 1]);
    const exportRepository = vi.fn().mockResolvedValue({
      bytes,
      filename: "repository.emk-repository.json.gz",
      mimeType: "application/gzip",
    });
    const saveAsFile = vi.fn(() => {
      throw new Error("download blocked");
    });
    const store = createRepositoryWorkingCopyStore(repository());
    const repositoryBefore = store.getRepository();
    const {
      container,
      onSelectionChange,
      onWorkingCopyOriginChanged,
      unmount,
    } = await renderPage({
      deps: { exportRepository, saveAsFile },
      loadedBlobId: "1",
      store,
    });

    await click(buttonWithText("Export working copy"));
    await waitUntil(
      () =>
        container.textContent?.includes("Could not export working copy") ===
        true,
      "visible file-save error",
    );

    expect(container.textContent).toContain("download blocked");
    expect(buttonWithText("Export working copy").disabled).toBe(false);
    expect(store.getRepository()).toBe(repositoryBefore);
    expect(store.getIsDirty()).toBe(false);
    expect(onSelectionChange).not.toHaveBeenCalled();
    expect(onWorkingCopyOriginChanged).not.toHaveBeenCalled();
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
      persistedPackageId: "does-not-exist",
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

  test("import package-resolution persistence failure still opens locally and reports the failure", async () => {
    const imported: Repository = {
      format: "esp32-macro-keyboard-repository",
      schemaVersion: 1,
      packages: [{ id: otherPackageId, name: "Imported package", macros: [] }],
    };
    const importResult: ImportResult = {
      ok: true,
      repository: imported,
      packageCount: 1,
      macroCount: 0,
    };
    const readFileBytes = vi.fn().mockResolvedValue(new Uint8Array([1]));
    const importRepository = vi.fn().mockResolvedValue(importResult);
    const failure = new Error("settings write failed");
    const persistSelectedPackageId = vi.fn().mockRejectedValue(failure);
    const {
      onOpenMacros,
      onSelectionChange,
      onSelectionPersistenceFailure,
      onSelectionPersistenceSuccess,
      onWorkingCopyOriginChanged,
      store,
      unmount,
    } = await renderPage({
      deps: {
        importRepository,
        persistSelectedPackageId,
        readFileBytes,
      },
      persistedPackageId: packageId,
    });

    await chooseFile(fakeFile());
    await waitUntil(
      () => document.body.textContent?.includes("1 packages") === true,
      "import counts to render",
    );
    await click(buttonWithText("Replace working copy with this import"));
    await waitUntil(
      () => onOpenMacros.mock.calls.length > 0,
      "local package open after failed preference write",
    );

    expect(onSelectionChange).toHaveBeenCalledWith(otherPackageId);
    expect(onSelectionPersistenceSuccess).not.toHaveBeenCalled();
    expect(onSelectionPersistenceFailure).toHaveBeenCalledWith({
      packageId: otherPackageId,
      previousPackageId: packageId,
      error: failure,
    });
    expect(onWorkingCopyOriginChanged).toHaveBeenCalledWith(null);
    expect(store.getRepository()).toBe(imported);
    expect(store.getIsDirty()).toBe(true);
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

describe("SnapshotsPage — H8 durable package-selection resolution", () => {
  test("snapshot load resolves from the persisted device preference rather than the previous local package", async () => {
    const loaded = twoPackageRepository();
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
    const persistSelectedPackageId = vi.fn().mockResolvedValue(null);
    const {
      onOpenMacros,
      onSelectionChange,
      onSelectionPersistenceFailure,
      onSelectionPersistenceSuccess,
      store,
      unmount,
    } = await renderPage({
      deps: { loadSnapshotIntoWorkingCopy, persistSelectedPackageId },
      persistedPackageId: otherPackageId,
    });

    await waitUntil(
      () => document.querySelector('[aria-label="Load snapshot 1"]') !== null,
      "snapshot row to render",
    );
    await click(
      requiredElement('[aria-label="Load snapshot 1"]', HTMLButtonElement),
    );
    await waitUntil(
      () => onOpenMacros.mock.calls.length > 0,
      "open macros after durable selection resolution",
    );

    expect(onSelectionChange).toHaveBeenCalledWith(otherPackageId);
    expect(persistSelectedPackageId).not.toHaveBeenCalled();
    expect(onSelectionPersistenceSuccess).toHaveBeenCalledWith(otherPackageId);
    expect(onSelectionPersistenceFailure).not.toHaveBeenCalled();
    expect(store.getIsDirty()).toBe(false);
    await unmount();
  });

  test("repository import also resolves from the persisted device preference", async () => {
    const imported = twoPackageRepository();
    const readFileBytes = vi.fn().mockResolvedValue(new Uint8Array([1]));
    const importRepository = vi.fn().mockResolvedValue({
      ok: true,
      repository: imported,
      packageCount: 2,
      macroCount: 0,
    } satisfies ImportResult);
    const persistSelectedPackageId = vi.fn().mockResolvedValue(null);
    const {
      onOpenMacros,
      onSelectionChange,
      onSelectionPersistenceFailure,
      onSelectionPersistenceSuccess,
      unmount,
    } = await renderPage({
      deps: {
        importRepository,
        persistSelectedPackageId,
        readFileBytes,
      },
      persistedPackageId: otherPackageId,
    });

    const input = requiredElement('input[type="file"]', HTMLInputElement);
    Object.defineProperty(input, "files", {
      configurable: true,
      value: [
        new File([new Uint8Array([1, 2, 3])], "repository.json.gz", {
          type: "application/gzip",
        }),
      ],
    });
    await act(async () => {
      input.dispatchEvent(new Event("change", { bubbles: true }));
      await Promise.resolve();
    });
    await waitUntil(
      () => document.body.textContent?.includes("2 packages") === true,
      "import confirmation",
    );
    await click(buttonWithText("Replace working copy with this import"));
    await waitUntil(
      () => onOpenMacros.mock.calls.length > 0,
      "open macros after imported durable selection resolution",
    );

    expect(onSelectionChange).toHaveBeenCalledWith(otherPackageId);
    expect(persistSelectedPackageId).not.toHaveBeenCalled();
    expect(onSelectionPersistenceSuccess).toHaveBeenCalledWith(otherPackageId);
    expect(onSelectionPersistenceFailure).not.toHaveBeenCalled();
    await unmount();
  });
});
