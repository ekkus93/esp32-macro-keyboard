import {
  V2ApiError,
  v2DeleteWithUnvalidatedJson,
  v2GetJson,
  v2PostJson,
} from "./apiClient";
import { isSendAcceptedResponse, isSendStatusResponse } from "./apiContracts";
import type {
  SendAcceptedResponse,
  SendRequest,
  SendState,
  SendStatusResponse,
} from "./apiTypes";

/**
 * The React send helper from SPEC_V2 §13.10-13.11 and TODO_V2 V2-075:
 * `sendMacro(request, { onStatus, onComplete })`.
 *
 * This module is framework-agnostic. What it guarantees on its own:
 *  - exactly one `POST /api/v1/send` per accepted send attempt;
 *  - no new POST while a previous execution is known to have unresolved status;
 *  - polling at a bounded interval no slower than once per second;
 *  - transient poll failures are retried only up to a fixed consecutive bound;
 *  - `onStatus` fires only for a meaningful state or progress change;
 *  - `onComplete` fires at most once, only after a terminal state;
 *  - degraded tracking is retained for explicit GET-only reconciliation.
 */

const pollIntervalMs = 1000;
const maxConsecutiveTransientPollFailures = 3;
const sendPath = "/api/v1/send";

const terminalStates: ReadonlySet<SendState> = new Set([
  "completed",
  "cancelled",
  "failed",
  "timed_out",
]);

export function isTerminalSendState(state: SendState): boolean {
  return terminalStates.has(state);
}

export interface SendMacroCallbacks {
  onStatus?: (status: SendStatusResponse) => void;
  onComplete?: (status: SendStatusResponse) => void;
  /** Called once when polling stops because status can no longer be trusted. */
  onError?: (error: unknown) => void;
}

export interface SendMacroHandle {
  /** The `202 Accepted` response from the initiating `POST /api/v1/send`. */
  readonly accepted: SendAcceptedResponse;
  /** Requests cancellation of this send via `DELETE /api/v1/send`. Idempotent. */
  cancel: () => Promise<void>;
  /** Stops client-side polling only. Does not cancel the send on the device. */
  stop: () => void;
}

export type ExecutionRecoveryState =
  | { kind: "clear" }
  | {
      kind: "unavailable";
      message: string;
      lastKnown: SendStatusResponse | null;
    };

type ExecutionRecoveryListener = () => void;

let recoveryState: ExecutionRecoveryState = { kind: "clear" };
let recoveryCallbacks: SendMacroCallbacks | null = null;
const recoveryListeners = new Set<ExecutionRecoveryListener>();

function publishRecoveryState(next: ExecutionRecoveryState): void {
  recoveryState = next;
  for (const listener of recoveryListeners) {
    listener();
  }
}

export function getExecutionRecoveryState(): ExecutionRecoveryState {
  return recoveryState;
}

export function subscribeExecutionRecovery(
  listener: ExecutionRecoveryListener,
): () => void {
  recoveryListeners.add(listener);
  return () => {
    recoveryListeners.delete(listener);
  };
}

/** Test-only reset for module state shared across test cases. */
export function resetExecutionRecoveryForTest(): void {
  recoveryCallbacks = null;
  publishRecoveryState({ kind: "clear" });
}

function markRecoveryUnavailable(
  error: unknown,
  lastKnown: SendStatusResponse | null,
  callbacks: SendMacroCallbacks | null,
): void {
  if (callbacks !== null) {
    recoveryCallbacks = callbacks;
  }
  const message =
    error instanceof V2ApiError
      ? `${error.code}: ${error.message}`
      : error instanceof Error
        ? error.message
        : "Execution status could not be refreshed.";
  publishRecoveryState({ kind: "unavailable", message, lastKnown });
}

function clearRecoveryState(): void {
  recoveryCallbacks = null;
  publishRecoveryState({ kind: "clear" });
}

function isTransientPollFailure(error: unknown): boolean {
  if (error instanceof DOMException && error.name === "AbortError") {
    return true;
  }
  if (error instanceof TypeError) {
    return true;
  }
  return error instanceof V2ApiError && error.status >= 500;
}

function isMeaningfulChange(
  previous: SendStatusResponse | null,
  next: SendStatusResponse,
): boolean {
  if (previous === null) {
    return true;
  }
  return (
    previous.state !== next.state || previous.actionIndex !== next.actionIndex
  );
}

interface SendTracker {
  stop: () => void;
}

