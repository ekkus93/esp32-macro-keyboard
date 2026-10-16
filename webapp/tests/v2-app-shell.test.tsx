import { describe, expect, test, vi } from "vitest";
import { AppShellV2 } from "../src/features/shell/v2/AppShellV2";
import { buttonWithText, click, render, requiredElement } from "./render";
import type { ActiveSendSummary } from "../src/features/shell/v2/activeSendSummary";
import type { ScreenV2 } from "../src/v2/routingV2";

function shellWith(
  activeSend: ActiveSendSummary | null,
  route: ScreenV2,
): React.JSX.Element {
  return (
    <AppShellV2
      activeSend={activeSend}
      deviceName="Desk Macro Keyboard"
      dirty={false}
      navigate={vi.fn()}
      onSaveSnapshot={vi.fn()}
      packageName="Build server login"
      route={route}
      saveError={null}
      saving={false}
      usbState="ready"
    >
      <p>content</p>
    </AppShellV2>
  );
}

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

  test("highlights Settings while on the diagnostics route (TODO_V2 V2-122: reachable only from Settings, not its own nav destination)", async () => {
    const { container, unmount } = await render(
      <AppShellV2
        deviceName="Desk Macro Keyboard"
        dirty={false}
        navigate={vi.fn()}
        onSaveSnapshot={vi.fn()}
        packageName={null}
        route="diagnostics"
        saveError={null}
        saving={false}
        usbState="ready"
      >
        <p>content</p>
      </AppShellV2>,
    );
    const active = container.querySelector('[aria-current="page"]');
    expect(active?.textContent).toBe("Settings");
    const navButtons = Array.from(
      container.querySelectorAll('nav[aria-label="Primary navigation"] button'),
    ).map((button) => button.textContent);
    expect(navButtons).toEqual(["Macros", "Packages", "Snapshots", "Settings"]);
    await unmount();
  });

  test("shows an active send's status and Cancel outside the Macros route (UXR-020)", async () => {
    const onCancel = vi.fn();
    const { container, unmount } = await render(
      shellWith(
        {
          macroName: "Open terminal",
          statusText: "Sending Open terminal…",
          onCancel,
        },
        "packages",
      ),
    );
    expect(container.textContent).toContain("Sending Open terminal…");
    await click(buttonWithText("Cancel and release all keys"));
    expect(onCancel).toHaveBeenCalledOnce();
    await unmount();
  });

  test("offers no Cancel while a send has nothing to cancel yet, and nothing when idle", async () => {
    const { container, rerender, unmount } = await render(
      shellWith(
        {
          macroName: "Open terminal",
          statusText: "Sending Open terminal…",
          onCancel: null,
        },
        "packages",
      ),
    );
    expect(container.textContent).toContain("Sending Open terminal…");
    expect(container.textContent).not.toContain("Cancel and release all keys");
    await rerender(shellWith(null, "packages"));
    expect(container.textContent).not.toContain("Sending Open terminal…");
    await unmount();
  });

  test("moves focus to the content region when the route changes, but not on first render", async () => {
    const { rerender, unmount } = await render(shellWith(null, "packages"));
    const main = requiredElement("#main-content", HTMLElement);
    expect(document.activeElement).not.toBe(main);
    await rerender(shellWith(null, "snapshots"));
    expect(document.activeElement).toBe(main);
    await unmount();
  });

  test("the skip control moves focus to the content region", async () => {
    const { unmount } = await render(shellWith(null, "macros"));
    await click(buttonWithText("Skip to main content"));
    expect(document.activeElement).toBe(
      requiredElement("#main-content", HTMLElement),
    );
    await unmount();
  });
});
