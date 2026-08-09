import { useEffect, useState } from "react";
import { getStatus as defaultGetStatus } from "../../../v2/statusClient";
import type { UsbState } from "../../../v2/apiTypes";

/**
 * Polls `GET /api/v1/status` for the live USB state the application shell
 * (TODO_V2 V2-090) and Macros page (TODO_V2 V2-091) need — "USB readiness is
 * visually prominent" (UI_UX_SPEC_V2 §2.1) and Quick Send is gated on
 * `usb.state === "ready"`. A transient poll failure keeps the last known
 * state rather than flashing an incorrect badge.
 */
const pollIntervalMs = 5_000;

export function useDeviceStatus(
  getStatus: typeof defaultGetStatus = defaultGetStatus,
): UsbState {
  const [usbState, setUsbState] = useState<UsbState>("uninitialized");

  useEffect(() => {
    let active = true;
    const poll = async (): Promise<void> => {
      try {
        const status = await getStatus();
        if (active) {
          setUsbState(status.usb.state);
        }
      } catch {
        // Keep the last known state; the next poll will retry.
      }
    };
    void poll();
    const timer = window.setInterval(() => {
      void poll();
    }, pollIntervalMs);
    return () => {
      active = false;
      window.clearInterval(timer);
    };
  }, [getStatus]);

  return usbState;
}
