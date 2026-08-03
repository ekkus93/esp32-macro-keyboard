import { act } from "react";
import { describe, expect, test, vi } from "vitest";
import { PackageManagementPage } from "../src/features/package/PackageManagementPage";
import type { MacroPackage } from "../src/types/models";
import { macroPackage, packageId, settings } from "./appFixtures";
import { getFetchCalls, planJsonResponse } from "./fakeFetch";
import {
  buttonWithText,
  click,
  flushReact,
  render,
  requiredElement,
  setInputValue,
  submit,
} from "./render";

const secondPackageId = "99999999-9999-4999-8999-999999999999";
const secondPackage: MacroPackage = {
  ...macroPackage,
  id: secondPackageId,
  revision: 3,
  name: "Second workflow",
};

function success(data: unknown): object {
  return { ok: true, data };
}

function jsonBody(index: number): unknown {
  const body = getFetchCalls()[index]?.body;
  if (typeof body !== "string") {
    throw new Error(`Request ${String(index)} has no JSON body.`);
  }
  return JSON.parse(body) as unknown;
}

describe("package management", () => {
  // SPEC 24.5 item: including that a reorder round-trips through the API
  test("offers keyboard reorder alternatives and commits exact order", async () => {
    const onPackagesChanged = vi.fn<(packages: MacroPackage[]) => void>();
    planJsonResponse(success([secondPackage, macroPackage]));
    const view = await render(
      <PackageManagementPage
        onPackagesChanged={onPackagesChanged}
        packages={[macroPackage, secondPackage]}
        settings={settings}
      />,
    );

    const moveDown = Array.from(document.querySelectorAll("button")).find(
      (button) =>
        button.getAttribute("aria-label") === `Move ${macroPackage.name} down`,
    );
    if (!(moveDown instanceof HTMLButtonElement)) {
      throw new Error("Missing accessible Move down button.");
    }
    await click(moveDown);
    await flushReact();

    expect(getFetchCalls()[0]?.url).toBe("/api/v1/package/order");
    expect(jsonBody(0)).toEqual({ ids: [secondPackageId, packageId] });
    expect(onPackagesChanged).toHaveBeenCalledWith([
      secondPackage,
      macroPackage,
    ]);
    expect(document.body.textContent).toContain(
      `Moved ${macroPackage.name} to position 2.`,
    );
    await view.unmount();
  });

  // SPEC 24.5 item: keyboard and touch accessibility

  test("traps modal focus, closes with Escape, and restores focus", async () => {
    const view = await render(
      <PackageManagementPage
        onPackagesChanged={() => undefined}
        packages={[macroPackage, secondPackage]}
        settings={settings}
      />,
    );
    const opener = buttonWithText("Create package");
    opener.focus();
    await click(opener);
    await flushReact();

    const dialog = requiredElement('[role="dialog"]', HTMLDivElement);
    expect(dialog.contains(document.activeElement)).toBe(true);
    expect((document.activeElement as HTMLElement).textContent).toBe("Close");

    await act(async () => {
      document.dispatchEvent(
        new KeyboardEvent("keydown", {
          key: "Tab",
          shiftKey: true,
          bubbles: true,
          cancelable: true,
        }),
      );
      await Promise.resolve();
    });
    expect(dialog.contains(document.activeElement)).toBe(true);
    expect((document.activeElement as HTMLElement).textContent).toBe("Cancel");

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
    expect(document.querySelector('[role="dialog"]')).toBeNull();
    expect(document.activeElement).toBe(opener);
    await view.unmount();
  });

  // SPEC 24.5 item: live validation

  test("creates a package only after UTF-8 validation succeeds", async () => {
    const onPackagesChanged = vi.fn<(packages: MacroPackage[]) => void>();
    const view = await render(
      <PackageManagementPage
        onPackagesChanged={onPackagesChanged}
        packages={[macroPackage]}
        settings={settings}
      />,
    );
    await click(buttonWithText("Create package"));
    await setInputValue(
      requiredElement("#package-name", HTMLInputElement),
      "New bench workflow",
    );

    const created: MacroPackage = {
      schema_version: 1,
      id: "aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa",
      revision: 1,
      name: "New bench workflow",
    };
    planJsonResponse(success(created), 201);
    await submit(requiredElement('[role="dialog"] form', HTMLFormElement));
    await flushReact();

    expect(getFetchCalls()[0]?.url).toBe("/api/v1/package");
    expect(getFetchCalls()[0]?.method).toBe("POST");
    expect(jsonBody(0)).toMatchObject({
      revision: 1,
      name: "New bench workflow",
    });
    expect(onPackagesChanged).toHaveBeenCalledWith([macroPackage, created]);
    expect(document.body.textContent).toContain("Created New bench workflow.");
    await view.unmount();
  });

  // SPEC 24.5 item: import/export/delete confirmations

  test("prevents active-package deletion and requires exact name for another package", async () => {
    const onPackagesChanged = vi.fn<(packages: MacroPackage[]) => void>();
    const view = await render(
      <PackageManagementPage
        onPackagesChanged={onPackagesChanged}
        packages={[macroPackage, secondPackage]}
        settings={settings}
      />,
    );

    const deleteButtons = Array.from(
      document.querySelectorAll("button"),
    ).filter((button) => button.textContent?.trim() === "Delete");
    expect(deleteButtons[0]?.disabled).toBe(true);
    expect(deleteButtons[1]?.disabled).toBe(false);
    const deletableButton = deleteButtons[1];
    if (deletableButton === undefined) {
      throw new Error("Missing deletable package control.");
    }
    await click(deletableButton);
    expect(buttonWithText("Delete permanently").disabled).toBe(true);
    await setInputValue(
      requiredElement("#delete-package-confirmation", HTMLInputElement),
      secondPackage.name,
    );
    expect(buttonWithText("Delete permanently").disabled).toBe(false);

    planJsonResponse(success({ deleted: true, id: secondPackageId }));
    await submit(requiredElement('[role="dialog"] form', HTMLFormElement));
    await flushReact();

    expect(jsonBody(0)).toEqual({ expectedRevision: secondPackage.revision });
    expect(onPackagesChanged).toHaveBeenCalledWith([macroPackage]);
    await view.unmount();
  });
});
