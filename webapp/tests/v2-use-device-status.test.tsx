import { act } from "react";
import { describe, expect, test, vi } from "vitest";
import { useDeviceStatus } from "../src/features/shell/v2/useDeviceStatus";
import type { StatusResponse } from "../src/v2/apiTypes";
import { render } from "./render";

async function tick(ms: number): Promise<void> {
  await act(async () => {
    await vi.advanceTimersByTimeAsync(ms);
  });
}

describe("useDeviceStatus", () => {
  test("polls and updates the USB state, keeping the last known value on a transient failure", async () => {
    vi.useFakeTimers();
    try {
      let call = 0;
      const getStatus = vi.fn((): Promise<StatusResponse> => {
        call += 1;
        if (call === 1) {
          return Promise.resolve({
            usb: { state: "disconnected" },
          } as unknown as StatusResponse);
        }
        if (call === 2) {
          return Promise.reject(new Error("device unreachable"));
        }
        return Promise.resolve({
          usb: { state: "ready" },
        } as unknown as StatusResponse);
      });

      function ProbeWithDeps(): React.JSX.Element {
        const usbState = useDeviceStatus(getStatus);
        return <p>usb:{usbState}</p>;
      }

      const { container, unmount } = await render(<ProbeWithDeps />);
      await tick(0);
      expect(container.textContent).toBe("usb:disconnected");

      await tick(5000);
      // Poll #2 failed: state unchanged rather than reset.
      expect(container.textContent).toBe("usb:disconnected");

      await tick(5000);
      expect(container.textContent).toBe("usb:ready");
      expect(getStatus).toHaveBeenCalledTimes(3);
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });

  test("starts as uninitialized before the first poll resolves", async () => {
    vi.useFakeTimers();
    try {
      const getStatus = vi.fn(
        () => new Promise<StatusResponse>(() => undefined),
      );
      function ProbeWithDeps(): React.JSX.Element {
        const usbState = useDeviceStatus(getStatus);
        return <p>usb:{usbState}</p>;
      }
      const { container, unmount } = await render(<ProbeWithDeps />);
      expect(container.textContent).toBe("usb:uninitialized");
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });

  test("stops polling after unmount", async () => {
    vi.useFakeTimers();
    try {
      const getStatus = vi.fn(
        (): Promise<StatusResponse> =>
          Promise.resolve({
            usb: { state: "ready" },
          } as unknown as StatusResponse),
      );
      function ProbeWithDeps(): React.JSX.Element {
        const usbState = useDeviceStatus(getStatus);
        return <p>usb:{usbState}</p>;
      }
      const { unmount } = await render(<ProbeWithDeps />);
      await tick(0);
      await unmount();
      const callsAtUnmount = getStatus.mock.calls.length;
      await tick(30_000);
      expect(getStatus).toHaveBeenCalledTimes(callsAtUnmount);
    } finally {
      vi.useRealTimers();
    }
  });
});
