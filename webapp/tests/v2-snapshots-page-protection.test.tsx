import { describe, expect, test, vi } from "vitest";
import { SnapshotsPage } from "../src/features/snapshots/v2/SnapshotsPage";
import { createRepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import type { RepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import type { LoadSnapshotResult } from "../src/v2/snapshotClient";
import {
  buttonWithText,
  click,
  flushReact,
  render,
  requiredElement,
} from "./render";
import {
  makeDependencies,
  packageId,
  renderPage,
  repository,
  twoPackageRepository,
  waitUntil,
} from "./snapshotsPageHarness";

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
        onSelectionPersistenceFailure={vi.fn()}
        onSelectionPersistenceSuccess={vi.fn()}
        onWorkingCopyOriginChanged={vi.fn()}
        persistedPackageId={packageId}
        retentionTarget={5}
        saveError={null}
        saving={false}
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
