import type { SendState, SendStatusResponse } from "../../../v2/apiTypes";

export interface MacroIdentity {
  id: string;
  name: string;
}

export type SendLifecycle =
  | { kind: "idle" }
  | { kind: "starting"; macro: MacroIdentity }
  | { kind: "active"; macro: MacroIdentity | null; status: SendStatusResponse }
  | { kind: "completed"; macro: MacroIdentity | null }
  | {
      kind: "terminal-issue";
      reason: "cancelled" | "failed" | "timed_out";
      macro: MacroIdentity | null;
      status: SendStatusResponse;
    };

/** UI_UX_SPEC_V2 §5.5: "an acknowledgement for approximately three to five seconds." */
export const completionAckMs = 4000;

/**
 * Narrows a terminal, non-"completed" {@link SendState} to the reason a
 * {@link SendLifecycle} "terminal-issue" banner names — `isTerminalSendState`
 * is a boolean check, not a type predicate, so callers that already know
 * they hold a terminal status still need this to get a properly narrowed
 * type rather than asserting one.
 */
export function terminalIssueReason(
  state: SendState,
): "cancelled" | "failed" | "timed_out" | null {
  switch (state) {
    case "cancelled":
    case "failed":
    case "timed_out":
      return state;
    case "completed":
    case "running":
    case "awaiting_confirmation":
      return null;
  }
}

export function activeStatusText(
  status: SendStatusResponse,
  macro: MacroIdentity | null,
): string {
  const label = macro !== null ? ` ${macro.name}` : "";
  if (status.state === "awaiting_confirmation") {
    return `Waiting for confirmation on the device to send${label}… Run confirm in the device serial console to continue.`;
  }
  return `Sending${label}… action ${String(status.actionIndex)} of ${String(status.actionCount)}`;
}

export function terminalIssueText(
  reason: "cancelled" | "failed" | "timed_out",
  macro: MacroIdentity | null,
  status: SendStatusResponse,
): string {
  const label = macro !== null ? ` ${macro.name}` : "";
  switch (reason) {
    case "cancelled":
      return `Send${label} was cancelled.`;
    case "timed_out":
      return `Send${label} timed out.`;
    case "failed":
      return status.error.length > 0
        ? `Send${label} failed: ${status.error}`
        : `Send${label} failed.`;
  }
}

/** TODO_V2 V2-133/UI_UX_SPEC_V2 §14 reordering alternative to drag and drop. */
export type MoveAction = "first" | "up" | "down" | "last";
