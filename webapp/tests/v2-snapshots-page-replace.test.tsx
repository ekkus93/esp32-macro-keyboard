import { act } from "react";
import { describe, expect, test, vi } from "vitest";
import type { ReplaceSnapshotResult } from "../src/v2/snapshotClient";
import type { BlobCreatedResponse } from "../src/v2/apiTypes";
import {
  buttonWithText,
  click,
  flushReact,
  requiredElement,
  setInputValue,
} from "./render";
import { renderPage, waitUntil } from "./snapshotsPageHarness";

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
