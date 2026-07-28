import type { ExecutionStatus } from "../../types/models";

export function executionResultTitle(execution: ExecutionStatus): string {
  if (execution.releaseError.length > 0) {
    return "Macro ended with a key-release error";
  }

  switch (execution.state) {
    case "completed":
      return "Macro completed";
    case "cancelled":
      return "Macro cancelled";
    case "failed":
      return "Macro failed";
    case "timed_out":
      return "Macro timed out";
    case "idle":
    case "running":
      throw new Error("Execution result requested for a non-terminal state.");
  }
}

export function isTerminalExecution(execution: ExecutionStatus): boolean {
  return ["completed", "cancelled", "failed", "timed_out"].includes(
    execution.state,
  );
}
