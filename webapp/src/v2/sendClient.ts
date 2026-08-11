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
 *  - transient poll failures are retried only up to a fixed consecutive bound;
 *  - `onStatus` fires only for a meaningful state or progress change;
 *  - `onComplete` fires at most once, only after a terminal state;
 *  - `onError` fires when status tracking gives up or sees a non-transient failure.
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

/**
 * The shared poll loop behind both {@link sendMacro} (which seeds it with no
 * prior status, right after a fresh `202`) and {@link trackSend} (which seeds
 * it with an already-known status recovered from `GET /api/v1/send`, without
 * ever issuing a `POST`). Factored out so a reload-recovered or
 * race-recovered send (TODO_V2 V2-095) polls with the exact same cadence,
 * meaningful-change filtering, and at-most-once `onComplete` guarantee as a
 * send this module itself initiated — no second implementation of the
 * protocol.
 */
function createSendTracker(
  id: string,
  seed: SendStatusResponse | null,
  callbacks: SendMacroCallbacks,
): SendTracker {
  const flags: { stopped: boolean; completed: boolean } = {
    stopped: false,
    completed: seed !== null && isTerminalSendState(seed.state),
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

  const tracker = createSendTracker(accepted.id, null, callbacks);

  async function cancel(): Promise<void> {
    await cancelSend();
  }

  return { accepted, cancel, stop: tracker.stop };
}

/**
 * Resumes polling an already-known, not-yet-dismissed send without issuing a
 * new `POST` — the reload-recovery and `409`-recovery case from TODO_V2
 * V2-095 ("recover inline send state after reload", "handle `409` by showing
 * the actual current send"). `seed` is normally the result of
 * {@link recoverSendState}. Polling starts immediately at the same bounded
 * cadence as {@link sendMacro}; if `seed` is already terminal, no request is
 * made and `stop()` is a no-op.
 */
export function trackSend(
  seed: SendStatusResponse,
  callbacks: SendMacroCallbacks = {},
): SendTracker {
  return createSendTracker(seed.id, seed, callbacks);
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
