import type { DeviceReconnectPhase } from "./useDeviceReconnect";
import { ErrorBanner } from "../../../components/ErrorBanner";

export type DeviceActionKind = "restart" | "reset-settings" | "factory-reset";

export interface DeviceReconnectScreenProps {
  kind: DeviceActionKind;
  phase: DeviceReconnectPhase;
  elapsedMs: number;
  errorMessage: string | null;
  onManualReload: () => void;
}

const actionLabel: Record<DeviceActionKind, string> = {
  restart: "restarting",
  "reset-settings": "resetting settings and restarting",
  "factory-reset": "performing a factory reset and restarting",
};

function waitingCopy(kind: DeviceActionKind, elapsedMs: number): string {
  const seconds = Math.floor(elapsedMs / 1000);
  const suffix =
    seconds >= 15
      ? " This is taking longer than usual — the device may still be starting up."
      : "";
  return `The device is ${actionLabel[kind]}. Its Wi-Fi access point briefly drops while it reboots, so this page will keep trying to reconnect automatically.${suffix}`;
}

/**
 * The real "reconnecting" state TODO_V2 V2-121 requires instead of firing a
 * device action and leaving the UI to time out on its own — SPEC_V2 §13.12's
 * restart/reset-settings/factory-reset routes all report
 * `connectionWillClose: true`, and this screen is what the application shows
 * for that window. `AppV2.tsx`'s authenticated shell renders this in place
 * of the normal screen content while `useDeviceReconnect`'s `active` polling
 * is running, so it replaces every route (not just Settings) — a device
 * restart is disruptive to the whole application, not one screen.
 */
export function DeviceReconnectScreen({
  kind,
  phase,
  elapsedMs,
  errorMessage,
  onManualReload,
}: DeviceReconnectScreenProps): React.JSX.Element {
  return (
    <main aria-busy={phase === "waiting"} className="standalone">
      <section aria-labelledby="reconnect-title">
        <h1 id="reconnect-title">Reconnecting…</h1>
        {phase === "waiting" ? (
          <p role="status">{waitingCopy(kind, elapsedMs)}</p>
        ) : null}
        {phase === "needs-reauth" ? (
          <p role="status">
            The device is back. Sign in again to continue — your unsaved work,
            if any, is still here.
          </p>
        ) : null}
        {phase === "error" ? (
          <>
            <ErrorBanner message={errorMessage} />
            <button onClick={onManualReload} type="button">
              Reload the application
            </button>
          </>
        ) : null}
      </section>
    </main>
  );
}
