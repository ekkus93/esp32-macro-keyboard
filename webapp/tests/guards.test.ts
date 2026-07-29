import { describe, expect, test } from "vitest";
import {
  isExecutionStatus,
  isMacro,
  isMacroSet,
  isMacroValidation,
  isSessionStatus,
  isSettings,
} from "../src/api/guards";
import { executionStatus, macro, macroSet, settings } from "./appFixtures";

describe("API response guards", () => {
  test("accepts exact server resource shapes", () => {
    expect(
      isSessionStatus({ authenticated: true, csrfToken: "csrf-token" }),
    ).toBe(true);
    expect(isMacroSet(macroSet)).toBe(true);
    expect(isMacro(macro)).toBe(true);
    expect(
      isMacroValidation({
        valid: true,
        actionCount: 3,
        estimatedDurationMs: 69,
      }),
    ).toBe(true);
    expect(isSettings(settings)).toBe(true);
    expect(isExecutionStatus(executionStatus("timed_out"))).toBe(true);
  });

  test("rejects missing, unknown, and mistyped fields", () => {
    expect(isMacroSet({ ...macroSet, revision: "2" })).toBe(false);
    expect(isMacro({ ...macro, source: "x".repeat(4097) })).toBe(false);
    expect(isMacro({ ...macro, secret: "must not pass" })).toBe(false);
    expect(
      isMacroValidation({
        valid: true,
        actionCount: 4097,
        estimatedDurationMs: 69,
      }),
    ).toBe(false);
    expect(isSessionStatus({ authenticated: true })).toBe(false);
    expect(
      isSessionStatus({
        authenticated: true,
        csrfToken: "csrf-token",
        sessionToken: "must not pass",
      }),
    ).toBe(false);
    expect(isSettings({ ...settings, secret: "must not pass" })).toBe(false);
    const execution = executionStatus("running") as Record<string, unknown>;
    delete execution.available;
    expect(isExecutionStatus(execution)).toBe(false);
  });

  test("rejects unknown execution states", () => {
    expect(
      isExecutionStatus({
        ...executionStatus("running"),
        state: "finished",
      }),
    ).toBe(false);
  });
});
