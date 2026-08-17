import { useState, useSyncExternalStore } from "react";
import { ErrorBanner } from "../../../components/ErrorBanner";
import { v2ErrorText } from "../../auth/v2/v2ErrorText";
import {
  cancelSend,
  getExecutionRecoveryState,
  retryExecutionRecovery,
  subscribeExecutionRecovery,
} from "../../../v2/sendClient";

/**
 * Cross-route H4 execution recovery surface. It stays mounted beside AppV2 so
 * a failed startup recovery, an exhausted active-send tracker, or a 409 whose
 * follow-up GET fails cannot disappear merely because the owning page changes.
 */
export function ExecutionRecoveryOverlay(): React.JSX.Element | null {
  const recovery = useSyncExternalStore(
    subscribeExecutionRecovery,
    getExecutionRecoveryState,
    getExecutionRecoveryState,
  );
  const [retrying, setRetrying] = useState(false);
  const [canceling, setCanceling] = useState(false);
  const [message, setMessage] = useState<string | null>(null);

  if (recovery.kind === "clear") {
    return null;
  }

  const retry = async (): Promise<void> => {
    setRetrying(true);
    setMessage(null);
    try {
      await retryExecutionRecovery();
    } catch (error: unknown) {
      setMessage(
        `Execution status is still unavailable: ${v2ErrorText(error)}`,
      );
    } finally {
      setRetrying(false);
    }
  };

  const cancel = async (): Promise<void> => {
    setCanceling(true);
    setMessage(null);
    try {
      await cancelSend();
      setMessage(
        "Cancellation was accepted. Retry execution status to confirm the terminal state.",
      );
    } catch (error: unknown) {
      setMessage(`Cancel could not be delivered: ${v2ErrorText(error)}`);
    } finally {
      setCanceling(false);
    }
  };

  return (
    // Sits above everything including dialogs (z-[1000]). Bottom-anchored
    // like .bottom-nav, so it takes the same safe-area inset rather than
    // sitting under a phone's home indicator -- bottom-[calc(...)], not
    // bottom-4, because env(safe-area-inset-bottom) is 0 on most devices and
    // nonzero on the ones this matters for.
    <aside
      aria-label="Execution recovery"
      className="fixed bottom-[calc(1rem+env(safe-area-inset-bottom))] left-4 right-4 z-[1000] max-w-[36rem] rounded-keycap border-2 border-legend bg-cap p-4 shadow-[0_0.5rem_1.5rem_rgb(33_30_26_/_18%)]"
    >
      <ErrorBanner
        message={`Execution state unavailable. ${recovery.message} An active send may still be running on the device.`}
      />
      {recovery.lastKnown !== null ? (
        <p>
          Last known state: {recovery.lastKnown.state}, action{" "}
          {String(recovery.lastKnown.actionIndex)} of{" "}
          {String(recovery.lastKnown.actionCount)}.
        </p>
      ) : null}
      {message !== null ? <p role="status">{message}</p> : null}
      <div className="card-actions">
        <button
          disabled={retrying || canceling}
          onClick={() => {
            void retry();
          }}
          type="button"
        >
          {retrying ? "Retrying execution status…" : "Retry execution status"}
        </button>
        <button
          disabled={retrying || canceling}
          onClick={() => {
            void cancel();
          }}
          type="button"
        >
          {canceling
            ? "Requesting cancellation…"
            : "Cancel and release all keys"}
        </button>
      </div>
    </aside>
  );
}
