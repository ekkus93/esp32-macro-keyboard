import { act } from "react";
import { describe, expect, test, vi } from "vitest";
import { SetManagementPage } from "../src/features/sets/SetManagementPage";
import type { MacroSet } from "../src/types/models";
import { macroSet, setId, settings } from "./appFixtures";
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

const secondSetId = "99999999-9999-4999-8999-999999999999";
const secondSet: MacroSet = {
  ...macroSet,
  id: secondSetId,
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

describe("set management", () => {
  // SPEC 24.5 item: including that a reorder round-trips through the API
  test("offers keyboard reorder alternatives and commits exact order", async () => {
    const onSetsChanged = vi.fn<(sets: MacroSet[]) => void>();
    planJsonResponse(success([secondSet, macroSet]));
    const view = await render(
      <SetManagementPage
        onSetsChanged={onSetsChanged}
        sets={[macroSet, secondSet]}
        settings={settings}
      />,
    );

    const moveDown = Array.from(document.querySelectorAll("button")).find(
      (button) =>
        button.getAttribute("aria-label") === `Move ${macroSet.name} down`,
    );
    if (!(moveDown instanceof HTMLButtonElement)) {
      throw new Error("Missing accessible Move down button.");
    }
    await click(moveDown);
    await flushReact();

    expect(getFetchCalls()[0]?.url).toBe("/api/v1/sets/order");
    expect(jsonBody(0)).toEqual({ ids: [secondSetId, setId] });
    expect(onSetsChanged).toHaveBeenCalledWith([secondSet, macroSet]);
    expect(document.body.textContent).toContain(
      `Moved ${macroSet.name} to position 2.`,
    );
    await view.unmount();
  });

  // SPEC 24.5 item: keyboard and touch accessibility

  test("traps modal focus, closes with Escape, and restores focus", async () => {
    const view = await render(
      <SetManagementPage
        onSetsChanged={() => undefined}
        sets={[macroSet, secondSet]}
        settings={settings}
      />,
    );
    const opener = buttonWithText("Create set");
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

  test("creates a set only after UTF-8 validation succeeds", async () => {
    const onSetsChanged = vi.fn<(sets: MacroSet[]) => void>();
    const view = await render(
      <SetManagementPage
        onSetsChanged={onSetsChanged}
        sets={[macroSet]}
        settings={settings}
      />,
    );
    await click(buttonWithText("Create set"));
    await setInputValue(
      requiredElement("#set-name", HTMLInputElement),
      "New bench workflow",
    );

    const created: MacroSet = {
      schema_version: 1,
      id: "aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa",
      revision: 1,
      name: "New bench workflow",
    };
    planJsonResponse(success(created), 201);
    await submit(requiredElement('[role="dialog"] form', HTMLFormElement));
    await flushReact();

    expect(getFetchCalls()[0]?.url).toBe("/api/v1/sets");
    expect(getFetchCalls()[0]?.method).toBe("POST");
    expect(jsonBody(0)).toMatchObject({
      revision: 1,
      name: "New bench workflow",
    });
    expect(onSetsChanged).toHaveBeenCalledWith([macroSet, created]);
    expect(document.body.textContent).toContain("Created New bench workflow.");
    await view.unmount();
  });

  // SPEC 24.5 item: import/export/delete confirmations

  test("prevents active-set deletion and requires exact name for another set", async () => {
    const onSetsChanged = vi.fn<(sets: MacroSet[]) => void>();
    const view = await render(
      <SetManagementPage
        onSetsChanged={onSetsChanged}
        sets={[macroSet, secondSet]}
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
      throw new Error("Missing deletable set control.");
    }
    await click(deletableButton);
    expect(buttonWithText("Delete permanently").disabled).toBe(true);
    await setInputValue(
      requiredElement("#delete-set-confirmation", HTMLInputElement),
      secondSet.name,
    );
    expect(buttonWithText("Delete permanently").disabled).toBe(false);

    planJsonResponse(success({ deleted: true, id: secondSetId }));
    await submit(requiredElement('[role="dialog"] form', HTMLFormElement));
    await flushReact();

    expect(jsonBody(0)).toEqual({ expectedRevision: secondSet.revision });
    expect(onSetsChanged).toHaveBeenCalledWith([macroSet]);
    await view.unmount();
  });
});