function createSendTracker(
  id: string,
  seed: SendStatusResponse | null,
  callbacks: SendMacroCallbacks,
): SendTracker {
  const flags: { stopped: boolean; completed: boolean } = {
    stopped: false,
    completed: seed !== null && isTerminalSendState(seed.state),
  };
  function isStopped(): boolean {
    return flags.stopped;
  }
  function isCompleted(): boolean {
    return flags.completed;
  }
  let previous: SendStatusResponse | null = seed;
  let consecutiveTransientFailures = 0;
  let timer: ReturnType<typeof setTimeout> | null = null;

  function schedulePoll(): void {
    if (isStopped() || isCompleted()) {
      return;
    }
    timer = setTimeout(() => {
      void poll();
    }, pollIntervalMs);
  }

  async function poll(): Promise<void> {
    if (isStopped()) {
      return;
    }
    let status: SendStatusResponse;
    try {
      status = await v2GetJson(sendPath, isSendStatusResponse);
    } catch (error: unknown) {
      if (isStopped()) {
        return;
      }
      if (isTransientPollFailure(error)) {
        consecutiveTransientFailures += 1;
        if (
          consecutiveTransientFailures < maxConsecutiveTransientPollFailures
        ) {
          schedulePoll();
          return;
        }
      }
      flags.stopped = true;
      markRecoveryUnavailable(error, previous, callbacks);
      callbacks.onError?.(error);
      return;
    }
    consecutiveTransientFailures = 0;
    if (isStopped() || status.id !== id) {
      return;
    }
    if (isMeaningfulChange(previous, status)) {
      previous = status;
      callbacks.onStatus?.(status);
    } else {
      previous = status;
    }
    if (isTerminalSendState(status.state)) {
      if (!isCompleted()) {
        flags.completed = true;
        clearRecoveryState();
        callbacks.onComplete?.(status);
      }
      return;
    }
    schedulePoll();
  }

  function stop(): void {
    flags.stopped = true;
    if (timer !== null) {
      clearTimeout(timer);
      timer = null;
    }
  }

  schedulePoll();

  return { stop };
}

/**
 * Sends a macro and tracks it to completion. A local fail-closed recovery
 * latch prevents a second POST while the client cannot prove whether a prior
 * execution is still active.
 */
export async function sendMacro(
  request: SendRequest,
  callbacks: SendMacroCallbacks = {},
): Promise<SendMacroHandle> {
  if (recoveryState.kind === "unavailable") {
    throw new Error(
      "Execution state is unavailable. Retry execution status before sending another macro.",
    );
  }

  let accepted: SendAcceptedResponse;
  try {
    accepted = await v2PostJson(sendPath, request, isSendAcceptedResponse);
  } catch (error: unknown) {
    if (error instanceof V2ApiError && error.status === 409) {
      markRecoveryUnavailable(error, null, callbacks);
    }
    throw error;
  }

  const tracker = createSendTracker(accepted.id, null, callbacks);

  async function cancel(): Promise<void> {
    await cancelSend();
  }

  return { accepted, cancel, stop: tracker.stop };
}

/** Resumes polling an already-known send without issuing a POST. */
export function trackSend(
  seed: SendStatusResponse,
  callbacks: SendMacroCallbacks = {},
): SendTracker {
  clearRecoveryState();
  return createSendTracker(seed.id, seed, callbacks);
}

/** `DELETE /api/v1/send`: requests cancellation of the current send. */
export async function cancelSend(): Promise<unknown> {
  return v2DeleteWithUnvalidatedJson(sendPath);
}

async function fetchRecoveredSend(): Promise<SendStatusResponse | null> {
  try {
    return await v2GetJson(sendPath, isSendStatusResponse);
  } catch (error: unknown) {
    if (error instanceof V2ApiError && error.status === 404) {
      return null;
    }
    throw error;
  }
}

/**
 * Recovers send state after a reload, per SPEC_V2 §13.11. A failed recovery
 * remains explicitly unavailable rather than collapsing to "no send".
 */
export async function recoverSendState(): Promise<SendStatusResponse | null> {
  try {
    const status = await fetchRecoveredSend();
    if (status === null || isTerminalSendState(status.state)) {
      clearRecoveryState();
    } else if (recoveryState.kind === "unavailable") {
      clearRecoveryState();
    }
    return status;
  } catch (error: unknown) {
    markRecoveryUnavailable(
      error,
      recoveryState.kind === "unavailable" ? recoveryState.lastKnown : null,
      recoveryCallbacks,
    );
    throw error;
  }
}

/**
 * GET-only reconciliation for a degraded execution. It never posts. When a
 * prior send callback set is available (tracking failure or a 409 conflict),
 * a recovered nonterminal send resumes the original tracker callbacks.
 */
export async function retryExecutionRecovery(): Promise<SendStatusResponse | null> {
  const callbacks = recoveryCallbacks;
  try {
    const status = await fetchRecoveredSend();
    if (status === null) {
      clearRecoveryState();
      return null;
    }
    if (callbacks !== null) {
      clearRecoveryState();
      callbacks.onStatus?.(status);
      if (isTerminalSendState(status.state)) {
        callbacks.onComplete?.(status);
      } else {
        createSendTracker(status.id, status, callbacks);
      }
    } else if (isTerminalSendState(status.state)) {
      clearRecoveryState();
    } else {
      publishRecoveryState({
        kind: "unavailable",
        message:
          "An active send was recovered. Use the page recovery control to resume tracking.",
        lastKnown: status,
      });
    }
    return status;
  } catch (error: unknown) {
    markRecoveryUnavailable(
      error,
      recoveryState.kind === "unavailable" ? recoveryState.lastKnown : null,
      callbacks,
    );
    throw error;
  }
}
