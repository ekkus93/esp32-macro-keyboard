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
 * This module is framework-agnostic — it does not itself guard against a
 * React component calling {@link sendMacro} twice for the same user action
 * (for example on a StrictMode double-invoke or an orientation-change
 * remount). That is the caller's responsibility (typically a ref-guarded
 * effect). What this module guarantees on its own:
 *  - exactly one `POST /api/v1/send` per {@link sendMacro} call;
 *  - polling at a bounded interval no slower than once per second;
 *  - `onStatus` fires only for a meaningful state or progress change;
 *  - `onComplete` fires at most once, only after a terminal state.
 */

const pollIntervalMs = 1000;
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
}

export interface SendMacroHandle {
  /** The `202 Accepted` response from the initiating `POST /api/v1/send`. */
  readonly accepted: SendAcceptedResponse;
  /** Requests cancellation of this send via `DELETE /api/v1/send`. Idempotent. */
  cancel: () => Promise<void>;
  /** Stops client-side polling only. Does not cancel the send on the device. */
  stop: () => void;
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

/**
 * Sends a macro and tracks it to completion.
 *
 * 1. Calls `POST /api/v1/send`; the returned promise rejects if this fails
 *    (invalid source, `409` already sending, network failure, etc.) and no
 *    polling ever starts.
 * 2. On acceptance, polls `GET /api/v1/send` no slower than once per second.
 * 3. Calls `onStatus` only when state or `actionIndex` changes.
 * 4. Calls `onComplete` exactly once, after a terminal state is observed.
 *
 * If the page closes, delivery of these callbacks is not guaranteed —
 * firmware continues the send independently. Use {@link recoverSendState}
 * after a reload to resume tracking.
 */
export async function sendMacro(
  request: SendRequest,
  callbacks: SendMacroCallbacks = {},
): Promise<SendMacroHandle> {
  const accepted: SendAcceptedResponse = await v2PostJson(
    sendPath,
    request,
    isSendAcceptedResponse,
  );

  const flags: { stopped: boolean; completed: boolean } = {
    stopped: false,
    completed: false,
  };
  // Reading these through functions (rather than the bare property) keeps
  // TypeScript from narrowing `flags.stopped`/`flags.completed` to a stale
  // literal across an `await`, during which `stop()` can genuinely run and
  // change them.
  function isStopped(): boolean {
    return flags.stopped;
  }
  function isCompleted(): boolean {
    return flags.completed;
  }
  let previous: SendStatusResponse | null = null;
  let timer: ReturnType<typeof setTimeout> | null = null;

  function schedulePoll(): void {
    if (isStopped()) {
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
    } catch {
      schedulePoll();
      return;
    }
    if (isStopped() || status.id !== accepted.id) {
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

  async function cancel(): Promise<void> {
    await cancelSend();
  }

  schedulePoll();

  return { accepted, cancel, stop };
}

/**
 * `DELETE /api/v1/send`: requests cancellation of the current non-terminal
 * send. Idempotent while the same send remains non-terminal. The exact
 * response body shape is not yet defined by the frozen v2 contract layer, so
 * the resolved value is intentionally unvalidated.
 */
export async function cancelSend(): Promise<unknown> {
  return v2DeleteWithUnvalidatedJson(sendPath);
}

/**
 * Recovers send state after a reload, per SPEC_V2 §13.11. Returns `null`
 * when no send exists since boot (`404`).
 */
export async function recoverSendState(): Promise<SendStatusResponse | null> {
  try {
    return await v2GetJson(sendPath, isSendStatusResponse);
  } catch (error: unknown) {
    if (error instanceof V2ApiError && error.status === 404) {
      return null;
    }
    throw error;
  }
}
