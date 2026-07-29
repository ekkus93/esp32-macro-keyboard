import { describe, expect, test } from "vitest";
import {
  executionConfirmationTargetFromHash,
  navigateToMacroConfirmation,
  navigateToProcedureMacroConfirmation,
} from "../src/routing";
import {
  macroId,
  macroStepId,
  procedureId,
} from "./appFixtures";
import { setHashSilently } from "./fakeLocation";

describe("execution confirmation routing", () => {
  test("parses a standalone macro confirmation", () => {
    setHashSilently(`/confirm?macroId=${macroId}`);
    expect(executionConfirmationTargetFromHash()).toEqual({
      kind: "valid",
      macroId,
      sourceContext: null,
    });
  });

  test("parses exact nested procedure context identifiers", () => {
    setHashSilently(
      `/confirm?stepId=${macroStepId}&macroId=${macroId}&procedureId=${procedureId}`,
    );
    expect(executionConfirmationTargetFromHash()).toEqual({
      kind: "valid",
      macroId,
      sourceContext: {
        procedureId,
        stepId: macroStepId,
      },
    });
  });

  test.each([
    "/confirm",
    `/confirm?macroId=${macroId}&procedureId=${procedureId}`,
    `/confirm?macroId=${macroId}&stepId=${macroStepId}`,
    `/confirm?macroId=${macroId}&extra=true`,
    `/confirm?macroId=${macroId}&macroId=${macroId}`,
    `/confirm?macroId=not-a-uuid`,
    `/macros?macroId=${macroId}`,
  ])("rejects malformed confirmation route %s", (hash) => {
    setHashSilently(hash);
    expect(executionConfirmationTargetFromHash()).toEqual({ kind: "invalid" });
  });

  test("navigation helpers always open a preview instead of executing", () => {
    navigateToMacroConfirmation(macroId);
    expect(window.location.hash).toBe(`#/confirm?macroId=${macroId}`);

    navigateToProcedureMacroConfirmation(
      procedureId,
      macroStepId,
      macroId,
    );
    expect(window.location.hash).toBe(
      `#/confirm?procedureId=${procedureId}&stepId=${macroStepId}&macroId=${macroId}`,
    );
  });
});
