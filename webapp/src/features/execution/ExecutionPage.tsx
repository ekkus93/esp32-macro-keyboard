import { useEffect, useState } from "react";
import { errorText } from "../../api/errors";
import { cancelCurrentExecution, getCurrentExecution } from "../../api/routes";
import { ErrorBanner } from "../../components/ErrorBanner";
import type { ExecutionStatus } from "../../types/models";
import { isTerminalExecution } from "./executionResult";

interface ExecutionPageProps {
  expectedExecutionId: string | null;
  onTerminal: (execution: ExecutionStatus) => void;
}

export function ExecutionPage({
  expectedExecutionId,
  onTerminal,
}: ExecutionPageProps): React.JSX.Element {
  const [execution, setExecution] = useState<ExecutionStatus | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [cancelling, setCancelling] = useState(false);

  useEffect(() => {
    let active = true;
    const refresh = async (): Promise<void> => {
      try {
        const current = await getCurrentExecution();
        if (!active) {
          return;
        }
        if (
          expectedExecutionId !== null &&
          current.executionId !== expectedExecutionId
        ) {
          setExecution(null);
          setError(
            "The device reported a different current execution. Waiting for the accepted execution identity.",
          );
          return;
        }
        setExecution(current);
        setError(null);
        if (isTerminalExecution(current)) {
          onTerminal(current);
        }
      } catch (pollError: unknown) {
        if (active) {
          setError(errorText(pollError));
        }
      }
    };

    void refresh();
    const timer = window.setInterval(() => {
      void refresh();
    }, 500);
    return () => {
      active = false;
      window.clearInterval(timer);
    };
  }, [expectedExecutionId, onTerminal]);

  const cancel = async (): Promise<void> => {
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
      {execution !== null && execution.currentAction !== "none" ? (
        <p>Current action: {execution.currentAction}</p>
      ) : null}
      {execution?.cancellationRequested === true ? (
        <p role="status">Cancellation has been requested.</p>
      ) : null}
      <button
        className="danger"
        disabled={
          cancelling ||
          execution?.state !== "running" ||
          execution.cancellationRequested
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
