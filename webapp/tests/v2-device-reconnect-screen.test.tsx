import { describe, expect, test, vi } from "vitest";
import { DeviceReconnectScreen } from "../src/features/settings/v2/DeviceReconnectScreen";
import { buttonWithText, click, render } from "./render";

describe("DeviceReconnectScreen (TODO_V2 V2-121)", () => {
  test("shows waiting copy naming the action while phase is 'waiting'", async () => {
    const { container, unmount } = await render(
      <DeviceReconnectScreen
        elapsedMs={0}
        errorMessage={null}
        kind="restart"
        onManualReload={vi.fn()}
        phase="waiting"
      />,
    );
    expect(container.textContent).toContain("Reconnecting");
    expect(container.textContent).toContain("restarting");
    await unmount();
  });

  test("names reset-settings and factory-reset distinctly", async () => {
    const { container, unmount } = await render(
      <DeviceReconnectScreen
        elapsedMs={0}
        errorMessage={null}
        kind="reset-settings"
        onManualReload={vi.fn()}
        phase="waiting"
      />,
    );
    expect(container.textContent).toContain("resetting settings");
    await unmount();

    const { container: container2, unmount: unmount2 } = await render(
      <DeviceReconnectScreen
        elapsedMs={0}
        errorMessage={null}
        kind="factory-reset"
        onManualReload={vi.fn()}
        phase="waiting"
      />,
    );
    expect(container2.textContent).toContain("factory reset");
    await unmount2();
  });

  test("shows a longer-than-usual note once elapsed time crosses 15 seconds", async () => {
    const { container, unmount } = await render(
      <DeviceReconnectScreen
        elapsedMs={16_000}
        errorMessage={null}
        kind="restart"
        onManualReload={vi.fn()}
        phase="waiting"
      />,
    );
    expect(container.textContent).toContain("taking longer than usual");
    await unmount();
  });

  test("shows reauthentication guidance without discarding-work language for 'needs-reauth'", async () => {
    const { container, unmount } = await render(
      <DeviceReconnectScreen
        elapsedMs={0}
        errorMessage={null}
        kind="restart"
        onManualReload={vi.fn()}
        phase="needs-reauth"
      />,
    );
    expect(container.textContent).toContain("Sign in again");
    expect(container.textContent).toContain("still here");
    await unmount();
  });

  test("shows the error message and a manual reload button for 'error'", async () => {
    const onManualReload = vi.fn();
    const { container, unmount } = await render(
      <DeviceReconnectScreen
        elapsedMs={0}
        errorMessage="Reconnecting to the device failed."
        kind="restart"
        onManualReload={onManualReload}
        phase="error"
      />,
    );
    expect(container.textContent).toContain(
      "Reconnecting to the device failed.",
    );
    await click(buttonWithText("Reload the application"));
    expect(onManualReload).toHaveBeenCalledOnce();
    await unmount();
  });
});
