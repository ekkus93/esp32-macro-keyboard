import { act } from "react";
import { describe, expect, test, vi } from "vitest";
import { V2ApiError } from "../src/v2/apiClient";
import { useDeviceReconnect } from "../src/features/settings/v2/useDeviceReconnect";
import { render } from "./render";

async function tick(ms = 0): Promise<void> {
  await act(async () => {
    await vi.advanceTimersByTimeAsync(ms);
  });
}

function Probe({
  active,
  checkReachable,
}: {
  active: boolean;
  checkReachable: () => Promise<unknown>;
}): React.JSX.Element {
  const result = useDeviceReconnect(active, checkReachable);
  return (
    <p>
      phase:{result.phase} elapsed:{String(result.elapsedMs)} error:
      {result.errorMessage ?? "none"}
    </p>
  );
}

describe("useDeviceReconnect (TODO_V2 V2-121)", () => {
  test("stays 'waiting' while every attempt fails at the network layer, then resolves 'reachable'", async () => {
    vi.useFakeTimers();
    try {
      let call = 0;
      const checkReachable = vi.fn((): Promise<unknown> => {
        call += 1;
        if (call < 3) {
          return Promise.reject(new TypeError("Failed to fetch"));
        }
        return Promise.resolve({ ok: true });
      });
      const { container, unmount } = await render(
        <Probe active={true} checkReachable={checkReachable} />,
      );
      await tick(0);
      expect(container.textContent).toContain("phase:waiting");

      await tick(1000);
      expect(container.textContent).toContain("phase:waiting");

      await tick(1000);
      expect(container.textContent).toContain("phase:reachable");
      expect(checkReachable).toHaveBeenCalledTimes(3);
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });

  test("treats a transient 503 the same as a network failure, not a final error", async () => {
    vi.useFakeTimers();
    try {
      let call = 0;
      const checkReachable = vi.fn((): Promise<unknown> => {
        call += 1;
        if (call === 1) {
          return Promise.reject(
            new V2ApiError(503, {
              code: "unavailable",
              message: "Storage not ready yet.",
            }),
          );
        }
        return Promise.resolve({ ok: true });
      });
      const { container, unmount } = await render(
        <Probe active={true} checkReachable={checkReachable} />,
      );
      await tick(0);
      expect(container.textContent).toContain("phase:waiting");
      await tick(1000);
      expect(container.textContent).toContain("phase:reachable");
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });

  test("resolves 'needs-reauth' on a 401 without further polling", async () => {
    vi.useFakeTimers();
    try {
      const checkReachable = vi.fn(() =>
        Promise.reject(
          new V2ApiError(401, {
            code: "unauthorized",
            message: "Session expired.",
          }),
        ),
      );
      const { container, unmount } = await render(
        <Probe active={true} checkReachable={checkReachable} />,
      );
      await tick(0);
      expect(container.textContent).toContain("phase:needs-reauth");
      const callsAtNeedsReauth = checkReachable.mock.calls.length;
      await tick(10_000);
      expect(checkReachable).toHaveBeenCalledTimes(callsAtNeedsReauth);
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });

  test("a genuine programming error stops polling and surfaces as 'error'", async () => {
    vi.useFakeTimers();
    try {
      const checkReachable = vi.fn(() =>
        Promise.reject(new RangeError("boom")),
      );
      const { container, unmount } = await render(
        <Probe active={true} checkReachable={checkReachable} />,
      );
      await tick(0);
      expect(container.textContent).toContain("phase:error");
      expect(container.textContent).toContain("error:boom");
      const callsAtError = checkReachable.mock.calls.length;
      await tick(10_000);
      expect(checkReachable).toHaveBeenCalledTimes(callsAtError);
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });

  test("does nothing while inactive, and stops polling once deactivated mid-wait", async () => {
    vi.useFakeTimers();
    try {
      const checkReachable = vi.fn(
        (): Promise<unknown> => new Promise(() => undefined),
      );
      let active = true;
      function Toggle(): React.JSX.Element {
        const result = useDeviceReconnect(active, checkReachable);
        return <p>phase:{result.phase}</p>;
      }
      const { container, rerender, unmount } = await render(<Toggle />);
      await tick(0);
      expect(container.textContent).toBe("phase:waiting");
      expect(checkReachable).toHaveBeenCalledTimes(1);

      active = false;
      await rerender(<Toggle />);
      expect(container.textContent).toBe("phase:waiting");
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });
});
