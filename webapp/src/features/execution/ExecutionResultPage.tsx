import type { ExecutionStatus } from "../../types/models";
import { executionResultTitle } from "./executionResult";

interface ExecutionResultPageProps {
  execution: ExecutionStatus | null;
  onReturn: () => void;
}

export function ExecutionResultPage({
  execution,
  onReturn,
}: ExecutionResultPageProps): React.JSX.Element {
  if (execution === null) {
    return (
      <section>
        <h2>No execution result</h2>
        <p>The device has not supplied a terminal execution result.</p>
        <button onClick={onReturn} type="button">
          Return
        </button>
      </section>
    );
  }

  return (
    <article className="card">
      <div>
        <h2>{executionResultTitle(execution)}</h2>
        <p>
          The next procedure step is ready but will not execute automatically.
        </p>
        {execution.error.length > 0 ? (
          <p>Execution error: {execution.error}</p>
        ) : null}
        {execution.releaseError.length > 0 ? (
          <p>Key-release error: {execution.releaseError}</p>
        ) : null}
      </div>
      <button onClick={onReturn} type="button">
        Return
      </button>
    </article>
  );
}
