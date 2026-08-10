import { act, useRef } from "react";
import { describe, expect, test, vi } from "vitest";
import { useDismissibleOverlay } from "../src/features/shell/v2/useDismissibleOverlay";
import { render } from "./render";

function dispatchKeydown(key: string): void {
  document.dispatchEvent(
    new KeyboardEvent("keydown", { key, bubbles: true, cancelable: true }),
  );
}

/**
 * jsdom does not implement the `PointerEvent` constructor, but the hook
 * under test only reads `event.target` — a plain `Event` dispatched as a
 * "pointerdown" is indistinguishable to its listener.
 */
function dispatchPointerdown(target: EventTarget): void {
  target.dispatchEvent(
    new Event("pointerdown", { bubbles: true, cancelable: true }),
  );
}

function Harness({
  active,
  onDismiss,
}: {
  active: boolean;
  onDismiss: () => void;
}): React.JSX.Element {
  const containerRef = useRef<HTMLDivElement>(null);
  useDismissibleOverlay(active, containerRef, onDismiss);
  return (
    <div>
      <button type="button">Outside</button>
      <div data-testid="overlay" ref={containerRef}>
        <button type="button">Inside</button>
      </div>
    </div>
  );
}

/**
 * TODO_V2 V2-133/UI_UX_SPEC_V2 §14: `MacrosPage.tsx`'s "More actions"
 * overflow menu had neither an `Escape`-to-close nor an outside-click
 * dismissal before this hook existed (the audit note this task started
 * from). Covered here in isolation from the menu's own markup.
 */
describe("useDismissibleOverlay — V2-133", () => {
  test("Escape calls onDismiss while active", async () => {
    const onDismiss = vi.fn();
    const { unmount } = await render(
      <Harness active={true} onDismiss={onDismiss} />,
    );
    await act(async () => {
      dispatchKeydown("Escape");
      await Promise.resolve();
    });
    expect(onDismiss).toHaveBeenCalledOnce();
    await unmount();
  });

  test("a pointer press outside the container calls onDismiss", async () => {
    const onDismiss = vi.fn();
    const { container, unmount } = await render(
      <Harness active={true} onDismiss={onDismiss} />,
    );
    const outside = Array.from(container.querySelectorAll("button")).find(
      (button) => button.textContent === "Outside",
    );
    expect(outside).not.toBeUndefined();
    await act(async () => {
      dispatchPointerdown(outside as EventTarget);
      await Promise.resolve();
    });
    expect(onDismiss).toHaveBeenCalledOnce();
    await unmount();
  });

  test("a pointer press inside the container does not call onDismiss", async () => {
    const onDismiss = vi.fn();
    const { unmount } = await render(
      <Harness active={true} onDismiss={onDismiss} />,
    );
    const inside = document.querySelector('[data-testid="overlay"] button');
    expect(inside).not.toBeNull();
    await act(async () => {
      dispatchPointerdown(inside as EventTarget);
      await Promise.resolve();
    });
    expect(onDismiss).not.toHaveBeenCalled();
    await unmount();
  });

  test("does nothing while inactive", async () => {
    const onDismiss = vi.fn();
    const { unmount } = await render(
      <Harness active={false} onDismiss={onDismiss} />,
    );
    await act(async () => {
      dispatchKeydown("Escape");
      await Promise.resolve();
    });
    expect(onDismiss).not.toHaveBeenCalled();
    await unmount();
  });

  test("a later onDismiss identity is honored", async () => {
    const first = vi.fn();
    const second = vi.fn();
    const { rerender, unmount } = await render(
      <Harness active={true} onDismiss={first} />,
    );
    await rerender(<Harness active={true} onDismiss={second} />);
    await act(async () => {
      dispatchKeydown("Escape");
      await Promise.resolve();
    });
    expect(second).toHaveBeenCalledOnce();
    expect(first).not.toHaveBeenCalled();
    await unmount();
  });
});
