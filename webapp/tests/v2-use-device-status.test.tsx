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
  test("retains the last value, degrades after three consecutive failures, and recovers", async () => {
    vi.useFakeTimers();
    try {
      let call = 0;
      const getStatus = vi.fn((): Promise<StatusResponse> => {
        call += 1;
        if (call === 1) {
          return Promise.resolve({
            usb: { state: "ready" },
          } as unknown as StatusResponse);
        }
        if (call <= 4) {
          return Promise.reject(new Error("device unreachable"));
        }
        return Promise.resolve({
          usb: { state: "disconnected" },
        } as unknown as StatusResponse);
      });

      function ProbeWithDeps(): React.JSX.Element {
        const status = useDeviceStatus(getStatus);
        return (
          <p>
            usb:{status.usbState};degraded:{String(status.degraded)}; failures:
            {String(status.consecutiveFailures)}
          </p>
        );
      }

      const { container, unmount } = await render(<ProbeWithDeps />);
      await tick(0);
      expect(container.textContent).toContain(
        "usb:ready;degraded:false; failures:0",
      );

      await tick(5000);
      expect(container.textContent).toContain(
        "usb:ready;degraded:false; failures:1",
      );

      await tick(5000);
      expect(container.textContent).toContain(
        "usb:ready;degraded:false; failures:2",
      );

      await tick(5000);
      expect(container.textContent).toContain(
        "usb:ready;degraded:true; failures:3",
      );

      await tick(5000);
      expect(container.textContent).toContain(
        "usb:disconnected;degraded:false; failures:0",
      );
      expect(getStatus).toHaveBeenCalledTimes(5);
      await unmount();
    } finally {
      vi.useRealTimers();
    }
  });

  test("starts uninitialized and non-degraded before the first poll resolves", async () => {
    vi.useFakeTimers();
    try {
      const getStatus = vi.fn(
        () => new Promise<StatusResponse>(() => undefined),
      );
      function ProbeWithDeps(): React.JSX.Element {
        const status = useDeviceStatus(getStatus);
        return (
          <p>
            usb:{status.usbState};degraded:{String(status.degraded)}
          </p>
        );
      }
      const { container, unmount } = await render(<ProbeWithDeps />);
      expect(container.textContent).toContain(
        "usb:uninitialized;degraded:false",
      );
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
        const status = useDeviceStatus(getStatus);
        return <p>usb:{status.usbState}</p>;
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
