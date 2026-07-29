import { useEffect, useState } from "react";
import { ErrorBanner } from "../../components/ErrorBanner";
import {
  cancelCurrentExecution,
  getCurrentExecution,
} from "../../api/routes";
import { errorText } from "../../api/errors";
import type { ExecutionStatus } from "../../types/models";

const pollDelayMs = 250;

export function ExecutionPage() {
  const [execution, setExecution] = useState<ExecutionStatus | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [cancelling, setCancelling] = useState(false);

  useEffect(() => {
    let cancelled = false;
    let timer: number | undefined;

    const poll = async () => {
      try {
        const next = await getCurrentExecution();
        if (!cancelled) {
          setExecution(next);
          setError(null);
          if (next.state === "running") {
            timer = window.setTimeout(() => {
              void poll();
            }, pollDelayMs);
          }
        }
      } catch (pollError: unknown) {
        if (!cancelled) {
          setError(errorText(pollError));
        }
      }
    };

    void poll();
    return () => {
      cancelled = true;
      if (timer !== undefined) {
        window.clearTimeout(timer);
      }
    };
  }, []);

  const cancel = async () => {
    setCancelling(true);
    setError(null);
    try {
      await cancelCurrentExecution();
    } catch (cancelError: unknown) {
      setError(errorText(cancelError));
    } finally {
      setCancelling(false);
    }
  };

  return (
    <section>
      <h2>Typing macro</h2>
      <ErrorBanner message={error} />
      <p aria-live="polite">
        {execution === null
          ? "Waiting for device status…"
          : `${String(execution.actionIndex)} / ${String(execution.actionCount)}`}
      </p>
      {execution?.cancellationRequested === true ? (
        <p role="status">Cancellation has been requested.</p>
      ) : null}
      <button
        className="danger"
        disabled={
          cancelling ||
          execution?.state !== "running" ||
          execution?.cancellationRequested === true
        }
        onClick={() => {
          void cancel();
        }}
        type="button"
      >
        {cancelling ? "Requesting cancellation…" : "Cancel and release keys"}
      </button>
    </section>
  );
}
