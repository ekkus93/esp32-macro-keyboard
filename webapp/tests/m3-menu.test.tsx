import { act } from "react";
import { describe, expect, test, vi } from "vitest";
import { Menu } from "../src/components/m3/Menu";
import type { MenuItem } from "../src/components/m3/Menu";
import { click, render, requiredElement } from "./render";

function press(target: Element, key: string): Promise<void> {
  return act(async () => {
    target.dispatchEvent(
      new KeyboardEvent("keydown", { key, bubbles: true, cancelable: true }),
    );
    await Promise.resolve();
  });
}

function items(onSelect: () => void): MenuItem[] {
  return [
    { key: "a", label: "First", onSelect },
    { key: "b", label: "Second (disabled)", disabled: true, onSelect },
    { key: "c", label: "Third", onSelect },
    { key: "d", label: "Fourth", tone: "danger", onSelect },
  ];
}

async function renderOpenMenu(onSelect = vi.fn()): Promise<{
  trigger: HTMLButtonElement;
  menu: HTMLElement;
  unmount: () => Promise<void>;
}> {
  const view = await render(
    <Menu
      items={items(onSelect)}
      menuLabel="Row actions"
      triggerLabel="More"
    />,
  );
  const trigger = requiredElement('[aria-label="More"]', HTMLButtonElement);
  await click(trigger);
  return {
    trigger,
    menu: requiredElement('[role="menu"]', HTMLElement),
    unmount: view.unmount,
  };
}

const labelOf = (element: Element | null): string | undefined =>
  element?.textContent?.trim();

describe("m3 Menu", () => {
  test("the trigger advertises a menu and its expanded state", async () => {
    const view = await render(
      <Menu
        items={items(vi.fn())}
        menuLabel="Row actions"
        triggerLabel="More"
      />,
    );
    const trigger = requiredElement('[aria-label="More"]', HTMLButtonElement);
    expect(trigger.getAttribute("aria-haspopup")).toBe("menu");
    expect(trigger.getAttribute("aria-expanded")).toBe("false");
    expect(trigger.getAttribute("title")).toBe("More");
    await click(trigger);
    expect(trigger.getAttribute("aria-expanded")).toBe("true");
    await view.unmount();
  });

  test("opening focuses the first enabled item", async () => {
    const { menu, unmount } = await renderOpenMenu();
    expect(menu.getAttribute("aria-label")).toBe("Row actions");
    expect(labelOf(document.activeElement)).toBe("First");
    await unmount();
  });

  test("arrow keys move between enabled items, skip disabled ones, and wrap", async () => {
    const { menu, unmount } = await renderOpenMenu();
    await press(menu, "ArrowDown");
    expect(labelOf(document.activeElement)).toBe("Third");
    await press(menu, "ArrowDown");
    expect(labelOf(document.activeElement)).toBe("Fourth");
    await press(menu, "ArrowDown");
    expect(labelOf(document.activeElement)).toBe("First");
    await press(menu, "ArrowUp");
    expect(labelOf(document.activeElement)).toBe("Fourth");
    await unmount();
  });

  test("Home and End jump to the first and last enabled items", async () => {
    const { menu, unmount } = await renderOpenMenu();
    await press(menu, "End");
    expect(labelOf(document.activeElement)).toBe("Fourth");
    await press(menu, "Home");
    expect(labelOf(document.activeElement)).toBe("First");
    await unmount();
  });

  test("Escape closes the menu and returns focus to the trigger", async () => {
    const { menu, trigger, unmount } = await renderOpenMenu();
    await press(menu, "Escape");
    expect(document.querySelector('[role="menu"]')).toBeNull();
    expect(document.activeElement).toBe(trigger);
    await unmount();
  });

  test("Tab closes the menu", async () => {
    const { menu, unmount } = await renderOpenMenu();
    await press(menu, "Tab");
    expect(document.querySelector('[role="menu"]')).toBeNull();
    await unmount();
  });

  test("choosing an item closes the menu, restores focus, and calls onSelect once", async () => {
    const onSelect = vi.fn();
    const { menu, trigger, unmount } = await renderOpenMenu(onSelect);
    const third = Array.from(
      menu.querySelectorAll<HTMLButtonElement>('[role="menuitem"]'),
    ).find((button) => labelOf(button) === "Third");
    if (third === undefined) {
      throw new Error("The Third menu item is missing.");
    }
    await click(third);
    expect(onSelect).toHaveBeenCalledOnce();
    expect(document.querySelector('[role="menu"]')).toBeNull();
    expect(document.activeElement).toBe(trigger);
    await unmount();
  });

  test("a disabled item is present, disabled, and cannot be chosen", async () => {
    const onSelect = vi.fn();
    const { menu, unmount } = await renderOpenMenu(onSelect);
    const disabled = Array.from(
      menu.querySelectorAll<HTMLButtonElement>('[role="menuitem"]'),
    ).find((button) => labelOf(button) === "Second (disabled)");
    expect(disabled?.disabled).toBe(true);
    await unmount();
  });
});
