import { describe, expect, test, vi } from "vitest";
import { PackageManagementPage } from "../src/features/macros/v2/PackageManagementPage";
import type { Repository } from "../src/v2/repository";
import { createRepositoryWorkingCopyStore } from "../src/v2/repositoryWorkingCopy";
import { buttonWithText, click, render, requiredElement, setInputValue } from "./render";

const packageAId = "550e8400-e29b-41d4-a716-446655440000";
const packageBId = "550e8400-e29b-41d4-a716-446655440001";
const uuidPattern =
  /^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$/;

function repository(): Repository {
  return {
    format: "esp32-macro-keyboard-repository",
    schemaVersion: 1,
    packages: [
      { id: packageAId, name: "Package A", macros: [] },
      { id: packageBId, name: "Package B", macros: [] },
    ],
  };
}

function renderPage(
  store: ReturnType<typeof createRepositoryWorkingCopyStore>,
  selectedPackageId: string,
) {
  const persistSelectedPackageId = vi.fn().mockResolvedValue(null);
  const onSelectionChange = vi.fn();
  const onOpenMacros = vi.fn();
  const renderPromise = render(
    <PackageManagementPage
      dependencies={{ persistSelectedPackageId }}
      onOpenMacros={onOpenMacros}
      onSelectionChange={onSelectionChange}
      selectedPackageId={selectedPackageId}
      store={store}
    />,
  );
  return renderPromise.then((result) => ({
    ...result,
    persistSelectedPackageId,
    onSelectionChange,
    onOpenMacros,
  }));
}

describe("PackageManagementPage — V2-102 create/rename/duplicate/reorder/delete", () => {
  test("creates a package with a canonical UUID v4 ID and dirties the working copy", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const { unmount } = await renderPage(store, packageAId);
    await setInputValue(
      requiredElement("#package-management-create-name", HTMLInputElement),
      "New package",
    );
    await click(buttonWithText("Create package"));

    expect(store.getIsDirty()).toBe(true);
    const created = store
      .getRepository()
      .packages.find((pkg) => pkg.name === "New package");
    expect(created).toBeDefined();
    expect(created?.id).toMatch(uuidPattern);
    await unmount();
  });

  test("renames a package and dirties the working copy", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const { unmount } = await renderPage(store, packageAId);
    await click(requiredElement('[aria-label="Rename Package A"]', HTMLButtonElement));
    const renameInput = requiredElement(
      `#package-rename-${packageAId}`,
      HTMLInputElement,
    );
    await setInputValue(renameInput, "Renamed package");
    await click(buttonWithText("Save name"));

    expect(store.getIsDirty()).toBe(true);
    expect(
      store.getRepository().packages.find((pkg) => pkg.id === packageAId)?.name,
    ).toBe("Renamed package");
    await unmount();
  });

  test("renaming to the exact same name does not dirty the working copy (avoids a no-op transition)", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const { unmount } = await renderPage(store, packageAId);
    await click(requiredElement('[aria-label="Rename Package A"]', HTMLButtonElement));
    await click(buttonWithText("Save name"));

    expect(store.getIsDirty()).toBe(false);
    await unmount();
  });

  test("duplicates a package with a fresh UUID and dirties the working copy", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const { container, unmount } = await renderPage(store, packageAId);
    await click(requiredElement('[aria-label="Duplicate Package A"]', HTMLButtonElement));

    expect(store.getIsDirty()).toBe(true);
    expect(container.textContent).toContain("Package A copy");
    const duplicated = store
      .getRepository()
      .packages.find((pkg) => pkg.name === "Package A copy");
    expect(duplicated?.id).toMatch(uuidPattern);
    expect(duplicated?.id).not.toBe(packageAId);
    await unmount();
  });

  test("reorders packages and dirties the working copy; boundary moves are disabled", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const { unmount } = await renderPage(store, packageAId);
    expect(
      requiredElement('[aria-label="Move Package A up"]', HTMLButtonElement)
        .disabled,
    ).toBe(true);
    expect(
      requiredElement('[aria-label="Move Package B down"]', HTMLButtonElement)
        .disabled,
    ).toBe(true);

    await click(requiredElement('[aria-label="Move Package A down"]', HTMLButtonElement));
    expect(store.getRepository().packages.map((pkg) => pkg.id)).toEqual([
      packageBId,
      packageAId,
    ]);
    expect(store.getIsDirty()).toBe(true);
    await unmount();
  });

  test("deleting a non-selected package requires confirmation, identifies it by name, and does not resolve selection", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const { container, onSelectionChange, unmount } = await renderPage(
      store,
      packageAId,
    );
    await click(requiredElement('[aria-label="Delete Package B"]', HTMLButtonElement));
    expect(container.textContent).toContain("Delete");
    expect(container.textContent).toContain("Package B");
    expect(store.getIsDirty()).toBe(false);

    await click(buttonWithText("Confirm delete"));
    expect(store.getRepository().packages.map((pkg) => pkg.id)).toEqual([
      packageAId,
    ]);
    expect(store.getIsDirty()).toBe(true);
    expect(onSelectionChange).not.toHaveBeenCalled();
    await unmount();
  });

  test("Cancel on a delete confirmation changes nothing", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const { unmount } = await renderPage(store, packageAId);
    await click(requiredElement('[aria-label="Delete Package B"]', HTMLButtonElement));
    await click(buttonWithText("Cancel"));
    expect(store.getIsDirty()).toBe(false);
    expect(store.getRepository().packages).toHaveLength(2);
    await unmount();
  });
});

