import { describe, expect, test, vi } from "vitest";
import { Snackbar } from "../src/components/m3/Snackbar";
import { click, render, requiredElement } from "./render";

describe("Snackbar", () => {
  test("announces a confirmation politely and can be dismissed", async () => {
    const onDismiss = vi.fn();
    const { container, unmount } = await render(
      <Snackbar message="Diagnostics copied." onDismiss={onDismiss} />,
    );
    const snackbar = container.querySelector('[role="status"]');
    expect(snackbar?.getAttribute("aria-live")).toBe("polite");
    expect(snackbar?.textContent).toContain("Diagnostics copied.");
    await click(
      requiredElement('[aria-label="Dismiss notification"]', HTMLButtonElement),
    );
    expect(onDismiss).toHaveBeenCalledOnce();
    await unmount();
  });
});
