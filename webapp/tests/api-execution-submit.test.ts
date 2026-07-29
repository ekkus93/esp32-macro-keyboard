import { describe, expect, test } from "vitest";
import { submitExecution } from "../src/api/routes";
import type { ExecutionSubmitRequest } from "../src/types/models";
import { getFetchCalls, planJsonResponse } from "./fakeFetch";

const setId = "11111111-1111-4111-8111-111111111111";
const macroId = "22222222-2222-4222-8222-222222222222";
const procedureId = "33333333-3333-4333-8333-333333333333";
const stepId = "44444444-4444-4444-8444-444444444444";
const executionId = "55555555-5555-4555-8555-555555555555";

function accepted(): { ok: true; data: Record<string, unknown> } {
  return {
    ok: true,
    data: {
      executionId,
      actionCount: 3,
      estimatedDurationMs: 125,
    },
  };
}

function parseJsonBody(body: BodyInit | null | undefined): unknown {
  if (typeof body !== "string") {
    throw new Error("Expected a JSON string request body.");
  }
  return JSON.parse(body) as unknown;
}

describe("submitExecution", () => {
  test("sends the exact standalone request shape", async () => {
    planJsonResponse(accepted(), 202);
    const request: ExecutionSubmitRequest = {
      setId,
      macroId,
      macroRevision: 7,
    };

    await expect(submitExecution(request)).resolves.toEqual(accepted().data);
    const call = getFetchCalls()[0];
    expect(call?.url).toBe("/api/v1/executions");
    expect(call?.method).toBe("POST");
    expect(parseJsonBody(call?.body)).toEqual({
      setId,
      macroId,
      macroRevision: 7,
    });
  });

  test("nests complete procedure context and does not flatten it", async () => {
    planJsonResponse(accepted(), 202);
    const request: ExecutionSubmitRequest = {
      setId,
      macroId,
      macroRevision: 7,
      sourceContext: { procedureId, stepId },
    };

    await submitExecution(request);
    expect(parseJsonBody(getFetchCalls()[0]?.body)).toEqual({
      setId,
      macroId,
      macroRevision: 7,
      sourceContext: { procedureId, stepId },
    });
  });

  test.each([
    {},
    { executionId, actionCount: 3 },
    { executionId: "bad", actionCount: 3, estimatedDurationMs: 125 },
    { executionId, actionCount: -1, estimatedDurationMs: 125 },
    { executionId, actionCount: 3, estimatedDurationMs: -1 },
    {
      executionId,
      actionCount: 3,
      estimatedDurationMs: 125,
      extra: true,
    },
  ])("rejects invalid accepted response %#", async (data: unknown) => {
    planJsonResponse({ ok: true, data }, 202);
    await expect(
      submitExecution({ setId, macroId, macroRevision: 7 }),
    ).rejects.toMatchObject({
      status: 202,
      body: { code: "invalid_response" },
    });
  });
});
