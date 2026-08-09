import { useEffect, useRef, useState } from "react";
import { V2ApiError } from "../../../v2/apiClient";

/**
 * Connection-loss-and-recovery handling for restart and reset settings
 * (TODO_V2 V2-121: "handle connection loss and recovery explicitly ... the
 * device will actually restart/lose the AP connection after these actions —
 * the UI needs a real 'reconnecting' state, not just fire-and-forget").
 *
 * `checkReachable` is any authenticated request (the caller passes
 * `getStatus`). While the device is rebooting, every attempt fails at the
 * network layer — a real `fetch` rejection (`TypeError`) or this module's own
 * request timeout (`AbortError`, surfaced as a `DOMException`) — so those are
 * "still down, keep polling". A transient `5xx`/`503` while firmware finishes
 * bringing subsystems up is treated the same way rather than as a final
 * error, since the HTTP server itself is often reachable before storage/Wi-Fi
 * report ready. Two outcomes end polling:
 *
 * - a `401` (`V2ApiError`) means the device answered but the RAM-only
 *   session firmware held is gone (exactly what a reboot does) — the caller
 *   is expected to already be subscribed to the existing `notifyUnauthorized`
 *   mechanism (`subscribeUnauthorized` in `apiClient.ts`), which will present
 *   Sign In without discarding the live working copy, per SPEC_V2 §7.3's
 *   "session expiry does not discard the in-memory working copy";
 * - any other successful response means the device is back and the current
 *   session, against expectation, is still valid — resume immediately.
 *
 * Any other exception (a real programming error, not a reachability signal)
 * stops polling and surfaces as `"error"` rather than retrying forever.
 */

export type DeviceReconnectPhase =
  | "waiting"
  | "reachable"
  | "needs-reauth"
  | "error";

export interface UseDeviceReconnectResult {
  phase: DeviceReconnectPhase;
  elapsedMs: number;
  errorMessage: string | null;
}

const pollIntervalMs = 1_000;

function isTransientFailure(error: unknown): boolean {
  if (error instanceof DOMException && error.name === "AbortError") {
    return true;
  }
  if (error instanceof TypeError) {
    return true;
  }
  return error instanceof V2ApiError && error.status >= 500;
}

export function useDeviceReconnect(
  active: boolean,
  checkReachable: () => Promise<unknown>,
): UseDeviceReconnectResult {
  const [phase, setPhase] = useState<DeviceReconnectPhase>("waiting");
  const [elapsedMs, setElapsedMs] = useState(0);
  const [errorMessage, setErrorMessage] = useState<string | null>(null);
  const checkRef = useRef(checkReachable);
  checkRef.current = checkReachable;

  useEffect(() => {
    if (!active) {
      setPhase("waiting");
      setElapsedMs(0);
      setErrorMessage(null);
      return;
    }
    let cancelled = false;
    let timer: number | null = null;
    const startedAt = Date.now();

    const poll = (): void => {
      checkRef
        .current()
        .then(() => {
          if (!cancelled) {
            setPhase("reachable");
          }
        })
        .catch((error: unknown) => {
          if (cancelled) {
            return;
          }
          if (error instanceof V2ApiError && error.status === 401) {
            setPhase("needs-reauth");
            return;
          }
          if (isTransientFailure(error)) {
            setElapsedMs(Date.now() - startedAt);
            timer = window.setTimeout(poll, pollIntervalMs);
            return;
          }
          setPhase("error");
          setErrorMessage(
            error instanceof Error
              ? error.message
              : "Reconnecting to the device failed.",
          );
        });
    };

    poll();
    return () => {
      cancelled = true;
      if (timer !== null) {
        window.clearTimeout(timer);
      }
    };
  }, [active]);

  return { phase, elapsedMs, errorMessage };
}
