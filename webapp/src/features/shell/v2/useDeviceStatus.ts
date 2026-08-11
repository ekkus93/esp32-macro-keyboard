import { useEffect, useState } from "react";
import { getStatus as defaultGetStatus } from "../../../v2/statusClient";
import type { UsbState } from "../../../v2/apiTypes";

/**
 * Polls `GET /api/v1/status` for the live USB state the application shell
 * (TODO_V2 V2-090) and Macros page (TODO_V2 V2-091) need. A small bounded
 * number of transient failures retains the last known state to avoid UI
 * flicker. After the threshold, the last value remains available for
 * display but is explicitly degraded and must not be trusted for sending.
 */
const pollIntervalMs = 5_000;
const degradedAfterConsecutiveFailures = 3;

export interface DeviceStatusPollState {
  readonly usbState: UsbState;
  readonly degraded: boolean;
  readonly consecutiveFailures: number;
}

export function useDeviceStatus(
  getStatus: typeof defaultGetStatus = defaultGetStatus,
): DeviceStatusPollState {
  const [status, setStatus] = useState<DeviceStatusPollState>({
    usbState: "uninitialized",
    degraded: false,
    consecutiveFailures: 0,
  });

  useEffect(() => {
    let active = true;
    let timer: number | null = null;

    function schedulePoll(): void {
      timer = window.setTimeout(() => {
        void poll();
      }, pollIntervalMs);
    }

    async function poll(): Promise<void> {
      try {
        const next = await getStatus();
        if (!active) {
          return;
        }
        setStatus({
          usbState: next.usb.state,
          degraded: false,
          consecutiveFailures: 0,
        });
      } catch {
        if (!active) {
          return;
        }
        setStatus((current) => {
          const consecutiveFailures = current.consecutiveFailures + 1;
          return {
            usbState: current.usbState,
            degraded: consecutiveFailures >= degradedAfterConsecutiveFailures,
            consecutiveFailures,
          };
        });
      }

      schedulePoll();
    }

    void poll();
    return () => {
      active = false;
      if (timer !== null) {
        window.clearTimeout(timer);
      }
    };
  }, [getStatus]);

  return status;
}