describe("PackageManagementPage — V2-102 selected-package deletion", () => {
  test("deleting the selected package explains that another package must be selected", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const { container, unmount } = await renderPage(store, packageAId);
    await click(requiredElement('[aria-label="Delete Package A"]', HTMLButtonElement));
    expect(container.textContent).toContain("currently selected package");
    await unmount();
  });

  test("resolves and persists the remaining sole package as the new selection", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const { onSelectionChange, persistSelectedPackageId, unmount } =
      await renderPage(store, packageAId);
    await click(requiredElement('[aria-label="Delete Package A"]', HTMLButtonElement));
    await click(buttonWithText("Confirm delete"));

    expect(onSelectionChange).toHaveBeenCalledWith(packageBId);
    expect(persistSelectedPackageId).toHaveBeenCalledWith(packageBId, packageAId);
    await unmount();
  });

  test("leaves selection unresolved when several packages still remain", async () => {
    const threePackages: Repository = {
      format: "esp32-macro-keyboard-repository",
      schemaVersion: 1,
      packages: [
        { id: packageAId, name: "Package A", macros: [] },
        { id: packageBId, name: "Package B", macros: [] },
        {
          id: "550e8400-e29b-41d4-a716-446655440002",
          name: "Package C",
          macros: [],
        },
      ],
    };
    const store = createRepositoryWorkingCopyStore(threePackages);
    const { onSelectionChange, unmount } = await renderPage(store, packageAId);
    await click(requiredElement('[aria-label="Delete Package A"]', HTMLButtonElement));
    await click(buttonWithText("Confirm delete"));

    expect(onSelectionChange).not.toHaveBeenCalled();
    expect(store.getRepository().packages).toHaveLength(2);
    await unmount();
  });
});

describe("PackageManagementPage — V2-102 ordinary switching never dirties", () => {
  test("Open persists selection, notifies the caller, and navigates without dirtying the repository", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const { onOpenMacros, onSelectionChange, persistSelectedPackageId, unmount } =
      await renderPage(store, packageAId);
    await click(requiredElement('[aria-label="Open Package B"]', HTMLButtonElement));

    expect(persistSelectedPackageId).toHaveBeenCalledWith(packageBId, packageAId);
    expect(onSelectionChange).toHaveBeenCalledWith(packageBId);
    expect(onOpenMacros).toHaveBeenCalledOnce();
    expect(store.getIsDirty()).toBe(false);
    await unmount();
  });
});

describe("PackageManagementPage — V2-102 search", () => {
  test("filters the package list by name", async () => {
    const store = createRepositoryWorkingCopyStore(repository());
    const { container, unmount } = await renderPage(store, packageAId);
    await setInputValue(
      requiredElement("#package-management-search", HTMLInputElement),
      "B",
    );
    expect(container.textContent).toContain("Package B");
    expect(container.textContent).not.toContain("Package A");
    await unmount();
  });
});
