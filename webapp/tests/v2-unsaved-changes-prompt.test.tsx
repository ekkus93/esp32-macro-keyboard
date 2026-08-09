import { describe, expect, test, vi } from "vitest";
import { UnsavedChangesPrompt } from "../src/features/shell/v2/UnsavedChangesPrompt";
import { buttonWithText, click, render } from "./render";

describe("UnsavedChangesPrompt — V2-103", () => {
  test("names the attempted action and offers Cancel, Export, Save, and Discard", async () => {
    const { container, unmount } = await render(
      <UnsavedChangesPrompt
        actionLabel="sign out"
        onCancel={vi.fn()}
        onDiscard={vi.fn()}
        onExport={vi.fn()}
        onSaveSnapshot={vi.fn()}
      />,
    );
    expect(container.textContent).toContain("sign out");
    expect(container.textContent).toContain("Unsaved changes");
    buttonWithText("Cancel");
    buttonWithText("Export working copy");
    buttonWithText("Save snapshot");
    buttonWithText("Discard changes");
    await unmount();
  });

  test("never claims a closed dirty working copy can be recovered", async () => {
    const { container, unmount } = await render(
      <UnsavedChangesPrompt
        actionLabel="load this snapshot"
        onCancel={vi.fn()}
        onDiscard={vi.fn()}
        onExport={vi.fn()}
        onSaveSnapshot={vi.fn()}
      />,
    );
    expect(container.textContent).toContain("cannot be recovered");
    expect(container.textContent).not.toMatch(/can be (restored|recovered)/i);
    await unmount();
  });

  test.each([
    ["Cancel", "onCancel"],
    ["Export working copy", "onExport"],
    ["Save snapshot", "onSaveSnapshot"],
    ["Discard changes", "onDiscard"],
  ] as const)("%s calls %s", async (label, handlerName) => {
    const handlers = {
      onCancel: vi.fn(),
      onExport: vi.fn(),
      onSaveSnapshot: vi.fn(),
      onDiscard: vi.fn(),
    };
    const { unmount } = await render(
      <UnsavedChangesPrompt actionLabel="reset settings" {...handlers} />,
    );
    await click(buttonWithText(label));
    expect(handlers[handlerName]).toHaveBeenCalledOnce();
    await unmount();
  });

  test("supports a context-specific discard label (V2-113 'Discard changes and load')", async () => {
    const { unmount } = await render(
      <UnsavedChangesPrompt
        actionLabel="load this snapshot"
        discardLabel="Discard changes and load"
        onCancel={vi.fn()}
        onDiscard={vi.fn()}
        onExport={vi.fn()}
        onSaveSnapshot={vi.fn()}
      />,
    );
    buttonWithText("Discard changes and load");
    await unmount();
  });

  test("disables Export and Save while their operations are in flight", async () => {
    const { unmount } = await render(
      <UnsavedChangesPrompt
        actionLabel="factory reset"
        exporting={true}
        onCancel={vi.fn()}
        onDiscard={vi.fn()}
        onExport={vi.fn()}
        onSaveSnapshot={vi.fn()}
        saving={true}
      />,
    );
    expect(buttonWithText("Exporting…").disabled).toBe(true);
    expect(buttonWithText("Saving…").disabled).toBe(true);
    await unmount();
  });
});
