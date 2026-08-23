import { act } from "react";
import { describe, expect, test } from "vitest";
import { buttonWithText, click, requiredElement } from "./render";
import { macroBId, packageId, renderMacrosPage } from "./macrosPageHarness";

describe("MacrosPage — V2-091 macro list", () => {
  test("shows ordered macros, macro count, and Add macro", async () => {
    const { container, callbacks } = await renderMacrosPage();
    expect(container.textContent).toContain("Build server login");
    expect(container.textContent).toContain("2 macros");
    const names = Array.from(container.querySelectorAll("h3")).map(
      (element) => element.textContent,
    );
    expect(names).toEqual(["Start the build", "Open terminal"]);

    await click(buttonWithText("Add macro"));
    expect(callbacks.onOpenAddMacro).toHaveBeenCalledOnce();

    await click(buttonWithText("Change"));
    expect(callbacks.onChangePackage).toHaveBeenCalledOnce();
  });

  test("Send is disabled unless USB is ready", async () => {
    const { container } = await renderMacrosPage({ usbState: "disconnected" });
    const sendButtons = Array.from(
      container.querySelectorAll<HTMLButtonElement>('[aria-label^="Send "]'),
    );
    expect(sendButtons).toHaveLength(2);
    for (const button of sendButtons) {
      expect(button.disabled).toBe(true);
    }
  });

  test("accessible reordering moves a macro and announces the move", async () => {
    const { container, store } = await renderMacrosPage();
    await click(
      requiredElement(
        '[aria-label="Move Start the build down"]',
        HTMLButtonElement,
      ),
    );
    const firstPackage = store
      .getRepository()
      .packages.find((pkg) => pkg.id === packageId);
    expect(firstPackage).toBeDefined();
    const names = firstPackage?.macros.map((macro) => macro.name);
    expect(names).toEqual(["Open terminal", "Start the build"]);
    expect(store.getIsDirty()).toBe(true);
    expect(container.textContent).toContain(
      "Moved Start the build to position 2.",
    );
  });

  test("Edit calls onOpenEditMacro with exactly the clicked row's macro ID", async () => {
    const { callbacks } = await renderMacrosPage();
    await click(
      requiredElement('[aria-label="Edit Open terminal"]', HTMLButtonElement),
    );
    expect(callbacks.onOpenEditMacro).toHaveBeenCalledExactlyOnceWith(macroBId);
  });

  test("the first row cannot move up and the last row cannot move down", async () => {
    await renderMacrosPage();
    const moveUpFirst = requiredElement(
      '[aria-label="Move Start the build up"]',
      HTMLButtonElement,
    );
    const moveDownLast = requiredElement(
      '[aria-label="Move Open terminal down"]',
      HTMLButtonElement,
    );
    expect(moveUpFirst.disabled).toBe(true);
    expect(moveDownLast.disabled).toBe(true);
  });

  // TODO_V2 V2-133/UI_UX_SPEC_V2 §14: "Move first"/"Move last" alongside
  // "Move up"/"Move down" — a keyboard-operable alternative to drag and drop.
  test("Move first and Move last are disabled at their respective ends", async () => {
    await renderMacrosPage();
    expect(
      requiredElement(
        '[aria-label="Move Start the build to first"]',
        HTMLButtonElement,
      ).disabled,
    ).toBe(true);
    expect(
      requiredElement(
        '[aria-label="Move Open terminal to last"]',
        HTMLButtonElement,
      ).disabled,
    ).toBe(true);
    expect(
      requiredElement(
        '[aria-label="Move Open terminal to first"]',
        HTMLButtonElement,
      ).disabled,
    ).toBe(false);
    expect(
      requiredElement(
        '[aria-label="Move Start the build to last"]',
        HTMLButtonElement,
      ).disabled,
    ).toBe(false);
  });

  test("Move last reorders to the end of the list and announces the move", async () => {
    const { container, store } = await renderMacrosPage();
    await click(
      requiredElement(
        '[aria-label="Move Start the build to last"]',
        HTMLButtonElement,
      ),
    );
    const firstPackage = store
      .getRepository()
      .packages.find((pkg) => pkg.id === packageId);
    const names = firstPackage?.macros.map((macro) => macro.name);
    expect(names).toEqual(["Open terminal", "Start the build"]);
    expect(store.getIsDirty()).toBe(true);
    expect(container.textContent).toContain(
      "Moved Start the build to position 2.",
    );
  });

  test("Move first reorders to the start of the list", async () => {
    const { store } = await renderMacrosPage();
    await click(
      requiredElement(
        '[aria-label="Move Open terminal to first"]',
        HTMLButtonElement,
      ),
    );
    const firstPackage = store
      .getRepository()
      .packages.find((pkg) => pkg.id === packageId);
    const names = firstPackage?.macros.map((macro) => macro.name);
    expect(names).toEqual(["Open terminal", "Start the build"]);
  });
});

