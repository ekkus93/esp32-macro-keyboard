import type { ExecutionAccepted } from "../types/models";
import { isRecord, isUuid } from "./guards";

function isNonNegativeInteger(value: unknown): value is number {
  return typeof value === "number" && Number.isInteger(value) && value >= 0;
}

export function isExecutionAccepted(
  value: unknown,
): value is ExecutionAccepted {
  if (!isRecord(value)) {
    return false;
  }
  const keys = Object.keys(value);
  return (
    keys.length === 3 &&
    keys.includes("executionId") &&
    keys.includes("actionCount") &&
    keys.includes("estimatedDurationMs") &&
    isUuid(value.executionId) &&
    isNonNegativeInteger(value.actionCount) &&
    isNonNegativeInteger(value.estimatedDurationMs)
  );
}
