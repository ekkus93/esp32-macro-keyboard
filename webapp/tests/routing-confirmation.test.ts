import { describe, expect, test } from "vitest";
import {
  executionConfirmationTargetFromHash,
  navigateToMacroConfirmation,
} from "../src/routing";
import { macroId } from "./appFixtures";
import { setHashSilently } from "./fakeLocation";

/* Not fixtures: procedures no longer exist in the domain. These are only the
   shape of a stale bookmark, kept so the rejection cases below stay real. */
const staleProcedureId = "44444444-4444-4444-8444-444444444444";
const staleStepId = "66666666-6666-4666-8666-666666666666";

describe("execution confirmation routing", () => {
  test("parses a standalone macro confirmation", () => {
    setHashSilently(`/confirm?macroId=${macroId}`);
    expect(executionConfirmationTargetFromHash()).toEqual({
      kind: "valid",
      macroId,
    });
  });

  test.each([
    "/confirm",
    /* Procedure context is no longer part of the route (SPEC 9.1). A stale
       link carrying it must be rejected outright rather than silently
       reinterpreted as a plain macro confirmation. */
    `/confirm?stepId=${staleStepId}&macroId=${macroId}&procedureId=${staleProcedureId}`,
    `/confirm?macroId=${macroId}&procedureId=${staleProcedureId}`,
    `/confirm?macroId=${macroId}&stepId=${staleStepId}`,
    `/confirm?macroId=${macroId}&extra=true`,
    `/confirm?macroId=${macroId}&macroId=${macroId}`,
    `/confirm?macroId=not-a-uuid`,
    `/macros?macroId=${macroId}`,
  ])("rejects malformed confirmation route %s", (hash) => {
    setHashSilently(hash);
    expect(executionConfirmationTargetFromHash()).toEqual({ kind: "invalid" });
  });

  test("navigation helper always opens a preview instead of executing", () => {
    navigateToMacroConfirmation(macroId);
    expect(window.location.hash).toBe(`#/confirm?macroId=${macroId}`);
  });
});
