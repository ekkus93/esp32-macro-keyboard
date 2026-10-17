import { useRef, useState, useSyncExternalStore } from "react";
import { Dialog } from "../../../components/Dialog";
import { ErrorBanner } from "../../../components/ErrorBanner";
import { v2ErrorText } from "../../auth/v2/v2ErrorText";
import { useFocusTrap } from "./useFocusTrap";
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
  const containerRef = useRef<HTMLDivElement>(null);
  useFocusTrap({
    active: recovery.kind === "unavailable",
    containerRef,
    onClose: () => {
      // Execution recovery cannot be dismissed until status is reconciled.
    },
  });

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
    <Dialog
      aboveOverlays
      aria-labelledby="execution-recovery-title"
      containerRef={containerRef}
      heading={<h2 id="execution-recovery-title">Execution recovery</h2>}
      role="alertdialog"
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
      <div className="flex flex-wrap content-start gap-[0.4rem] [&_button]:flex-initial [@media(width<=32rem)]:w-full">
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
    </Dialog>
  );
}
