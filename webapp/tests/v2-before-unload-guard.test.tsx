import { describe, expect, test } from "vitest";
import { useBeforeUnloadGuard } from "../src/features/shell/v2/useBeforeUnloadGuard";
import { render } from "./render";

function Harness({ dirty }: { dirty: boolean }): React.JSX.Element {
  useBeforeUnloadGuard(dirty);
  return <p>harness</p>;
}

function dispatchBeforeUnload(): Event {
  const event = new Event("beforeunload", { cancelable: true });
  window.dispatchEvent(event);
  return event;
}

describe("useBeforeUnloadGuard — V2-103", () => {
  test("registers a beforeunload warning while dirty", async () => {
    const { unmount } = await render(<Harness dirty={true} />);
    const event = dispatchBeforeUnload();
    expect(event.defaultPrevented).toBe(true);
    await unmount();
  });

  test("does not register a warning while clean", async () => {
    const { unmount } = await render(<Harness dirty={false} />);
    const event = dispatchBeforeUnload();
    expect(event.defaultPrevented).toBe(false);
    await unmount();
  });

  test("deregisters the warning the instant the working copy becomes clean", async () => {
    const { rerender, unmount } = await render(<Harness dirty={true} />);
    await rerender(<Harness dirty={false} />);
    const event = dispatchBeforeUnload();
    expect(event.defaultPrevented).toBe(false);
    await unmount();
  });

  test("deregisters the warning on unmount", async () => {
    const { unmount } = await render(<Harness dirty={true} />);
    await unmount();
    const event = dispatchBeforeUnload();
    expect(event.defaultPrevented).toBe(false);
  });
});
