import { act, useRef } from "react";
import { describe, expect, test, vi } from "vitest";
import { useFocusTrap } from "../src/features/shell/v2/useFocusTrap";
import { buttonWithText, click, render, requiredElement } from "./render";

function dispatchKeydown(
  key: string,
  options: { shiftKey?: boolean } = {},
): void {
  document.dispatchEvent(
    new KeyboardEvent("keydown", {
      key,
      shiftKey: options.shiftKey ?? false,
      bubbles: true,
      cancelable: true,
    }),
  );
}

function Harness({
  active,
  onClose,
}: {
  active: boolean;
  onClose: () => void;
}): React.JSX.Element {
  const containerRef = useRef<HTMLDivElement>(null);
  useFocusTrap({ active, containerRef, onClose });
  return (
    <div>
      <button type="button">Outside opener</button>
      <div ref={containerRef} role="dialog" tabIndex={-1}>
        <button type="button">First</button>
        <button type="button">Middle</button>
        <button type="button">Last</button>
      </div>
    </div>
  );
}

/**
 * TODO_V2 V2-133/UI_UX_SPEC_V2 §14 "Dialogs trap focus and restore it to
 * their invoking control" — unit coverage for the shared hook every v2
 * dialog (`MacrosPage`'s delete confirmation, `PackageManagementPage`'s
 * danger zone, `SnapshotsPage`'s danger zones, `SettingsPage`'s confirm
 * dialogs, `UnsavedChangesPrompt`) now uses in place of having no focus
 * management at all.
 */
describe("useFocusTrap — V2-133", () => {
  test("moves initial focus into the container's first focusable element", async () => {
    const opener = document.createElement("button");
    document.body.append(opener);
    opener.focus();
    const { unmount } = await render(
      <Harness active={true} onClose={vi.fn()} />,
    );
    expect(document.activeElement?.textContent).toBe("First");
    await unmount();
    opener.remove();
  });

  test("does nothing while inactive", async () => {
    const { unmount } = await render(
      <Harness active={false} onClose={vi.fn()} />,
    );
    expect(document.activeElement?.textContent).not.toBe("First");
    await unmount();
  });

  test("Tab wraps from the last focusable element back to the first", async () => {
    const { unmount } = await render(
      <Harness active={true} onClose={vi.fn()} />,
    );
    requiredElement('[role="dialog"]', HTMLDivElement)
      .querySelectorAll("button")[2]
      ?.focus();
    await act(async () => {
      dispatchKeydown("Tab");
      await Promise.resolve();
    });
    expect(document.activeElement?.textContent).toBe("First");
    await unmount();
  });

  test("Shift+Tab wraps from the first focusable element back to the last", async () => {
    const { unmount } = await render(
      <Harness active={true} onClose={vi.fn()} />,
    );
    expect(document.activeElement?.textContent).toBe("First");
    await act(async () => {
      dispatchKeydown("Tab", { shiftKey: true });
      await Promise.resolve();
    });
    expect(document.activeElement?.textContent).toBe("Last");
    await unmount();
  });

  test("Escape calls onClose", async () => {
    const onClose = vi.fn();
    const { unmount } = await render(
      <Harness active={true} onClose={onClose} />,
    );
    await act(async () => {
      dispatchKeydown("Escape");
      await Promise.resolve();
    });
    expect(onClose).toHaveBeenCalledOnce();
    await unmount();
  });

  test("restores focus to the invoking control once the trap deactivates", async () => {
    const opener = document.createElement("button");
    document.body.append(opener);
    opener.focus();
    const { rerender, unmount } = await render(
      <Harness active={true} onClose={vi.fn()} />,
    );
    expect(document.activeElement?.textContent).toBe("First");
    await rerender(<Harness active={false} onClose={vi.fn()} />);
    expect(document.activeElement).toBe(opener);
    await unmount();
    opener.remove();
  });

  test("restores focus to the invoking control on unmount", async () => {
    const opener = document.createElement("button");
    document.body.append(opener);
    opener.focus();
    const { unmount } = await render(
      <Harness active={true} onClose={vi.fn()} />,
    );
    expect(document.activeElement?.textContent).toBe("First");
    await unmount();
    expect(document.activeElement).toBe(opener);
    opener.remove();
  });

  test("a later onClose identity is honored without re-running the trap", async () => {
    const firstOnClose = vi.fn();
    const secondOnClose = vi.fn();
    const { rerender, unmount } = await render(
      <Harness active={true} onClose={firstOnClose} />,
    );
    await rerender(<Harness active={true} onClose={secondOnClose} />);
    await act(async () => {
      dispatchKeydown("Escape");
      await Promise.resolve();
    });
    expect(secondOnClose).toHaveBeenCalledOnce();
    expect(firstOnClose).not.toHaveBeenCalled();
    await unmount();
  });
});

// A trivial smoke check that a real click on "First" still works alongside
// the trap (i.e. the trap does not swallow ordinary interaction).
describe("useFocusTrap — V2-133 — does not interfere with ordinary clicks", () => {
  test("a button inside the trapped container is still clickable", async () => {
    const onClick = vi.fn();
    function ClickableHarness(): React.JSX.Element {
      const containerRef = useRef<HTMLDivElement>(null);
      useFocusTrap({ active: true, containerRef, onClose: vi.fn() });
      return (
        <div ref={containerRef} role="dialog" tabIndex={-1}>
          <button onClick={onClick} type="button">
            Confirm
          </button>
        </div>
      );
    }
    const { unmount } = await render(<ClickableHarness />);
    await click(buttonWithText("Confirm"));
    expect(onClick).toHaveBeenCalledOnce();
    await unmount();
  });
});
