import { describe, expect, test, vi } from "vitest";
import { AppShellV2 } from "../src/features/shell/v2/AppShellV2";
import { buttonWithText, click, render, requiredElement } from "./render";

describe("AppShellV2 — V2-090 application shell", () => {
  test("shows device name, selected package, USB state, and repository state", async () => {
    const { container, unmount } = await render(
      <AppShellV2
        deviceName="Desk Macro Keyboard"
        dirty={false}
        navigate={vi.fn()}
        onSaveSnapshot={vi.fn()}
        packageName="Build server login"
        route="macros"
        saveError={null}
        saving={false}
        usbState="ready"
      >
        <p>content</p>
      </AppShellV2>,
    );
    expect(container.textContent).toContain("Desk Macro Keyboard");
    expect(container.textContent).toContain("Build server login");
    expect(container.textContent).toContain("USB ready");
    expect(container.textContent).toContain("Saved");
    expect(container.textContent).not.toContain("Save snapshot");
    await unmount();
  });

  test('shows "No package selected" when there is none', async () => {
    const { container, unmount } = await render(
      <AppShellV2
        deviceName="Desk Macro Keyboard"
        dirty={false}
        navigate={vi.fn()}
        onSaveSnapshot={vi.fn()}
        packageName={null}
        route="macros"
        saveError={null}
        saving={false}
        usbState="ready"
      >
        <p>content</p>
      </AppShellV2>,
    );
    expect(container.textContent).toContain("No package selected");
    await unmount();
  });

  test("shows Save snapshot whenever dirty and calls the handler", async () => {
    const onSaveSnapshot = vi.fn();
    const { container, unmount } = await render(
      <AppShellV2
        deviceName="Desk Macro Keyboard"
        dirty={true}
        navigate={vi.fn()}
        onSaveSnapshot={onSaveSnapshot}
        packageName="Build server login"
        route="macros"
        saveError={null}
        saving={false}
        usbState="ready"
      >
        <p>content</p>
      </AppShellV2>,
    );
    expect(container.textContent).toContain("Unsaved changes");
    await click(buttonWithText("Save snapshot"));
    expect(onSaveSnapshot).toHaveBeenCalledOnce();
    await unmount();
  });

  test("implements the fixed bottom navigation with an accessible current marker", async () => {
    const navigate = vi.fn();
    const { container, unmount } = await render(
      <AppShellV2
        deviceName="Desk Macro Keyboard"
        dirty={false}
        navigate={navigate}
        onSaveSnapshot={vi.fn()}
        packageName={null}
        route="snapshots"
        saveError={null}
        saving={false}
        usbState="ready"
      >
        <p>content</p>
      </AppShellV2>,
    );
    const nav = requiredElement(
      'nav[aria-label="Primary navigation"]',
      HTMLElement,
    );
    const labels = Array.from(nav.querySelectorAll("button")).map(
      (button) => button.textContent,
    );
    expect(labels).toEqual(["Macros", "Packages", "Snapshots", "Settings"]);

    const active = container.querySelector('[aria-current="page"]');
    expect(active?.textContent).toBe("Snapshots");

    await click(buttonWithText("Packages"));
    expect(navigate).toHaveBeenCalledWith("packages");
    await unmount();
  });

  test("highlights Macros while on the macro-preview route", async () => {
    const { container, unmount } = await render(
      <AppShellV2
        deviceName="Desk Macro Keyboard"
        dirty={false}
        navigate={vi.fn()}
        onSaveSnapshot={vi.fn()}
        packageName={null}
        route="macro-preview"
        saveError={null}
        saving={false}
        usbState="ready"
      >
        <p>content</p>
      </AppShellV2>,
    );
    const active = container.querySelector('[aria-current="page"]');
    expect(active?.textContent).toBe("Macros");
    await unmount();
  });
});
