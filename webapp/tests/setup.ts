import { afterEach, beforeEach, vi } from "vitest";
import { setCsrfToken } from "../src/api/client";
import {
  assertNoPendingFetchPlans,
  installFakeFetch,
  resetFakeFetch,
} from "./fakeFetch";
import { resetFakeLocation } from "./fakeLocation";

const reactEnvironment = globalThis as typeof globalThis & {
  IS_REACT_ACT_ENVIRONMENT?: boolean;
};
reactEnvironment.IS_REACT_ACT_ENVIRONMENT = true;

let consoleErrors: unknown[][] = [];
let unhandledRejections: unknown[] = [];

const onUnhandledRejection = (event: PromiseRejectionEvent): void => {
  event.preventDefault();
  unhandledRejections.push(event.reason);
};

beforeEach(() => {
  document.body.replaceChildren();
  resetFakeLocation();
  resetFakeFetch();
  installFakeFetch();
  setCsrfToken(null);
  consoleErrors = [];
  unhandledRejections = [];
  vi.spyOn(console, "error").mockImplementation((...arguments_: unknown[]) => {
    consoleErrors.push(arguments_);
  });
  window.addEventListener("unhandledrejection", onUnhandledRejection);
});

afterEach(() => {
  window.removeEventListener("unhandledrejection", onUnhandledRejection);
  document.body.replaceChildren();

  let pendingPlanError: Error | null = null;
  try {
    assertNoPendingFetchPlans();
  } catch (error: unknown) {
    pendingPlanError =
      error instanceof Error ? error : new Error(String(error));
  }

  if (vi.isFakeTimers()) {
    vi.clearAllTimers();
    vi.useRealTimers();
  }
  vi.unstubAllGlobals();
  vi.restoreAllMocks();

  if (pendingPlanError !== null) {
    throw pendingPlanError;
  }
  if (consoleErrors.length !== 0) {
    throw new Error(
      `Unexpected console.error: ${String(consoleErrors[0]?.[0])}`,
    );
  }
  if (unhandledRejections.length !== 0) {
    throw new Error(`Unhandled rejection: ${String(unhandledRejections[0])}`);
  }
});