describe("MacrosPage — V2-133 overflow menu dismissal", () => {
  test("Escape closes an open overflow menu", async () => {
    const { unmount } = await renderMacrosPage();
    await click(
      requiredElement(
        '[aria-label="More actions for Start the build"]',
        HTMLButtonElement,
      ),
    );
    expect(
      document.querySelector('[aria-label="Actions for Start the build"]'),
    ).not.toBeNull();

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
    expect(
      document.querySelector('[aria-label="Actions for Start the build"]'),
    ).toBeNull();
    await unmount();
  });

  test("a click outside the overflow menu closes it", async () => {
    const { unmount } = await renderMacrosPage();
    await click(
      requiredElement(
        '[aria-label="More actions for Start the build"]',
        HTMLButtonElement,
      ),
    );
    expect(
      document.querySelector('[aria-label="Actions for Start the build"]'),
    ).not.toBeNull();

    // A real user press fires `pointerdown` before `click`; the dismiss hook
    // listens for `pointerdown` specifically so a press-and-drag-off does
    // not leave the menu open. jsdom has no `PointerEvent` constructor, but
    // the hook only reads `event.target`, so a plain `Event` dispatched as
    // "pointerdown" is indistinguishable to it.
    await act(async () => {
      requiredElement("#macros-title", HTMLHeadingElement).dispatchEvent(
        new Event("pointerdown", { bubbles: true, cancelable: true }),
      );
      await Promise.resolve();
    });
    expect(
      document.querySelector('[aria-label="Actions for Start the build"]'),
    ).toBeNull();
    await unmount();
  });

  test("the delete confirmation traps focus and Escape cancels without deleting", async () => {
    const { store, unmount } = await renderMacrosPage();
    await click(
      requiredElement(
        '[aria-label="More actions for Start the build"]',
        HTMLButtonElement,
      ),
    );
    await click(
      requiredElement(
        '[aria-label="Delete Start the build"]',
        HTMLButtonElement,
      ),
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
    expect(
      store
        .getRepository()
        .packages.find((candidate) => candidate.id === packageId)?.macros,
    ).toHaveLength(2);
    // The original "Delete Start the build" button was itself unmounted
    // (replaced by the confirmation panel) and a new one just remounted in
    // its place — asserts focus landed on that reappeared trigger.
    expect(document.activeElement).toBe(
      requiredElement(
        '[aria-label="Delete Start the build"]',
        HTMLButtonElement,
      ),
    );
    await unmount();
  });
});

describe("MacrosPage — V2-092 macro-source handling", () => {
  test("always shows every macro's source, with no reveal/hide control", async () => {
    const { container } = await renderMacrosPage();
    expect(container.textContent).toContain("make -j8{ENTER}");
    expect(container.textContent).toContain("[{CTRL}{ALT}t]");
    expect(container.textContent).not.toContain("Source hidden");
    expect(
      container.querySelector('[aria-label^="Reveal source for"]'),
    ).toBeNull();
    expect(
      container.querySelector('[aria-label^="Hide source for"]'),
    ).toBeNull();
  });
});
