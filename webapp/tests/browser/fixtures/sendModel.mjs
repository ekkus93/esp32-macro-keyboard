// advanceSend and isTerminal are gathered here rather than left beside the
// server that defines each: startApplicationServer needs isTerminal and
// startStartupFixtureServer needs advanceSend, so leaving them in place would
// make the two fixture servers import each other.

export function advanceSend(send) {
  send.pollCount += 1;
  if (send.cancellationRequested) {
    return {
      state: "cancelled",
      actionIndex: send.pollCount,
      error: "",
      releaseError: "",
    };
  }
  if (send.source === "AWAIT_CONFIRM" && send.pollCount < 2) {
    return {
      state: "awaiting_confirmation",
      actionIndex: 0,
      error: "",
      releaseError: "",
    };
  }
  if (send.source === "FAIL") {
    return {
      state: "failed",
      actionIndex: send.pollCount,
      error: "simulated_failure",
      releaseError: "",
    };
  }
  if (send.source === "TIMEOUT") {
    return {
      state: "timed_out",
      actionIndex: send.pollCount,
      error: "",
      releaseError: "",
    };
  }
  if (send.pollCount < 2) {
    return {
      state: "running",
      actionIndex: send.pollCount,
      error: "",
      releaseError: "",
    };
  }
  const releaseError =
    send.source === "RELEASE_ERROR" ? "stuck_key_left_ctrl" : "";
  return { state: "completed", actionIndex: 2, error: "", releaseError };
}

export function isTerminal(state_) {
  return (
    state_ === "completed" ||
    state_ === "cancelled" ||
    state_ === "failed" ||
    state_ === "timed_out"
  );
}

/**
 * A second, independently configurable fixture server for TODO_V2 Phase 8
 * exit-gate startup scenarios (first phone, refresh, expired session, no
 * blobs, invalid newest blob, send recovery) plus the Phase 9 USB-unavailable
 * and Phase 10 macro-editing/package-management scenarios below. These need
 * device states -- unprovisioned, session-invalidated, blob-less, a corrupt
 * newest blob, a non-ready USB state from first load -- that the fixed
 * single-scenario `startApplicationServer` above cannot represent without
 * changing the behavior the Phase 9/11/12 workflows already depend on; this
 * is new, additive code, not a change to that function.
 */
